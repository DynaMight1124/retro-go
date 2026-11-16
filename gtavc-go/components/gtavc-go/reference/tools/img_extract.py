#!/usr/bin/env python3
"""Pull files out of a GTA III / Vice City gta3.img + gta3.dir (version 1 archive).

  python3 tools/img_extract.py path/to/gta3.img out_dir admiral.dff admiral.txd ...
  python3 tools/img_extract.py path/to/gta3.img --list | grep dff

Entries in gta3.dir are 32 bytes: u32 offset, u32 size (both in 2048-byte sectors), char name[24].
gta3.img itself is too big for GitHub (100 MB file limit), so keep it outside the repo and
copy only the models you need into assets/.
"""
import os, sys, struct

def read_dir(img):
    d = open(os.path.splitext(img)[0] + '.dir', 'rb').read()
    out = {}
    for i in range(len(d) // 32):
        off, size = struct.unpack_from('<II', d, i * 32)
        out[d[i * 32 + 8:i * 32 + 32].split(b'\0')[0].decode('latin1').lower()] = (off, size)
    return out

if __name__ == '__main__':
    img = sys.argv[1]
    ents = read_dir(img)
    if sys.argv[2] == '--list':
        for n in sorted(ents): print(n)
        sys.exit(0)
    out = sys.argv[2]; os.makedirs(out, exist_ok=True)
    with open(img, 'rb') as f:
        for n in sys.argv[3:]:
            off, size = ents[n.lower()]
            f.seek(off * 2048)
            open(os.path.join(out, n.lower()), 'wb').write(f.read(size * 2048))
            print('extracted', n)
