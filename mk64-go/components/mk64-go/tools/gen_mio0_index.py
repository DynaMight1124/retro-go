#!/usr/bin/env python3
"""Generate the ROM MIO0 block index from the PSP port's data-free recipes."""

import json
import pathlib
import sys


def main() -> None:
    source = pathlib.Path(sys.argv[1])
    destination = pathlib.Path(sys.argv[2])
    blocks = json.loads(source.read_text(encoding="utf-8"))["blocks"]
    if len(blocks) != 52:
        raise ValueError(f"Expected 52 MIO0 blocks, found {len(blocks)}")
    rows = []
    seen = set()
    for block in blocks:
        offset, compressed, unpacked = (
            block["rom_off"], block["rom_len"], block["decomp_len"]
        )
        if offset in seen or min(offset, compressed, unpacked) <= 0:
            raise ValueError(f"Invalid block: {block}")
        seen.add(offset)
        rows.append(f"    {{{offset}u, {compressed}u, {unpacked}u}},")
    destination.write_text(
        "/* Generated from tools/psp/recipes.json; contains no game data. */\n"
        "#pragma once\n"
        "#include <stdint.h>\n"
        "typedef struct { uint32_t offset, compressed_size, output_size; } mk64_mio0_block_t;\n"
        f"static const mk64_mio0_block_t mk64_mio0_blocks[{len(rows)}] = {{\n"
        + "\n".join(rows)
        + "\n};\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
