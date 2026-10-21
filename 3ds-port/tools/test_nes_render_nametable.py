#!/usr/bin/env python3
"""Regression tests for NES nametable composition."""
import unittest
from nes_chr_to_ppm import NES_RGB
from nes_render_nametable import render_nametable, split_nametable


class TestNametableRenderer(unittest.TestCase):
    def setUp(self):
        self.chr = bytearray(8192)
        # Tile 1: every pixel is color index 1 (plane 0 bits set).
        self.chr[16:24] = bytes([0xFF] * 8)
        self.nam = bytes([1] * 960)
        self.attr = bytearray(64)
        # Top-right 2x2-tile quadrant of the first 4x4-tile attribute block -> palette 2.
        self.attr[0] = 2 << 2
        self.palette = bytes([
            0x0F, 0x16, 0x27, 0x30,
            0x0F, 0x06, 0x17, 0x28,
            0x0F, 0x21, 0x12, 0x23,
            0x0F, 0x09, 0x19, 0x29,
        ])

    def test_attribute_quadrant_selects_correct_palette(self):
        rgb = render_nametable(bytes(self.chr), self.nam, bytes(self.attr), self.palette)
        # Tile (0,0) is palette 0; tile (2,0) is palette 2.
        self.assertEqual(rgb[(0 * 256 + 0) * 3:(0 * 256 + 0) * 3 + 3], bytes(NES_RGB[0x16]))
        self.assertEqual(rgb[(0 * 256 + 16) * 3:(0 * 256 + 16) * 3 + 3], bytes(NES_RGB[0x21]))
        self.assertEqual(len(rgb), 256 * 240 * 3)

    def test_universal_background_color_is_shared(self):
        palette = bytearray(self.palette)
        palette[4] = 0x30
        rgb = render_nametable(bytes(self.chr), self.nam, bytes(64), bytes(palette))
        # Pixel index 0 uses palette RAM $3F00 regardless of subpalette.
        empty_chr = bytes(8192)
        bg = render_nametable(empty_chr, self.nam, bytes([0xFF] * 64), bytes(palette))
        self.assertEqual(bg[:3], bytes(NES_RGB[0x0F]))
        self.assertEqual(len(rgb), 256 * 240 * 3)

    def test_split_combined_nametable(self):
        tilemap = bytes(960)
        attrs = bytes(range(64))
        self.assertEqual(split_nametable(tilemap + attrs), (tilemap, attrs))
        self.assertEqual(split_nametable(tilemap, attrs), (tilemap, attrs))

    def test_rejects_missing_attributes_and_short_chr(self):
        with self.assertRaisesRegex(ValueError, "needs a 64-byte"):
            split_nametable(bytes(960))
        with self.assertRaisesRegex(ValueError, "requires 4096 bytes"):
            render_nametable(bytes(2048), bytes(960), bytes(64))

    def test_rejects_invalid_palette_index(self):
        bad_palette = bytearray(self.palette)
        bad_palette[1] = 0x40
        with self.assertRaisesRegex(ValueError, "0..63"):
            render_nametable(bytes(self.chr), self.nam, bytes(self.attr), bytes(bad_palette))


if __name__ == "__main__":
    unittest.main()
