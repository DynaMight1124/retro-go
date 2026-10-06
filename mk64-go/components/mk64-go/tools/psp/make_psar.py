#!/usr/bin/env python3
"""DATA.PSAR container for the EBOOT: the recipe blob plus the splash background.

  MK64PSAR | u32 count | count x { char tag[4]; u32 off; u32 size } | payloads
  RCP1 = recipes.bin (numbers only), PIC1 = 480x272 RGB565 of icon/MK64-PIC1.png

usage: make_psar.py --recipes recipes.bin --pic1 pic1.png -o data.psar
"""
import argparse, struct
from PIL import Image

def rgb565(png):
    im = Image.open(png).convert('RGB').resize((480, 272))
    out = bytearray()
    for r, g, b in im.getdata():
        out += struct.pack('<H', ((b >> 3) << 11) | ((g >> 2) << 5) | (r >> 3))  # PSP 565: R in the low bits
    return bytes(out)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--recipes', required=True); ap.add_argument('--pic1'); ap.add_argument('-o', required=True)
    a = ap.parse_args()
    entries = [(b'RCP1', open(a.recipes, 'rb').read())]
    if a.pic1: entries.append((b'PIC1', rgb565(a.pic1)))
    hdr_len = 8 + 4 + 12 * len(entries)
    off = (hdr_len + 15) & ~15
    table = b''; payload = b''
    for tag, data in entries:
        table += struct.pack('<4sII', tag, off + len(payload), len(data))
        payload += data + b'\0' * (-len(data) % 16)
    out = b'MK64PSAR' + struct.pack('<I', len(entries)) + table
    out += b'\0' * (off - len(out)) + payload
    open(a.o, 'wb').write(out)
    print('wrote %s: %d entries, %d bytes' % (a.o, len(entries), len(out)))

if __name__ == '__main__':
    main()
