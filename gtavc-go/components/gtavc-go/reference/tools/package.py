#!/usr/bin/env python3
"""Packs build/cldc classes + res/ into the final JAR/JAD.

  python3 tools/package.py dist                 full map
  MAP_REGION=i0,j0,i1,j1 python3 tools/package.py dist   only tiles in that index box (smaller JAR);
                                                tiles outside are simply missing (sea/void in game)
  JAR_NAME=GTA_VC_J2ME_lite                     output name
"""
import os, sys, zipfile, struct

HERE = os.path.dirname(os.path.abspath(__file__))
P = os.path.join(HERE, '..')
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(P, 'dist')
os.makedirs(OUT, exist_ok=True)
NAME = os.environ.get('JAR_NAME', 'GTA_VC_J2ME')
VER = os.environ.get('MIDLET_VERSION', '0.3.0')
REGION = os.environ.get('MAP_REGION')
manifest = '\r\n'.join([
    'Manifest-Version: 1.0',
    'MIDlet-Name: GTA Vice City',
    'MIDlet-Version: ' + VER,
    'MIDlet-Vendor: AidenCantCode',
    'MIDlet-1: GTA Vice City, /icon.png, GtaMidlet',
    'MIDlet-Description: Vice City open world port',
    'Nokia-MIDlet-Category: Game',
    'Nokia-MIDlet-On-Screen-Keypad: no',
    'MicroEdition-Configuration: CLDC-1.1',
    'MicroEdition-Profile: MIDP-2.0',
    'Created-By: 1.3 (preverified)', '', ''])

res = os.path.join(P, 'res')
LEGACY = os.environ.get('LEGACY') == '1'
if LEGACY:
    import io
    from PIL import Image
    NAME = os.environ.get('JAR_NAME', 'GTA_VC_J2ME_legacy')
    resL = os.path.join(P, 'res_legacy')
    CARS = ['police', 'admiral', 'taxi', 'pcj600']
    PEDS = ['player', 'cop', 'hmyst', 'wfyst', 'wmybe']
    VOICES = ['v_mpanic0', 'v_fpanic0', 'v_mdeath0', 'v_fdeath0', 'v_fight0', 'v_chat0', 'v_tommy0']
    def small_png(path, size):
        im = Image.open(path); im = im.convert('RGBA' if im.mode in ('RGBA', 'LA', 'P') else 'RGB')
        im = im.resize((min(size, im.size[0]), min(size, im.size[1])), Image.LANCZOS)
        b = io.BytesIO(); im.save(b, 'PNG', optimize=True); return b.getvalue()
    def car_tex(name):
        d = open(os.path.join(res, 'car_%s.bin' % name), 'rb').read(); n = d[4]; p = 5; out = []
        for _ in range(n): l = d[p]; out.append(d[p + 1:p + 1 + l].decode()); p += 1 + l
        return out
    def wtex(name):
        d = open(os.path.join(res, name), 'rb').read(); n = d[0]; p = 1; out = []
        for _ in range(n): l = d[p]; out.append(d[p + 1:p + 1 + l].decode()); p += 1 + l
        return out
    manifest = manifest.replace('MIDlet-Description: Vice City open world port', 'MIDlet-Description: Vice City legacy (Ocean Drive)')
    jar = os.path.join(OUT, NAME + '.jar')
    tilesL = sorted(os.listdir(os.path.join(resL, 't')))
    usedL = set()
    for f in tilesL:
        d = open(os.path.join(resL, 't', f), 'rb').read(); n = struct.unpack_from('>H', d, 2)[0]
        usedL.update(struct.unpack_from('>%dH' % n, d, 4))
    texs = set(['wheel_saloon64', 'tyre64a', 'blood'])
    for c in CARS: texs.update(car_tex(c))
    for w in ('w_bat.bin', 'w_colt45.bin'): texs.update(wtex(w))
    with zipfile.ZipFile(jar, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        z.writestr('META-INF/MANIFEST.MF', manifest)
        cl = os.path.join(P, 'build', 'cldc')
        for f in sorted(os.listdir(cl)):
            if f.endswith('.class'): z.write(os.path.join(cl, f), f)
        z.writestr('legacy.txt', 'ocean drive\n')
        z.writestr('cars.lst', '\n'.join(CARS) + '\n')
        pl = [l for l in open(os.path.join(res, 'peds.lst')).read().split('\n') if l.split(' ')[0] in PEDS]
        z.writestr('peds.lst', '\n'.join(pl) + '\n')
        for f in sorted(os.listdir(res)):
            fp = os.path.join(res, f)
            if os.path.isdir(fp) or f in ('peds.lst', 'cars.lst', 'mapidx.bin') or f.endswith('.3gp'): continue
            if f.startswith('car_') and f[4:-4] not in CARS: continue
            if f.startswith('p_') and f[2:-4].replace('_lo', '') not in PEDS: continue
            if f.startswith('t_p_'):
                if f[4:-4] in PEDS: z.writestr(f, small_png(fp, 32))
                continue
            if f.startswith('t_'):
                if f[2:-4] in texs: z.writestr(f, small_png(fp, 32))
                continue
            if f.startswith('v_') and f[:-4] not in VOICES: continue
            if f in ('ls1.png', 'ls2.png', 'ls3.png'): continue
            if f == 'radar.png': z.writestr(f, small_png(fp, 224)); continue
            if f == 'radar.txt':
                ax, az, cell = [int(v) for v in open(fp).read().split()]
                w0 = Image.open(os.path.join(res, 'radar.png')).size[0]
                z.writestr(f, '%d %d %d\n' % (ax, az, cell * w0 // min(224, w0))); continue
            z.write(fp, f)
        z.write(os.path.join(resL, 'mapidx.bin'), 'mapidx.bin')
        for f in tilesL: z.write(os.path.join(resL, 't', f), 't/' + f)
        for t in sorted(usedL): z.write(os.path.join(res, 'x', '%d.png' % t), 'x/%d.png' % t, compress_type=zipfile.ZIP_STORED)
    size = os.path.getsize(jar)
    jad = manifest.rstrip('\r\n').split('\r\n') + ['MIDlet-Jar-URL: ' + NAME + '.jar', 'MIDlet-Jar-Size: %d' % size]
    open(os.path.join(OUT, NAME + '.jad'), 'w', newline='').write('\r\n'.join(jad) + '\r\n')
    print('legacy jar', jar, size, 'bytes,', len(tilesL), 'tiles,', len(usedL), 'map textures')
    sys.exit(0)
tiles = sorted(os.listdir(os.path.join(res, 't'))) if os.path.isdir(os.path.join(res, 't')) else []
if REGION:
    i0, j0, i1, j1 = [int(v) for v in REGION.split(',')]
    keep = []
    for f in tiles:
        i, j = [int(v) for v in f[:-4].split('_')]
        if i0 <= i <= i1 and j0 <= j <= j1: keep.append(f)
    tiles = keep
used = set()
for f in tiles:
    d = open(os.path.join(res, 't', f), 'rb').read()
    n = struct.unpack_from('>H', d, 2)[0]
    used.update(struct.unpack_from('>%dH' % n, d, 4))

jar = os.path.join(OUT, NAME + '.jar')
with zipfile.ZipFile(jar, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
    z.writestr('META-INF/MANIFEST.MF', manifest)
    cl = os.path.join(P, 'build', 'cldc')
    for f in sorted(os.listdir(cl)):
        if f.endswith('.class'): z.write(os.path.join(cl, f), f)
    for f in sorted(os.listdir(res)):
        p = os.path.join(res, f)
        if os.path.isdir(p): continue
        if f.endswith('.3gp'): z.write(p, f, compress_type=zipfile.ZIP_STORED)
        else: z.write(p, f)
    for f in tiles: z.write(os.path.join(res, 't', f), 't/' + f)
    for t in sorted(used):
        f = os.path.join(res, 'x', '%d.png' % t)
        if os.path.exists(f): z.write(f, 'x/%d.png' % t, compress_type=zipfile.ZIP_STORED)
size = os.path.getsize(jar)
jad = manifest.rstrip('\r\n').split('\r\n')
jad += ['MIDlet-Jar-URL: ' + NAME + '.jar', 'MIDlet-Jar-Size: %d' % size]
open(os.path.join(OUT, NAME + '.jad'), 'w', newline='').write('\r\n'.join(jad) + '\r\n')
print('jar', jar, size, 'bytes,', len(tiles), 'tiles,', len(used), 'map textures')
