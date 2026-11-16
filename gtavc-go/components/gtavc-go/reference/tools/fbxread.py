"""Minimal binary FBX reader (nodes + properties, zlib arrays) for static meshes and skins."""
import struct, zlib
class Node:
    def __init__(s, name): s.name = name; s.props = []; s.kids = []
    def find(s, n): return [k for k in s.kids if k.name == n]
    def first(s, n):
        r = s.find(n); return r[0] if r else None
def read_fbx(path):
    d = open(path, 'rb').read()
    assert d[:20] == b'Kaydara FBX Binary  ', 'not binary fbx'
    ver = struct.unpack('<I', d[23:27])[0]; w64 = ver >= 7500
    def rec(o):
        if w64: end, np_, pl, nl = struct.unpack('<QQQB', d[o:o+25]); o += 25
        else: end, np_, pl, nl = struct.unpack('<IIIB', d[o:o+13]); o += 13
        if end == 0: return None, o
        n = Node(d[o:o+nl].decode('latin1')); o += nl
        for _ in range(np_):
            t = chr(d[o]); o += 1
            if t == 'Y': n.props.append(struct.unpack('<h', d[o:o+2])[0]); o += 2
            elif t == 'C': n.props.append(d[o] != 0); o += 1
            elif t == 'I': n.props.append(struct.unpack('<i', d[o:o+4])[0]); o += 4
            elif t == 'F': n.props.append(struct.unpack('<f', d[o:o+4])[0]); o += 4
            elif t == 'D': n.props.append(struct.unpack('<d', d[o:o+8])[0]); o += 8
            elif t == 'L': n.props.append(struct.unpack('<q', d[o:o+8])[0]); o += 8
            elif t in 'fdlib':
                ln, enc, clen = struct.unpack('<III', d[o:o+12]); o += 12
                raw = d[o:o+clen]; o += clen
                if enc == 1: raw = zlib.decompress(raw)
                code = {'f': 'f', 'd': 'd', 'l': 'q', 'i': 'i', 'b': 'b'}[t]
                import numpy as np
                n.props.append(np.frombuffer(raw, {'f': '<f4', 'd': '<f8', 'q': '<i8', 'i': '<i4', 'b': 'u1'}[code], ln))
            elif t == 'S':
                ln = struct.unpack('<I', d[o:o+4])[0]; o += 4; n.props.append(d[o:o+ln].decode('latin1')); o += ln
            elif t == 'R':
                ln = struct.unpack('<I', d[o:o+4])[0]; o += 4; n.props.append(d[o:o+ln]); o += ln
        while o < end:
            k, o = rec(o)
            if k is None: break
            n.kids.append(k)
        return n, end
    root = Node('root'); o = 27
    while o < len(d) - 40:
        k, o2 = rec(o)
        if k is None: break
        root.kids.append(k); o = o2
    return root, ver
def summarize(path):
    root, ver = read_fbx(path)
    print(path.split('/')[-1], 'version', ver)
    obj = root.first('Objects')
    import collections
    cnt = collections.Counter(k.name + ':' + (k.props[2] if len(k.props) > 2 and isinstance(k.props[2], str) else '') for k in obj.kids)
    print(dict(cnt))
    return root
if __name__ == '__main__':
    import sys
    for p in sys.argv[1:]: summarize(p)

# ---------------------------------------------------------------- skeleton + mesh helpers
import numpy as np
def euler_mat(rx, ry, rz):                       # FBX eEulerXYZ, degrees: R = Rz * Ry * Rx
    a, b, c = np.radians([rx, ry, rz])
    Rx = np.array([[1, 0, 0], [0, np.cos(a), -np.sin(a)], [0, np.sin(a), np.cos(a)]])
    Ry = np.array([[np.cos(b), 0, np.sin(b)], [0, 1, 0], [-np.sin(b), 0, np.cos(b)]])
    Rz = np.array([[np.cos(c), -np.sin(c), 0], [np.sin(c), np.cos(c), 0], [0, 0, 1]])
    return Rz @ Ry @ Rx
def fbx_skeleton(root):
    """returns {name: (world_pos, world_rot)} for the Null/bone models using the connection hierarchy"""
    obj = root.first('Objects'); conn = root.first('Connections')
    models = {}
    for k in obj.kids:
        if k.name == 'Model':
            nm = k.props[1].split('\x00')[0]; p = {'Lcl Translation': [0, 0, 0], 'Lcl Rotation': [0, 0, 0], 'PreRotation': [0, 0, 0]}
            for q in k.first('Properties70').kids:
                if q.props and q.props[0] in p: p[q.props[0]] = [float(x) for x in q.props[4:7]]
            models[k.props[0]] = (nm, k.props[2], p)
    parent = {}
    for c in conn.kids:
        if c.props[0] == 'OO' and c.props[1] in models and c.props[2] in models: parent[c.props[1]] = c.props[2]
    cache = {}
    def world(i):
        if i in cache: return cache[i]
        nm, kind, p = models[i]
        R = euler_mat(*p['PreRotation']) @ euler_mat(*p['Lcl Rotation']); T = np.array(p['Lcl Translation'])
        if i in parent:
            PR, PT = world(parent[i]); res = (PR @ R, PR @ T + PT)
        else: res = (R, T)
        cache[i] = res; return res
    out = {}
    for i, (nm, kind, p) in models.items():
        if kind == 'Mesh': continue
        R, T = world(i); out[nm] = (T, R)
    return out
def fbx_mesh(root):
    g = [k for k in root.first('Objects').kids if k.name == 'Geometry'][0]
    V = g.first('Vertices').props[0].reshape(-1, 3)
    pol = g.first('PolygonVertexIndex').props[0]
    uvn = g.first('LayerElementUV'); UV = uvn.first('UV').props[0].reshape(-1, 2); UVI = uvn.first('UVIndex').props[0]
    faces, uvidx = [], []
    for i in range(0, len(pol), 3):
        t = [int(x) if x >= 0 else ~int(x) for x in pol[i:i+3]]
        faces.append(t); uvidx.append([int(u) for u in UVI[i:i+3]])
    return V, np.array(faces), UV, np.array(uvidx)
