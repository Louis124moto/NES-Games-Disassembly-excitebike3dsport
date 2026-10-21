#!/usr/bin/env python3
"""Regression tests for the C-header CHR extractor."""
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).with_name("nes_chr_to_c.py")


class TestChrToC(unittest.TestCase):
    def test_raw_chr_generates_flat_64_pixel_tile(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "chr.bin"
            output = root / "nes_chr_assets.h"
            # Tile 0 has the high bit set in plane 0 of its first row.
            source.write_bytes(bytes([0x80] + [0] * 7 + [0] * 8)
                               + bytes(16))
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(source), str(output)],
                check=True, capture_output=True, text=True)
            generated = output.read_text(encoding="utf-8")
            self.assertIn("#define NES_CHR_TILE_COUNT 2u", generated)
            self.assertIn("static const uint8_t nesChrPixels[NES_CHR_TILE_COUNT][64]", generated)
            self.assertIn("{1u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,", generated)
            self.assertIn("Generated 2 tiles", result.stdout)

    def test_chr_size_requires_offset(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "chr.bin"
            output = root / "out.h"
            source.write_bytes(bytes(16))
            result = subprocess.run(
                [sys.executable, str(SCRIPT), str(source), str(output), "--chr-size", "16"],
                capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("--chr-size requires --chr-offset", result.stderr)


if __name__ == "__main__":
    unittest.main()
