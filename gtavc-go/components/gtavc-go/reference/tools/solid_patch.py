#!/usr/bin/env python3
"""Recomputes only the solid-cell grid and patches it into existing res/t tiles (last 1300 bytes of each)."""
import os, sys, re, math
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
import obj_cache
import map_full_export as M
SRC = sys.argv[1]
d = obj_cache.load(SRC)
V, FV, FO, names = d['V'], d['FV'], d['FO'], d['names']
W = np.stack([V[:, 0], V[:, 1] - M.WATER, -V[:, 2]], 1).astype(np.float64)
base = np.array([re.sub(r'\.\d+$', '', n) for n in names], dtype=object)
keep = np.array([not M.DROP.match(n) for n in base])[FO]
FV = FV[keep]
A, Bv, C = W[FV[:, 0]], W[FV[:, 1]], W[FV[:, 2]]
N = np.cross(Bv - A, C - A); ny = N[:, 1] / (np.linalg.norm(N, axis=1) + 1e-12)
cen = (A + Bv + C) / 3
ti = np.floor(cen[:, 0] / M.TILE).astype(int); tj = np.floor(cen[:, 2] / M.TILE).astype(int)
TI0, TJ0 = ti.min(), tj.min(); NX, NZ = ti.max() - TI0 + 1, tj.max() - TJ0 + 1
GX0, GZ0 = TI0 * M.TILE, TJ0 * M.TILE
z = np.load(os.path.splitext(SRC)[0] + '_grids.npz')
low, has = z['low'], z['has']
GW, GH = low.shape[1], low.shape[0]
SW, SH = NX * M.NS, NZ * M.NS
solid = np.zeros((SH, SW), bool)
for k in np.where(np.abs(ny) < 0.5)[0]:
    a, b, c = A[k], Bv[k], C[k]
    L = max(np.linalg.norm(b - a), np.linalg.norm(c - a), np.linalg.norm(c - b))
    n = int(min(60, max(2, L / 0.5)))
    u, v = np.meshgrid(np.linspace(0, 1, n), np.linspace(0, 1, n)); m = (u + v) <= 1
    Pp = a[None] + u[m][:, None] * (b - a)[None] + v[m][:, None] * (c - a)[None]
    gi = np.clip(((Pp[:, 0] - GX0) / M.HC).astype(int), 0, GW - 1); gj = np.clip(((Pp[:, 2] - GZ0) / M.HC).astype(int), 0, GH - 1)
    g = np.where(has[gj, gi], low[gj, gi], -50)
    if min(a[1], b[1], c[1]) > g.min() + 0.6: continue
    hit = (Pp[:, 1] > g + 0.4) & (Pp[:, 1] < g + 2.4)
    if not hit.any(): continue
    si = np.clip(((Pp[hit, 0] - GX0) / M.SC).astype(int), 0, SW - 1); sj = np.clip(((Pp[hit, 2] - GZ0) / M.SC).astype(int), 0, SH - 1)
    solid[sj, si] = True
print('solid cells', int(solid.sum()))
zz = dict(np.load(os.path.splitext(SRC)[0] + '_grids.npz')); zz['solid'] = solid; np.savez(os.path.splitext(SRC)[0] + '_grids.npz', **zz)
T = os.path.join(M.RES, 't')
for f in os.listdir(T):
    I, J = [int(x) for x in f[:-4].split('_')]
    sb = np.packbits(solid[J * M.NS:(J + 1) * M.NS, I * M.NS:(I + 1) * M.NS].astype(np.uint8), axis=1).tobytes()
    p = os.path.join(T, f); data = bytearray(open(p, 'rb').read())
    data[-len(sb):] = sb
    open(p, 'wb').write(data)
print('patched', len(os.listdir(T)), 'tiles')
