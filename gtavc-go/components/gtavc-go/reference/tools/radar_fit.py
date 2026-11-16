#!/usr/bin/env python3
"""Fits a Vice City radar/map picture onto the game world and writes res/radar.png + res/radar.txt.

  python3 tools/radar_fit.py path/to/map.jpg
The land outline of the picture is matched against the land in res/mapidx.bin (scale + offset search),
so any reasonably complete VC map image works. radar.txt: x of the left edge (mm), z of the top edge (mm), mm per pixel.
"""
import os, sys, struct
import numpy as np
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__))
RES = os.path.join(HERE, '..', 'res')
OUTW = 448

d = open(os.path.join(RES, 'mapidx.bin'), 'rb').read()
x0, z0, _, nx, nz = struct.unpack_from('>iiHHH', d, 4)
p = 4 + 14 + (nx * nz + 7) // 8 + 2
rx0, rz0, rc, rw, rh = struct.unpack_from('>iiHHH', d, p)
rad = np.frombuffer(d, np.uint8, rw * rh, p + 14).reshape(rh, rw)
land = rad > 0                                                  # rows: z increasing
cell = rc / 1024.0                                              # metres

im = Image.open(sys.argv[1]).convert('RGB')
a = np.asarray(im).astype(int)
sea = (np.abs(a[..., 0] - 120) < 45) & (np.abs(a[..., 1] - 190) < 45) & (a[..., 2] > 200)
iland = ~sea

def bbox(m):
    ys, xs = np.where(m)
    return xs.min(), xs.max(), ys.min(), ys.max()

# world land bbox (metres, engine x / z)
wx0, wx1, wz0, wz1 = bbox(land)
WX0, WX1 = rx0 / 1024 + wx0 * cell, rx0 / 1024 + (wx1 + 1) * cell
WZ0, WZ1 = rz0 / 1024 + wz0 * cell, rz0 / 1024 + (wz1 + 1) * cell
px0, px1, py0, py1 = bbox(iland)
s0 = ((WX1 - WX0) / (px1 - px0) + (WZ1 - WZ0) / (py1 - py0)) / 2      # metres per pixel
best = None
small = Image.fromarray(iland.astype(np.uint8) * 255)
for ds in np.linspace(0.92, 1.08, 17):
    s = s0 * ds
    for ox in np.linspace(-120, 120, 25):
        for oz in np.linspace(-120, 120, 25):
            ax = WX0 + ox - px0 * s                                 # world x at picture column 0
            az = WZ1 + oz + py0 * s                                 # world z at picture row 0 (north up)
            # sample the picture at world land cells (subsampled)
            ys, xs = np.mgrid[0:rh:3, 0:rw:3]
            wx = rx0 / 1024 + (xs + 0.5) * cell; wz = rz0 / 1024 + (ys + 0.5) * cell
            pi = ((wx - ax) / s).astype(int); pj = ((az - wz) / s).astype(int)
            ok = (pi >= 0) & (pj >= 0) & (pi < iland.shape[1]) & (pj < iland.shape[0])
            pl = np.zeros_like(ok); pl[ok] = iland[pj[ok], pi[ok]]
            wl = land[ys, xs]
            iou = (pl & wl).sum() / max(1, (pl | wl).sum())
            if best is None or iou > best[0]: best = (iou, s, ax, az)
iou, s, ax, az = best
print('fit IoU %.3f, %.3f m/px' % (iou, s))
f = OUTW / im.size[0]
out = im.resize((OUTW, int(im.size[1] * f)), Image.LANCZOS)
out.save(os.path.join(RES, 'radar.png'), optimize=True)
open(os.path.join(RES, 'radar.txt'), 'w').write('%d %d %d\n' % (int(ax * 1024), int(az * 1024), int(s / f * 1024)))
