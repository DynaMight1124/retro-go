import os
import re

dir_path = "src/goddard/dynlists"

for filename in os.listdir(dir_path):
    if filename.endswith(".c"):
        path = os.path.join(dir_path, filename)
        with open(path, "r") as f:
            content = f.read()
        
        # Add const to struct DynList
        content = re.sub(r'(?:const\s+)*struct DynList ([a-zA-Z0-9_]+)\[', r'const struct DynList \1[', content)
        # Add const to s16 animdata
        content = re.sub(r'(?:const\s+)*s16 (animdata_[a-zA-Z0-9_]+)\[', r'const s16 \1[', content)
        # Add const to struct AnimDataInfo
        content = re.sub(r'(?:const\s+)*struct AnimDataInfo ([a-zA-Z0-9_]+)\[', r'const struct AnimDataInfo \1[', content)
        
        with open(path, "w") as f:
            f.write(content)
