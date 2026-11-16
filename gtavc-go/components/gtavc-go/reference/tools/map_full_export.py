#!/usr/bin/env python3
"""Full Vice City map (Blender OBJ, 1 unit = 1 m, Y up) -> streamed tiles for the J2ME engine.

  python3 tools/map_full_export.py "assets/map_full/GTA Vice City Map Full.obj"

Output (res/):
  mapidx.bin     tile grid, texture count, radar, spawn, lighthouse lamp
  t/<i>_<j>.bin  one 100 m tile: near mesh, far mesh (clustered), ground grids (2 layers), surface, solid bits, water
  x/<n>.png      32 px textures, loaded on demand by the game
Units: positions mm (1/1024 m) in headers, 1/64 m in meshes (relative to the tile centre).
"""
import os, sys, re, math, struct
import numpy as np
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
import obj_cache

ROOT = os.path.join(HERE, '..')
LEGACY = os.environ.get('LEGACY') == '1'
RES = os.path.join(ROOT, 'res_legacy' if LEGACY else 'res')
LEG_REGION = (20, 7, 22, 9)          # tile box around the Ocean View Hotel (Ocean Drive)
SRC = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'assets', 'map_full', 'GTA Vice City Map Full.obj')
TEXDIR = os.path.join(os.path.dirname(SRC), 'textures')
WATERTEX = os.path.join(ROOT, 'assets', 'particles', 'waterclear256.png')

WATER = 6.0          # sea level (GTA z)
TILE = 100.0
TEX = 32
MAX_UV = 7.5         # repeats per triangle at 32 px (renderer limit 255 texels)
MAX_EDGE = 60.0
HC = 2.0             # ground grid cell
SC = 1.0             # solid grid cell
NH = int(TILE / HC)  # 50
NS = int(TILE / SC)  # 100
RCELL = 8.0          # radar pixel
SPAWN_OBJ = 'wshbuildws09'
DROP = re.compile(r'^(od_lightint|od_bighotsign01)', re.I)
F_DOUBLE, F_EMISSIVE, F_WATER, F_NIGHT, F_DAY = 1, 2, 4, 8, 128
NEON = re.compile(r'neon', re.I)

SURF_ROAD, SURF_PAVE, SURF_GRASS, SURF_SAND, SURF_ROCK = 4, 3, 2, 1, 5
def surf_of(t):
    t = t.lower()
    if re.search(r'road|tarmac|asphalt|junction|crossing|hiway|highway|freeway|strip|whiteroadline|yellowline|lane', t): return SURF_ROAD
    if re.search(r'sand|beach', t): return SURF_SAND
    if re.search(r'grass|veg|lawn|bush|hedge|plant|leaf|mud|dirt', t): return SURF_GRASS
    if re.search(r'rock|cliff|seawall|stone', t): return SURF_ROCK
    return SURF_PAVE


def main():
    d = obj_cache.load(SRC)
    mtl = obj_cache.load_mtl(os.path.splitext(SRC)[0] + '.mtl')
    V, VT, FV, FT, FM, FO, names, mats = d['V'], d['VT'], d['FV'], d['FT'], d['FM'], d['FO'], d['names'], d['mats']
    W = np.stack([V[:, 0], V[:, 1] - WATER, -V[:, 2]], 1).astype(np.float64)          # engine metres
    base = np.array([re.sub(r'\.\d+$', '', n) for n in names], dtype=object)
    keepo = np.array([not DROP.match(n) for n in base])
    keep = keepo[FO]
    FV, FT, FM, FO = FV[keep], FT[keep], FM[keep], FO[keep]
    print('faces', len(FV))

    # ---- materials -> global texture ids
    mtex = [(mtl.get(m) or {}).get('tex') for m in mats]
    malpha = np.array([(mtl.get(m) or {}).get('alpha', False) for m in mats] + [False])
    mkd = np.array([(mtl.get(m) or {}).get('kd', (1, 1, 1)) for m in mats] + [(1, 1, 1)])
    texnames, tid_of = [], {}
    mtid = np.full(len(mats) + 1, -1, np.int32)
    for i, t in enumerate(mtex):
        if not t or not os.path.exists(os.path.join(TEXDIR, t)): continue
        k = t.lower()
        if k not in tid_of: tid_of[k] = len(texnames); texnames.append(t)
        mtid[i] = tid_of[k]
    wtid = len(texnames); texnames.append('__water__')
    msurf = np.array([surf_of(t or '') for t in mtex] + [SURF_PAVE], np.uint8)
    ftid = mtid[FM]; falpha = malpha[FM]; fsurf = msurf[FM]
    # VC time objects: *_nt only at night (lit windows, emissive), their *_dy twin only by day; neon always glows
    bl = [n.lower() for n in base]
    names_set = set(bl)
    oflag = np.zeros(len(names), np.uint8)
    for i, n in enumerate(bl):
        f = 0
        if n.endswith('_nt'): f |= F_NIGHT | F_EMISSIVE
        elif n.endswith('_dy') and (n[:-3] + '_nt') in names_set: f |= F_DAY
        if NEON.search(n): f |= F_EMISSIVE
        oflag[i] = f
    tneon = np.array([bool(t and NEON.search(t)) for t in mtex] + [False])
    fobj = oflag[FO] | np.where(tneon[FM], F_EMISSIVE, 0).astype(np.uint8)
    # neon colours: the OBJ lost the vertex colours, so take them from the DFF prelight in gta3.img (pink / blue tubes)
    global FCOL
    FCOL = np.full((len(FV), 3), -1, np.int16)
    img = os.environ.get('IMG')
    if img and os.path.exists(img):
        import img_extract, rwtools
        ents = img_extract.read_dir(img); fimg = open(img, 'rb')
        Vidx = None
        for i in np.where([bool(NEON.search(n)) for n in bl])[0]:
            key = bl[i] + '.dff'
            if key not in ents: continue
            off, sz = ents[key]; fimg.seek(off * 2048); open('/tmp/_neon.dff', 'wb').write(fimg.read(sz * 2048))
            try: g = rwtools.read_dff('/tmp/_neon.dff')['geoms'][0]
            except Exception: continue
            pre = g.get('prelit')
            if pre is None: continue
            pre = pre[:, :3].astype(int)
            sel = np.where(FO == i)[0]
            if len(sel) == 0: continue
            vi = np.unique(FV[sel])
            if len(vi) == len(pre):                                     # same vertex order as the DFF
                lut = {int(v): k for k, v in enumerate(vi)}
                for k in sel: FCOL[k] = pre[[lut[int(v)] for v in FV[k]]].mean(0)
            else:
                FCOL[sel] = pre.mean(0)
        print('neon colours for', int((FCOL[:, 0] >= 0).sum()), 'faces')
    fobj = np.where(FCOL[:, 0] >= 0, fobj | 16 | F_EMISSIVE, fobj).astype(np.uint8)

    # ---- object size (for LOD)
    P0 = W[FV[:, 0]]
    omin = np.full((len(names), 3), 1e9); omax = np.full((len(names), 3), -1e9)
    for k in range(3):
        for c in range(3):
            np.minimum.at(omin[:, c], FO, W[FV[:, k], c]); np.maximum.at(omax[:, c], FO, W[FV[:, k], c])
    odiag = np.linalg.norm(omax - omin, axis=1)
    fdiag = odiag[FO]
    small = fdiag < 12.0

    # ---- geometry per face
    A, Bv, C = W[FV[:, 0]], W[FV[:, 1]], W[FV[:, 2]]
    N = np.cross(Bv - A, C - A); NL = np.linalg.norm(N, axis=1) + 1e-12
    ny = N[:, 1] / NL
    cen = (A + Bv + C) / 3
    ti = np.floor(cen[:, 0] / TILE).astype(int); tj = np.floor(cen[:, 2] / TILE).astype(int)
    TI0, TJ0 = ti.min(), tj.min()
    NX, NZ = ti.max() - TI0 + 1, tj.max() - TJ0 + 1
    VTf = VT[np.clip(FT, 0, len(VT) - 1)]
    VTf[FT < 0] = 0
    print('tile grid', NX, 'x', NZ)

    # ---- ground grids (whole map, 2 m) : top + low walkable layer   (cached: the conversion can be resumed)
    GX0, GZ0 = TI0 * TILE, TJ0 * TILE
    GW, GH = NX * NH, NZ * NH
    gcache = os.path.splitext(SRC)[0] + '_grids.npz'
    if os.path.exists(gcache):
        z = np.load(gcache); top, low, surf, has, solid = z['top'], z['low'], z['surf'], z['has'], z['solid']
        print('grids from cache')
    else:
        top, low, surf, has, solid = grids(A, Bv, C, cen, ny, small, fsurf, GX0, GZ0, GW, GH, NX, NZ)
        np.savez(gcache, top=top, low=low, surf=surf, has=has, solid=solid)
    texture_out = not os.path.exists(os.path.join(RES, 'x', '%d.png' % (len(texnames) - 1)))
    SW, SH = NX * NS, NZ * NS
    if texture_out: write_textures(texnames)
    tiles_out(texnames, wtid, A, Bv, C, VTf, ftid, falpha, mkd, FM, small, fdiag, ny, ti, tj, TI0, TJ0, NX, NZ, top, low, surf, has, solid, GX0, GZ0, fobj)
    finalize(texnames, base, FO, A, Bv, C, cen, ftid, top, low, surf, has, solid, GX0, GZ0, GW, GH, NX, NZ)


def grids(A, Bv, C, cen, ny, small, fsurf, GX0, GZ0, GW, GH, NX, NZ):
    top = np.full((GH, GW), -1e9, np.float32); low = np.full((GH, GW), 1e9, np.float32)
    surf = np.zeros((GH, GW), np.uint8)
    walk = np.where(np.abs(ny) > 0.55)[0]
    for k in walk:
        a, b, c = A[k], Bv[k], C[k]
        x0 = min(a[0], b[0], c[0]); x1 = max(a[0], b[0], c[0]); z0 = min(a[2], b[2], c[2]); z1 = max(a[2], b[2], c[2])
        i0 = int(math.floor((x0 - GX0) / HC)); i1 = int(math.floor((x1 - GX0) / HC)); j0 = int(math.floor((z0 - GZ0) / HC)); j1 = int(math.floor((z1 - GZ0) / HC))
        if small[k] and (x1 - x0) * (z1 - z0) < 1.0: continue          # tiny props (lamp tops etc.) are not floors
        I, J = np.meshgrid(np.arange(i0, i1 + 1), np.arange(j0, j1 + 1))
        px = GX0 + (I + 0.5) * HC; pz = GZ0 + (J + 0.5) * HC
        den = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
        if abs(den) < 1e-9: continue
        l1 = ((b[2] - c[2]) * (px - c[0]) + (c[0] - b[0]) * (pz - c[2])) / den
        l2 = ((c[2] - a[2]) * (px - c[0]) + (a[0] - c[0]) * (pz - c[2])) / den
        l3 = 1 - l1 - l2
        ins = (l1 >= -0.05) & (l2 >= -0.05) & (l3 >= -0.05)
        if not ins.any():
            I = np.array([[min(max(int((cen[k][0] - GX0) / HC), 0), GW - 1)]]); J = np.array([[min(max(int((cen[k][2] - GZ0) / HC), 0), GH - 1)]])
            y = np.array([[cen[k][1]]]); ins = np.ones((1, 1), bool)
        else:
            y = l1 * a[1] + l2 * b[1] + l3 * c[1]
        I, J, y = I[ins], J[ins], y[ins]
        ok = (I >= 0) & (J >= 0) & (I < GW) & (J < GH)
        I, J, y = I[ok], J[ok], y[ok]
        bt = y > top[J, I]
        top[J[bt], I[bt]] = y[bt]; surf[J[bt], I[bt]] = fsurf[k]
        bl = y < low[J, I]
        low[J[bl], I[bl]] = y[bl]
    has = top > -1e8
    low[~has] = -1e9
    surf[~has] = 0
    surf[has & (top < -0.3)] = 0                                # sand under the sea
    print('ground cells', int(has.sum()))

    # solid cells: thin vertical things standing on the ground (lamps, palms, poles, fences, walls)
    SW, SH = NX * NS, NZ * NS
    solid = np.zeros((SH, SW), bool)
    steep = np.where(np.abs(ny) < 0.5)[0]
    for k in steep:
        a, b, c = A[k], Bv[k], C[k]
        L = max(np.linalg.norm(b - a), np.linalg.norm(c - a), np.linalg.norm(c - b))
        n = int(min(60, max(2, L / 0.5)))
        u, v = np.meshgrid(np.linspace(0, 1, n), np.linspace(0, 1, n)); m = (u + v) <= 1
        Pp = a[None] + u[m][:, None] * (b - a)[None] + v[m][:, None] * (c - a)[None]
        gi = np.clip(((Pp[:, 0] - GX0) / HC).astype(int), 0, GW - 1); gj = np.clip(((Pp[:, 2] - GZ0) / HC).astype(int), 0, GH - 1)
        g = np.where(has[gj, gi], low[gj, gi], -50)
        # only things that stand on the ground block (poles, palms, walls) - not awnings/balconies hanging over the street
        if min(a[1], b[1], c[1]) > g.min() + 0.6: continue
        hit = (Pp[:, 1] > g + 0.4) & (Pp[:, 1] < g + 2.4)
        if not hit.any(): continue
        si = np.clip(((Pp[hit, 0] - GX0) / SC).astype(int), 0, SW - 1); sj = np.clip(((Pp[hit, 2] - GZ0) / SC).astype(int), 0, SH - 1)
        solid[sj, si] = True
    print('solid cells', int(solid.sum()))
    return top, low, surf, has, solid


def write_textures(texnames):
    os.makedirs(os.path.join(RES, 'x'), exist_ok=True)
    for i, t in enumerate(texnames):
        src = WATERTEX if t == '__water__' else os.path.join(TEXDIR, t)
        im = Image.open(src).convert('RGBA').resize((TEX, TEX), Image.LANCZOS)
        a = np.array(im)
        if t != '__water__' and a[..., 3].min() < 250: a[..., 3] = np.where(a[..., 3] > 110, 255, 0)
        else: a[..., 3] = 255
        Image.fromarray(a, 'RGBA').save(os.path.join(RES, 'x', '%d.png' % i), optimize=True)
    print('textures', len(texnames))


def tiles_out(texnames, wtid, A, Bv, C, VTf, ftid, falpha, mkd, FM, small, fdiag, ny, ti, tj, TI0, TJ0, NX, NZ, top, low, surf, has, solid, GX0, GZ0, fobj):
    global FOBJ
    FOBJ = fobj
    os.makedirs(os.path.join(RES, 't'), exist_ok=True)
    order = np.lexsort((tj, ti))
    tri_near = tri_far = 0
    tile_flags = np.zeros((NZ, NX), np.uint8)
    ks = ti * 100000 + tj
    sk = ks[order]
    bounds = np.flatnonzero(np.diff(sk)) + 1
    groups = np.split(order, bounds)
    gmap = {(int(ti[g[0]]), int(tj[g[0]])): g for g in groups}
    for J in range(NZ):
        for I in range(NX):
            tx, tz = I + TI0, J + TJ0
            if LEGACY and not (LEG_REGION[0] <= I <= LEG_REGION[2] and LEG_REGION[1] <= J <= LEG_REGION[3]): continue
            g = gmap.get((tx, tz), np.zeros(0, int))
            g = g[fdiag[g] > 2.0]                                    # tiny clutter is not worth the bytes
            if os.path.exists(os.path.join(RES, 't', '%d_%d.bin' % (I, J))): tile_flags[J, I] = 1; continue
            ox, oz = (tx + 0.5) * TILE, (tz + 0.5) * TILE
            hb = slice(J * NH, (J + 1) * NH), slice(I * NH, (I + 1) * NH)
            tTop, tLow, tSurf = top[hb], low[hb], surf[hb]
            tHas = has[hb]
            water = water_faces(tTop, tHas, I, J, GX0, GZ0, ox, oz)
            if len(g) == 0 and not water and not tHas.any(): continue
            if LEGACY: g = g[(fdiag[g] > 4.0) & ((FOBJ[g] & F_NIGHT) == 0)]
            near, ntex_n = build_mesh(g, A, Bv, C, VTf, ftid, falpha, mkd, FM, small, ox, oz, cluster=0.7 if LEGACY else None, extra=water, wtid=wtid)
            for cl in (0.3, 0.6, 1.2):
                if len(near[0]) < 60000 and near[1] < 60000: break
                print('tile', I, J, 'too dense (%d verts), clustering %.1f m' % (len(near[0]), cl))
                near, ntex_n = build_mesh(g, A, Bv, C, VTf, ftid, falpha, mkd, FM, small, ox, oz, cluster=cl, extra=water, wtid=wtid)
            gfar = g[(fdiag[g] > 25.0) & (np.abs(ny[g]) > 0.25) | (fdiag[g] > 50.0)]
            if LEGACY: gfar = gfar[:0]
            far, _ = build_mesh(gfar, A, Bv, C, VTf, ftid, falpha, mkd, FM, small, ox, oz, cluster=4.0, extra=[] if LEGACY else water, wtid=wtid)
            tri_near += near[1]; tri_far += far[1]
            out = bytearray(b'T1')
            texs = sorted(set(near[2]) | set(far[2]))
            loc = {t: n for n, t in enumerate(texs)}
            out += struct.pack('>H', len(texs)) + b''.join(struct.pack('>H', t) for t in texs)
            out += mesh_bytes(near, loc) + mesh_bytes(far, loc)
            q = lambda h: np.where(h < -1e8, -32768, np.clip(np.round(h * 64), -32767, 32767)).astype('>i2').tobytes()
            out += q(tTop) + q(tLow) + tSurf.astype(np.uint8).tobytes()
            sb = solid[J * NS:(J + 1) * NS, I * NS:(I + 1) * NS]
            out += np.packbits(sb.astype(np.uint8), axis=1).tobytes()
            open(os.path.join(RES, 't', '%d_%d.bin' % (I, J)), 'wb').write(out)
            tile_flags[J, I] = 1
    print('tris near', tri_near, 'far', tri_far, 'tiles', int(tile_flags.sum()))
    np.save(os.path.join(RES, '..', 'build_tileflags%s.npy' % ('_legacy' if LEGACY else '')), tile_flags)


def finalize(texnames, base, FO, A, Bv, C, cen, ftid, top, low, surf, has, solid, GX0, GZ0, GW, GH, NX, NZ):
    tile_flags = np.load(os.path.join(RES, '..', 'build_tileflags%s.npy' % ('_legacy' if LEGACY else '')))
    SW, SH = NX * NS, NZ * NS

    # ---- radar (8 m) from surfaces + building footprints (top much higher than low ground)
    RW, RH = int(NX * TILE / RCELL), int(NZ * TILE / RCELL)
    f = int(RCELL / HC)
    sub = surf[:RH * f, :RW * f].reshape(RH, f, RW, f)
    rad = np.zeros((RH, RW), np.uint8)
    for cls in (SURF_SAND, SURF_GRASS, SURF_PAVE, SURF_ROCK, SURF_ROAD):
        cnt = (sub == cls).sum(axis=(1, 3))
        rad[cnt >= (f * f) // 3] = cls
    tsub = top[:RH * f, :RW * f].reshape(RH, f, RW, f).max(axis=(1, 3))
    rad[(tsub > 9.0) & (rad != SURF_ROAD)] = 6

    # ---- spawn: the alley beside wshbuildws09 (Ken Rosenberg's office)
    oid = [i for i, n in enumerate(base) if n.lower() == SPAWN_OBJ]
    sel = np.isin(FO, oid)
    bp = np.concatenate([A[sel], Bv[sel], C[sel]])
    bx0, bx1, bz0, bz1 = bp[:, 0].min(), bp[:, 0].max(), bp[:, 2].min(), bp[:, 2].max()
    best, bscore = None, -1
    for x in np.arange(bx0 - 14, bx1 + 14, 1.0):
        for z in np.arange(bz0 - 14, bz1 + 14, 1.0):
            inside = bx0 + 1 < x < bx1 - 1 and bz0 + 1 < z < bz1 - 1
            if inside: continue
            gi, gj = int((x - GX0) / HC), int((z - GZ0) / HC)
            if not has[gj, gi] or top[gj, gi] > 6.0 or surf[gj, gi] == SURF_ROAD or surf[gj, gi] == 0: continue
            si, sj = int((x - GX0) / SC), int((z - GZ0) / SC)
            if solid[sj - 1:sj + 2, si - 1:si + 2].any(): continue
            # alley: walls close on two opposite sides (step up of > 3 m within 7 m), but open along the alley
            walls = 0
            for dx, dz in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                for r in range(1, 5):
                    hi, hj = gi + dx * r, gj + dz * r
                    if 0 <= hi < GW and 0 <= hj < GH and top[hj, hi] > top[gj, gi] + 3.0: walls += 1; break
            if walls < 2: continue
            dbx = min(abs(x - bx0), abs(x - bx1), abs(z - bz0), abs(z - bz1))
            score = walls * 10 - dbx
            if score > bscore: bscore, best = score, (x, z)
    if best is None: best = (bx0 - 4, (bz0 + bz1) / 2)
    sx, sz = best
    if LEGACY:                                    # Ocean View Hotel, Ocean Drive
        cands = []
        for x in np.arange(170, 260, 1.0):
            for z in np.arange(-1340, -1240, 1.0):
                gi, gj = int((x - GX0) / HC), int((z - GZ0) / HC)
                if has[gj, gi] and surf[gj, gi] == SURF_PAVE and 0 < top[gj, gi] < 10 and not solid[int((z - GZ0) / SC), int((x - GX0) / SC)]:
                    cands.append((abs(x - 215) + abs(z + 1290), x, z))
        if cands: _, sx, sz = min(cands)
    gi, gj = int((sx - GX0) / HC), int((sz - GZ0) / HC)
    sy = top[gj, gi]
    # face along the alley towards the open street
    yaw, longest = 0, -1
    for a in range(0, 1024, 32):
        ang = a / 1024 * 2 * math.pi; L = 0
        for r in range(1, 40):
            x, z = sx + math.sin(ang) * r, sz + math.cos(ang) * r
            hi, hj = int((x - GX0) / HC), int((z - GZ0) / HC)
            if not (0 <= hi < GW and 0 <= hj < GH) or top[hj, hi] > sy + 1.0: break
            L = r
        if L > longest: longest, yaw = L, a
    print('spawn', sx, sz, sy, 'yaw', yaw, 'building', (bx0, bx1, bz0, bz1))

    # lighthouse lamp (lens faces)
    lens = [i for i, t in enumerate(texnames) if t.lower().startswith(('lightlens', 'lightglass'))]
    lm = np.isin(ftid, lens)
    lp = cen[lm].mean(0) if lm.any() else np.array([0, -100, 0])

    out = bytearray(b'VCX1')
    out += struct.pack('>iiHHH', int(GX0 * 1024), int(GZ0 * 1024), int(TILE), NX, NZ)
    out += np.packbits(tile_flags.astype(np.uint8)).tobytes()
    out += struct.pack('>H', len(texnames))
    out += struct.pack('>iiHHH', int(GX0 * 1024), int(GZ0 * 1024), int(RCELL * 1024), RW, RH) + rad.tobytes()
    out += struct.pack('>iiiH', int(sx * 1024), int(sz * 1024), int(sy * 64), yaw)
    out += struct.pack('>iii', int(lp[0] * 1024), int(lp[1] * 64), int(lp[2] * 1024))
    open(os.path.join(RES, 'mapidx.bin'), 'wb').write(out)
    print('mapidx', len(out), 'bytes')


def water_faces(tTop, tHas, I, J, GX0, GZ0, ox, oz):
    """20 m water quads wherever the ground is missing or under the sea"""
    out = []
    S = 20.0
    n = int(TILE / S); f = int(S / HC)
    for b in range(n):
        for a in range(n):
            blk = tTop[b * f:(b + 1) * f, a * f:(a + 1) * f]; hb = tHas[b * f:(b + 1) * f, a * f:(a + 1) * f]
            if hb.all() and (blk > 0.3).all(): continue
            x0 = ox - TILE / 2 + a * S; z0 = oz - TILE / 2 + b * S
            p = [np.array([x0, 0, z0]), np.array([x0 + S, 0, z0]), np.array([x0 + S, 0, z0 + S]), np.array([x0, 0, z0 + S])]
            u = [np.array([0., 0.]), np.array([2., 0.]), np.array([2., 2.]), np.array([0., 2.])]
            out.append((p[0], p[1], p[2], u[0], u[1], u[2]))
            out.append((p[0], p[2], p[3], u[0], u[2], u[3]))
    return out


def split(p, u, textured, depth=0):
    """edge-consistent subdivision (no T-junctions): returns list of (p3, u3)"""
    if depth >= 6: return [(p, u)]
    def es(i, j):
        if np.linalg.norm(p[j] - p[i]) > MAX_EDGE: return True
        return textured and np.abs(u[j] - u[i]).max() > MAX_UV
    s = [es(0, 1), es(1, 2), es(2, 0)]
    n = sum(s)
    if n == 0: return [(p, u)]
    m = lambda a, b: (a + b) / 2
    if n == 3:
        pab, pbc, pca = m(p[0], p[1]), m(p[1], p[2]), m(p[2], p[0]); uab, ubc, uca = m(u[0], u[1]), m(u[1], u[2]), m(u[2], u[0])
        parts = [((p[0], pab, pca), (u[0], uab, uca)), ((pab, p[1], pbc), (uab, u[1], ubc)), ((pca, pbc, p[2]), (uca, ubc, u[2])), ((pab, pbc, pca), (uab, ubc, uca))]
    else:
        k = s.index(True) if n == 1 else [i for i in range(3) if s[i] and s[(i + 1) % 3]][0]
        a, b, c = p[k], p[(k + 1) % 3], p[(k + 2) % 3]; A, B, C = u[k], u[(k + 1) % 3], u[(k + 2) % 3]
        pab, uab = m(a, b), m(A, B)
        if n == 1: parts = [((a, pab, c), (A, uab, C)), ((pab, b, c), (uab, B, C))]
        else:
            pbc, ubc = m(b, c), m(B, C)
            parts = [((a, pab, c), (A, uab, C)), ((pab, b, pbc), (uab, B, ubc)), ((pab, pbc, c), (uab, ubc, C))]
    out = []
    for q, w in parts: out += split(np.array(q), np.array(w), textured, depth + 1)
    return out


def build_mesh(g, A, Bv, C, VTf, ftid, falpha, mkd, FM, small, ox, oz, cluster, extra, wtid):
    P = np.stack([A[g], Bv[g], C[g]], 1)                      # (n,3,3)
    U = VTf[g].astype(np.float64).copy()                       # (n,3,2)
    U[:, :, 1] = -U[:, :, 1]
    tex = ftid[g]; alpha = falpha[g]; kd = mkd[FM[g]].copy(); ofl = FOBJ[g] if len(g) else np.zeros(0, np.uint8)
    if len(g):
        fc = FCOL[g]; hasc = fc[:, 0] >= 0
        kd[hasc] = np.clip(fc[hasc] / 255.0 * 1.15, 0, 1)
    if cluster:
        Q = np.round(P / cluster) * cluster
        e1 = np.linalg.norm(Q[:, 1] - Q[:, 0], axis=1); e2 = np.linalg.norm(Q[:, 2] - Q[:, 1], axis=1); e3 = np.linalg.norm(Q[:, 0] - Q[:, 2], axis=1)
        ok = (e1 > 1e-6) & (e2 > 1e-6) & (e3 > 1e-6)
        area = np.linalg.norm(np.cross(Q[:, 1] - Q[:, 0], Q[:, 2] - Q[:, 0]), axis=1)
        ok &= area > cluster * cluster * 0.3
        P, U, tex, alpha, kd, ofl = Q[ok], U[ok], tex[ok], alpha[ok], kd[ok], ofl[ok]
    else:
        # density cut: small props snap to 0.4 m, everything else to 0.15 m (drops sliver/detail triangles)
        q = np.where(small[g], 0.5, 0.3)[:, None, None]
        P = np.round(P / q) * q
        area = np.linalg.norm(np.cross(P[:, 1] - P[:, 0], P[:, 2] - P[:, 0]), axis=1)
        e = np.minimum(np.minimum(np.linalg.norm(P[:, 1] - P[:, 0], axis=1), np.linalg.norm(P[:, 2] - P[:, 1], axis=1)), np.linalg.norm(P[:, 0] - P[:, 2], axis=1))
        ok = (area > 0.02) & (e > 1e-6)
        P, U, tex, alpha, kd, ofl = P[ok], U[ok], tex[ok], alpha[ok], kd[ok], ofl[ok]
    faces = []
    du = np.abs(U[:, :, None, :] - U[:, None, :, :]).max(axis=(1, 2, 3))
    dp = np.linalg.norm(P[:, :, None, :] - P[:, None, :, :], axis=3).max(axis=(1, 2))
    for k in range(len(P)):
        t = int(tex[k]); fl = (F_DOUBLE if alpha[k] else 0) | int(ofl[k])
        rgb = tuple(int(c * 255) for c in kd[k])
        if dp[k] <= MAX_EDGE and (t < 0 or du[k] <= MAX_UV):
            faces.append((P[k], U[k], t, fl, rgb))
        else:
            if t >= 0 and du[k] > MAX_UV * 24:                     # absurd tiling: stretch the texture instead of exploding
                U[k] = U[k].mean(0) + (U[k] - U[k].mean(0)) * (MAX_UV * 24 / du[k])
            for q, w in split(P[k], U[k], t >= 0): faces.append((q, w, t, fl, rgb))
    for e in extra:
        faces.append((np.array(e[:3]), np.array(e[3:]), wtid, F_WATER, (255, 255, 255)))
    verts, vid, tris, used = [], {}, [], set()
    for p, u, t, fl, rgb in faces:
        uu = u - np.floor(u.min(0)) if t >= 0 else np.zeros((3, 2))
        idx = []
        for c in range(3):
            key = (int(round((p[c][0] - ox) * 64)), int(round(p[c][1] * 64)), int(round((p[c][2] - oz) * 64)),
                   int(round(uu[c][0] * TEX * 16)), int(round(uu[c][1] * TEX * 16)))
            if key not in vid: vid[key] = len(verts); verts.append(key)
            idx.append(vid[key])
        if len(set(idx)) < 3: continue
        tris.append((idx[0], idx[2], idx[1], t, fl, rgb))       # mirror -> swap winding
        if t >= 0: used.add(t)
    return (verts, len(tris), used, tris), len(used)


def mesh_bytes(m, loc):
    verts, nt, used, tris = m
    b = bytearray(struct.pack('>H', len(verts)))
    for v in verts: b += struct.pack('>hhhhh', *[max(-32768, min(32767, x)) for x in v])
    b += struct.pack('>H', len(tris))
    for t in tris:
        b += struct.pack('>HHHHB', t[0], t[1], t[2], loc[t[3]] if t[3] >= 0 else 0xFFFF, t[4]) + bytes(t[5])
    return b


if __name__ == '__main__':
    main()
