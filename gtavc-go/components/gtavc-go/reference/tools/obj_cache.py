#!/usr/bin/env python3
"""Parses a (huge) Blender OBJ once into a numpy cache (<obj>.npz) so the map tools don't re-read 230 MB of text.
Arrays: V (n,3) f32, VT (n,2) f32, FV/FT (m,3) i32, FM (m,) i16 material, FO (m,) i32 object, names, mats."""
import os, sys
import numpy as np


def load(path):
    cache = os.path.splitext(path)[0] + '.npz'
    if os.path.exists(cache) and os.path.getmtime(cache) > os.path.getmtime(path):
        z = np.load(cache, allow_pickle=True)
        return {k: z[k] for k in z.files}
    V, VT, FV, FT, FM, FO = [], [], [], [], [], []
    names, mats, mid = [], [], {}
    cm, co = -1, -1
    with open(path, 'r', encoding='latin1') as f:
        for l in f:
            c = l[:2]
            if c == 'v ':
                p = l.split(); V.append((float(p[1]), float(p[2]), float(p[3])))
            elif c == 'vt':
                p = l.split(); VT.append((float(p[1]), float(p[2])))
            elif c == 'f ':
                p = l.split()[1:]
                vi, ti = [], []
                for t in p:
                    q = t.split('/')
                    vi.append(int(q[0]) - 1); ti.append(int(q[1]) - 1 if len(q) > 1 and q[1] else -1)
                for k in range(1, len(vi) - 1):
                    FV.append((vi[0], vi[k], vi[k + 1])); FT.append((ti[0], ti[k], ti[k + 1])); FM.append(cm); FO.append(co)
            elif c == 'o ':
                names.append(l[2:].strip()); co = len(names) - 1
            elif l.startswith('usemtl'):
                n = l.split(None, 1)[1].strip()
                if n not in mid: mid[n] = len(mats); mats.append(n)
                cm = mid[n]
    d = {'V': np.array(V, np.float32), 'VT': np.array(VT if VT else [(0, 0)], np.float32), 'FV': np.array(FV, np.int32),
         'FT': np.array(FT, np.int32), 'FM': np.array(FM, np.int16), 'FO': np.array(FO, np.int32),
         'names': np.array(names, dtype=object), 'mats': np.array(mats, dtype=object)}
    np.savez(cache, **d)
    return d


def load_mtl(path):
    mtl, cur = {}, None
    for l in open(path, encoding='latin1'):
        s = l.split()
        if not s: continue
        if s[0] == 'newmtl': cur = l.split(None, 1)[1].strip(); mtl[cur] = {'kd': (1, 1, 1), 'tex': None, 'alpha': False}
        elif s[0] == 'Kd': mtl[cur]['kd'] = tuple(float(x) for x in s[1:4])
        elif s[0] == 'map_Kd': mtl[cur]['tex'] = l.split(None, 1)[1].strip().replace('\\', '/').split('/')[-1]
        elif s[0] == 'map_d': mtl[cur]['alpha'] = True
    return mtl


if __name__ == '__main__':
    d = load(sys.argv[1])
    print({k: v.shape for k, v in d.items()})
