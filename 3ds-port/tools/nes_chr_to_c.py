#!/usr/bin/env python3
"""Convert verified NES 2bpp CHR data into a C header for the native 3DS atlas renderer."""
import argparse
from pathlib import Path
import sys

TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))
from nes_chr_to_ppm import decode_tile, extract_chr  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="iNES/NES 2.0 ROM or raw CHR dump")
    parser.add_argument("output", type=Path, help="C header output path (e.g. include/nes_chr_assets.h)")
    parser.add_argument("--chr-offset", type=lambda s: int(s, 0), help="verified byte offset for raw CHR stored in another file")
    parser.add_argument("--chr-size", type=lambda s: int(s, 0), help="verified byte length; requires --chr-offset")
    args = parser.parse_args()
    if args.chr_size is not None and args.chr_offset is None:
        parser.error("--chr-size requires --chr-offset")

    data = args.source.read_bytes()
    chr_data, source_offset = extract_chr(data, args.chr_offset, args.chr_size)
    if not chr_data:
        parser.error("No CHR data found")
    tiles = [decode_tile(chr_data[i:i + 16]) for i in range(0, len(chr_data), 16)]
    if not tiles:
        parser.error("No complete 8x8 tiles found")

    lines = [
        "#ifndef EXCITEBIKE_NES_CHR_ASSETS_H",
        "#define EXCITEBIKE_NES_CHR_ASSETS_H",
        "",
        "#include <stdint.h>",
        "",
        "/* Generated file: do not edit by hand. Source ROM/dump is not embedded. */",
        f"/* Source: {args.source.name}; CHR byte offset: 0x{source_offset:X}; size: 0x{len(chr_data):X}. */",
        f"#define NES_CHR_TILE_COUNT {len(tiles)}u",
        "static const uint8_t nesChrPixels[NES_CHR_TILE_COUNT][64] = {",
    ]
    for index, pixels in enumerate(tiles):
        lines.append(f"    /* tile 0x{index:04X} */")
        vals = ", ".join(f"{p}u" for p in pixels)
        lines.append(f"    {{{vals}}}" + ("," if index != len(tiles) - 1 else ""))
    lines.extend(["};", "", "#endif", ""])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines), encoding="utf-8")
    print(f"Generated {len(tiles)} tiles from {args.source} (offset 0x{source_offset:X}) -> {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
