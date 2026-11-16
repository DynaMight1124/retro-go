#!/usr/bin/env python3
"""FBX / SMD pedestrians -> npcN.bin (same 9-part rig as Tommy) + textures."""
import os, sys, struct, glob
import numpy as np
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
import mesh_tools
from fbxread import read_fbx, fbx_skeleton, fbx_mesh
RES = os.path.join(HERE, '..', 'res')
SRC = os.environ.get('NPCS', '/home/claude/work/pk/npc')
# part order = PedModel: torso, armA, foreA, armB, foreB, thighA, shinA, thighB, shinB ; A = character's RIGHT (OBJ x<0)
BONE_PART = {'R UpperArm': 1, 'R Forearm': 2, 'R Hand': 2, 'R Finger': 2, 'L UpperArm': 3, 'L Forearm': 4, 'L Hand': 4, 'L Finger': 4,
             'R Thigh': 5, 'R Calf': 6, 'R Foot': 6, 'R Toe0': 6, 'L Thigh': 7, 'L Calf': 8, 'L Foot': 8, 'L Toe0': 8}
def part_of(bone): return BONE_PART.get(bone, 0)
def seg_dist(P, a, b):
    ab = b - a; t = np.clip(((P - a) @ ab) / max(ab @ ab, 1e-9), 0, 1)
    return np.linalg.norm(P - (a + t[:, None] * ab), axis=1)
def mini(a, b):
    a = a / np.linalg.norm(a); b = b / np.linalg.norm(b); v = np.cross(a, b); c = a @ b; s = np.linalg.norm(v)
    if s < 1e-8: return np.eye(3)
    K = np.array([[0, -v[2], v[1]], [v[2], 0, -v[0]], [-v[1], v[0], 0]]); return np.eye(3) + K + K @ K * ((1 - c) / (s * s))

def segments(J):
    up = np.array([0, 1.0, 0]); S = {}
    def add(name, a, b): S[name] = (J[a], J[b])
    add('Pelvis', 'Pelvis', 'Spine'); add('Spine', 'Spine', 'Spine1'); add('Spine1', 'Spine1', 'Neck'); add('Neck', 'Neck', 'Head')
    S['Head'] = (J['Head'], J['Head'] + up * 22)
    add('LClav', 'Spine1', 'L UpperArm'); add('RClav', 'Spine1', 'R UpperArm')
    for s in 'LR':
        add(f'{s} UpperArm', f'{s} UpperArm', f'{s} Forearm'); add(f'{s} Forearm', f'{s} Forearm', f'{s} Hand')
        S[f'{s} Hand'] = (J[f'{s} Hand'], J[f'{s} Hand'] + (J[f'{s} Hand'] - J[f'{s} Forearm']) * 0.6)
        add(f'{s} Thigh', f'{s} Thigh', f'{s} Calf'); add(f'{s} Calf', f'{s} Calf', f'{s} Foot'); add(f'{s} Foot', f'{s} Foot', f'{s} Toe0')
    return S

def load_fbx(name):
    root, ver = read_fbx(os.path.join(SRC, name + '-Skin.fbx'))
    sk = fbx_skeleton(root); V, F, UV, UVI = fbx_mesh(root)
    J = {k: v[0] for k, v in sk.items()}
    S = segments(J)
    cen = V[F].mean(axis=1)
    names = list(S.keys()); D = np.stack([seg_dist(cen, *S[n]) * (0.85 if n in ('Pelvis', 'Spine', 'Spine1', 'Neck', 'Head', 'LClav', 'RClav') else 1.0) for n in names], 1)
    near = np.argmin(D, 1)
    fpart = np.array([part_of(names[k]) for k in near])
    return V, F, UV, UVI, fpart, J

# ---- SMD (Source) with skin weights
def load_smd(path):
    lines = open(path).read().replace('\r', '').split('\n'); i = 0
    nodes = {}; parent = {}
    while lines[i].strip() != 'nodes': i += 1
    i += 1
    while lines[i].strip() != 'end':
        t = lines[i].split('"'); idx = int(t[0]); nodes[idx] = t[1]; parent[idx] = int(t[2]); i += 1
    while lines[i].strip() != 'skeleton': i += 1
    i += 2; loc = {}
    while lines[i].strip() != 'end':
        v = lines[i].split(); loc[int(v[0])] = [float(x) for x in v[1:7]]; i += 1
    while lines[i].strip() != 'triangles': i += 1
    i += 1; P = []; UVs = []; W = []; F = []
    while lines[i].strip() != 'end':
        i += 1                                           # material line
        tri = []
        for k in range(3):
            v = lines[i].split(); i += 1
            P.append([float(x) for x in v[1:4]]); UVs.append([float(v[7]), float(v[8])])
            n = int(v[9]); W.append([(int(v[10 + 2*j]), float(v[11 + 2*j])) for j in range(n)])
            tri.append(len(P) - 1)
        F.append(tri)
    return nodes, parent, loc, np.array(P), np.array(UVs), W, np.array(F)
def smd_world(nodes, parent, loc):
    def eul(rx, ry, rz):
        a, b, c = rx, ry, rz
        Rx = np.array([[1, 0, 0], [0, np.cos(a), -np.sin(a)], [0, np.sin(a), np.cos(a)]])
        Ry = np.array([[np.cos(b), 0, np.sin(b)], [0, 1, 0], [-np.sin(b), 0, np.cos(b)]])
        Rz = np.array([[np.cos(c), -np.sin(c), 0], [np.sin(c), np.cos(c), 0], [0, 0, 1]])
        return Rz @ Ry @ Rx
    cache = {}
    def w(i):
        if i in cache: return cache[i]
        x, y, z, rx, ry, rz = loc[i]; R = eul(rx, ry, rz); T = np.array([x, y, z])
        if parent[i] >= 0 and parent[i] in loc:
            PR, PT = w(parent[i]); r = (PR @ R, PR @ T + PT)
        else: r = (R, T)
        cache[i] = r; return r
    return {nodes[i]: w(i)[1] for i in nodes if i in loc}

def normalize_pose(parts_v, J, sc_unused=None):
    """rotate arms to horizontal and legs to vertical about their root joints (parts_v: list of vertex arrays per part)"""
    for side, (arm, fore, th, sh), sgn in (('R', (1, 2, 5, 6), -1.0), ('L', (3, 4, 7, 8), 1.0)):
        sh_j = J[f'{side} UpperArm']; el = J[f'{side} Forearm']
        d = el - sh_j; R = mini(d, np.array([sgn, 0, 0]))
        for p in (arm, fore):
            parts_v[p] = (R @ (parts_v[p] - sh_j).T).T + sh_j
        J[f'{side} Forearm'] = R @ (el - sh_j) + sh_j; J[f'{side} Hand'] = R @ (J[f'{side} Hand'] - sh_j) + sh_j
        hip = J[f'{side} Thigh']; kn = J[f'{side} Calf']
        R2 = mini(kn - hip, np.array([0, -1.0, 0]))
        for p in (th, sh): parts_v[p] = (R2 @ (parts_v[p] - hip).T).T + hip
        J[f'{side} Calf'] = R2 @ (kn - hip) + hip
    return parts_v, J

def build_and_export(tag, V, F, UV, UVI, fpart, J, texw, texh, out_height_m, ratio_total=300):
    tag0 = tag
    """V: positions (y up, facing +z), F faces (pos idx), UV per face corner via UVI, fpart: part per face"""
    UV = UV.copy()
    for ax in (0, 1):                                    # UVs that sit in a neighbouring tile (e.g. v in [-1,0]) -> shift into [0,1+]
        mn = UV[:, ax].min()
        if mn < -1e-6: UV[:, ax] -= np.floor(mn)
    h = V[:, 1].max() - V[:, 1].min(); sc = 64.0 * out_height_m / h
    V = V.copy(); ymin = V[:, 1].min()
    # weld positions, islands via shared (pos, uv) corner pairs
    key = {}; pid = np.array([key.setdefault(tuple(np.round(p, 3)), len(key)) for p in V])
    PP = np.zeros((len(key), 3))
    for i, p in enumerate(V): PP[pid[i]] = p
    Fp = pid[F]
    par = list(range(len(F)))
    def find(x):
        while par[x] != x: par[x] = par[par[x]]; x = par[x]
        return x
    owner = {}
    for k in range(len(F)):
        for c in range(3):
            kk = (int(Fp[k][c]), int(UVI[k][c]))
            if kk in owner: par[find(k)] = find(owner[kk])
            else: owner[kk] = k
    isl = np.array([find(k) for k in range(len(F))])
    # per-part meshes, decimated
    parts = []
    tot0 = len(F); tgt_ratio = min(1.0, ratio_total / tot0)
    pv = []   # vertex arrays (for pose normalisation) built after decimation
    exported = []
    for pi in range(9):
        ks = [k for k in range(len(F)) if fpart[k] == pi and len({*Fp[k]}) == 3]
        Fk = Fp[ks]; FI = isl[ks]
        UVd = {}
        for j, k in enumerate(ks):
            for c in range(3): UVd[(int(Fp[k][c]), int(isl[k]))] = tuple(UV[UVI[k][c]])
        part = mesh_tools.Part(PP, Fk, FI, UVd)
        part.decimate(max(10, int(len(ks) * tgt_ratio)))
        vid = {}; verts = []; tris = []
        for k, f in part.faces.items():
            t = []
            for a in f:
                kk = (a, part.isl[k])
                if kk not in vid:
                    vid[kk] = len(verts); p = part.P[a]; u, v = part.uv[kk]
                    verts.append([p[0], p[1], p[2], u, v])
                t.append(vid[kk])
            tris.append(t)
        exported.append((np.array(verts), tris))
    # pose normalisation on decimated vertices
    pvs = [e[0][:, :3].copy() for e in exported]
    Jn = {k: v.copy() for k, v in J.items()}
    pvs, Jn = normalize_pose(pvs, Jn)
    pelvis = Jn['Pelvis']
    data = bytearray(); data += struct.pack('>B', 9)
    root_h = int(round((pelvis[1] - ymin) * sc)); data += struct.pack('>h', root_h)
    joint_of = {0: pelvis, 1: Jn['R UpperArm'], 2: Jn['R Forearm'], 3: Jn['L UpperArm'], 4: Jn['L Forearm'],
                5: Jn['R Thigh'], 6: Jn['R Calf'], 7: Jn['L Thigh'], 8: Jn['L Calf']}
    total = 0
    for pi in range(9):
        verts, tris = exported[pi]; jt = joint_of[pi]
        rel = (jt - pelvis) * sc
        data += struct.pack('>hhh', *[int(round(x)) for x in rel])
        data += struct.pack('>h', len(verts))
        for vi, v in enumerate(verts):
            p = (pvs[pi][vi] - jt) * sc
            data += struct.pack('>hhhhh', int(round(p[0])), int(round(p[1])), int(round(p[2])), int(round(v[3] * texw * 16)), int(round((1 - v[4]) * texh * 16)))
        data += struct.pack('>h', len(tris))
        for t in tris: data += struct.pack('>hhh', *t)
        total += len(tris)
    open(os.path.join(RES, f'npc_{tag}.bin'), 'wb').write(data)
    print(f'{tag}: {tot0} -> {total} tris, height {h:.1f} units -> {out_height_m} m, rootH {root_h}, {len(data)} bytes, parts', [len(e[1]) for e in exported])

def facing_fix(V, J, F=None):
    toe, foot = J['L Toe0'], J['L Foot']
    if toe[2] < foot[2]:                               # facing -z: rotate 180 deg about y
        R = np.diag([-1.0, 1, -1]); V = V @ R.T
        J = {k: R @ v for k, v in J.items()}
    return V, J

def texture(path, name, w, h):
    im = Image.open(path).convert('RGB').resize((w, h), Image.LANCZOS)
    im.save(os.path.join(RES, f't_{name}.png'), optimize=True)

def merge_list(names):
    p = os.path.join(RES, 'cartex.lst'); cur = open(p).read().split() if os.path.exists(p) else []
    for n in names:
        if n not in cur: cur.append(n)
    open(p, 'w').write('\n'.join(cur) + '\n')

MODELS = [('Constuction Worker', 'construction', 1.80, False), ('Hispanic Male Young Beach', 'hispanic', 1.78, False),
          ('White Female Young Skater', 'skater', 1.66, True), ('White Female Young Street', 'street', 1.66, True)]
def export_cop():
    nodes, parent, loc, P, UVs, W, F = load_smd(os.path.join(SRC, 'Cop.smd'))
    J = smd_world(nodes, parent, loc)
    # per-corner part from the dominant skin weight; faces take the part with the largest summed weight
    fpart = []
    for f in F:
        acc = np.zeros(9)
        for c in f:
            for b, w in W[c]: acc[part_of(nodes[b])] += w
        fpart.append(int(np.argmax(acc)))
    UVu, inv = np.unique(np.round(UVs, 4), axis=0, return_inverse=True)
    UVs = UVu; UVI = inv.reshape(-1)[F]
    texture(os.path.join(SRC, 'cop.png'), 'npc_cop', 64, 128)
    build_and_export('cop', P, F, UVs, UVI, np.array(fpart), J, 64, 128, 1.80)
    build_and_export('cop_lo', P, F, UVs, UVI, np.array(fpart), J, 64, 128, 1.80, 120)
    return ['npc_cop']

if __name__ == '__main__':
    texnames = export_cop()
    for fn, tag, hm, fem in MODELS:
        V, F, UV, UVI, fpart, J = load_fbx(fn)
        V, J = facing_fix(V, J)
        texture(os.path.join(SRC, fn + '-Texture.tga'), 'npc_' + tag, 64, 128); texnames.append('npc_' + tag)
        build_and_export(tag, V, F, UV, UVI, fpart, J, 64, 128, hm)
        build_and_export(tag + '_lo', V, F, UV, UVI, fpart, J, 64, 128, hm, 120)
    merge_list(texnames)
