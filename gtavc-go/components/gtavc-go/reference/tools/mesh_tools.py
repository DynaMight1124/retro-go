#!/usr/bin/env python3
"""Split the Tommy Vercetti OBJ into animatable body parts, decimate each part
(position-welded QEM with UV-island preservation) and export a compact binary
model for the J2ME renderer.

Binary layout (big endian, DataInputStream friendly):
  u8  nparts
  s16 rootHeight                 (pelvis height above feet, 1/64 m units)
  per part:
     s16 px,py,pz                (joint position relative to pelvis, 1/64 m)
     s16 nv ; nv * (s16 x,y,z relative to joint ; s16 u,v  [1/16 texel of 128px tex])
     s16 nt ; nt * 3 * s16 indices
"""
import heapq, struct, sys, collections
import numpy as np

SCALE = 64.0 * 0.97          # model units -> 1/64 m   (Tommy ends up ~1.78 m tall)
TEX = 128                    # runtime texture size

# joint positions in OBJ space
HIP_Y = -0.10
JOINTS = {
    'torso': (0.0, HIP_Y, 0.0),
    'armL': (-0.265, 0.50, 0.0), 'foreL': (-0.55, 0.50, 0.0),
    'armR': (0.265, 0.50, 0.0), 'foreR': (0.55, 0.50, 0.0),
    'thighL': (-0.10, HIP_Y, 0.0), 'shinL': (-0.10, -0.53, 0.0),
    'thighR': (0.10, HIP_Y, 0.0), 'shinR': (0.10, -0.53, 0.0),
}
PARTS = ['torso', 'armL', 'foreL', 'armR', 'foreR', 'thighL', 'shinL', 'thighR', 'shinR']
# NOTE: model +x is the character's *left* hand side after the engine mirror, so L/R only
# matter for animation phase.  (The OBJ faces +z; we keep that.)


def load_obj(path):
    v, vt, f = [], [], []
    for l in open(path):
        p = l.split()
        if not p:
            continue
        if p[0] == 'v':
            v.append([float(x) for x in p[1:4]])
        elif p[0] == 'vt':
            vt.append([float(x) for x in p[1:3]])
        elif p[0] == 'f':
            f.append([int(t.split('/')[0]) - 1 for t in p[1:4]])
    return np.array(v), np.array(vt), np.array(f)


def classify(c):
    x, y = c[0], c[1]
    ax = abs(x)
    if ax > 0.255 and y > 0.12:
        side = 'R' if x > 0 else 'L'
        return ('fore' if ax > 0.55 else 'arm') + side
    if y < HIP_Y - 0.0:
        side = 'R' if x > 0 else 'L'
        return ('shin' if y < -0.53 else 'thigh') + side
    return 'torso'


class Part:
    """One decimatable body part: position-welded verts, faces with UV islands."""

    def __init__(self, P, F, FI, UV):
        # P: positions (n,3) ; F: faces (m,3) of position ids ; FI: island per face ; UV: dict (pos,island)->uv
        self.P = {i: np.array(P[i], float) for i in set(F.flatten())}
        self.faces = {k: [int(a) for a in F[k]] for k in range(len(F))}
        self.isl = {k: int(FI[k]) for k in range(len(F))}
        self.uv = dict(UV)
        self.build()

    def build(self):
        self.vf = collections.defaultdict(set)
        for k, f in self.faces.items():
            for a in f:
                self.vf[a].add(k)
        ec = collections.Counter()
        for k, f in self.faces.items():
            for i in range(3):
                ec[tuple(sorted((f[i], f[(i + 1) % 3])))] += 1
        self.frozen = set()
        for e, c in ec.items():
            if c != 2:
                self.frozen.update(e)
        self.Q = {}
        for v in self.P:
            self.Q[v] = np.zeros((4, 4))
        for k, f in self.faces.items():
            q = self.plane_q(f)
            for a in f:
                self.Q[a] += q

    def plane_q(self, f):
        a, b, c = (self.P[i] for i in f)
        n = np.cross(b - a, c - a)
        ar = np.linalg.norm(n)
        if ar < 1e-12:
            return np.zeros((4, 4))
        n = n / ar
        d = -np.dot(n, a)
        p = np.array([n[0], n[1], n[2], d])
        return np.outer(p, p) * ar

    def normal(self, f, pos=None):
        pts = [self.P[i] if pos is None or i != pos[0] else pos[1] for i in f]
        n = np.cross(pts[1] - pts[0], pts[2] - pts[0])
        return n

    def islands_at(self, v, skip=None):
        return {self.isl[k] for k in self.vf[v] if skip is None or skip not in self.faces[k]}

    def neighbors(self, v):
        s = set()
        for k in self.vf[v]:
            s.update(self.faces[k])
        s.discard(v)
        return s

    def cost(self, b, a):
        """cost of collapsing b into a (a keeps its position)"""
        if b in self.frozen:
            return None
        # faces of b that survive (don't contain a)
        surv = [k for k in self.vf[b] if a not in self.faces[k]]
        dying = [k for k in self.vf[b] if a in self.faces[k]]
        if not dying:
            return None
        # UV islands: a must have a corner in every island of surviving faces
        a_isl = {self.isl[k] for k in self.vf[a]}
        for k in surv:
            if self.isl[k] not in a_isl:
                return None
        # link condition
        common = self.neighbors(a) & self.neighbors(b)
        if len(common) != len(dying):
            return None
        # no flipped / degenerate faces
        for k in surv:
            f = self.faces[k]
            n0 = self.normal(f)
            n1 = self.normal(f, (b, self.P[a]))
            l0, l1 = np.linalg.norm(n0), np.linalg.norm(n1)
            if l1 < 1e-9 or np.dot(n0, n1) < 0.35 * l0 * l1:
                return None
        q = self.Q[a] + self.Q[b]
        v = np.append(self.P[a], 1.0)
        e = float(v @ q @ v)
        ln = np.linalg.norm(self.P[a] - self.P[b])
        return e + 1e-7 * ln

    def collapse(self, b, a):
        for k in list(self.vf[b]):
            f = self.faces[k]
            if a in f:
                # remove face
                for x in f:
                    self.vf[x].discard(k)
                del self.faces[k]
                del self.isl[k]
            else:
                self.faces[k] = [a if x == b else x for x in f]
                self.vf[a].add(k)
        self.vf.pop(b, None)
        self.Q[a] = self.Q[a] + self.Q[b]
        self.P.pop(b)
        self.Q.pop(b)

    def decimate(self, target):
        stamp = collections.defaultdict(int)
        heap = []

        def push(b, a):
            c = self.cost(b, a)
            if c is not None:
                heapq.heappush(heap, (c, b, a, stamp[b], stamp[a]))

        for v in list(self.P):
            for n in self.neighbors(v):
                push(v, n)
        while len(self.faces) > target and heap:
            c, b, a, sb, sa = heapq.heappop(heap)
            if b not in self.P or a not in self.P or stamp[b] != sb or stamp[a] != sa:
                continue
            c2 = self.cost(b, a)
            if c2 is None:
                continue
            if c2 > c * 1.0001 + 1e-9:
                heapq.heappush(heap, (c2, b, a, sb, sa))
                continue
            self.collapse(b, a)
            stamp[a] += 1
            for n in self.neighbors(a):
                stamp[n] += 0
                push(n, a)
                push(a, n)
                for m in self.neighbors(n):
                    push(m, n) if False else None
        return len(self.faces)

    def export(self, joint):
        """returns (verts [(x,y,z,u,v)], tris) relative to joint, in 1/64m / 1/16 texel"""
        vid = {}
        verts, tris = [], []
        for k, f in self.faces.items():
            t = []
            for a in f:
                key = (a, self.isl[k])
                if key not in vid:
                    p = self.P[a]
                    u, v = self.uv[key]
                    vid[key] = len(verts)
                    verts.append((int(round((p[0] - joint[0]) * SCALE)), int(round((p[1] - joint[1]) * SCALE)),
                                  int(round((p[2] - joint[2]) * SCALE)), int(round(u * TEX * 16)),
                                  int(round((1 - v) * TEX * 16))))
                t.append(vid[key])
            tris.append(t)
        return verts, tris


def build_parts(obj):
    v, vt, F = load_obj(obj)
    # weld positions
    key = {}
    pid = np.array([key.setdefault(tuple(np.round(p, 4)), len(key)) for p in v])
    P = np.zeros((len(key), 3))
    for i, p in enumerate(v):
        P[pid[i]] = p
    # islands: faces sharing an original (pos,uv) vertex
    par = list(range(len(F)))

    def find(x):
        while par[x] != x:
            par[x] = par[par[x]]
            x = par[x]
        return x
    owner = {}
    for k, f in enumerate(F):
        for a in f:
            if a in owner:
                par[find(k)] = find(owner[a])
            else:
                owner[a] = k
    isl = np.array([find(k) for k in range(len(F))])
    # drop degenerate faces
    groups = collections.defaultdict(list)
    for k, f in enumerate(F):
        pf = pid[f]
        if len(set(pf)) < 3:
            continue
        c = P[pf].mean(axis=0)
        groups[classify(c)].append(k)
    parts = {}
    for name in PARTS:
        ks = groups[name]
        Fp = np.array([pid[F[k]] for k in ks])
        FI = np.array([isl[k] for k in ks])
        UV = {}
        for k in ks:
            for a in F[k]:
                UV[(int(pid[a]), int(isl[k]))] = (float(vt[a][0]), float(vt[a][1]))
        parts[name] = Part(P, Fp, FI, UV)
    return parts


def export(obj, out, ratio, minfaces=14):
    parts = build_parts(obj)
    total0 = sum(len(p.faces) for p in parts.values())
    data = bytearray()
    data += struct.pack('>B', len(PARTS))
    # root height: pelvis above feet
    allmin = min(min(p.P[i][1] for i in p.P) for p in parts.values())
    root_h = int(round((HIP_Y - allmin) * SCALE))
    data += struct.pack('>h', root_h)
    tot = 0
    for name in PARTS:
        p = parts[name]
        n0 = len(p.faces)
        tgt = max(minfaces, int(n0 * ratio))
        n1 = p.decimate(tgt)
        j = JOINTS[name]
        verts, tris = p.export(j)
        tot += len(tris)
        rel = (int(round((j[0] - 0) * SCALE)), int(round((j[1] - HIP_Y) * SCALE)), int(round(j[2] * SCALE)))
        data += struct.pack('>hhh', *rel)
        data += struct.pack('>h', len(verts))
        for vv in verts:
            data += struct.pack('>hhhhh', *vv)
        data += struct.pack('>h', len(tris))
        for t in tris:
            data += struct.pack('>hhh', *t)
        print(f'  {name:7s} {n0:4d} -> {n1:4d} tris, {len(verts):4d} verts')
    open(out, 'wb').write(data)
    print(f'{out}: {total0} -> {tot} tris, {len(data)} bytes')
    return parts


if __name__ == '__main__':
    obj = sys.argv[1]
    export(obj, sys.argv[2], float(sys.argv[3]))
