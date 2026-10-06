"""Validate source-based repairs for imported animation/course pointers."""
import importlib.util
import json
import unittest

from test_rg_render import ROOT


class AssetFixupTests(unittest.TestCase):
    def test_animation_and_course_pointers_are_rebuilt_from_source(self):
        spec = importlib.util.spec_from_file_location(
            "relocs", ROOT / "tools/gen_course_relocs.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        recipes = json.loads((ROOT / "tools/psp/recipes.json").read_text())["recipes"]
        by_name = {row["name"]: row for row in recipes}
        patches = dict(module.structured_fixups(ROOT, by_name))
        # This short recipe matched ROM offset 888, rather than an animation.
        offset = by_name["D_02006708"]["off"]
        self.assertEqual(patches[offset], "(uintptr_t)D_020051C8")
        self.assertEqual(patches[offset + 4], "0x00000032")
        self.assertEqual(patches[offset + 8], "0")
        self.assertEqual(patches[offset + 12], "0x00000000")
        # Race startup must use resident vertices and the relocated texture table.
        offset = by_name["gCourseTable"]["off"]
        self.assertEqual(patches[offset + 24], "(uintptr_t)d_course_mario_raceway_vertex")
        self.assertEqual(patches[offset + 40], "(uintptr_t)mario_raceway_textures")
        for course in range(20):
            self.assertIn(offset + course * 48 + 24, patches)
            self.assertIn(offset + course * 48 + 40, patches)
        for letter in "ABC":
            offset = by_name["seg2_textureFontLetter" + letter]["off"]
            self.assertEqual(patches[offset], "5u")
            self.assertEqual(patches[offset + 4], "(uintptr_t)font_letter_" + letter)
            self.assertEqual(patches[offset + 8], str(26 | (16 << 16)) + "u")
            self.assertEqual(patches[offset + 24], "0")


if __name__ == "__main__":
    unittest.main()
