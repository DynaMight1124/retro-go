#!/usr/bin/env python3
"""
Convert gfx.bmp sprite sheet to C array.
BMP is 128x64, 4-bit indexed (16 colors).
Output is 8bpp array for easier access.
"""

import sys
from pathlib import Path

def read_bmp_4bit(filepath):
    """Read a 4-bit BMP and return (width, height, pixels, palette)"""
    with open(filepath, 'rb') as f:
        # BMP header
        magic = f.read(2)
        if magic != b'BM':
            raise ValueError("Not a BMP file")

        f.read(8)  # file size, reserved
        data_offset = int.from_bytes(f.read(4), 'little')

        # DIB header
        header_size = int.from_bytes(f.read(4), 'little')
        width = int.from_bytes(f.read(4), 'little', signed=True)
        height = int.from_bytes(f.read(4), 'little', signed=True)
        f.read(2)  # planes
        bpp = int.from_bytes(f.read(2), 'little')

        if bpp != 4:
            raise ValueError(f"Expected 4bpp, got {bpp}bpp")

        compression = int.from_bytes(f.read(4), 'little')
        if compression != 0:
            raise ValueError(f"Compression not supported: {compression}")

        f.read(20)  # rest of DIB header (image size, resolution, colors)

        # Read color palette (16 entries, BGRA format)
        palette = []
        for i in range(16):
            b, g, r, a = f.read(4)
            palette.append((r, g, b))

        # Read pixel data
        f.seek(data_offset)

        # BMP rows are padded to 4-byte boundary
        row_size = (width + 1) // 2  # 4bpp = 2 pixels per byte
        row_padding = (4 - (row_size % 4)) % 4
        padded_row_size = row_size + row_padding

        # BMP is stored bottom-up
        bottom_up = height > 0
        height = abs(height)

        pixels = [[0] * width for _ in range(height)]

        for y in range(height):
            row_data = f.read(padded_row_size)
            target_y = (height - 1 - y) if bottom_up else y

            for x in range(width):
                byte_idx = x // 2
                if x % 2 == 0:
                    pixel = (row_data[byte_idx] >> 4) & 0x0F
                else:
                    pixel = row_data[byte_idx] & 0x0F
                pixels[target_y][x] = pixel

        return width, height, pixels, palette

def main():
    bmp_path = Path(__file__).parent / "data" / "gfx.bmp"
    out_path = Path(__file__).parent / "sprites.c"

    print(f"Reading {bmp_path}...")
    width, height, pixels, palette = read_bmp_4bit(bmp_path)
    print(f"  Size: {width}x{height}, 16 colors")

    # Print palette for reference
    print("  Palette:")
    for i, (r, g, b) in enumerate(palette):
        print(f"    {i:2d}: RGB({r:3d}, {g:3d}, {b:3d})")

    # Generate C file
    with open(out_path, 'w') as f:
        f.write("/* Auto-generated sprite sheet data */\n")
        f.write("/* 128x64 pixels, 8bpp (palette indices 0-15) */\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"const int SPRITE_SHEET_W = {width};\n")
        f.write(f"const int SPRITE_SHEET_H = {height};\n\n")
        f.write(f"const uint8_t sprite_sheet[{width * height}] = {{\n")

        for y in range(height):
            f.write("    ")
            for x in range(width):
                f.write(f"{pixels[y][x]:2d},")
            f.write(f"  // row {y}\n")

        f.write("};\n")

    print(f"Written {out_path}")
    print(f"  Total size: {width * height} bytes")

if __name__ == "__main__":
    main()
