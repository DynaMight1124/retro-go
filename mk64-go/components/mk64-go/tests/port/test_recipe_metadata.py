"""Regression for semantic descriptor locations and mixed-endian structures."""
import importlib.util
import json
import unittest
from test_rg_render import ROOT


class RecipeMetadataTests(unittest.TestCase):
    def test_named_course_models_have_segment_entries(self):
        import sys
        sys.path.insert(0, str(ROOT / "tools"))
        spec = importlib.util.spec_from_file_location("segments", ROOT / "tools/gen_course_segments.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        recipes = {r["name"]: r for r in json.loads((ROOT / "tools/psp/recipes.json").read_text())["recipes"]}
        for course, name, offset in (
            ("mario_raceway", "d_course_mario_raceway_tree_model", 0x6A28),
            ("mario_raceway", "d_course_mario_sign_model", 0x6B08),
            ("luigi_raceway", "d_course_luigi_raceway_tree_model", 0xFBF0),
            ("mario_raceway", "d_course_mario_sign_left", 0x7068),
            ("mario_raceway", "d_course_mario_sign_right", 0x8068),
        ):
            rows = dict(module.entries(ROOT / "courses" / course / "course_data.h", 6, recipes))
            self.assertIn(offset, rows, name)
            self.assertEqual(rows[offset][0], recipes[name]["off"])

    def test_all_course_render_arrays_have_segment_entries(self):
        import re
        import sys
        sys.path.insert(0, str(ROOT / "tools"))
        from gen_recipe_index import correct_structured_recipes
        from gen_course_segments import entries
        data = json.loads((ROOT / "tools/psp/recipes.json").read_text())
        correct_structured_recipes(data, ROOT)
        recipes = {r["name"]: r for r in data["recipes"]}
        for path in (ROOT / "courses").glob("*/course_data.c"):
            mapped = {asset for _, (asset, size) in entries(path.with_suffix(".h"), 6, recipes)}
            for name in re.findall(r"\b(?:u8|Vtx|Gfx)\s+(\w+)\[[^]]*\]\s*=", path.read_text()):
                if name in recipes:
                    self.assertIn(recipes[name]["off"], mapped, name)
        rows = dict(entries(ROOT / "courses/luigi_raceway/course_data.h", 6, recipes))
        for name, offset in (("sign_left", 0xC588), ("sign_right", 0xD588)):
            row = recipes["d_course_luigi_raceway_" + name]
            self.assertEqual(rows[offset][0], row["off"])
            self.assertEqual((row["src"], row["extra"]), (0x84E8E0, offset))

    def test_truncated_palette_does_not_move_following_tree(self):
        import sys
        sys.path.insert(0, str(ROOT / "tools"))
        from gen_recipe_index import correct_structured_recipes
        from gen_course_segments import entries
        data = json.loads((ROOT / "tools/psp/recipes.json").read_text())
        correct_structured_recipes(data, ROOT)
        recipes = {r["name"]: r for r in data["recipes"]}
        rows = dict(entries(ROOT / "courses/frappe_snowland/course_data.h", 6, recipes))
        # The palette recipe omits twelve trailing zero bytes; source also
        # documents unrepresented lighting between it and the vertex batch.
        for name, offset in (("d_frappe_snowland_tree", 0x7520),
                             ("d_course_frappe_snowland_dl_tree", 0x75A0)):
            self.assertEqual(rows[offset][0], recipes[name]["off"])
            self.assertEqual(recipes[name]["extra"], offset)

    def test_static_segment_addresses_are_offsets(self):
        source = (ROOT / "course_segments.c").read_text()
        import re
        for start, end in re.findall(r"\{0x([0-9a-f]+)u, 0x([0-9a-f]+)u, \d+u\}", source):
            self.assertLess(int(start, 16), 0x1000000)
            self.assertLessEqual(int(end, 16), 0x1000000)
        recipes = {r["name"]: r for r in json.loads((ROOT / "tools/psp/recipes.json").read_text())["recipes"]}
        for name, offset in (("common_vtx_itembox", 0x1CE8), ("D_0D003090", 0x3090)):
            self.assertTrue(re.search(rf"\{{0x{offset:06x}u, 0x[0-9a-f]+u, {recipes[name]['off']}u\}}", source), name)
        for offset in (0x5238, 0x5278, 0x58E0):
            name = f"common_data_seg13_vtx_{offset:X}"
            self.assertRegex(source, rf"\{{0x{offset:06x}u, 0x[0-9a-f]+u, {recipes[name]['off']}u\}}")

    def test_anonymous_common_vertices_keep_their_address_and_byte_fields(self):
        spec = importlib.util.spec_from_file_location("index", ROOT / "tools/gen_recipe_index.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        data = json.loads((ROOT / "tools/psp/recipes.json").read_text())
        module.correct_structured_recipes(data, ROOT)
        recipes = {row["name"]: row for row in data["recipes"]}
        for offset in (0x5238, 0x5278, 0x58E0):
            row = recipes[f"common_data_seg13_vtx_{offset:X}"]
            self.assertEqual((row["kind"], row["src"], row["extra"]),
                             ("MIO0", 0x132B50, offset))
            self.assertTrue(row["xform"].startswith("pat"), row["name"])
            self.assertEqual(data["patterns"][int(row["xform"][3:])],
                             ["22", "22", "22", "1111"])

    def test_course_display_lists_use_their_named_segment_offsets(self):
        import re
        spec = importlib.util.spec_from_file_location("index", ROOT / "tools/gen_recipe_index.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        data = json.loads((ROOT / "tools/psp/recipes.json").read_text())
        module.correct_structured_recipes(data, ROOT)
        count = 0
        for row in data["recipes"]:
            match = re.fullmatch(r"d_course_(\w+)_dl_([0-9A-Fa-f]+)", row["name"])
            if match and not match[1].endswith("_packed"):
                count += 1
                self.assertEqual(row["kind"], "MIO0", row["name"])
                self.assertEqual(row["extra"], int(match[2], 16), row["name"])
                # Even dl_0 was falsely matched into another course's block.
                if match[1] == "sherbet_land":
                    self.assertEqual(row["src"], 0x86ECF0, row["name"])
                if match[1] == "dks_jungle_parkway":
                    self.assertEqual(row["src"], 0x885A10, row["name"])
        self.assertGreater(count, 1000)

    def test_menu_descriptors_use_segment_two_and_collision_has_native_pointers(self):
        spec = importlib.util.spec_from_file_location("index", ROOT / "tools/gen_recipe_index.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        data = json.loads((ROOT / "tools/psp/recipes.json").read_text())
        module.correct_structured_recipes(data, ROOT)
        recipes = {row["name"]: row for row in data["recipes"]}
        for name in ("D_020051C8", "D_02001A8C"):
            row = recipes[name]
            self.assertEqual(row["kind"], "RAW")
            self.assertEqual(row["extra"], 0x12AAE0 + int(name[2:], 16) % 0x1000000)
            self.assertEqual(row["size"], 40)
            self.assertEqual(data["patterns"][int(row["xform"][3:])],
                             ["22", "4", "22", "22", "22"])
        row = recipes["d_course_mario_raceway_addr"]
        self.assertEqual(data["patterns"][int(row["xform"][3:])], ["4", "112"])
        row = recipes["D_0D005AE0"]
        self.assertEqual(data["patterns"][int(row["xform"][3:])],
                         ["22", "22", "22", "1111"])
        for name, offset in (("D_0D0069E0", 0x69E0), ("D_0D006980", 0x6980),
                             ("D_0D008E48", 0x8E48), ("D_0D008120", 0x8120)):
            row = recipes[name]
            self.assertEqual(row["kind"], "MIO0")
            self.assertEqual(row["src"], 0x132B50)
            self.assertEqual(row["extra"], offset)
        for name, offset in (("seg2_50_CC_texture", 0x48F4),
                             ("seg2_100_CC_texture", 0x491C),
                             ("seg2_150_CC_texture", 0x4944),
                             ("seg2_extra_CC_texture", 0x496C)):
            row = recipes[name]
            self.assertEqual(row["extra"], 0x12AAE0 + offset)
            self.assertEqual(row["size"], 40)
            self.assertEqual(data["patterns"][int(row["xform"][3:])],
                             ["22", "4", "22", "22", "22"])


if __name__ == "__main__":
    unittest.main()
