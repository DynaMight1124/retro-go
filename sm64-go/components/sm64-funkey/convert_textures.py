import os
import glob
from PIL import Image

def convert_rgba16(img):
    img = img.convert("RGBA")
    data = img.getdata()
    out = bytearray()
    for r, g, b, a in data:
        r = r >> 3
        g = g >> 3
        b = b >> 3
        a = 1 if a > 0 else 0
        val = (r << 11) | (g << 6) | (b << 1) | a
        out.append((val >> 8) & 0xFF)
        out.append(val & 0xFF)
    return out

def convert_rgba32(img):
    img = img.convert("RGBA")
    data = img.getdata()
    out = bytearray()
    for r, g, b, a in data:
        out.extend([r, g, b, a])
    return out

def convert_ia16(img):
    img = img.convert("RGBA")
    data = img.getdata()
    out = bytearray()
    for r, g, b, a in data:
        i = int(0.299 * r + 0.587 * g + 0.114 * b)
        out.extend([i, a])
    return out

def convert_ia8(img):
    img = img.convert("RGBA")
    data = img.getdata()
    out = bytearray()
    for r, g, b, a in data:
        i = int(0.299 * r + 0.587 * g + 0.114 * b)
        val = ((i >> 4) << 4) | (a >> 4)
        out.append(val)
    return out

def convert_ia4(img):
    img = img.convert("RGBA")
    data = list(img.getdata())
    out = bytearray()
    for j in range(0, len(data), 2):
        val = 0
        for k in range(2):
            if j+k < len(data):
                r, g, b, a = data[j+k]
                i = int(0.299 * r + 0.587 * g + 0.114 * b)
                v = ((i >> 5) << 1) | (1 if a > 0 else 0)
            else:
                v = 0
            val |= (v << (4 if k == 0 else 0))
        out.append(val)
    return out

def convert_ia1(img):
    img = img.convert("RGBA")
    data = list(img.getdata())
    out = bytearray()
    for j in range(0, len(data), 8):
        val = 0
        for k in range(8):
            if j+k < len(data):
                r, g, b, a = data[j+k]
                i = int(0.299 * r + 0.587 * g + 0.114 * b)
                v = 1 if i > 127 else 0
            else:
                v = 0
            val |= (v << (7 - k))
        out.append(val)
    return out

def write_c_array(out_path, data):
    with open(out_path, "w") as f:
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            f.write("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")

def process_file(path):
    parts = path.split('.')
    if len(parts) < 3: return
    fmt = parts[-2]
    try:
        img = Image.open(path)
    except Exception as e:
        print(f"Error opening {path}: {e}")
        return
        
    funcs = {
        'rgba16': convert_rgba16,
        'rgba32': convert_rgba32,
        'ia16': convert_ia16,
        'ia8': convert_ia8,
        'ia4': convert_ia4,
        'ia1': convert_ia1,
    }
    
    if fmt in funcs:
        data = funcs[fmt](img)
        out_path = path[:-4] + ".inc.c"
        write_c_array(out_path, data)
        print(f"Converted {path} -> {out_path}")

if __name__ == "__main__":
    for root, dirs, files in os.walk("textures"):
        for file in files:
            if file.endswith(".png") and not file.endswith(".ci8.png") and not file.endswith(".ci4.png"):
                process_file(os.path.join(root, file))
    
    # Also do it for any missed in actors/levels/etc if they don't have .inc.c
    for root, dirs, files in os.walk("."):
        if "textures" in root: continue
        for file in files:
            if file.endswith(".png"):
                path = os.path.join(root, file)
                if not os.path.exists(path + ".inc.c"):
                    process_file(path)
