#!/usr/bin/env python3
"""Turn derive_recipes.py's JSON (link-independent) plus the current link's
assets.bin (pointer values) into the binary recipe blob that rides in the
EBOOT's DATA.PSAR (numbers only, no game data).  The PSP reads it into scratch
memory while extracting, so it costs no RAM afterwards.

usage: emit_recipes.py recipes.json assets.bin -o recipes.bin
"""
import argparse, json, struct, zlib

SHAPE = {'4': 0, '22': 1, '211': 2, '112': 3, '1111': 4}
KIND = {'RAW': 1, 'MIO0': 2, 'LITERAL': 3, 'RELOCS': 4, 'UNPACK': 5}
XF = {'id': 0, 'sw16': 1, 'sw32': 2}

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('json'); ap.add_argument('assets'); ap.add_argument('-o', required=True)
    a = ap.parse_args()
    js = json.load(open(a.json))
    d = open(a.assets, 'rb').read()
    magic, ver, addr, size, nrel, nsym, nnames, bid, crc, pad = struct.unpack_from('<8sIIIIIIIII', d, 0)
    assert size == js['region_size'], 'assets.bin region size differs from the recipes'
    relocs = list(struct.unpack_from('<%dI' % nrel, d, 44))
    p = (44 + nrel * 4 + nsym * 16 + nnames + 15) & ~15
    data = bytearray(d[p:p + size])
    rel_entries = []
    for off in relocs:
        val = struct.unpack_from('<I', data, off)[0]
        rel_entries.append((off, val - addr) if addr <= val <= addr + size else (off | 0x80000000, val))  # flag on the offset: the value may use bit 31
        data[off:off + 4] = b'\0\0\0\0'
    data_crc = zlib.crc32(bytes(data)) & 0xffffffff

    # recipes: resolve literals, then merge runs with the same source/transform
    lit = bytearray(); recs = []
    for r in js['recipes']:
        kind = KIND[r['kind']]; xf = r['xform']
        x = 16 + int(xf[3:]) if xf.startswith('pat') else XF[xf]
        if r['kind'] == 'LITERAL':
            src = len(lit); lit += bytes.fromhex(r['extra']); extra = 0
        elif r['kind'] == 'RAW':
            src = r['extra']; extra = 0
        else:
            src = r['src']; extra = r['extra']
        recs.append([r['off'], r['size'], kind, x, src, extra])
    recs.sort(key=lambda r: (r[2] == 2 and r[4] or 0, r[0]))  # MIO0 grouped by block, else by dst
    merged = []
    for r in recs:
        m = merged[-1] if merged else None
        if m and m[2] == r[2] and m[3] == r[3] and m[2] in (1, 2, 3) and m[0] + m[1] == r[0] \
           and ((m[2] == 1 and m[4] + m[1] == r[4]) or (m[2] == 2 and m[4] == r[4] and m[5] + m[1] == r[5]) or (m[2] == 3 and m[4] + m[1] == r[4])) \
           and (m[3] < 16 or (m[1] % (4 * js['patterns'][m[3] - 16].__len__())) == 0):
            m[1] += r[1]
        else:
            merged.append(list(r))
    blocks = {b['rom_off']: b for b in js['blocks']}
    out = bytearray()
    out += struct.pack('<8sIIIIIIIIII', b'MK64RCP1', size, data_crc, len(merged), len(lit), len(js['patterns']),
                       len(js['courses']), len(js['blocks']), len(rel_entries),
                       max(b['decomp_len'] for b in js['blocks']), max(c['unpacked_len'] for c in js['courses']))
    for r in merged:
        out += struct.pack('<IIHHII', r[0], r[1], r[2], r[3], r[4], r[5])
    out += lit; out += b'\0' * (-len(out) % 4)
    for pat in js['patterns']:
        out += struct.pack('<I16B', len(pat), *([SHAPE[s] for s in pat] + [0] * (16 - len(pat))))
    for c in js['courses']:
        out += struct.pack('<IIIII', c['idx'], c['unpacked_len'], c['rom_off'], c['rom_len'], c['packed_off'])
    for b in js['blocks']:
        out += struct.pack('<III', b['rom_off'], b['rom_len'], b['decomp_len'])
    for off, t in rel_entries:
        out += struct.pack('<II', off, t)
    open(a.o, 'wb').write(out)
    print('wrote %s: %d bytes (%d recipes merged from %d, %d literal bytes, %d relocs, %d outside), data crc %08X' % (
        a.o, len(out), len(merged), len(recs), len(lit), len(rel_entries), sum(1 for o, t in rel_entries if o & 0x80000000), data_crc))

if __name__ == '__main__':
    main()
