#!/usr/bin/env python3
"""Check that the port keeps the N64's data layout where the game relies on it.

The decomp names many data symbols after their N64 address (D_800E8254 ...),
and the game indexes past the end of several tables into their neighbours
(menu_items.c: D_800E8274[type - 0x12], D_800E70A0[type - 0xA], ...).  The
port compiles the game code with -fno-toplevel-reorder so consecutive globals
keep their source (= ROM) order; this lists the consecutive address-named
symbols whose spacing still differs from the N64's, and checks the tables
known to be walked.

usage: check_layout.py build/psp/mk64.elf
"""
import os, re, subprocess, sys

WALKED = [('D_800E8254', 'D_800E8274'), ('D_800E8274', 'D_800E8284'), ('D_800E8284', 'D_800E828C'),
          ('D_800E70A0', 'D_800E70E8'), ('D_800E7248', 'D_800E7258'), ('D_800E7258', 'D_800E7268'),
          ('D_800E7148', 'D_800E7168')]


def main():
    elf = sys.argv[1]
    nm = subprocess.run(['psp-nm', '-n', '-S', elf], capture_output=True, text=True).stdout.splitlines()
    rows, addr = [], {}
    for l in nm:
        f = l.split()
        if len(f) == 4 and f[2] in 'DdBbRr':
            addr[f[3]] = int(f[0], 16)
            m = re.fullmatch(r'D_(80[01][0-9A-F]{5})', f[3])
            if m:
                rows.append((int(f[0], 16), int(m.group(1), 16), f[3]))
    rows.sort()
    ok, bad = 0, []
    for (a1, n1, s1), (a2, n2, s2) in zip(rows, rows[1:]):
        if 0 < n2 - n1 <= 0x4000 and a2 - a1 <= 0x4000:
            if a2 - a1 == n2 - n1:
                ok += 1
            else:
                bad.append((s1, s2, n2 - n1, a2 - a1))
    print(f'{ok} consecutive address-named symbols keep their N64 spacing, {len(bad)} differ')
    fail = 0
    for a, b in WALKED:
        port, n64 = addr[b] - addr[a], int(b[2:], 16) - int(a[2:], 16)
        print(f'  {a} -> {b}: port {port:#x} N64 {n64:#x} {"ok" if port == n64 else "MISMATCH"}')
        fail += port != n64
    if '-v' in sys.argv:
        for b in bad:
            print('  %s -> %s: N64 delta %#x, port delta %#x' % b)
    sys.exit(1 if fail else 0)


if __name__ == '__main__':
    os.environ['PATH'] = os.path.expanduser('~/pspdev/bin') + ':' + os.environ['PATH']
    main()
