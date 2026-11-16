"""Minimal RenderWare DFF + TXD readers (GTA VC / PC D3D8 flavour)."""
import struct, numpy as np
from PIL import Image
def chunks(d, o, end):
    while o + 12 <= end:
        t, s, v = struct.unpack('<III', d[o:o+12])
        yield t, o+12, s
        o += 12 + s
# ---------------------------------------------------------------- TXD
def dxt1_block(b):
    c0, c1, bits = struct.unpack('<HHI', b[:8])
    def rgb(c): return ((c>>11&31)*255//31, (c>>5&63)*255//63, (c&31)*255//31)
    p = [rgb(c0), rgb(c1)]
    if c0 > c1: p += [tuple((2*a+b_)//3 for a, b_ in zip(p[0], p[1])), tuple((a+2*b_)//3 for a, b_ in zip(p[0], p[1]))]; al = [255]*4
    else: p += [tuple((a+b_)//2 for a, b_ in zip(p[0], p[1])), (0, 0, 0)]; al = [255, 255, 255, 0]
    out = np.zeros((4, 4, 4), np.uint8)
    for i in range(16):
        k = bits >> (2*i) & 3; out[i//4, i%4, :3] = p[k]; out[i//4, i%4, 3] = al[k]
    return out
def decode_dxt(data, w, h, kind):
    img = np.zeros((h, w, 4), np.uint8); bs = 8 if kind == 1 else 16; o = 0
    for by in range((h+3)//4):
        for bx in range((w+3)//4):
            blk = data[o:o+bs]; o += bs
            if kind == 1: t = dxt1_block(blk)
            else:
                t = dxt1_block(blk[8:]); 
                if kind == 3:
                    a = int.from_bytes(blk[:8], 'little')
                    for i in range(16): t[i//4, i%4, 3] = (a >> (4*i) & 15) * 17
                else:
                    a0, a1 = blk[0], blk[1]; bits = int.from_bytes(blk[2:8], 'little')
                    al = [a0, a1] + ([(( 6-i)*a0+i*a1)//7 for i in range(1, 7)] if a0 > a1 else [((5-i)*a0+i*a1)//5 for i in range(1, 5)]+[0, 255])
                    for i in range(16): t[i//4, i%4, 3] = al[bits >> (3*i) & 7]
            hh, ww = min(4, h-by*4), min(4, w-bx*4)
            img[by*4:by*4+hh, bx*4:bx*4+ww] = t[:hh, :ww]
    return img
def read_txd(path):
    d = open(path, 'rb').read(); out = {}
    for t, o, s in chunks(d, 0, len(d)):
        if t != 0x16: continue
        for t2, o2, s2 in chunks(d, o, o+s):
            if t2 != 0x15: continue
            for t3, o3, s3 in chunks(d, o2, o2+s2):
                if t3 != 0x01: continue
                plat, filt = struct.unpack('<II', d[o3:o3+8])
                name = d[o3+8:o3+40].split(b'\0')[0].decode('latin1'); p = o3+72
                rfmt, alpha = struct.unpack('<II', d[p:p+8]); w, h = struct.unpack('<HH', d[p+8:p+12])
                depth, nlev, rtype, comp = d[p+12], d[p+13], d[p+14], d[p+15]; p += 16
                pal = None
                if rfmt & 0x2000: pal = np.frombuffer(d[p:p+1024], np.uint8).reshape(256, 4); p += 1024
                size = struct.unpack('<I', d[p:p+4])[0]; data = d[p+4:p+4+size]
                if comp in (1, 3, 4, 5): img = decode_dxt(data, w, h, {1:1, 3:3, 4:3, 5:5}[comp]) if comp in (1, 3, 5) else decode_dxt(data, w, h, 3)
                elif pal is not None:
                    idx = np.frombuffer(data[:w*h], np.uint8).reshape(h, w); img = pal[idx]
                elif depth == 32:
                    a = np.frombuffer(data[:w*h*4], np.uint8).reshape(h, w, 4); img = a[..., [2, 1, 0, 3]]
                elif depth == 16:
                    v = np.frombuffer(data[:w*h*2], '<u2').reshape(h, w).astype(int); fm = rfmt & 0xF00
                    if fm == 0x100: img = np.stack([(v>>10&31)*255//31, (v>>5&31)*255//31, (v&31)*255//31, (v>>15)*255], -1)
                    else: img = np.stack([(v>>11&31)*255//31, (v>>5&63)*255//63, (v&31)*255//31, v*0+255], -1)
                else: img = np.zeros((h, w, 4), np.uint8) + 128
                out[name.lower()] = Image.fromarray(np.asarray(img, np.uint8), 'RGBA')
    return out
# ---------------------------------------------------------------- DFF
def read_dff(path):
    d = open(path, 'rb').read()
    res = {'frames': [], 'geoms': [], 'atomics': []}
    def walk_clump(o, s):
        for t, o2, s2 in chunks(d, o, o+s):
            if t == 0x0E:                                # frame list
                n = struct.unpack('<I', d[o2+12:o2+16])[0]; p = o2+12+4; fr = []
                for i in range(n):
                    v = struct.unpack('<12fII', d[p:p+56]); p += 56
                    fr.append({'rot': np.array(v[:9]).reshape(3, 3), 'pos': np.array(v[9:12]), 'parent': struct.unpack('<i', struct.pack('<I', v[12]))[0], 'name': ''})
                q = o2 + 12 + s2 if False else None
                # extensions follow the struct chunk
                p2 = o2 + 12 + struct.unpack('<I', d[o2+4:o2+8])[0]; i = 0
                end = o2 + s2
                while p2 + 12 <= end and i < n:
                    t3, s3, v3 = struct.unpack('<III', d[p2:p2+12])
                    if t3 == 0x03:
                        for t4, o4, s4 in chunks(d, p2+12, p2+12+s3):
                            if t4 == 0x253F2FE: fr[i]['name'] = d[o4:o4+s4].split(b'\0')[0].decode('latin1')
                    i += 1; p2 += 12 + s3
                res['frames'] = fr
            elif t == 0x1A:                              # geometry list
                for t3, o3, s3 in chunks(d, o2, o2+s2):
                    if t3 == 0x0F: res['geoms'].append(read_geom(o3, s3))
            elif t == 0x14:                              # atomic
                fi, gi, fl, _ = struct.unpack('<IIII', d[o2+12:o2+28]); res['atomics'].append((fi, gi))
    def read_geom(o, s):
        g = {'mats': [], 'tris': None}
        for t, o2, s2 in chunks(d, o, o+s):
            if t == 0x01:
                p = o2; flags, ntri, nvert, nmorph = struct.unpack('<IIII', d[p:p+16]); p += 16
                lv = struct.unpack('<I', d[o2-4:o2])[0]
                ver = (((lv >> 14) & 0x3FF00) | ((lv >> 16) & 0x3F) | 0x30000) if (lv & 0xFFFF0000) else (lv << 8)
                if ver < 0x34000: p += 12
                nuv = (flags >> 16) & 0xFF
                if nuv == 0: nuv = 2 if flags & 0x80 else (1 if flags & 0x04 else 0)
                if flags & 0x08: g['prelit'] = np.frombuffer(d[p:p+nvert*4], np.uint8).reshape(nvert, 4); p += nvert*4
                uvs = []
                for k in range(nuv): uvs.append(np.frombuffer(d[p:p+nvert*8], '<f4').reshape(nvert, 2)); p += nvert*8
                g['uv'] = uvs[0] if uvs else None
                tr = np.frombuffer(d[p:p+ntri*8], '<u2').reshape(ntri, 4); p += ntri*8
                g['tris'] = np.stack([tr[:, 1], tr[:, 0], tr[:, 3], tr[:, 2]], 1)   # v1,v2,v3,mat
                p += 16; hasv, hasn = struct.unpack('<II', d[p:p+8]) if False else (1, 1); 
                bs = struct.unpack('<4f', d[p-16:p]); p += 0
                hv = struct.unpack('<I', d[p:p+4])[0]; hn = struct.unpack('<I', d[p+4:p+8])[0]; p += 8
                g['verts'] = np.frombuffer(d[p:p+nvert*12], '<f4').reshape(nvert, 3); p += nvert*12
                if hn: g['norms'] = np.frombuffer(d[p:p+nvert*12], '<f4').reshape(nvert, 3)
            elif t == 0x08:
                for t2, o3, s3 in chunks(d, o2, o2+s2):
                    if t2 == 0x07:
                        m = {'color': (255, 255, 255, 255), 'tex': None}
                        for t4, o4, s4 in chunks(d, o3, o3+s3):
                            if t4 == 0x01:
                                m['color'] = tuple(d[o4+4:o4+8]); m['textured'] = struct.unpack('<I', d[o4+12:o4+16])[0]
                            elif t4 == 0x06:
                                for t5, o5, s5 in chunks(d, o4, o4+s4):
                                    if t5 == 0x02 and m['tex'] is None: m['tex'] = d[o5:o5+s5].split(b'\0')[0].decode('latin1').lower()
                        g['mats'].append(m)
        return g
    for t, o, s in chunks(d, 0, len(d)):
        if t == 0x10: walk_clump(o, s)
    return res
if __name__ == '__main__':
    import sys
    r = read_dff(sys.argv[1])
    print(len(r['frames']), 'frames,', len(r['geoms']), 'geoms,', len(r['atomics']), 'atomics')
    for i, f in enumerate(r['frames']): print(i, f['name'], 'parent', f['parent'], np.round(f['pos'], 2))
    for fi, gi in r['atomics']:
        g = r['geoms'][gi]; print('atomic frame', fi, r['frames'][fi]['name'], 'geom', gi, 'tris', len(g['tris']), 'verts', len(g['verts']), 'mats', [(m['tex'], m['color'][:3]) for m in g['mats']])
