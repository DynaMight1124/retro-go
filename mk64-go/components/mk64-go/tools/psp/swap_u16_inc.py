#!/usr/bin/env python3
"""Byte-swap the 16-bit hex literals of a torch-generated .inc.c file.

Torch emits RGBA16 textures and TLUTs as `u16 name[] = { 0x1234, ... }`.
Compiled for the PSP those are little-endian in memory, but the port's
texture importers read texel data big-endian, byte for byte, like the N64
RDP does.  The PSP build shadows those .inc.c files with copies whose
values are swapped so the bytes in memory match the ROM.
"""
import re
import sys

src, dst = sys.argv[1], sys.argv[2]
text = open(src).read()

def swap(m):
    v = int(m.group(1), 16)
    if v > 0xFFFF:
        raise SystemExit(f"{src}: value {m.group(0)} is not 16-bit")
    return "0x%04X" % (((v & 0xFF) << 8) | (v >> 8))

open(dst, "w").write(re.sub(r"0x([0-9A-Fa-f]+)", swap, text))
