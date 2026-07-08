import os
import sys
from PIL import Image

def write_cake_c(img_path, out_path, is_eu=False):
    img = Image.open(img_path).convert("RGBA")
    width, height = img.size
    
    if is_eu:
        cols, rows = 5, 7
    else:
        cols, rows = 4, 12
        
    tile_w = width // cols
    tile_h = height // rows
    
    suffix = "eu_" if is_eu else ""
    
    with open(out_path, "w") as f:
        for i in range(cols * rows):
            row = i // cols
            col = i % cols
            tile = img.crop((col * tile_w, row * tile_h, (col + 1) * tile_w, (row + 1) * tile_h))
            data = tile.getdata()
            
            f.write(f"ALIGNED8 static const u8 cake_end_texture_{suffix}{i}[] = {{\n")
            out = bytearray()
            for r, g, b, a in data:
                r5 = r >> 3
                g5 = g >> 3
                b5 = b >> 3
                a1 = 1 if a > 0 else 0
                val = (r5 << 11) | (g5 << 6) | (b5 << 1) | a1
                out.append((val >> 8) & 0xFF)
                out.append(val & 0xFF)
            
            for j in range(0, len(out), 16):
                chunk = out[j:j+16]
                f.write("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
            f.write("};\n\n")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: generate_cake_c.py <input.png> <output.inc.c> [--eu]")
        sys.exit(1)
        
    in_file = sys.argv[1]
    out_file = sys.argv[2]
    is_eu = len(sys.argv) > 3 and sys.argv[3] == "--eu"
    
    write_cake_c(in_file, out_file, is_eu)
