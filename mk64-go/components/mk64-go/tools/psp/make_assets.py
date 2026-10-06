#!/usr/bin/env python3
"""Build assets.bin: the ROM-derived data of the port, taken OUT of the EBOOT.

The asset objects' .data/.rodata are renamed to .assetdata/.assetrodata and
collected by the linker into one output section placed after .bss:
  * in the shipped link the section is NOLOAD -> symbols and addresses exist,
    the PRX carries no bytes for it;
  * in a second, identical link it is PROGBITS -> this tool takes its bytes,
    the R_MIPS_32 relocations inside it and the symbol table, and writes
    assets.bin.  At boot the port reads the file straight into the region
    and adds the module base to every relocated word.

usage: make_assets.py <progbits.elf> <noload.elf> -o assets.bin
"""
import argparse, struct, sys, zlib

SECTION = b'.assetdata'


def parse_elf(path):
    d = open(path, 'rb').read()
    assert d[:4] == b'\x7fELF' and d[5] == 1, 'need a little-endian ELF32'
    (e_shoff,) = struct.unpack_from('<I', d, 0x20)
    e_shentsize, e_shnum, e_shstrndx = struct.unpack_from('<HHH', d, 0x2e)
    secs = []
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        name, typ, flags, addr, offset, size, link, info, align, entsize = struct.unpack_from('<IIIIIIIIII', d, off)
        secs.append(dict(name=name, type=typ, flags=flags, addr=addr, offset=offset, size=size, link=link, info=info, entsize=entsize))
    shstr = secs[e_shstrndx]
    def sname(s):
        b = d[shstr['offset'] + s['name']:]
        return b[:b.index(b'\x00')]
    for s in secs:
        s['sname'] = sname(s)
    byname = {s['sname']: s for s in secs}
    return d, secs, byname


def symbols(d, secs, byname, sec_index):
    st = byname[b'.symtab']; strt = secs[st['link']]
    out = []
    for i in range(st['size'] // 16):
        name, value, size, info, other, shndx = struct.unpack_from('<IIIBBH', d, st['offset'] + i * 16)
        if shndx == sec_index and (info & 0xf) in (0, 1):  # NOTYPE/OBJECT; asm glabels have size 0
            b = d[strt['offset'] + name:]
            nm = b[:b.index(b'\x00')].decode()
            if nm and not nm.startswith('.L'):
                out.append((nm, value, size))
    # One entry per address; extent = up to the next symbol (covers unsized asm
    # data and tables whose declared size is short), the last one to the end.
    out.sort(key=lambda s: (s[1], -s[2], s[0]))
    dedup = []
    for nm, value, size in out:
        if dedup and dedup[-1][1] == value:
            continue
        dedup.append([nm, value, size])
    return dedup


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('progbits')
    ap.add_argument('noload')
    ap.add_argument('-o', required=True)
    a = ap.parse_args()

    d, secs, byname = parse_elf(a.progbits)
    sec = byname.get(SECTION)
    if sec is None or sec['type'] != 1:
        sys.exit(f'{a.progbits}: no PROGBITS {SECTION.decode()} section')
    idx = secs.index(sec)
    data = d[sec['offset']:sec['offset'] + sec['size']]

    dn, nsecs, nbyname = parse_elf(a.noload)
    nsec = nbyname.get(SECTION)
    if nsec is None or nsec['addr'] != sec['addr'] or nsec['size'] != sec['size']:
        sys.exit(f'layout mismatch: progbits {sec["addr"]:#x}/{sec["size"]:#x} vs noload '
                 f'{nsec["addr"] if nsec else 0:#x}/{nsec["size"] if nsec else 0:#x}')
    if nsec['type'] != 8:
        sys.exit(f'{a.noload}: {SECTION.decode()} is not NOBITS (type {nsec["type"]})')

    rel = byname.get(b'.rel' + SECTION)
    relocs = []
    if rel is not None:
        for i in range(rel['size'] // 8):
            r_offset, r_info = struct.unpack_from('<II', d, rel['offset'] + i * 8)
            if (r_info & 0xff) != 2:
                sys.exit(f'unexpected relocation type {r_info & 0xff} at {r_offset:#x}')
            relocs.append(r_offset - sec['addr'])
    relocs.sort()

    syms = symbols(d, secs, byname, idx)
    for i, sym in enumerate(syms):
        nxt = syms[i + 1][1] if i + 1 < len(syms) else sec['addr'] + sec['size']
        sym[2] = nxt - sym[1]
    names = b''
    symtab = b''
    for name, value, size in syms:
        symtab += struct.pack('<IIII', value - sec['addr'], size, zlib.crc32(name.encode()) & 0xffffffff, len(names))
        names += name.encode() + b'\x00'

    reltab = b''.join(struct.pack('<I', r) for r in relocs)
    build_id = zlib.crc32(reltab + symtab + struct.pack('<II', sec['addr'], sec['size'])) & 0xffffffff
    hdr = struct.pack('<8sIIIIIIIII', b'MK64ASST', 1, sec['addr'], sec['size'], len(relocs), len(syms), len(names),
                      build_id, zlib.crc32(data) & 0xffffffff, 0)
    with open(a.o, 'wb') as f:
        f.write(hdr); f.write(reltab); f.write(symtab); f.write(names)
        f.write(b'\x00' * (-f.tell() % 16))
        f.write(data)
    print(f'{a.o}: region {sec["addr"]:#x}+{sec["size"]:#x} ({sec["size"] / 1048576:.1f} MB), '
          f'{len(relocs)} relocs, {len(syms)} symbols, build id {build_id:08x}')


if __name__ == '__main__':
    main()
