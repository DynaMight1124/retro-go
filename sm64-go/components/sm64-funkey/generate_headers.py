import re

with open('levels/level_defines.h', 'r') as f:
    data = f.read()

out = []
for m in re.finditer(r'DEFINE_LEVEL\([^,]+,\s*[^,]+,\s*[^,]+,\s*([^,]+)', data):
    folder = m.group(1).strip()
    out.append(f'#include "levels/{folder}/header.h"\n')

with open('include/level_headers.h', 'w') as f:
    f.writelines(out)
