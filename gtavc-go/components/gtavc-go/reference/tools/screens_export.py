#!/usr/bin/env python3
"""Vice City loading screens (LOADSC*.TXD, SPLASH1.TXD) -> res/ls0..ls3.png + intro.png at 320x240."""
import os, sys
from PIL import Image
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
from rwtools import read_txd
RES = os.path.join(HERE, '..', 'res')

def first(path):
    t = read_txd(path)
    return list(t.values())[0].convert('RGB')

if __name__ == '__main__':
    d = sys.argv[1]
    files = {f.upper(): os.path.join(d, f) for f in os.listdir(d)}
    first(files['SPLASH1.TXD']).resize((320, 240), Image.LANCZOS).save(os.path.join(RES, 'intro.png'), optimize=True)
    for i, n in enumerate(['LOADSC2.TXD', 'LOADSC5.TXD', 'LOADSC8.TXD', 'LOADSC11.TXD']):
        first(files[n]).resize((320, 240), Image.LANCZOS).save(os.path.join(RES, 'ls%d.png' % i), optimize=True)
    print('screens done')
