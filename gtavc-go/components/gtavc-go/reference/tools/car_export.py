#!/usr/bin/env python3
"""Vice City car DFF+TXD -> res/car_<name>.bin (format CAR2) + 64 px textures.

The cars.obj you exported from Blender is exactly these DFFs (same part names, same _ok/_dam
pairs) but its .mtl lost every texture link, so the textures are taken from the matching
DFF/TXD inside gta3.img (copied to assets/cars/).  The generic wheel comes from cars.obj
(wheel_saloon_l1) with its textures from wheels.txd.

CAR2 layout (big endian):
  'CAR2', u8 ntex, ntex*(u8 len, name)
  s16 dimX dimY dimZ, s16 seatX seatY seatZ          (1/64 m, x right, y up, z forward)
  u8 nwheel, nwheel*(s16 x y z, s16 radius), u8 flags (1 = bike: wheels are body parts)
  u8 npart, npart*(u8 partId, s16 px py pz, u8 nvar, nvar*mesh)   var 0 = ok, var 1 = damaged
  mesh lod1 (chassis_vlo)
mesh: u16 nv, nv*(s16 x y z u v), u16 nt, nt*(u16 a b c, s8 tex, u8 flags, u8 r g b)
flags: 2 emissive, 4 tint primary, 8 glass, 16 tint secondary
"""
import os, sys, struct
import numpy as np
from PIL import Image
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from rwtools import read_dff, read_txd
import mesh_tools

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
RES = os.path.join(ROOT, 'res')
SRC = os.environ.get('CARS', os.path.join(ROOT, 'assets', 'cars'))
WHEELSRC = os.path.join(ROOT, 'assets', 'cars')
WHEEL_R = 0.34
PRIMARY, SECONDARY = (60, 255, 0), (255, 0, 175)
SKIP_TEX = {'white64a', 'white128a', 'black64'}
F_EMISS, F_TINT, F_GLASS, F_TINT2 = 2, 4, 8, 16
PART_IDS = ['chassis', 'bonnet', 'boot', 'door_lf', 'door_rf', 'door_lr', 'door_rr', 'wing_lf', 'wing_rf',
            'bump_front', 'bump_rear', 'windscreen', 'handlebars', 'forks_front', 'forks_rear', 'mudguard',
            'wheel_front', 'wheel_rear']


def frame_world(frames):
    W = []
    for f in frames:
        R, t = f['rot'], f['pos']
        if f['parent'] < 0: W.append((R.copy(), t.copy()))
        else:
            PR, Pt = W[f['parent']]
            W.append((R @ PR, t @ PR + Pt))
    return W


def union_islands(tris):
    par = list(range(len(tris)))
    def find(x):
        while par[x] != x: par[x] = par[par[x]]; x = par[x]
        return x
    owner = {}
    for k, t in enumerate(tris):
        for v in t[:3]:
            key = (int(v), int(t[3]))
            if key in owner: par[find(k)] = find(owner[key])
            else: owner[key] = k
    return np.array([find(k) for k in range(len(tris))])


def classify(m, partname):
    c = tuple(m['color'][:3]); tex = m['tex']
    if tex in SKIP_TEX or (tex and tex.endswith('speca')): return None
    if partname.startswith('windscreen') or (tex is None and c in ((89, 89, 89), (81, 81, 81))): return ('glass', c, None)
    if c == PRIMARY: return ('tint', c, tex)
    if c == SECONDARY: return ('tint2', c, tex)
    if tex: return ('tex', c, tex)
    return ('flat', c, None)


def build_mesh(P, g, ground, tid, txd, ratio, name):
    tris = g['tris']
    cls = [classify(g['mats'][int(t[3])], name) for t in tris]
    keep = [k for k in range(len(tris)) if cls[k] is not None and len({*tris[k][:3]}) == 3]
    if not keep: return None
    tri2 = tris[keep]; cl = [cls[k] for k in keep]
    uv = g['uv'] if g['uv'] is not None else np.zeros((len(P), 2))
    key = {}; pid = np.array([key.setdefault(tuple(np.round(p, 4)), len(key)) for p in P])
    PP = np.zeros((len(key), 3))
    for i, p in enumerate(P): PP[pid[i]] = p
    F = np.array([pid[t[:3]] for t in tri2])
    isl = union_islands(tri2)
    UV = {}
    for k, t in enumerate(tri2):
        for v in t[:3]: UV[(int(pid[v]), int(isl[k]))] = (float(uv[v][0]), float(uv[v][1]))
    part = mesh_tools.Part(PP, F, isl, UV)
    if ratio < 1.0: part.decimate(max(6, int(len(F) * ratio)))
    V, T, vid = [], [], {}
    for k, f in part.faces.items():
        kind, col, tx = cl[k]
        if tx and tx not in txd: tx = None; kind = 'flat' if kind == 'tex' else kind
        texi = tid(tx) if tx and kind in ('tex', 'tint', 'tint2') else -1
        us = np.array([part.uv[(q, part.isl[k])] for q in f], float) if texi >= 0 else np.zeros((3, 2))
        if texi >= 0: us -= np.floor(us.min(0))
        idx = []
        for n, q in enumerate(f):
            p = part.P[q]
            kk = (q, part.isl[k], round(us[n][0], 4), round(us[n][1], 4))
            if kk not in vid:
                vid[kk] = len(V)
                V.append([p[0] * 64, (p[2] - ground) * 64, p[1] * 64, us[n][0] * 64 * 16, us[n][1] * 64 * 16])
            idx.append(vid[kk])
        flags, rgb = 0, tuple(col)
        if kind == 'tint': flags = F_TINT; rgb = (255, 255, 255)
        elif kind == 'tint2': flags = F_TINT2; rgb = (255, 255, 255)
        elif kind == 'tex': rgb = (255, 255, 255)
        elif kind == 'glass': flags = F_GLASS; rgb = (40, 62, 78)
        T.append((idx[0], idx[2], idx[1], texi, flags, *rgb))
    return V, T


def mesh_bytes(m):
    V, T = m
    b = bytearray(struct.pack('>H', len(V)))
    for v in V: b += struct.pack('>hhhhh', *[max(-32768, min(32767, int(round(x)))) for x in v])
    b += struct.pack('>H', len(T))
    for t in T: b += struct.pack('>HHHbBBBB', *t)
    return b


def convert(name):
    r = read_dff(os.path.join(SRC, name + '.dff'))
    txd = read_txd(os.path.join(SRC, name + '.txd'))
    W = frame_world(r['frames'])
    fr = {f['name']: i for i, f in enumerate(r['frames'])}
    bike = 'wheel_front' in fr
    if bike:
        wf = W[fr['wheel_front']][1]; wr = W[fr['wheel_rear']][1]
        ground = min(wf[2], wr[2]) - 0.32
        wheels = [(wf, 0.32), (wr, 0.32)]
    else:
        dz = [W[i][1] for n, i in fr.items() if n.startswith('wheel_') and n.endswith('dummy')]
        ground = min(p[2] for p in dz) - WHEEL_R
        wheels = [(p, WHEEL_R) for p in dz]
    texnames = []
    def tid(n):
        if n not in texnames: texnames.append(n)
        return texnames.index(n)
    parts = {}
    atom = {r['frames'][fi]['name']: gi for fi, gi in r['atomics']}
    for fname, gi in atom.items():
        if fname == 'chassis_vlo': continue
        base = fname.replace('_hi_ok', '').replace('_hi_dam', '').replace('chassis_hi', 'chassis')
        if base not in PART_IDS: continue
        var = 1 if fname.endswith('_dam') else 0
        fi = fr[fname]; R, t = W[fi]
        P = r['geoms'][gi]['verts'].astype(float) @ R + t
        ratio = 0.30 if base == 'chassis' else 0.45
        m = build_mesh(P, r['geoms'][gi], ground, tid, txd, ratio, fname)
        if m is None: continue
        piv = W[r['frames'][fi]['parent']][1] if r['frames'][fi]['parent'] >= 0 else t
        e = parts.setdefault(base, {'pivot': piv, 'var': [None, None]})
        e['var'][var] = m
    vlo = 'chassis_vlo' if 'chassis_vlo' in atom else 'chassis_hi'
    fi = fr[vlo]; R, t = W[fi]
    P = r['geoms'][atom[vlo]]['verts'].astype(float) @ R + t
    lod1 = build_mesh(P, r['geoms'][atom[vlo]], ground, tid, txd, 1.0 if vlo == 'chassis_vlo' else 0.15, vlo)
    seat = W[fr['ped_frontseat']][1] if 'ped_frontseat' in fr else np.array([0.45, 0.0, 0.0]) + np.array([0, 0, ground + 0.6])
    allv = np.array([v[:3] for p in parts.values() for m in p['var'] if m for v in m[0]])
    dims = (int(allv[:, 0].max() - allv[:, 0].min()), int(allv[:, 1].max()), int(allv[:, 2].max() - allv[:, 2].min()))
    out = bytearray(b'CAR2') + struct.pack('>B', len(texnames))
    for n in texnames: out += struct.pack('>B', len(n)) + n.encode()
    out += struct.pack('>hhh', *dims)
    out += struct.pack('>hhh', int(seat[0] * 64), int((seat[2] - ground) * 64), int(seat[1] * 64))
    out += struct.pack('>B', len(wheels))
    for p, rad in wheels: out += struct.pack('>hhhh', int(p[0] * 64), int((p[2] - ground) * 64), int(p[1] * 64), int(rad * 64))
    out += struct.pack('>B', 1 if bike else 0)
    order = [p for p in PART_IDS if p in parts]
    out += struct.pack('>B', len(order))
    ntri = 0
    for pn in order:
        e = parts[pn]; vs = [m for m in e['var'] if m is not None]
        if e['var'][0] is None: vs = [e['var'][1]]
        piv = e['pivot']
        out += struct.pack('>BhhhB', PART_IDS.index(pn), int(piv[0] * 64), int((piv[2] - ground) * 64), int(piv[1] * 64), len(vs))
        for m in vs: out += mesh_bytes(m)
        ntri += len(vs[0][1])
    out += mesh_bytes(lod1)
    for n in texnames:
        im = txd[n].convert('RGB').resize((64, 64), Image.LANCZOS)
        im.save(os.path.join(RES, 't_%s.png' % n), optimize=True)
    open(os.path.join(RES, 'car_%s.bin' % name), 'wb').write(out)
    print('%s: %d tris near (ok parts), %d far, %d parts, tex %s, %d bytes' % (name, ntri, len(lod1[1]), len(order), texnames, len(out)))
    return texnames


def wheel():
    """generic wheel from cars.obj (wheel_saloon_l1) -> res/wheel.bin (single mesh, centred, radius-normalised)"""
    V, VT, cur, mat, F = [], [], None, None, []
    for l in open(os.path.join(WHEELSRC, 'cars.obj')):
        if l.startswith('v '): V.append([float(x) for x in l.split()[1:4]])
        elif l.startswith('vt '): VT.append([float(x) for x in l.split()[1:3]])
        elif l.startswith('o '): cur = l[2:].strip()
        elif l.startswith('usemtl'): mat = l.split()[1]
        elif l.startswith('f ') and cur == 'wheel_saloon_l1':
            p = [t.split('/') for t in l.split()[1:]]
            for k in range(1, len(p) - 1):
                F.append(([int(p[i][0]) - 1 for i in (0, k, k + 1)], [int(p[i][1]) - 1 for i in (0, k, k + 1)], mat))
    V = np.array(V); VT = np.array(VT)
    idx = sorted({i for f in F for i in f[0]})
    c = V[idx].mean(0)
    names = ['wheel_saloon64', 'tyre64a']
    verts, tris, vid = [], [], {}
    for vi, ti, m in F:
        tex = 1 if m.endswith('l1.0') else (0 if m.endswith('l1.1') else -1)
        uv = np.array([VT[t] for t in ti]); uv[:, 1] = -uv[:, 1]; uv -= np.floor(uv.min(0))
        ids = []
        for n in range(3):
            p = V[vi[n]] - c     # obj: x across (axle), y up, z back -> engine x, y, -z
            key = (int(round(p[0] * 64)), int(round(p[1] * 64)), int(round(-p[2] * 64)), int(uv[n][0] * 1024), int(uv[n][1] * 1024))
            if key not in vid: vid[key] = len(verts); verts.append(key)
            ids.append(vid[key])
        tris.append((ids[0], ids[2], ids[1], tex, 0, 40, 40, 40) if tex < 0 else (ids[0], ids[2], ids[1], tex, 0, 255, 255, 255))
    out = bytearray(struct.pack('>B', 2))
    for n in names: out += struct.pack('>B', len(n)) + n.encode()
    out += mesh_bytes((verts, tris))
    open(os.path.join(RES, 'wheel.bin'), 'wb').write(out)
    for n, f in (('wheel_saloon64', 'wheels.txd_wheel_saloon64_0.png'), ('tyre64a', 'wheels.txd_tyre64a_0.png')):
        Image.open(os.path.join(WHEELSRC, 'wheels_tex', f)).convert('RGB').resize((64, 64), Image.LANCZOS).save(os.path.join(RES, 't_%s.png' % n))
    print('wheel: %d tris' % len(tris))
    return names


if __name__ == '__main__':
    os.makedirs(RES, exist_ok=True)
    names = sys.argv[1:] or ['admiral', 'oceanic', 'taxi', 'pcj600', 'police']
    allt = wheel()
    ok = []
    for n in names:
        try: ts = convert(n)
        except Exception as e: print('skip', n, e); continue
        ok.append(n)
        for t in ts:
            if t not in allt: allt.append(t)
    open(os.path.join(RES, 'cars.lst'), 'w').write('\n'.join(ok) + '\n')
    open(os.path.join(RES, 'cartex.lst'), 'w').write('\n'.join(allt) + '\n')
