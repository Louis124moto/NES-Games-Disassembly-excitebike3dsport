#!/usr/bin/env python3
"""Smoke tests for the NES CHR extractor."""
import tempfile
import unittest
from pathlib import Path
import nes_chr_to_ppm as tool

class TestChrExtractor(unittest.TestCase):
    def test_tile_bitplanes(self):
        tile = bytes([128] + [0] * 7 + [64] + [0] * 7)
        pixels = tool.decode_tile(tile)
        self.assertEqual(pixels[:8], [1, 0, 0, 0, 0, 0, 0, 0])
        self.assertEqual(pixels[8:16], [0, 2, 0, 0, 0, 0, 0, 0])

    def test_raw_input(self):
        raw = bytes(range(32))
        data, offset = tool.extract_chr(raw)
        self.assertEqual(data, raw)
        self.assertEqual(offset, 0)

    def test_png_writer(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "test.png"
            tool.write_png(path, 1, 1, bytes((1, 2, 3)))
            self.assertTrue(path.read_bytes().startswith(bytes((137, 80, 78, 71, 13, 10, 26, 10))))

if __name__ == "__main__":
    unittest.main()
