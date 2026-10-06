#!/usr/bin/env python3
"""Rewrite GAS data directives so the emitted bytes are big-endian.

The N64 is big-endian and the game reads the sound tables (sequences.s,
instrument_sets.s) as raw ROM data.  The PSP assembler emits .word/.hword
little-endian, so those directives are turned into equivalent .byte lists.
Everything else (.incbin, labels, .balign, .byte) passes through unchanged.

usage: asm_be.py <in.s> <out.s>
"""
import re
import sys

DIRECTIVE_RE = re.compile(r"^(\s*)\.(word|hword|half|short|dword)\s+(.*)$")


def split_operands(text):
    """Split on top-level commas (ignore commas inside parentheses)."""
    out, depth, cur = [], 0, []
    for ch in text:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    if "".join(cur).strip():
        out.append("".join(cur).strip())
    return out


def convert_line(line):
    code, _, comment = line.partition("#")
    m = DIRECTIVE_RE.match(code)
    if not m:
        return line
    indent, kind, operands = m.groups()
    size = {"word": 4, "hword": 2, "half": 2, "short": 2, "dword": 8}[kind]
    bytes_out = []
    for op in split_operands(operands):
        for i in range(size - 1, -1, -1):
            shift = i * 8
            bytes_out.append(f"(({op}) >> {shift}) & 0xff")
    text = f"{indent}.byte " + ", ".join(bytes_out)
    if comment:
        text += " #" + comment.rstrip("\n")
    return text + "\n"


def main():
    src, dst = sys.argv[1], sys.argv[2]
    with open(src) as f:
        lines = f.readlines()
    with open(dst, "w") as f:
        for line in lines:
            f.write(convert_line(line))


if __name__ == "__main__":
    main()
