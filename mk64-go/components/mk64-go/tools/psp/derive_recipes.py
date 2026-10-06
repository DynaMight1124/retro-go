#!/usr/bin/env python3
"""Derive, per asset symbol, how its bytes come from the ROM, and emit the recipe
table the PSP executes on first run (port_asset_recipes.c).

Kinds
  RAW      bytes at a ROM offset            (xform: id / sw16 / sw32 / pattern)
  MIO0     bytes inside a decompressed mio0 block at a ROM offset (same xforms)
  LITERAL  <= 16-byte constants carried verbatim (numbers, not assets)
  RELOCS   symbol made only of pointers (filled by the relocation pass)
  UNPACK   slice of a course's unpacked packed-display-list stream
           (displaylist_unpack on the d_course_X_packed symbol, as the N64 does)

usage: derive_recipes.py assets.bin baserom.z64 -o port_asset_recipes.c [--cache f]
"""
import struct, sys, re, os, pickle, collections, argparse

# ---------------------------------------------------------------- helpers
def mio0_decode(d, off):
    if d[off:off+4] != b'MIO0': return None
    size, coff, uoff = struct.unpack('>III', d[off+4:off+16])
    out = bytearray(); layout = off + 16; cp = off + coff; up = off + uoff
    bits = 0; nbits = 0; hi = off + 16
    while len(out) < size:
        if nbits == 0:
            bits = struct.unpack('>I', d[layout:layout+4])[0]; layout += 4; nbits = 32
        if bits & 0x80000000:
            out.append(d[up]); up += 1
        else:
            b0, b1 = d[cp], d[cp+1]; cp += 2
            length = (b0 >> 4) + 3; dist = ((b0 & 0xF) << 8 | b1) + 1
            for _ in range(length): out.append(out[-dist])
        bits = (bits << 1) & 0xffffffff; nbits -= 1
    mio0_decode.consumed = max(layout, cp, up) - off
    return bytes(out[:size])

def swap16(b):
    n = len(b)//2*2; a = bytearray(b)
    a[0:n:2], a[1:n:2] = b[1:n:2], b[0:n:2]; return bytes(a)
def swap32(b):
    n = len(b)//4*4; a = bytearray(b)
    a[0:n:4], a[1:n:4], a[2:n:4], a[3:n:4] = b[3:n:4], b[2:n:4], b[1:n:4], b[0:n:4]; return bytes(a)
def sorted4(b):
    n = len(b)//4*4
    return b''.join(bytes(sorted(b[i:i+4])) for i in range(0, n, 4))

# word shapes: field widths inside one 4-byte word (LE target from BE source)
SHAPES = {'4': [4], '22': [2, 2], '211': [2, 1, 1], '112': [1, 1, 2], '1111': [1, 1, 1, 1]}
def apply_shape(word, shape):
    out = b''; i = 0
    for w in SHAPES[shape]:
        out += word[i:i+w][::-1]; i += w
    return out
def apply_pattern(b, shapes):
    n = len(b)//4*4; out = bytearray(b)
    for i in range(0, n, 4):
        out[i:i+4] = apply_shape(b[i:i+4], shapes[(i//4) % len(shapes)])
    return bytes(out)

def load_assets(path):
    d = open(path,'rb').read()
    magic, ver, addr, size, nrel, nsym, nnames, bid, crc, pad = struct.unpack_from('<8sIIIIIIIII', d, 0)
    p = 44
    relocs = list(struct.unpack_from('<%dI'%nrel, d, p)); p += nrel*4
    symtab = p; p += nsym*16
    names = d[p:p+nnames]; p += nnames
    syms = []
    for i in range(nsym):
        off, sz, h, nm = struct.unpack_from('<IIII', d, symtab + i*16)
        syms.append((names[nm:names.index(b'\x00', nm)].decode(), off, sz))
    p = (p + 15) & ~15
    return addr, size, relocs, syms, d[p:p+size]

# ---------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('assets'); ap.add_argument('rom'); ap.add_argument('-o', required=True)
    ap.add_argument('--cache', default=None); ap.add_argument('--course-table', default='courses/courseTable.c')
    ap.add_argument('--report', default=None)
    a = ap.parse_args()
    rom = open(a.rom, 'rb').read()
    addr, size, relocs, syms, data = load_assets(a.assets)
    relset = set(relocs)

    # candidate sources (cached: mio0 decode in python is slow)
    cache = a.cache or (a.assets + '.mio0cache')
    if os.path.exists(cache):
        blocks = pickle.load(open(cache, 'rb'))
    else:
        blocks = []
        for m in re.finditer(b'MIO0', rom):
            b = mio0_decode(rom, m.start())
            if b: blocks.append((m.start(), b, mio0_decode.consumed))
        pickle.dump(blocks, open(cache, 'wb'))
    if len(blocks[0]) == 2:  # old cache without lengths
        blocks = [(o, b, len(mio0_decode(rom, o)) and mio0_decode.consumed) for o, b in blocks]
        pickle.dump(blocks, open(cache, 'wb'))
    block_len = {o: (l, len(b)) for o, b, l in blocks}
    cands = [('RAW', 0, rom)] + [('MIO0', o, b) for o, b, l in blocks]
    variants = []
    for kind, off, b in cands:
        variants.append((kind, 'id', off, b)); variants.append((kind, 'sw16', off, swap16(b))); variants.append((kind, 'sw32', off, swap32(b)))
    sorted_cands = None  # built lazily for pattern inference

    course_order = re.findall(r'd_course_(\w+?)_packed,', open(a.course_table).read())

    recipes = []      # (name, off, size, kind, xform, src, extra)
    patterns = {}     # shapes tuple -> id
    class _PInv(dict):
        def __getitem__(self, k):
            pid = int(k[3:])
            return next(p for p, i in patterns.items() if i == pid)
    patterns_inv = _PInv()
    unmatched = []
    stats = collections.Counter()

    def match_simple(b, sz, rel_in):
        runs = []; prev = 0
        for r in rel_in + [sz]:
            if r - prev >= 8: runs.append((prev, r))
            prev = r + 4
        if not runs: return None
        ps, pe = max(runs, key=lambda x: x[1]-x[0])
        probe = b[ps:min(pe, ps+64)]
        for kind, tr, coff, cb in variants:
            k = cb.find(probe)
            while k >= 0:
                start = k - ps
                if start >= 0 and start + sz <= len(cb):
                    ok = True; cur = 0
                    for r in rel_in:
                        if cb[start+cur:start+r] != b[cur:r]: ok = False; break
                        cur = r + 4
                    if ok and cb[start+cur:start+sz] == b[cur:sz]:
                        return (kind, tr, coff, start)
                k = cb.find(probe, k+1)
        return None

    def infer_pattern(b, sz, rel_in):
        nonlocal sorted_cands
        if sorted_cands is None:
            sorted_cands = [(kind, off, ph, sorted4(cb[ph:])) for kind, off, cb in cands for ph in range(4)]
        n4 = sz // 4 * 4
        if n4 < 16: return None
        relw = set(r // 4 for r in rel_in)
        # probe: longest run of reloc-free aligned words
        best = (0, 0); cur = None
        for w in range(n4 // 4):
            if w in relw:
                cur = None; continue
            if cur is None: cur = w
            if w - cur + 1 > best[1] - best[0]: best = (cur, w + 1)
        if best[1] - best[0] < 4: return None
        pw0, pw1 = best; pw1 = min(pw1, pw0 + 16)
        probe = sorted4(b[pw0*4:pw1*4])
        for kind, coff, ph, scb in sorted_cands:
            k = scb.find(probe)
            while k >= 0:
                start = ph + k - pw0 * 4
                cb = next(c for kk, o, c in cands if kk == kind and o == coff)
                if k % 4 == 0 and start >= 0 and start + sz <= len(cb):
                    # per-word shapes
                    shapes = []
                    for w in range(n4 // 4):
                        if w in relw: shapes.append(None); continue
                        tw = b[w*4:w*4+4]; cw = cb[start+w*4:start+w*4+4]
                        ok = [s for s in SHAPES if apply_shape(cw, s) == tw]
                        if not ok: shapes = None; break
                        shapes.append(ok)
                    if shapes is None:
                        k = scb.find(probe, k+1); continue
                    # smallest period consistent with all words
                    for p in [1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16]:
                        pat = []
                        for ph in range(p):
                            opts = None
                            for w in range(ph, len(shapes), p):
                                if shapes[w] is None: continue
                                opts = set(shapes[w]) if opts is None else opts & set(shapes[w])
                            if opts is None: opts = {'1111'}
                            if not opts: pat = None; break
                            pat.append(sorted(opts, key=lambda s: ['1111','22','4','211','112'].index(s))[0])
                        if pat is None: continue
                        n4r = (sz + 3) & ~3
                        seg = cb[start:start+n4r]; seg += b'\0' * (n4r - len(seg))
                        rebuilt = apply_pattern(seg, pat)[:sz]
                        okall = True; c2 = 0
                        for r in rel_in:
                            if rebuilt[c2:r] != b[c2:r]: okall = False; break
                            c2 = r + 4
                        if okall and rebuilt[c2:sz] == b[c2:sz]:
                            return (kind, coff, start, tuple(pat))
                k = scb.find(probe, k+1)
        return None

    unpack_syms = collections.defaultdict(list)
    pending = []   # (name, off, sz, b, rel_in)

    def verify_at(b, sz, rel_in, cb, start, xf):
        """Does candidate stream cb (untransformed) at `start` reproduce b under transform xf?"""
        if start < 0 or start + sz > len(cb): return False
        n4 = sz // 4 * 4
        seg = cb[start:start + sz]
        if xf == 'id': t = seg
        elif xf == 'sw16': t = swap16(cb[start:start + ((sz + 1) & ~1)])[:sz]
        elif xf == 'sw32': t = swap32(cb[start:start + ((sz + 3) & ~3)])[:sz]
        else:  # like the generator: transform the word-rounded range, keep sz bytes
            n4r = (sz + 3) & ~3
            seg = cb[start:start + n4r]; seg += b'\0' * (n4r - len(seg))  # symbol may end at the block end
            t = apply_pattern(seg, xf)[:sz]
        cur = 0
        for r in rel_in:
            if t[cur:r] != b[cur:r]: return False
            cur = r + 4
        return t[cur:sz] == b[cur:sz]

    for name, off, sz in syms:
        # trailing zero bytes are alignment padding (the region starts zeroed): match the content only
        full = sz
        stripped = len(data[off:off+sz].rstrip(b'\0'))
        if stripped == 0:
            stats['ZERO'] += 1; continue
        if stripped < sz and (sz - stripped) <= 256 and '_packed_dl_' not in name:  # packed DLs must keep exact extents (contiguous stream)
            sz = stripped
        b = data[off:off+sz]
        rel_in = sorted(r - off for r in relocs if off <= r < off+sz)
        m = re.match(r'd_course_(\w+?)_packed_dl_([0-9A-F]+)$', name)
        if m:
            unpack_syms[m.group(1)].append((int(m.group(2), 16), off, sz, name)); continue
        if 4*len(rel_in) >= sz:
            recipes.append((name, off, sz, 'RELOCS', 'id', 0, 0)); stats['RELOCS'] += 1; continue
        f = match_simple(b, sz, rel_in)
        if f:
            recipes.append((name, off, sz, f[0], f[1], f[2], f[3])); stats['%s/%s' % (f[0], f[1])] += 1; continue
        f = infer_pattern(b, sz, rel_in)
        if f:
            pid = patterns.setdefault(f[3], len(patterns))
            recipes.append((name, off, sz, f[0], 'pat%d' % pid, f[1], f[2])); stats['%s/pattern' % f[0]] += 1; continue
        pending.append((name, off, sz, b, rel_in))

    # ---- locality: a symbol sits at the same distance from its matched neighbours in the source as in the region
    cand_by = {('RAW', 0): rom}
    for o, cb, l in blocks: cand_by[('MIO0', o)] = cb
    def xf_list():
        return ['id', 'sw16', 'sw32'] + [tuple(p) for p in patterns]
    def xf_name(xf):
        if isinstance(xf, tuple): return 'pat%d' % patterns.setdefault(xf, len(patterns))
        return xf
    def infer_at(b, sz, rel_in, cb, start):
        """Per-word field shapes of b vs candidate cb at a known start -> periodic pattern or None."""
        n4 = sz // 4 * 4; n4r = (sz + 3) & ~3
        if start < 0 or start + sz > len(cb) or n4r < 4: return None
        relw = set(r // 4 for r in rel_in)
        shapes = []
        for w in range(n4r // 4):
            if w in relw: shapes.append(None); continue
            tw = b[w*4:w*4+4]; cw = cb[start+w*4:start+w*4+4]
            if len(cw) < 4: cw = cw + b'\0' * (4 - len(cw))
            ok = [sh for sh in SHAPES if apply_shape(cw, sh)[:len(tw)] == tw]
            if not ok: return None
            shapes.append(ok)
        for p in [1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16]:
            pat = []
            for ph in range(p):
                opts = None
                for w in range(ph, len(shapes), p):
                    if shapes[w] is None: continue
                    opts = set(shapes[w]) if opts is None else opts & set(shapes[w])
                if opts is None: opts = {'1111'}
                if not opts: pat = None; break
                pat.append(sorted(opts, key=lambda x: ['1111','22','4','211','112'].index(x))[0])
            if pat and verify_at(b, sz, rel_in, cb, start, tuple(pat)): return tuple(pat)
        return None

    progress = True
    while pending and progress:
        progress = False
        by_off = sorted(recipes, key=lambda r: r[1])
        offs = [r[1] for r in by_off]
        still = []
        import bisect
        for name, off, sz, b, rel_in in pending:
            i = bisect.bisect_left(offs, off)
            neigh = [r for r in by_off[max(0, i-3):i] + by_off[i:i+3] if r[3] in ('RAW', 'MIO0')]
            found = None
            for nb in neigh:
                kind, xf0, coff, start0 = nb[3], nb[4], nb[5], nb[6]
                cb = cand_by[(kind, coff)]
                start = start0 + (off - nb[1])
                xfs = [tuple(patterns_inv[xf0]) if xf0.startswith('pat') else xf0]
                xfs += [x for x in xf_list() if x not in xfs]
                for xf in xfs:
                    if verify_at(b, sz, rel_in, cb, start, xf):
                        found = (kind, xf_name(xf), coff, start); break
                if not found:
                    pat = infer_at(b, sz, rel_in, cb, start)
                    if pat: found = (kind, xf_name(pat), coff, start)
                if found: break
            if found:
                recipes.append((name, off, sz) + found); stats['%s/locality' % found[0]] += 1; progress = True
            else:
                still.append((name, off, sz, b, rel_in))
        pending = still

    # ---- masked search: pointer words wild-carded.  The transforms are involutions,
    # so the SYMBOL is transformed (per pattern and phase) and the plain streams
    # are searched, near the matched neighbours first (same block / a +-1 MB ROM
    # window, all patterns), then globally with the cheap transforms only.
    if pending:
        plain = {(kind, coff): cb for kind, xf, coff, cb in variants if xf == 'id'}
        all_pats = [('id',), ('22',), ('4',), ('112',), ('211',)] + [tuple(p) for p in patterns]
        cheap_pats = [('id',), ('22',), ('4',), ('112',), ('211',)]
        by_off = sorted(recipes, key=lambda r: r[1]); offs = [r[1] for r in by_off]
        import bisect
        def try_search(b, sz, rel_in, pats, streams):
            n4 = sz // 4 * 4
            for pat in pats:
                for ph in range(len(pat)):
                    rot = pat[ph:] + pat[:ph]
                    tb = b[:n4] if rot == ('id',) else apply_pattern(b[:n4], rot)
                    if n4 == 0: tb = b[:sz]
                    parts = []; cur = 0
                    for r in rel_in:
                        if r >= len(tb): break
                        parts.append(re.escape(tb[cur:r])); parts.append(b'.{4}'); cur = r + 4
                    parts.append(re.escape(tb[cur:len(tb)]))
                    rx = re.compile(b''.join(parts), re.S)
                    xf = 'id' if rot == ('id',) else ('sw16' if rot == ('22',) else ('sw32' if rot == ('4',) else rot))
                    for kind, coff, cb, lo, hi in streams:
                        nhit = 0
                        for mm in rx.finditer(cb, lo, hi):
                            nhit += 1
                            if nhit > 500: break
                            if verify_at(b, sz, rel_in, cb, mm.start(), xf):
                                return (kind, xf_name(xf) if isinstance(xf, tuple) else xf, coff, mm.start())
            return None
        for name, off, sz, b, rel_in in list(pending):
            i = bisect.bisect_left(offs, off)
            near = []
            for nb in by_off[max(0, i-4):i+4]:
                if nb[3] not in ('RAW', 'MIO0'): continue
                key = (nb[3], nb[5]); cb = plain[key]
                if nb[3] == 'RAW':
                    lo, hi = max(0, nb[6] - (1 << 20)), min(len(cb), nb[6] + (1 << 20))
                else:
                    lo, hi = 0, len(cb)
                if (key, lo, hi) not in [(n[0], n[3], n[4]) for n in near]:
                    near.append((key, key[0], key[1], cb, lo, hi))
            near_streams = [(k[1], k[2], k[3], k[4], k[5]) for k in near]
            found = try_search(b, sz, rel_in, all_pats, near_streams)
            if not found:
                found = try_search(b, sz, rel_in, cheap_pats, [(k[0], k[1], cb, 0, len(cb)) for k, cb in plain.items()])
            if found:
                recipes.append((name, off, sz) + found); stats['%s/masked' % found[0]] += 1; pending.remove((name, off, sz, b, rel_in))
    # ---- small symbols: brute force -- every occurrence of the first non-zero
    # non-pointer word (under every field shape) in every stream, then infer
    # the pattern at that position.
    if pending:
        plain_streams = [(kind, coff, cb) for kind, xf, coff, cb in variants if xf == 'id']
        for item in list(pending):
            name, off, sz, b, rel_in = item
            if sz > 64: continue
            n4r = (sz + 3) & ~3
            wi = next((i for i in range(0, n4r, 4) if i not in rel_in and any(b[i:i+4])), None)
            if wi is None: continue
            w0 = data[off+wi:off+wi+4]; w0 += b'\0' * (4 - len(w0))  # the partial last word, padded as in the region
            found = None
            for sh in SHAPES:
                probe = apply_shape(w0, sh)
                for kind, coff, cb in plain_streams:
                    k = cb.find(probe); tries = 0
                    while k >= 0 and tries < 20000:
                        tries += 1
                        st = k - wi
                        if st >= 0 and st % 4 == 0:
                            pat = infer_at(b, sz, rel_in, cb, st)
                            if pat: found = (kind, xf_name(pat), coff, st); break
                        k = cb.find(probe, k + 1)
                    if found: break
                if found: break
            if found:
                recipes.append((name, off, sz) + found); stats['%s/brute' % found[0]] += 1; pending.remove(item)

    # ---- explicit port-computed tables: numbers the port derives itself (not ROM bytes)
    PORT_TABLES = {'gCourseTable'}  # vertex counts = ARRAY_COUNT of the port's arrays; the rest is pointers
    for item in list(pending):
        name, off, sz, b, rel_in = item
        if name in PORT_TABLES:
            recipes.append((name, off, sz, 'LITERAL', 'id', 0, b)); stats['LITERAL(port)'] += 1; pending.remove(item)
    for name, off, sz, b, rel_in in pending:
        unmatched.append((name, off, sz, len(rel_in))); stats['UNMATCHED'] += 1

    # packed display lists: contiguous unpacked stream per course
    courses = []
    for cname, lst in unpack_syms.items():
        lst.sort()
        end = 0
        for uoff, off, sz, name in lst:
            if uoff != end:
                sys.exit('%s: packed dl %s not contiguous (expected %#x)' % (cname, name, end))
            end = uoff + sz
        if cname not in course_order:
            sys.exit('unknown course %s' % cname)
        cidx = course_order.index(cname)
        courses.append((cidx, cname, end))
        for uoff, off, sz, name in lst:
            recipes.append((name, off, sz, 'UNPACK', 'id', cidx, uoff)); stats['UNPACK'] += 1

    total_bytes = size
    un_bytes = sum(u[2] for u in unmatched)
    rep = ['symbols %d' % len(syms)] + ['  %-16s %5d' % (k, v) for k, v in stats.most_common()]
    rep.append('unmatched: %d symbols, %d bytes (%.2f%%)' % (len(unmatched), un_bytes, 100.0 * un_bytes / total_bytes))
    for u in unmatched: rep.append('  %s size %d relocs %d' % (u[0], u[2], u[3]))
    rep.append('patterns: %d' % len(patterns))
    for pat, pid in patterns.items(): rep.append('  pat%d: %s' % (pid, ' '.join(pat)))
    print('\n'.join(rep))
    if a.report: open(a.report, 'w').write('\n'.join(rep))
    if unmatched:
        print('NOT emitting recipes: unmatched symbols remain'); sys.exit(1)

    # ------------------------------------------------------------ emit JSON (link-independent)
    import json
    # packed display lists: raw bytes right after each course's mio0 vertex block
    # (course_geography.mio0.s: glabel d_course_X_vertex .incbin vertices.mio0,
    #  .balign 4, glabel d_course_X_packed .incbin displaylists_packed.bin)
    vtx_block = {}
    for name, off, sz, kind, xf, src, extra in recipes:
        m = re.match(r'd_course_(\w+?)_vertex$', name)
        if m and kind == 'MIO0': vtx_block[m.group(1)] = src
    sorted_blocks = sorted(o for o, b, l in blocks)
    course_dl = {}; course_len = {}
    for c in vtx_block:
        vb = vtx_block[c]; poff = (vb + block_len[vb][0] + 3) & ~3
        nxt = min([s for s in sorted_blocks if s > poff] + [poff + 131072])
        course_dl[c] = poff; course_len[c] = min(nxt - poff, 131072)
    packed_off = {c: 0 for c in course_dl}
    used_blocks = sorted(set(r[5] for r in recipes if r[3] == 'MIO0'))
    js = {
        'region_size': size,
        'patterns': [list(p) for p, pid in sorted(patterns.items(), key=lambda kv: kv[1])],
        'recipes': [{'name': n, 'off': o, 'size': sz, 'kind': k, 'xform': x, 'src': src,
                     'extra': (extra.hex() if k == 'LITERAL' else extra)} for n, o, sz, k, x, src, extra in recipes],
        'courses': [{'idx': cidx, 'name': cname, 'unpacked_len': end, 'rom_off': course_dl[cname], 'rom_len': course_len[cname], 'packed_off': 0} for cidx, cname, end in sorted(courses)],
        'blocks': [{'rom_off': o, 'rom_len': block_len[o][0], 'decomp_len': block_len[o][1]} for o in used_blocks],
    }
    json.dump(js, open(a.o, 'w'))
    print('wrote %s: %d recipes, %d patterns, %d courses, %d mio0 blocks (max decomp %d)' % (
        a.o, len(recipes), len(patterns), len(courses), len(used_blocks), max(block_len[o][1] for o in used_blocks)))

if __name__ == '__main__':
    main()
