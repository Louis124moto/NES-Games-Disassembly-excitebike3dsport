import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


TOOLS = Path(__file__).resolve().parent


class ExtractionPipelineTests(unittest.TestCase):
    def test_raw_chr_pipeline_generates_inspection_and_native_assets(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "synthetic.chr"
            output = root / "generated"
            # Two valid 8x8 NES 2bpp tiles: zero-filled and all plane-0 bits set.
            source.write_bytes(bytes(16) + bytes([0xFF] * 8 + [0] * 8))
            result = subprocess.run(
                [
                    sys.executable,
                    str(TOOLS / "extract_nes_assets.py"),
                    str(source),
                    "--output-dir",
                    str(output),
                    "--scale",
                    "1",
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr + result.stdout)
            self.assertTrue((output / "chr-atlas.png").is_file())
            self.assertTrue((output / "nes_chr_assets.h").is_file())
            metadata = json.loads((output / "chr-tiles.json").read_text(encoding="utf-8"))
            self.assertEqual(metadata["tile_count"], 2)
            header = (output / "nes_chr_assets.h").read_text(encoding="utf-8")
            self.assertIn("#define NES_CHR_TILE_COUNT 2u", header)

    def test_chr_size_requires_offset(self):
        with tempfile.TemporaryDirectory() as tmp:
            source = Path(tmp) / "synthetic.chr"
            source.write_bytes(bytes(16))
            result = subprocess.run(
                [
                    sys.executable,
                    str(TOOLS / "extract_nes_assets.py"),
                    str(source),
                    "--chr-size",
                    "16",
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("--chr-size requires --chr-offset", result.stderr)


if __name__ == "__main__":
    unittest.main()
