#!/usr/bin/env python3
"""OBJ map piece -> res/map.bin + res/t_m_*.png for the J2ME engine.

Usage: python3 tools/map_export.py [assets/map/gtavc_beacpart.obj]

What it does
  * OBJ (Blender, Y up, scaled 1/100) -> engine units (1/64 m, x right, y up, z forward)
  * drops night duplicates (_nt), interiors, shadow decals and the underwater sea bed
  * splits big triangles (fog gradient + the renderer's 255 texel UV span limit)
  * buckets triangles into CHUNK x CHUNK metre chunks (frustum / distance culling)
  * bakes a ground height field, a surface-type grid (road / pavement / grass / sand / rock),
    a solid-obstacle grid (palms, lamps, lighthouse ...), the radar image and spawn points
Everything is written big-endian so DataInputStream can read it on the phone.
"""
import os, sys, struct, math, re
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..')
RES = os.path.join(ROOT, 'res')
SRC = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'assets', 'map', 'gtavc_beacpart.obj')
TEXDIR = os.path.join(os.path.dirname(SRC), 'textures')

SCALE = 100.0            # OBJ unit -> metres
WATER_Y = 1.6            # OBJ height of the sea surface
SPAWN_OBJ = 'od_oceanrdN1_dy_DFF'
TEX = 64                 # runtime texture size (power of two)
CHUNK = 64.0             # metres
MAX_EDGE = 40.0          # metres
MAX_UV = 3.8             # texture repeats per triangle (255 texel limit at 64 px)
HCELL = 2.0              # height/surface grid cell (m)
SCELL = 1.0              # solid grid cell (m)
RCELL = 4.0              # radar pixel (m)

DROP = re.compile(r'(_nt_DFF|od_lightint|treeshadws|cdseabed|^water$)')
PROP = re.compile(r'^(veg_|Streetlamp|bouy|new_bush|plants|od_lightext)')
NONSOLID_PROP = re.compile(r'^(new_bush|plants)')
EMISSIVE_TEX = {'lightlens', 'lightglass'}

SURF = {'road': 4, 'pave': 3, 'grass': 2, 'sand': 1, 'rock': 5}
def surf_of(tex):
    t = (tex or '').lower()
    if t.startswith(('road2', 'whiteroadline')): return SURF['road']
    if t.startswith(('kbpavement_testconc', 'conc_slab', 'heliconcrete', 'kbpath', 'walkway2', 'alleywall')): return SURF['pave']
    if t.startswith(('whitesand', 'beachtyre', 'grass2sand')): return SURF['sand']
    if t.startswith(('rock', 'kbseawall', 'gravelkb')): return SURF['rock']
    return SURF['grass']

def tname(path):
    b = os.path.splitext(os.path.basename(path))[0].lower()
    return 'm_' + re.sub(r'[^a-z0-9]+', '_', b).strip('_')

# ------------------------------------------------------------------ load
def load():
    mtl, cur = {}, None
    for l in open(os.path.splitext(SRC)[0] + '.mtl', encoding='latin1'):
        s = l.split()
        if not s: continue
        if s[0] == 'newmtl': cur = s[1]; mtl[cur] = {'kd': (1, 1, 1), 'tex': None, 'alpha': False}
        elif s[0] == 'Kd': mtl[cur]['kd'] = tuple(float(x) for x in s[1:4])
        elif s[0] == 'map_Kd': mtl[cur]['tex'] = ' '.join(s[1:]).replace('\\', '/').split('/')[-1]
        elif s[0] == 'map_d': mtl[cur]['alpha'] = True
    V, VT, tris = [], [], []
    obj, cm = None, None
    for l in open(SRC, encoding='latin1'):
        if l.startswith('v '): V.append([float(x) for x in l.split()[1:4]])
        elif l.startswith('vt '): VT.append([float(x) for x in l.split()[1:3]])
        elif l.startswith('o '): obj = l[2:].strip()
        elif l.startswith('usemtl'): cm = l.split(None, 1)[1].strip()
        elif l.startswith('f '):
            p = [t.split('/') for t in l.split()[1:]]
            vi = [int(a[0]) - 1 for a in p]
            ti = [int(a[1]) - 1 if len(a) > 1 and a[1] else -1 for a in p]
            for k in range(1, len(vi) - 1):
                tris.append((obj, cm, (vi[0], vi[k], vi[k + 1]), (ti[0], ti[k], ti[k + 1])))
    return mtl, np.array(V), np.array(VT) if VT else np.zeros((1, 2)), tris

mtl, V, VT, TRIS = load()
cx_o, cz_o = None, None
sp = [t for t in TRIS if t[0] == SPAWN_OBJ]
P0 = V[[i for t in sp for i in t[2]]]
cx_o, cz_o = P0[:, 0].mean(), P0[:, 2].mean()

def to_world(p):
    """OBJ -> metres in engine axes (x east, y up, z north), origin at the spawn road"""
    return np.stack([(p[..., 0] - cx_o) * SCALE, (p[..., 1] - WATER_Y) * SCALE, -(p[..., 2] - cz_o) * SCALE], -1)

W = to_world(V)

# ------------------------------------------------------------------ textures
texnames, texsrc = [], {}
def tex_id(fname):
    if fname is None: return -1
    n = tname(fname)
    if n not in texnames:
        texnames.append(n); texsrc[n] = fname
    return texnames.index(n)

# ------------------------------------------------------------------ triangles (metres, uv)
faces = []     # (pa, pb, pc, uva, uvb, uvc, tex, flags, rgb, obj)
seabed = []    # sea floor triangles: only used to tell open sea from the unfinished edge of the map piece
F_DOUBLE, F_EMISSIVE, F_WATER = 1, 2, 4
for obj, m, vi, ti in TRIS:
    if (obj or '').startswith('cdseabed'): seabed.append([W[i] for i in vi])
    if DROP.search(obj or ''): continue
    mm = mtl.get(m, {'kd': (1, 1, 1), 'tex': None, 'alpha': False})
    p = [W[i] for i in vi]
    uv = [VT[i] if i >= 0 else np.zeros(2) for i in ti]
    t = tex_id(mm['tex'])
    fl = 0
    if mm['alpha'] or PROP.match(obj or ''): fl |= F_DOUBLE if mm['alpha'] else 0
    if mm['tex'] and tname(mm['tex'])[2:] in EMISSIVE_TEX: fl |= F_EMISSIVE
    rgb = tuple(int(c * 255) for c in mm['kd'])
    faces.append((p[0], p[1], p[2], uv[0], uv[1], uv[2], t, fl, rgb, obj))

# water tiles (generated, cheap): 40 m tiles over the whole map footprint
allp = W[[i for t in TRIS for i in t[2]]]
X0, X1 = allp[:, 0].min() - 60, allp[:, 0].max() + 60
Z0, Z1 = allp[:, 2].min() - 60, allp[:, 2].max() + 60
wt = tex_id('waterclear256.png')

def edge_split(pa, pb, ua, ub, textured):
    """split decision depends only on the edge itself, so both neighbours agree (no T-junction cracks)"""
    if np.linalg.norm(pb - pa) > MAX_EDGE: return True
    return textured and np.abs(ub - ua).max() > MAX_UV

def split(f):
    pa, pb, pc, ua, ub, uc = f[:6]
    tx = f[6] >= 0
    s0, s1, s2 = edge_split(pa, pb, ua, ub, tx), edge_split(pb, pc, ub, uc, tx), edge_split(pc, pa, uc, ua, tx)
    n = s0 + s1 + s2
    if n == 0: return [f]
    m = lambda a, b: (a + b) / 2
    rest = f[6:]
    if n == 3:
        pab, pbc, pca = m(pa, pb), m(pb, pc), m(pc, pa); uab, ubc, uca = m(ua, ub), m(ub, uc), m(uc, ua)
        parts = [(pa, pab, pca, ua, uab, uca), (pab, pb, pbc, uab, ub, ubc), (pca, pbc, pc, uca, ubc, uc), (pab, pbc, pca, uab, ubc, uca)]
    else:
        # rotate so the (first) split edge is a-b
        P, U, S = [pa, pb, pc], [ua, ub, uc], [s0, s1, s2]
        k = S.index(True) if n == 1 else [i for i in range(3) if S[i] and S[(i + 1) % 3]][0] if (S[0] and S[1]) or (S[1] and S[2]) or (S[2] and S[0]) else S.index(True)
        a, b, c = P[k], P[(k + 1) % 3], P[(k + 2) % 3]; A, B, C = U[k], U[(k + 1) % 3], U[(k + 2) % 3]
        pab, uab = m(a, b), m(A, B)
        if n == 1:
            parts = [(a, pab, c, A, uab, C), (pab, b, c, uab, B, C)]
        else:   # edges a-b and b-c split
            pbc, ubc = m(b, c), m(B, C)
            parts = [(a, pab, c, A, uab, C), (pab, b, pbc, uab, B, ubc), (pab, pbc, c, uab, ubc, C)]
    out = []
    for g in parts: out += split(g + rest)
    return out

# ------------------------------------------------------------------ grids (terrain = non-prop)
terrain = [f for f in faces if not PROP.match(f[9] or '')]
props = [f for f in faces if PROP.match(f[9] or '') and not NONSOLID_PROP.match(f[9] or '')]
tp = np.array([[f[0], f[1], f[2]] for f in terrain])
GX0, GX1 = tp[:, :, 0].min() - 4, tp[:, :, 0].max() + 4
GZ0, GZ1 = tp[:, :, 2].min() - 4, tp[:, :, 2].max() + 4
HW, HH = int(math.ceil((GX1 - GX0) / HCELL)) + 1, int(math.ceil((GZ1 - GZ0) / HCELL)) + 1
NONE = -1e9
height = np.full((HH, HW), NONE)
surf = np.zeros((HH, HW), np.uint8)

def raster(f, cell, x0, z0, w, h, cb):
    a, b, c = f[0], f[1], f[2]
    xs = [a[0], b[0], c[0]]; zs = [a[2], b[2], c[2]]
    i0 = max(0, int(math.floor((min(xs) - x0) / cell))); i1 = min(w - 1, int(math.ceil((max(xs) - x0) / cell)))
    j0 = max(0, int(math.floor((min(zs) - z0) / cell))); j1 = min(h - 1, int(math.ceil((max(zs) - z0) / cell)))
    if i1 < i0 or j1 < j0: return
    I, J = np.meshgrid(np.arange(i0, i1 + 1), np.arange(j0, j1 + 1))
    px = x0 + I * cell; pz = z0 + J * cell
    d = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
    if abs(d) < 1e-9: return
    l1 = ((b[2] - c[2]) * (px - c[0]) + (c[0] - b[0]) * (pz - c[2])) / d
    l2 = ((c[2] - a[2]) * (px - c[0]) + (a[0] - c[0]) * (pz - c[2])) / d
    l3 = 1 - l1 - l2
    e = -0.02
    ins = (l1 >= e) & (l2 >= e) & (l3 >= e)
    if not ins.any(): return
    y = l1 * a[1] + l2 * b[1] + l3 * c[1]
    cb(I[ins], J[ins], y[ins])

for f in terrain:
    n = np.cross(f[1] - f[0], f[2] - f[0]); ln = np.linalg.norm(n)
    if ln < 1e-9 or abs(n[1]) / ln < 0.45: continue          # walls are not floor
    s = surf_of(texsrc.get(texnames[f[6]]) if f[6] >= 0 else None)
    def cb(I, J, Y):
        better = Y > height[J, I]
        height[J[better], I[better]] = Y[better]
        surf[J[better], I[better]] = s
    raster(f, HCELL, GX0, GZ0, HW, HH, cb)

# beach sand that dips below the sea counts as water
under = height < -0.4
surf[under] = 0
# cells with neither land nor sea floor are the unfinished edge of this map piece: VOID (solid, never water death)
sea = np.zeros((HH, HW), bool)
for t in seabed:
    def cbs(I, J, Y): sea[J, I] = True
    raster((t[0], t[1], t[2]), HCELL, GX0, GZ0, HW, HH, cbs)
VOID = 7
void = (height < -1e8) & ~sea
surf[void] = VOID

SW, SH = int(math.ceil((GX1 - GX0) / SCELL)) + 1, int(math.ceil((GZ1 - GZ0) / SCELL)) + 1
solid = np.zeros((SH, SW), bool)
def ground_at(x, z):
    i = np.clip(((x - GX0) / HCELL).astype(int), 0, HW - 1); j = np.clip(((z - GZ0) / HCELL).astype(int), 0, HH - 1)
    g = height[j, i]
    return np.where(g < -1e8, -2.0, g)
for f in props:
    a, b, c = f[0], f[1], f[2]
    L = max(np.linalg.norm(b - a), np.linalg.norm(c - a), np.linalg.norm(c - b))
    n = max(2, int(L / 0.4) + 1)
    u, v = np.meshgrid(np.linspace(0, 1, n), np.linspace(0, 1, n))
    k = (u + v) <= 1
    u, v = u[k], v[k]
    P = a[None] + u[:, None] * (b - a)[None] + v[:, None] * (c - a)[None]
    g = ground_at(P[:, 0], P[:, 2])
    hit = (P[:, 1] > g + 0.35) & (P[:, 1] < g + 2.3)
    if not hit.any(): continue
    I = np.clip(((P[hit, 0] - GX0) / SCELL).astype(int), 0, SW - 1); J = np.clip(((P[hit, 2] - GZ0) / SCELL).astype(int), 0, SH - 1)
    solid[J, I] = True

for j in range(SH):
    hj = min(HH - 1, int((j + 0.5) * SCELL / HCELL))
    row = void[hj, np.clip(((np.arange(SW) + 0.5) * SCELL / HCELL).astype(int), 0, HW - 1)]
    solid[j, row] = True

# ------------------------------------------------------------------ chunks / meshes
mesh_faces = []
for f in faces: mesh_faces += split(f)
nwt = 0
TS = 64.0
for zz in np.arange(Z0, Z1, TS):
    for xx in np.arange(X0, X1, TS):
        c = np.array([[xx, zz], [xx + TS, zz], [xx + TS, zz + TS], [xx, zz + TS], [xx + TS / 2, zz + TS / 2]])
        g = ground_at(c[:, 0], c[:, 1])
        if (g > 0.6).all(): continue
        p = [np.array([x, 0.0, z]) for x, z in c[:4]]
        q = [np.array([0., 0.]), np.array([2., 0.]), np.array([2., 2.]), np.array([0., 2.])]
        for tri in ((0, 1, 2), (0, 2, 3)):          # facing up after the mirror swap below
            mesh_faces.append((p[tri[0]], p[tri[1]], p[tri[2]], q[tri[0]], q[tri[1]], q[tri[2]], wt, F_WATER, (255, 255, 255), 'water'))
            nwt += 1

chunks = {}
for f in mesh_faces:
    c = (f[0] + f[1] + f[2]) / 3
    key = (int(math.floor(c[0] / CHUNK)), int(math.floor(c[2] / CHUNK)))
    chunks.setdefault(key, []).append(f)

def mesh_bytes(fs, ox, oz):
    verts, vid, tris = [], {}, []
    for f in fs:
        us = np.array([f[3], f[4], f[5]], float)
        if f[6] >= 0:
            us[:, 1] = -us[:, 1]
            us -= np.floor(us.min(0))
        idx = []
        for k in range(3):
            p = f[k]
            key = (int(round((p[0] - ox) * 64)), int(round(p[1] * 64)), int(round((p[2] - oz) * 64)),
                   int(round(us[k][0] * TEX * 16)) if f[6] >= 0 else 0, int(round(us[k][1] * TEX * 16)) if f[6] >= 0 else 0)
            if key not in vid: vid[key] = len(verts); verts.append(key)
            idx.append(vid[key])
        if len(set(idx)) < 3: continue
        tris.append((idx[0], idx[2], idx[1], f[6], f[7], f[8]))     # OBJ->engine is a mirror: swap winding
    b = bytearray(struct.pack('>H', len(verts)))
    for v in verts:
        b += struct.pack('>hhhhh', *[max(-32768, min(32767, x)) for x in v])
    b += struct.pack('>H', len(tris))
    for t in tris:
        b += struct.pack('>HHHBBBBB', t[0], t[1], t[2], t[3] & 255, t[4], *t[5])
    return b, len(tris)

out = bytearray(b'VCM1')
out += struct.pack('>B', len(texnames))
for n in texnames: out += struct.pack('>B', len(n)) + n.encode()
out += struct.pack('>H', len(chunks))
ntri = 0
for (i, j), fs in sorted(chunks.items()):
    ox, oz = (i + 0.5) * CHUNK, (j + 0.5) * CHUNK
    mb, n = mesh_bytes(fs, ox, oz)
    ntri += n
    out += struct.pack('>ii', int(ox * 1024), int(oz * 1024)) + mb

# height (1/64 m, -32768 = sea) + surface grids
out += struct.pack('>iiHHH', int(GX0 * 1024), int(GZ0 * 1024), int(HCELL * 1024), HW, HH)
hq = np.where(height < -1e8, -32768, np.clip(np.round(height * 64), -32767, 32767)).astype('>i2')
out += hq.tobytes() + surf.astype(np.uint8).tobytes()
out += struct.pack('>iiHHH', int(GX0 * 1024), int(GZ0 * 1024), int(SCELL * 1024), SW, SH)
bits = np.packbits(solid.astype(np.uint8), axis=1)            # row-major, MSB first
out += struct.pack('>H', bits.shape[1]) + bits.tobytes()

# radar: palette indices at RCELL metres, generous sea margin
RX0, RZ0 = GX0 - 200, GZ0 - 200
RW, RH = int((GX1 - GX0 + 400) / RCELL), int((GZ1 - GZ0 + 400) / RCELL)
rad = np.zeros((RH, RW), np.uint8)
for j in range(RH):
    z = RZ0 + (j + 0.5) * RCELL
    hj = int((z - GZ0) / HCELL)
    if hj < 0 or hj >= HH: continue
    for i in range(RW):
        x = RX0 + (i + 0.5) * RCELL
        hi = int((x - GX0) / HCELL)
        if 0 <= hi < HW: rad[j, i] = 0 if surf[hj, hi] == VOID else surf[hj, hi]
si = np.clip(((RX0 + (np.arange(RW) + 0.5) * RCELL - GX0) / SCELL).astype(int), 0, SW - 1)
for j in range(RH):
    z = RZ0 + (j + 0.5) * RCELL
    sj = int((z - GZ0) / SCELL)
    if 0 <= sj < SH:
        row = solid[sj, si]
        rad[j, row & (rad[j] > 0) & (rad[j] != 0)] = 6
out += struct.pack('>iiHHH', int(RX0 * 1024), int(RZ0 * 1024), int(RCELL * 1024), RW, RH) + rad.tobytes()

# spawn: on od_oceanrdN1 at the spot with the most land around it, Tommy on the pavement next to it
spw = to_world(P0)
valid = (surf != 0) & (surf != VOID)
def cell_surf(x, z):
    i, j = int((x - GX0) / HCELL), int((z - GZ0) / HCELL)
    return surf[j, i] if 0 <= i < HW and 0 <= j < HH else 0
def land_score(x, z, rad=40):
    i, j = int((x - GX0) / HCELL), int((z - GZ0) / HCELL); k = int(rad / HCELL)
    return valid[max(0, j - k):j + k + 1, max(0, i - k):i + k + 1].sum()
cands = [p for p in spw[:, [0, 2]] if cell_surf(p[0], p[1]) == SURF['road']] or list(spw[:, [0, 2]])
mu = max(cands, key=lambda p: land_score(p[0], p[1]))
near = np.array([p for p in spw[:, [0, 2]] if np.linalg.norm(p - mu) < 25])
u_, s_, vt_ = np.linalg.svd(near - near.mean(0), full_matrices=False)
axis = vt_[0]
yaw = int(round(math.atan2(axis[0], axis[1]) / (2 * math.pi) * 1024)) & 1023
side = np.array([axis[1], -axis[0]])
def solid_at(x, z):
    i, j = int((x - GX0) / SCELL), int((z - GZ0) / SCELL)
    return not (0 <= i < SW and 0 <= j < SH) or solid[j, i]
best = None
for d in np.arange(3, 30, 1.0):
    for sgn in (1, -1):
        p = mu + side * d * sgn
        if cell_surf(p[0], p[1]) in (SURF['pave'], SURF['grass']) and not solid_at(*p) and land_score(p[0], p[1], 8) > 40:
            best = (p, sgn); break
    if best: break
tommy = best[0] if best else mu
kerb = best[1] if best else 1
cars = []
for along in (-8.0, 4.0, 16.0, -20.0, 28.0, -32.0):
    if len(cars) == 4: break
    for lat in (2.6, 0.0, -2.6):
        p = mu + axis * along + side * kerb * lat
        if cell_surf(p[0], p[1]) == SURF['road'] and not solid_at(*p): cars.append((p, yaw)); break
gy = lambda x, z: float(ground_at(np.array([x]), np.array([z]))[0])
out += struct.pack('>iiiH', int(tommy[0] * 1024), int(tommy[1] * 1024), int(gy(*tommy) * 64), (yaw + 512) & 1023)
out += struct.pack('>B', len(cars))
for p, y in cars: out += struct.pack('>iiH', int(p[0] * 1024), int(p[1] * 1024), y)

# lighthouse lamp (centre of the lens faces)
lens = [f for f in faces if f[6] >= 0 and texnames[f[6]] in ('m_lightlens', 'm_lightglass')]
if lens:
    lp = np.mean([(f[0] + f[1] + f[2]) / 3 for f in lens], 0)
else:
    lh = [f for f in faces if (f[9] or '').startswith('od_lightext')]
    pts = np.array([q for f in lh for q in f[:3]]); lp = np.array([pts[:, 0].mean(), pts[:, 1].max() - 3, pts[:, 2].mean()])
out += struct.pack('>iii', int(lp[0] * 1024), int(lp[1] * 64), int(lp[2] * 1024))

os.makedirs(RES, exist_ok=True)
open(os.path.join(RES, 'map.bin'), 'wb').write(out)
for n in texnames:
    im = Image.open(os.path.join(TEXDIR, texsrc[n])).convert('RGBA').resize((TEX, TEX), Image.LANCZOS)
    a = np.array(im)
    if n != 'm_waterclear256' and a[..., 3].min() < 250:
        a[..., 3] = np.where(a[..., 3] > 110, 255, 0)
    else:
        a[..., 3] = 255
    Image.fromarray(a, 'RGBA').save(os.path.join(RES, 't_%s.png' % n), optimize=True)
print('map: %d chunks, %d tris (%d water), %d textures, grid %dx%d, solid %d cells, %d bytes' %
      (len(chunks), ntri, nwt, len(texnames), HW, HH, int(solid.sum()), len(out)))
print('spawn', tommy, 'yaw', yaw, 'lamp', lp)
open(os.path.join(RES, 'maptex.lst'), 'w').write('\n'.join(texnames) + '\n')
