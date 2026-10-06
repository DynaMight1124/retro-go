#!/usr/bin/env python3
"""Describe course-sized overlays from the PSP port's data-free recipes."""

import json
import pathlib
import sys


def main() -> None:
    source = pathlib.Path(sys.argv[1])
    destination = pathlib.Path(sys.argv[2])
    data = json.loads(source.read_text(encoding="utf-8"))
    recipes = data["recipes"]
    ranges = []
    for course in data["courses"]:
        prefix = f"d_course_{course['name']}_"
        entries = [r for r in recipes if r["name"].startswith(prefix)]
        if not entries:
            raise ValueError(f"No assets for {course['name']}")
        begin = min(r["off"] for r in entries)
        end = max(r["off"] + r["size"] for r in entries)
        ranges.append((course["idx"], course["name"], begin, end))

    if len(ranges) != 20 or sorted(idx for idx, _, _, _ in ranges) != list(range(20)):
        raise ValueError("Expected 20 indexed courses")
    ordered = sorted(ranges, key=lambda r: r[2])
    if any(a[3] > b[2] for a, b in zip(ordered, ordered[1:])):
        raise ValueError("Course asset ranges overlap")
    window_begin, window_end = ordered[0][2], ordered[-1][3]
    for recipe in recipes:
        start = recipe["off"]
        end = start + recipe["size"]
        if start < window_end and end > window_begin and not any(
            begin <= start and end <= finish for _, _, begin, finish in ranges
        ):
            raise ValueError(f"Asset crosses a course boundary: {recipe['name']}")

    rows = [None] * 20
    for idx, name, begin, end in ranges:
        rows[idx] = f"    {{{begin}u, {end - begin}u}}, /* {name} */"
    destination.write_text(
        "/* Generated from tools/psp/recipes.json; contains no game data. */\n"
        "#pragma once\n#include <stdint.h>\n"
        f"#define MK64_ASSET_REGION_SIZE {data['region_size']}u\n"
        f"#define MK64_COURSE_WINDOW_BEGIN {window_begin}u\n"
        f"#define MK64_COURSE_WINDOW_END {window_end}u\n"
        "typedef struct { uint32_t offset, size; } mk64_course_range_t;\n"
        "static const mk64_course_range_t mk64_course_ranges[20] = {\n"
        + "\n".join(rows)
        + "\n};\n",
        encoding="utf-8",
    )
    print(
        f"Course window {window_end - window_begin} bytes; "
        f"largest course {max(end - begin for _, _, begin, end in ranges)} bytes"
    )


if __name__ == "__main__":
    main()
