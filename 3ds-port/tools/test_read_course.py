import json
import tempfile
import unittest
from pathlib import Path

from read_course import render_glyphs, render_hex, validate_course


def sample():
    return {
        "schema": "excitebike.first-course-geometry.v1",
        "columns": [{
            "ordinal": 0, "course_class": "$0A", "ring_column": "$34",
            "ppu_row": "$14", "nametable": "$25",
            "values_by_band": ["$3B", "$3C", "$3D", "$E4", "$E6", "$FC"],
        }],
        "summary": {"column_count": 1, "complete": True,
                    "cycle_detected": False, "truncated": False},
    }


class CourseReaderTests(unittest.TestCase):
    def test_validates_complete_six_band_geometry(self):
        self.assertEqual(len(validate_course(sample())), 1)

    def test_rejects_truncated_course(self):
        data = sample()
        data["summary"]["truncated"] = True
        with self.assertRaisesRegex(ValueError, "incomplete or truncated"):
            validate_course(data)

    def test_rejects_wrong_band_count(self):
        data = sample()
        data["columns"][0]["values_by_band"].pop()
        with self.assertRaisesRegex(ValueError, "six vertical bands"):
            validate_course(data)

    def test_hex_reader_displays_raw_ids(self):
        output = render_hex(validate_course(sample()), 0, 1)
        self.assertIn("3B 3C 3D E4 E6 FC", output)

    def test_glyph_reader_is_explicitly_diagnostic(self):
        output = render_glyphs(validate_course(sample()), 0, 1)
        self.assertIn("#=-.^ ", output)
        self.assertIn("not NES artwork", output)


if __name__ == "__main__":
    unittest.main()
