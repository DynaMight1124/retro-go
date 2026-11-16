#!/usr/bin/env python3
"""Pre-decode 32x32 map textures to a raw ARGB cache.

The device has no VRAM; stb PNG decoding during tile streaming was the main
cause of multi-second freezes.  Each /x/<id>.png gets a sibling /x/<id>.raw:
u32 width, u32 height, then width*height little-endian ARGB pixels.  The
pixels are exactly what Assets_decode() produces from the PNG (alpha < 128 is
transparent 0, otherwise alpha is forced to 0xFF).
"""
import glob
import os
import struct
from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'assets', 'x')

def main():
    files = sorted(glob.glob(os.path.join(ROOT, '*.png')))
    total = 0
    for n, path in enumerate(files):
        im = Image.open(path).convert('RGBA')
        w, h = im.size
        pix = []
        for r, g, b, a in im.getdata():
            pix.append(0 if a < 128 else (0xFF000000 | (r << 16) | (g << 8) | b))
        out = os.path.splitext(path)[0] + '.raw'
        data = struct.pack('<II', w, h) + struct.pack('<%dI' % len(pix), *pix)
        with open(out, 'wb') as f:
            f.write(data)
        total += len(data)
    print('predecoded', len(files), 'textures,', total / 1048576.0, 'MB')

if __name__ == '__main__':
    main()
