import json
import tempfile
import unittest
from pathlib import Path

from read_course import render_glyphs, render_hex, validate_course


def sample():
    return {
        "schema": "excitebike.first-course-geometry.v2",
        "termination": "F638 source lap completion",
        "columns": [{
            "ordinal": 0, "course_class": "$0A", "ring_column": "$34",
            "ppu_row": "$14", "nametable": "$25",
            "values_by_band": ["$3B", "$3C", "$3D", "$E4", "$E6", "$FC"],
            "next_column_00EB": "$0B", "repeat_counter_0057": "$02",
            "counter_03A4": "$01", "counter_03A5": "$00",
            "flag_03A2": "$00", "flag_03BC": "$00",
            "spawn_request_03A8": "$00",
            "stream_buffer_0300_0311": ["$00"] * 18,
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

    def test_rejects_incomplete_stream_buffer(self):
        data = sample()
        data["columns"][0]["stream_buffer_0300_0311"].pop()
        with self.assertRaisesRegex(ValueError, "all 18 bytes"):
            validate_course(data)

    def test_hex_reader_displays_raw_ids(self):
        output = render_hex(validate_course(sample()), 0, 1)
        self.assertIn("0  | 3B", output)
        self.assertIn("5  | FC", output)

    def test_glyph_reader_is_explicitly_diagnostic(self):
        output = render_glyphs(validate_course(sample()), 0, 1)
        self.assertIn("0: #", output)
        self.assertIn("4: ^", output)
        self.assertIn("not NES artwork", output)


if __name__ == "__main__":
    unittest.main()
