#!/usr/bin/env python3
import argparse
import os
import struct
import zlib


SKYBOX_SRC_SIZE = 248
SKYBOX_SRC_TILE = 31
SKYBOX_DST_TILE = 32
SKYBOX_COLS = 8
SKYBOX_ROWS = 8


def read_png_rgba(path):
    with open(path, "rb") as f:
        data = f.read()

    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: not a PNG")

    width = height = bit_depth = color_type = interlace = None
    compressed = bytearray()
    offset = 8
    while offset + 12 <= len(data):
        length = struct.unpack(">I", data[offset:offset + 4])[0]
        chunk_type = data[offset + 4:offset + 8]
        chunk = data[offset + 8:offset + 8 + length]
        offset += 12 + length

        if chunk_type == b"IHDR":
            width, height, bit_depth, color_type, _, _, interlace = struct.unpack(">IIBBBBB", chunk)
        elif chunk_type == b"IDAT":
            compressed.extend(chunk)
        elif chunk_type == b"IEND":
            break

    if (width, height, bit_depth, color_type, interlace) != (SKYBOX_SRC_SIZE, SKYBOX_SRC_SIZE, 8, 6, 0):
        raise ValueError(f"{path}: expected 248x248 8-bit non-interlaced RGBA PNG")

    raw = zlib.decompress(bytes(compressed))
    stride = width * 4
    rows = []
    previous = bytearray(stride)
    pos = 0

    for _ in range(height):
        filter_type = raw[pos]
        pos += 1
        scan = bytearray(raw[pos:pos + stride])
        pos += stride

        for i in range(stride):
            left = scan[i - 4] if i >= 4 else 0
            up = previous[i]
            up_left = previous[i - 4] if i >= 4 else 0
            if filter_type == 1:
                scan[i] = (scan[i] + left) & 0xFF
            elif filter_type == 2:
                scan[i] = (scan[i] + up) & 0xFF
            elif filter_type == 3:
                scan[i] = (scan[i] + ((left + up) >> 1)) & 0xFF
            elif filter_type == 4:
                p = left + up - up_left
                pa = abs(p - left)
                pb = abs(p - up)
                pc = abs(p - up_left)
                pred = left if pa <= pb and pa <= pc else (up if pb <= pc else up_left)
                scan[i] = (scan[i] + pred) & 0xFF
            elif filter_type != 0:
                raise ValueError(f"{path}: unsupported PNG filter {filter_type}")

        rows.append(bytes(scan))
        previous = scan

    return width, height, rows


def rgba16_bytes(tile):
    out = bytearray()
    for r, g, b, a in tile:
        r5 = r >> 3
        g5 = g >> 3
        b5 = b >> 3
        a1 = 1 if a else 0
        out.append((r5 << 3) | (g5 >> 2))
        out.append(((g5 & 0x3) << 6) | (b5 << 1) | a1)
    return bytes(out)


def make_tiles(rows):
    tiles = []
    for row in range(SKYBOX_ROWS):
        for col in range(SKYBOX_COLS):
            tile = [(0, 0, 0, 0)] * (SKYBOX_DST_TILE * SKYBOX_DST_TILE)

            for y in range(SKYBOX_SRC_TILE):
                src_y = row * SKYBOX_SRC_TILE + y
                scan = rows[src_y]
                for x in range(SKYBOX_SRC_TILE):
                    src_x = col * SKYBOX_SRC_TILE + x
                    off = src_x * 4
                    tile[y * SKYBOX_DST_TILE + x] = tuple(scan[off:off + 4])

            tiles.append(tile)

    for row in range(SKYBOX_ROWS):
        for col in range(SKYBOX_COLS):
            tile = tiles[row * SKYBOX_COLS + col]
            next_col_tile = tiles[row * SKYBOX_COLS + ((col + 1) % SKYBOX_COLS)]
            for y in range(SKYBOX_DST_TILE - 1):
                tile[(SKYBOX_DST_TILE - 1) + y * SKYBOX_DST_TILE] = next_col_tile[y * SKYBOX_DST_TILE]

    for row in range(SKYBOX_ROWS):
        for col in range(SKYBOX_COLS):
            tile = tiles[row * SKYBOX_COLS + col]
            if row < SKYBOX_ROWS - 1:
                next_row_tile = tiles[(row + 1) * SKYBOX_COLS + col]
                for x in range(SKYBOX_DST_TILE):
                    tile[x + (SKYBOX_DST_TILE - 1) * SKYBOX_DST_TILE] = next_row_tile[x]
            else:
                for x in range(SKYBOX_DST_TILE):
                    tile[x + (SKYBOX_DST_TILE - 1) * SKYBOX_DST_TILE] = tile[x + (SKYBOX_DST_TILE - 2) * SKYBOX_DST_TILE]

    return tiles


def assign_positions(tile_bytes):
    positions = []
    unique = []
    for tile in tile_bytes:
        try:
            idx = unique.index(tile)
        except ValueError:
            idx = len(unique)
            unique.append(tile)
        positions.append(idx)
    return positions, unique


def write_c(path, name, positions, unique):
    with open(path, "w", newline="\n") as f:
        f.write('#include "sm64.h"\n\n#include "make_const_nonconst.h"\n\n')

        for idx, tile in enumerate(unique):
            f.write(f"ALIGNED8 static const u8 {name}_skybox_texture_{idx:05X}[] = {{\n")
            for off in range(0, len(tile), 32):
                f.write("".join(f"0x{b:X}," for b in tile[off:off + 32]))
                f.write("\n")
            f.write("};\n\n")

        f.write(f"const u8 *const {name}_skybox_ptrlist[] = {{\n")
        for row in range(SKYBOX_ROWS):
            for col in range(10):
                idx = positions[row * SKYBOX_COLS + (col % SKYBOX_COLS)]
                f.write(f"{name}_skybox_texture_{idx:05X},\n")
        f.write("};\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input")
    parser.add_argument("output")
    args = parser.parse_args()

    _, _, rows = read_png_rgba(args.input)
    tiles = make_tiles(rows)
    tile_bytes = [rgba16_bytes(tile) for tile in tiles]
    positions, unique = assign_positions(tile_bytes)

    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    name = os.path.basename(args.input).split(".")[0]
    write_c(args.output, name, positions, unique)


if __name__ == "__main__":
    main()
