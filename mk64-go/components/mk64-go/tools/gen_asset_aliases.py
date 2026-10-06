#!/usr/bin/env python3
"""Create stable PSRAM symbols for the ROM-derived resident asset region."""

import json
import pathlib
import re
import sys


def main() -> None:
    source = pathlib.Path(sys.argv[1])
    destination = pathlib.Path(sys.argv[2])
    data = json.loads(source.read_text(encoding="utf-8"))
    region_size = data["region_size"]
    seen = set()
    lines = [
        "/* Generated from tools/psp/recipes.json; contains no game data. */",
        '.section .ext_ram.bss, "aw", @nobits',
        ".balign 16",
        ".globl gMk64AssetRegion",
        ".type gMk64AssetRegion, @object",
        "gMk64AssetRegion:",
        f".space {region_size}",
        f".size gMk64AssetRegion, {region_size}",
    ]
    for recipe in data["recipes"]:
        name, offset = recipe["name"], recipe["off"]
        if name in seen or not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", name):
            raise ValueError(f"Invalid or duplicate symbol: {name}")
        if not 0 <= offset < region_size or offset + recipe["size"] > region_size:
            raise ValueError(f"Symbol outside resident region: {name}")
        seen.add(name)
        lines += [f".globl {name}", f".set {name}, gMk64AssetRegion + {offset}"]
    destination.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote {len(seen)} resident asset symbols")


if __name__ == "__main__":
    main()
