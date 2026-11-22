import tempfile
import unittest
from pathlib import Path

import extract_first_course


class CourseHeaderTests(unittest.TestCase):
    def sample(self):
        return {
            "columns": [{
                "values_by_band": ["$3B", "$3C", "$3D", "$E4", "$E6", "$FC"],
                "course_class": "$0A", "ring_column": "$34",
                "ppu_row": "$14", "nametable": "$25",
                "stream_offset_before": 0, "stream_address_before": "$ED46",
                "stream_offset_after": 1, "template_id": 9,
                "template_address": "$F063", "template_offset_next": 2,
                "next_column_00EB": "$0B", "repeat_counter_0057": "$02",
                "counter_03A4": "$01", "counter_03A5": "$00",
                "flag_03A2": "$00", "flag_03BC": "$00",
                "spawn_request_03A8": "$01",
                "stream_buffer_0300_0311": ["$00"] * 17 + ["$8E"],
            }]
        }

    def test_header_contains_track_pages_and_full_stream_buffer(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "course.h"
            extract_first_course.emit_c_header(self.sample(), output)
            header = output.read_text(encoding="utf-8")
        self.assertIn("EXCITEBIKE_COURSE1_GEOMETRY_COLUMNS 1u", header)
        self.assertIn("excitebike_course1_track_pages", header)
        self.assertIn("excitebike_course1_spawn_request_03A8", header)
        self.assertIn("excitebike_course1_stream_address_before", header)
        self.assertIn("excitebike_course1_stream_buffer", header)
        self.assertIn("0x8Eu", header)

    def test_c_literal_checks_width(self):
        self.assertEqual(extract_first_course.c_literal("$ED46", 16), "0xED46u")
        with self.assertRaises(ValueError):
            extract_first_course.c_literal("$100")


if __name__ == "__main__":
    unittest.main()
