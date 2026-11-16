#!/usr/bin/env python3
"""Pack x/*.raw into x/xatlas.bin (one open per texture instead of ~70)."""
import os, struct
ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'assets', 'x')
out = os.path.join(ROOT, 'xatlas.bin')
ids = sorted(int(f[:-4]) for f in os.listdir(ROOT) if f.endswith('.raw'))
with open(out, 'wb') as o:
    o.write(b'XAT1')
    o.write(struct.pack('<I', len(ids)))
    for i in ids:
        b = open(os.path.join(ROOT, '%d.raw' % i), 'rb').read()
        if len(b) != 4104 or b[:8] != struct.pack('<II', 32, 32):
            raise SystemExit('bad raw %d (%d bytes)' % (i, len(b)))
        o.write(b[8:])
print('atlas', len(ids), 'textures,', os.path.getsize(out) / 1048576.0, 'MB')
