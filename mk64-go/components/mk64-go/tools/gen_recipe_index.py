#!/usr/bin/env python3
"""Emit the PSP ROM recipes as C metadata without including game data."""

import json
import pathlib
import re
import sys

KIND = {"RAW": 1, "MIO0": 2, "LITERAL": 3, "RELOCS": 4, "UNPACK": 5}
XFORM = {"id": 0, "sw16": 1, "sw32": 2}
SHAPE = {"4": 0, "22": 1, "211": 2, "112": 3, "1111": 4}

# USA segment-6 course data blocks, in the linker COURSE_DATA_SEG order.
# Do not infer these from dl_0's byte match: tiny lists can match another
# course, while pointer operands ignored by the original matcher differ.
COURSE_DATA_BLOCKS = dict(zip((
    "mario_raceway", "choco_mountain", "bowsers_castle", "banshee_boardwalk",
    "yoshi_valley", "frappe_snowland", "koopa_troopa_beach", "royal_raceway",
    "luigi_raceway", "moo_moo_farm", "toads_turnpike", "kalimari_desert",
    "sherbet_land", "rainbow_road", "wario_stadium", "block_fort",
    "skyscraper", "double_deck", "dks_jungle_parkway", "big_donut",
), (
    0x8284D0, 0x82B620, 0x82DF40, 0x831DC0, 0x835BA0,
    0x83F740, 0x842E40, 0x84ABD0, 0x84E8E0, 0x852E20,
    0x857E80, 0x8666A0, 0x86ECF0, 0x872A00, 0x8804A0,
    0x885630, 0x885780, 0x8858A0, 0x885A10, 0x88CC50,
)))


def course_model_locations(path, recipes):
    """Recover named models between address anchors in the source layout."""
    source = path.read_text()
    result = {}
    previous_end = 0
    next_offset = None
    last_kind = None
    for match in re.finditer(r"\b(Gfx|Vtx|u8)\s+(\w+)\[[^]]*\]\s*=\s*\{(.*?)\};", source, re.S | re.M):
        previous_kind = last_kind
        kind, name, body = match.groups()
        last_kind = kind
        if name not in recipes:
            next_offset = None
            previous_end = match.end()
            continue
        between = source[previous_end:match.start()]
        encoded = re.search(r"_dl_([0-9A-Fa-f]+)$|_0?60([0-9A-Fa-f]{5})$", name)
        comment = re.search(r"//\s*0x([0-9A-Fa-f]+)[^\n]*\s*$", between)
        adjacent = not re.sub(r"/\*.*?\*/|//[^\n]*", "", between, flags=re.S).strip()
        row = recipes[name]
        if kind == "u8" and row["kind"] == "MIO0" and row["src"] == COURSE_DATA_BLOCKS[path.parent.name]:
            # Keep verified byte matches within this course: a few texture
            # comments are duplicated. Cross-block matches still need their
            # course address, recovered from comments or adjacent arrays.
            offset = row["extra"]
        elif encoded:
            offset = int(encoded[1] or encoded[2], 16)
        elif comment:
            offset = int(comment[1], 16) & 0xFFFFFF
        elif adjacent and next_offset is not None and not (
                kind == "Vtx" and previous_kind == "u8" and
                row["kind"] == "MIO0" and row["src"] == COURSE_DATA_BLOCKS[path.parent.name]):
            offset = next_offset
        elif row["kind"] == "MIO0" and row["src"] == COURSE_DATA_BLOCKS[path.parent.name] and (
                kind == "Vtx" or name == f"d_course_{path.parent.name}_dl" and row["extra"] == 0):
            # Texture recipes can omit more than eight trailing zero bytes,
            # and unrepresented lighting can follow them. Do not infer a
            # vertex address from such a texture's truncated length.
            # Some vertex batches follow non-render data without an address
            # comment; their byte match is safe within the correct course.
            # Battle courses also name their initial list without a suffix.
            offset = row["extra"]
        else:
            next_offset = None
            previous_end = match.end()
            continue
        result[name] = offset
        if kind == "Vtx":
            # Each source initializer is one 16-byte N64 vertex; colors are
            # byte fields and must not be included in a blanket sw16 transform.
            size = 16 * len(re.findall(r"\{\s*\{\s*\{", body))
        else:
            # Textures and display lists are aligned to eight bytes.
            # Original recipes strip END's zero operand, so round back to
            # complete 8-byte commands before locating an adjacent named list.
            size = (recipes[name]["size"] + 7) & ~7
        next_offset = offset + size
        previous_end = match.end()
    return result


def correct_structured_recipes(data, root):
    """Use known ROM structure locations instead of ambiguous short-byte matches."""
    by_name = {row["name"]: row for row in data["recipes"]}

    for row in data["recipes"]:
        match = re.fullmatch(r"d_course_(\w+)_dl_([0-9A-Fa-f]+)", row["name"])
        if match and not match[1].endswith("_packed"):
            if match[1] not in COURSE_DATA_BLOCKS:
                raise ValueError(f"Unknown segment-6 course: {row['name']}")
            row.update(kind="MIO0", src=COURSE_DATA_BLOCKS[match[1]],
                       extra=int(match[2], 16), xform="sw32")

    def pattern(shapes):
        if shapes not in data["patterns"]:
            data["patterns"].append(shapes)
        return "pat" + str(data["patterns"].index(shapes))

    menu_pattern = pattern(["22", "4", "22", "22", "22"])
    source = (root / "src/data/textures.c").read_text()
    offset = None
    for kind, name, count in re.findall(
            r"(MenuTexture|MkAnimation)\s+(\w+)\[(\d+)\]\s*=\s*\{", source):
        if re.fullmatch(r"D_020[0-9A-Fa-f]+", name):
            known = int(name[2:], 16) & 0xFFFFFF
            if offset is not None and offset != known:
                raise ValueError(f"Unexpected segment-2 layout at {name}")
            offset = known
        if offset is None:
            raise ValueError(f"Missing segment-2 anchor for {name}")
        size = int(count) * (20 if kind == "MenuTexture" else 8)
        if kind == "MenuTexture" and name in by_name:
            by_name[name].update(kind="RAW", src=0, extra=0x12AAE0 + offset,
                                 size=size, xform=menu_pattern)
        offset += size

    collision_pattern = pattern(["4", "112"])
    for path in (root / "courses").glob("*/course_data.c"):
        for name in re.findall(r"TrackSections\s+(\w+)\[", path.read_text()):
            if name not in by_name:
                raise ValueError(f"Missing collision recipe {name}")
            # Pointer, two byte-sized fields, then a 16-bit flags field.
            by_name[name]["xform"] = collision_pattern

    vertex_pattern = pattern(["22", "22", "22", "1111"])
    for name, row in by_name.items():
        batch = re.fullmatch(r"common_data_seg13_vtx_([0-9A-Fa-f]+)", name)
        if batch:
            # Anonymous quads are absent from YAML/extern Vtx declarations.
            # Their symbol records the authoritative original vertex address.
            row.update(kind="MIO0", src=0x132B50,
                       extra=int(batch[1], 16), xform=vertex_pattern)
    for path in (root / "courses").glob("*/course_data.c"):
        locations = course_model_locations(path, by_name)
        gfx_names = set(re.findall(r"\bGfx\s+(\w+)\[", path.read_text()))
        texture_names = set(re.findall(r"\bu8\s+(\w+)\[", path.read_text()))
        for name, offset in locations.items():
            kind = "sw32" if name in gfx_names else "id" if name in texture_names else vertex_pattern
            by_name[name].update(kind="MIO0", src=COURSE_DATA_BLOCKS[path.parent.name],
                                 extra=offset, xform=kind)
    for directory in (root / "include", root / "src", root / "courses"):
        for path in directory.rglob("*.h"):
            for name in re.findall(r"\bextern\s+Vtx\s+(\w+)\[", path.read_text(errors="replace")):
                if name in by_name:
                    # Six 16-bit fields, then four byte-sized color/normal fields.
                    by_name[name]["xform"] = vertex_pattern

    # Linked PSP lists can have identical bytes before pointer relocations.
    # Matching those bytes picks the first list, losing its distinct VTX/DL
    # operands. The source YAML provides each list's actual segment-D offset.
    common = (root / "yamls/us/common_data.yml").read_text()
    for name, offset in re.findall(
            r"^(\w+):\n  symbol: \w+\n  type: gfx\n  offset: (0x[0-9A-Fa-f]+)",
            common, re.MULTILINE):
        if name in by_name:
            by_name[name].update(kind="MIO0", src=0x132B50,
                                 extra=int(offset, 16) & 0xFFFFFF, xform="sw32")


def main() -> None:
    source = pathlib.Path(sys.argv[1])
    destination = pathlib.Path(sys.argv[2])
    data = json.loads(source.read_text(encoding="utf-8"))
    correct_structured_recipes(data, pathlib.Path(__file__).resolve().parents[1])
    rows = []
    blocks = {b["rom_off"]: b for b in data["blocks"]}
    for recipe in data["recipes"]:
        kind = KIND[recipe["kind"]]
        transform = recipe["xform"]
        xform = 16 + int(transform[3:]) if transform.startswith("pat") else XFORM[transform]
        dst, size = recipe["off"], recipe["size"]
        if size <= 0 or dst < 0 or dst + size > data["region_size"]:
            raise ValueError(f"Invalid asset range: {recipe['name']}")
        src = recipe["extra"] if kind == 1 else recipe["src"]
        extra = recipe["extra"] if kind in (2, 5) else 0
        if kind == 1 and (src < 0 or src + size > 0xC00000):
            raise ValueError(f"Invalid ROM range: {recipe['name']}")
        if kind == 2 and (src not in blocks or extra + size > blocks[src]["decomp_len"]):
            raise ValueError(f"Invalid MIO0 range: {recipe['name']}")
        if kind == 3:
            raise ValueError("Literal game data cannot be embedded in the index")
        rows.append((dst, size, src, extra, kind, xform))

    # Each destination is disjoint. Group MIO0 consumers by source block so
    # the device reads each cached block once.
    rows.sort(key=lambda r: (0 if r[4] == 1 else 1 if r[4] == 2 else 2,
                             r[2] if r[4] == 2 else r[0], r[0]))
    if len(rows) > 0xFFFF:
        raise ValueError("Recipe destination index exceeds uint16_t")
    dst_order = sorted(range(len(rows)), key=lambda index: rows[index][0])
    for previous, current in zip(dst_order, dst_order[1:]):
        if rows[previous][0] + rows[previous][1] > rows[current][0]:
            raise ValueError("Overlapping recipe destinations")
    rendered_rows = [
        f"    {{{dst}u, {size}u, {src}u, {extra}u, {kind}u, {xform}u}},"
        for dst, size, src, extra, kind, xform in rows
    ]

    patterns = []
    for pattern in data["patterns"]:
        if not 0 < len(pattern) <= 16:
            raise ValueError("Invalid transform pattern")
        shapes = [SHAPE[word] for word in pattern] + [0] * (16 - len(pattern))
        patterns.append(f"    {{{len(pattern)}u, {{{', '.join(str(x) for x in shapes)}}}}},")

    courses = [None] * len(data["courses"])
    for course in data["courses"]:
        idx = course["idx"]
        if not 0 <= idx < len(courses) or courses[idx] is not None:
            raise ValueError(f"Invalid course index: {idx}")
        courses[idx] = (
            f"    {{{course['rom_off']}u, {course['rom_len']}u, "
            f"{course['unpacked_len']}u, {course['packed_off']}u}},"
        )
    if any(course is None for course in courses):
        raise ValueError("Missing course index")

    header = destination.with_suffix(".h")
    header.write_text(
        "/* Generated from tools/psp/recipes.json; contains no game data. */\n"
        "#pragma once\n#include <stdint.h>\n#include <stddef.h>\n"
        "typedef struct { uint32_t dst, size, src, extra; uint16_t kind, xform; } mk64_recipe_t;\n"
        "typedef struct { uint8_t nwords, shapes[16]; } mk64_pattern_t;\n"
        "extern const mk64_recipe_t mk64_recipes[];\n"
        "extern const size_t mk64_recipe_count;\n"
        "extern const uint16_t mk64_recipe_dst_order[];\n"
        "extern const mk64_pattern_t mk64_patterns[];\n"
        "extern const size_t mk64_pattern_count;\n"
        "typedef struct { uint32_t rom_off, rom_len, unpacked_len, packed_off; } mk64_course_dl_t;\n"
        "extern const mk64_course_dl_t mk64_course_displaylists[];\n"
        "extern const size_t mk64_course_displaylist_count;\n",
        encoding="utf-8",
    )
    destination.write_text(
        "/* Generated from tools/psp/recipes.json; contains no game data. */\n"
        f'#include "{header.name}"\n'
        f"const mk64_recipe_t mk64_recipes[{len(rows)}] = {{\n"
        + "\n".join(rendered_rows)
        + "\n};\n"
        f"const size_t mk64_recipe_count = {len(rows)}u;\n"
        f"const uint16_t mk64_recipe_dst_order[{len(rows)}] = {{\n"
        + "\n".join("    " + ", ".join(f"{index}u" for index in dst_order[start:start + 16]) + ","
                    for start in range(0, len(rows), 16))
        + "\n};\n"
        f"const mk64_pattern_t mk64_patterns[{len(patterns)}] = {{\n"
        + "\n".join(patterns)
        + "\n};\n"
        f"const size_t mk64_pattern_count = {len(patterns)}u;\n"
        f"const mk64_course_dl_t mk64_course_displaylists[{len(courses)}] = {{\n"
        + "\n".join(courses)
        + "\n};\n"
        f"const size_t mk64_course_displaylist_count = {len(courses)}u;\n",
        encoding="utf-8",
    )
    print(f"Wrote {len(rows)} recipes and {len(patterns)} patterns")


if __name__ == "__main__":
    main()
