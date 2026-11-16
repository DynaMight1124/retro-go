#!/usr/bin/env python3
"""handling.cfg -> res/handling.txt: name mass traction maxKmh accel brake lock (lower-case names)."""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, '..', 'assets', 'data', 'handling.cfg')
out = []
for l in open(src, encoding='latin1'):
    p = l.split()
    if len(p) < 21 or l.startswith((';', '!', '$', '%')) or not p[0][0].isalpha(): continue
    try:
        out.append('%s %d %d %d %d %d %d' % (p[0].lower(), float(p[1]), float(p[9]) * 100, float(p[13]), float(p[14]) * 10, float(p[17]) * 100, float(p[20])))
    except ValueError: pass
open(os.path.join(HERE, '..', 'res', 'handling.txt'), 'w').write('\n'.join(out) + '\n')
print(len(out), 'vehicles')
