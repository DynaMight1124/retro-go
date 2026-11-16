#!/usr/bin/env python3
"""Weapon DFF+TXD (gta3.img) -> res/w_<name>.bin (one CAR2-style mesh, longest axis along +x, grip at the origin)."""
import os, sys, struct
import numpy as np
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
from rwtools import read_dff, read_txd
import img_extract
RES = os.path.join(HERE, '..', 'res')

def export(name, dff, txd):
    r = read_dff(dff)
    g = max(r['geoms'], key=lambda g: len(g['tris']))
    P = g['verts'].astype(float)
    c = P.mean(0); Q = P - c
    _, _, vt = np.linalg.svd(Q, full_matrices=False)
    X = Q @ vt.T                                   # principal axes: x = longest
    # grip end = the thinner end (smaller cross section) goes to the origin
    lo, hi = X[:, 0].min(), X[:, 0].max(); L = hi - lo
    thick = lambda m: np.ptp(X[m][:, 1:], axis=0).sum() if m.sum() > 2 else 9
    a, b = X[:, 0] < lo + L * 0.25, X[:, 0] > hi - L * 0.25
    if thick(b) < thick(a): X[:, 0] = -X[:, 0]; X[:, 1] = -X[:, 1]
    lo = X[:, 0].min()
    grip = 0.12 if name == 'bat' else 0.30
    X[:, 0] -= lo + L * grip
    tex = next((m['tex'] for m in g['mats'] if m['tex'] in txd), None)
    names = [tex] if tex else []
    uv = g['uv'] if g['uv'] is not None else np.zeros((len(P), 2))
    V, T = [], []
    for t in g['tris']:
        us = uv[t[:3]].copy(); us -= np.floor(us.min(0))
        idx = []
        for k in range(3):
            p = X[t[k]] * 64
            idx.append(len(V)); V.append((p[0], p[1], p[2], us[k][0] * 64 * 16, us[k][1] * 64 * 16))
        T.append((idx[0], idx[2], idx[1], 0 if tex else -1, 0, 200, 200, 200))
    out = bytearray(struct.pack('>B', len(names)))
    for n in names: out += struct.pack('>B', len('w_' + n)) + ('w_' + n).encode()
    out += struct.pack('>H', len(V))
    for v in V: out += struct.pack('>hhhhh', *[int(round(x)) for x in v])
    out += struct.pack('>H', len(T))
    for t in T: out += struct.pack('>HHHbBBBB', *t)
    open(os.path.join(RES, 'w_%s.bin' % name), 'wb').write(out)
    if tex: txd[tex].convert('RGB').resize((64, 64), Image.LANCZOS).save(os.path.join(RES, 't_w_%s.png' % tex))
    print(name, len(T), 'tris, length %.2f m' % L)

if __name__ == '__main__':
    img = sys.argv[1]; ents = img_extract.read_dir(img); f = open(img, 'rb')
    for n in ['colt45', 'bat']:
        for e in ('.dff', '.txd'):
            off, s = ents[n + e]; f.seek(off * 2048); open('/tmp/w' + e, 'wb').write(f.read(s * 2048))
        export(n, '/tmp/w.dff', read_txd('/tmp/w.txd'))
