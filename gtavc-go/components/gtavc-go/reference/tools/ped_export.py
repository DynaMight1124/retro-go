#!/usr/bin/env python3
"""Vice City ped DFF+TXD (from gta3.img) -> res/p_<name>.bin (+ _lo) on the 9-part rig, res/t_p_<name>.png.

  python3 tools/ped_export.py path/to/gta3.img            (all street peds, cops, Tommy)
Faces go to the part of the nearest bone segment (same method as the FBX peds), the pose is normalised
(arms horizontal, legs straight) so the baked IFP clips in anim.bin drive them like Tommy.
Writes res/peds.lst: name, flags (1 female, 2 cop, 4 gang, 8 player)."""
import os, sys, re, struct, io, contextlib
import numpy as np
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
os.environ.setdefault('NPCS', '/nonexistent')
with contextlib.redirect_stdout(io.StringIO()):
    import npc_export
from rwtools import read_dff, read_txd
import img_extract

RES = os.path.join(HERE, '..', 'res')
npc_export.RES = RES
AMBIENT = re.compile(r'^[bhw][fm][oy][a-z]{2,3}$')
SPECIAL = ['cop', 'swat', 'fbi', 'army', 'medic', 'fireman', 'cba', 'cbb', 'hna', 'hnb', 'sga', 'sgb', 'sgc', 'pga', 'pgb',
           'cla', 'clb', 'gda', 'gdb', 'bka', 'bkb', 'bga', 'bgb', 'mba', 'mbb', 'taxi_d', 'c_worker', 'burger', 'chef']


def frame_world(frames):
    Wd = []
    for f in frames:
        R, t = f['rot'], f['pos']
        if f['parent'] < 0: Wd.append((R.copy(), t.copy()))
        else:
            PR, Pt = Wd[f['parent']]; Wd.append((R @ PR, t @ PR + Pt))
    return Wd


def to_rig(p):
    # VC skinned peds (bind pose) are already stored y-up, facing +z, character's right at -x: same as the rig space
    return np.array(p, dtype=float)


def export(name, dff, txd, hi=320, lo=110):
    r = read_dff(dff)
    W = frame_world(r['frames'])
    J = {}
    for i, f in enumerate(r['frames']):
        n = f['name'].strip()
        if n and n not in J: J[n] = to_rig(W[i][1])
    need = ['Pelvis', 'Spine', 'Spine1', 'Neck', 'Head', 'L UpperArm', 'L Forearm', 'L Hand', 'R UpperArm', 'R Forearm', 'R Hand',
            'L Thigh', 'L Calf', 'L Foot', 'R Thigh', 'R Calf', 'R Foot']
    if any(k not in J for k in need): return None
    for s in 'LR':
        if f'{s} Toe0' not in J: J[f'{s} Toe0'] = J[f'{s} Foot'] + np.array([0, -0.05, 0.12])
    # biggest geometry = the body
    g = max(r['geoms'], key=lambda g: len(g['tris']))
    V = to_rig(g['verts'].astype(float))
    tris = g['tris']
    F = np.stack([tris[:, 0], tris[:, 1], tris[:, 2]], 1)
    UV = g['uv'].copy() if g['uv'] is not None else np.zeros((len(V), 2))
    UV[:, 1] = 1.0 - UV[:, 1]          # RW uvs are top-down already; build_and_export flips v again
    UVI = F.copy()
    tex = None
    for m in g['mats']:
        if m['tex'] and m['tex'] in txd: tex = m['tex']; break
    if tex is None and txd: tex = list(txd.keys())[0]
    S = npc_export.segments(J)
    cen = V[F].mean(axis=1)
    names = list(S.keys())
    D = np.stack([npc_export.seg_dist(cen, *S[n]) * (0.85 if n in ('Pelvis', 'Spine', 'Spine1', 'Neck', 'Head', 'LClav', 'RClav') else 1.0) for n in names], 1)
    fpart = np.array([npc_export.part_of(names[k]) for k in np.argmin(D, 1)])
    V, J = npc_export.facing_fix(V, J)
    h = V[:, 1].max() - V[:, 1].min()
    if tex: txd[tex].convert('RGB').resize((64, 64), Image.LANCZOS).save(os.path.join(RES, 't_p_%s.png' % name), optimize=True)
    with contextlib.redirect_stdout(io.StringIO()):
        npc_export.build_and_export('p_' + name, V, F, UV, UVI, fpart, {k: v.copy() for k, v in J.items()}, 64, 64, h, hi)
        npc_export.build_and_export('p_' + name + '_lo', V, F, UV, UVI, fpart, {k: v.copy() for k, v in J.items()}, 64, 64, h, lo)
    # npc_export writes npc_<tag>.bin; rename to the short form
    for suf in ('', '_lo'):
        os.replace(os.path.join(RES, 'npc_p_%s%s.bin' % (name, suf)), os.path.join(RES, 'p_%s%s.bin' % (name, suf)))
    return True


if __name__ == '__main__':
    img = sys.argv[1]
    ents = img_extract.read_dir(img)
    peds = sorted(n[:-4] for n in ents if n.endswith('.dff') and (AMBIENT.match(n[:-4]) or n[:-4] in SPECIAL or n[:-4] == 'player'))
    tmp = '/tmp/pedx'; os.makedirs(tmp, exist_ok=True)
    f = open(img, 'rb')
    def get(n, out):
        off, size = ents[n]; f.seek(off * 2048); open(out, 'wb').write(f.read(size * 2048))
    lst = []
    for p in peds:
        if p + '.txd' not in ents: continue
        get(p + '.dff', tmp + '/a.dff'); get(p + '.txd', tmp + '/a.txd')
        try:
            ok = export(p, tmp + '/a.dff', read_txd(tmp + '/a.txd'), 420 if p == 'player' else 300, 110)
        except Exception as e:
            print('skip', p, e); continue
        if not ok: print('skip', p, '(no skeleton)'); continue
        fl = (1 if (AMBIENT.match(p) and p[1] == 'f') else 0) | (2 if p in ('cop', 'swat', 'fbi', 'army') else 0) | (8 if p == 'player' else 0)
        if p in SPECIAL[6:25]: fl |= 4
        lst.append('%s %d' % (p, fl))
        print('ped', p, fl)
    open(os.path.join(RES, 'peds.lst'), 'w').write('\n'.join(lst) + '\n')
