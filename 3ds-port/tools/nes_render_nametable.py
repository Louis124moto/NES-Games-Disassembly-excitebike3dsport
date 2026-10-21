#!/usr/bin/env python3
"""Render a NES background nametable using CHR patterns, attribute bytes and PPU palette RAM.

Inspired by the separation of CHR, nametable, attributes and palettes used by
NES-Screen-Tool and makechr. This tool renders supplied data; it does not guess
game-specific PPU state or recover compressed graphics from PRG code.
"""
import argparse
from pathlib import Path
import sys

TOOLS_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS_DIR))
from nes_chr_to_ppm import NES_RGB, extract_chr, write_png  # noqa: E402

SCREEN_WIDTH = 256
SCREEN_HEIGHT = 240
TILES_WIDE = 32
TILES_HIGH = 30
NAMETABLE_BYTES = TILES_WIDE * TILES_HIGH
ATTRIBUTE_BYTES = 64
PATTERN_TABLE_BYTES = 4096


def split_nametable(data: bytes, attribute_data: bytes | None = None) -> tuple[bytes, bytes]:
    """Accept a 960-byte tile map, a 1024-byte nametable, or separate attributes."""
    if len(data) == 1024:
        if attribute_data is not None:
            raise ValueError("A 1024-byte nametable already includes attributes; omit --attribute-file")
        return data[:NAMETABLE_BYTES], data[NAMETABLE_BYTES:]
    if len(data) != NAMETABLE_BYTES:
        raise ValueError("Nametable must contain exactly 960 tile bytes or 1024 bytes including attributes")
    if attribute_data is None:
        raise ValueError("A 960-byte nametable needs a 64-byte attribute table via --attribute-file")
    if len(attribute_data) == 1024:
        attribute_data = attribute_data[NAMETABLE_BYTES:]
    if len(attribute_data) != ATTRIBUTE_BYTES:
        raise ValueError("Attribute file must contain exactly 64 bytes, or 1024 bytes with attributes at the end")
    return data, attribute_data


def render_nametable(chr_data: bytes, nametable: bytes, attributes: bytes,
                     palette_data: bytes | None = None, pattern_table: int = 0) -> bytes:
    """Return a 256x240 RGB buffer from raw CHR, nametable and attributes."""
    if pattern_table not in (0, 1):
        raise ValueError("Pattern table must be 0 or 1")
    if len(nametable) != NAMETABLE_BYTES:
        raise ValueError("Nametable tile map must be exactly 960 bytes")
    if len(attributes) != ATTRIBUTE_BYTES:
        raise ValueError("Attribute table must be exactly 64 bytes")
    base = pattern_table * PATTERN_TABLE_BYTES
    if len(chr_data) < base + PATTERN_TABLE_BYTES:
        raise ValueError("Selected CHR pattern table is incomplete (requires 4096 bytes)")
    if palette_data is not None:
        if len(palette_data) not in (16, 32):
            raise ValueError("Palette file must contain 16 background bytes or 32 PPU palette bytes")
        bg_palette = palette_data[:16]
        if any(value > 0x3F for value in bg_palette):
            raise ValueError("Palette entries must be NES color indices 0..63")
        colors = [NES_RGB[index] for index in bg_palette]
    else:
        # Diagnostic mode: useful for layout checks, not claimed as game colors.
        colors = [(0, 0, 0), (85, 85, 85), (170, 170, 170), (255, 255, 255)]

    rgb = bytearray(SCREEN_WIDTH * SCREEN_HEIGHT * 3)
    for tile_y in range(TILES_HIGH):
        for tile_x in range(TILES_WIDE):
            tile_index = nametable[tile_y * TILES_WIDE + tile_x]
            tile_start = base + tile_index * 16
            if tile_start + 16 > len(chr_data):
                raise ValueError(f"Nametable references missing CHR tile 0x{tile_index:02X} at ({tile_x}, {tile_y})")
            attr_index = (tile_y // 4) * 8 + (tile_x // 4)
            attr = attributes[attr_index]
            shift = ((tile_y & 2) << 1) | (tile_x & 2)
            palette_index = (attr >> shift) & 3
            palette_base = palette_index * 4
            tile = chr_data[tile_start:tile_start + 16]
            for row in range(8):
                plane0 = tile[row]
                plane1 = tile[row + 8]
                y = tile_y * 8 + row
                for col in range(8):
                    bit = 7 - col
                    pixel_index = ((plane0 >> bit) & 1) | (((plane1 >> bit) & 1) << 1)
                    if palette_data is None:
                        color = colors[pixel_index]
                    else:
                        # NES background color index 0 is the shared universal backdrop.
                        nes_color = bg_palette[0] if pixel_index == 0 else bg_palette[palette_base + pixel_index]
                        color = NES_RGB[nes_color]
                    x = tile_x * 8 + col
                    dst = (y * SCREEN_WIDTH + x) * 3
                    rgb[dst:dst + 3] = bytes(color)
    return bytes(rgb)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="iNES/NES 2.0 ROM or raw CHR dump")
    parser.add_argument("nametable", type=Path, help="960-byte tile map or 1024-byte nametable including attributes")
    parser.add_argument("output", type=Path, help="PNG output path (256x240)")
    parser.add_argument("--attribute-file", type=Path, help="64-byte attribute table, or 1024-byte nametable dump")
    parser.add_argument("--palette-file", type=Path, help="16-byte background or 32-byte PPU palette RAM dump")
    parser.add_argument("--pattern-table", type=int, choices=(0, 1), default=0,
                        help="CHR pattern table used by background (0 or 1; default: 0)")
    parser.add_argument("--chr-offset", type=lambda x: int(x, 0), help="verified raw CHR offset inside source")
    parser.add_argument("--chr-size", type=lambda x: int(x, 0), help="verified raw CHR byte count; requires --chr-offset")
    args = parser.parse_args()
    if args.chr_size is not None and args.chr_offset is None:
        parser.error("--chr-size requires --chr-offset")
    try:
        source = args.source.read_bytes()
        chr_data, source_offset = extract_chr(source, args.chr_offset, args.chr_size)
        nametable, attributes = split_nametable(
            args.nametable.read_bytes(),
            args.attribute_file.read_bytes() if args.attribute_file else None,
        )
        palette_data = args.palette_file.read_bytes() if args.palette_file else None
        pixels = render_nametable(chr_data, nametable, attributes, palette_data, args.pattern_table)
        write_png(args.output, SCREEN_WIDTH, SCREEN_HEIGHT, pixels)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    print(f"Rendered {SCREEN_WIDTH}x{SCREEN_HEIGHT} nametable from CHR offset 0x{source_offset:X}: {args.output}")
    if palette_data is None:
        print("Diagnostic grayscale used: supply a source-verified PPU palette dump for original colors.")
    print("This renders supplied PPU data only; it does not infer scrolling, sprite OAM, mapper bank changes or mid-frame writes.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
