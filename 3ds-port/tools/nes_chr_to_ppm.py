#!/usr/bin/env python3
"""Decode NES 2bpp CHR tiles into a dependency-free PPM diagnostic atlas.

The output uses grayscale indices, not Excitebike's original palette. It is a
source-data inspection tool, not the final in-game renderer or texture pack.
Accepts either an iNES ROM with CHR-ROM or a raw CHR byte dump.
"""
from __future__ import annotations

import argparse
import math
from pathlib import Path


def extract_chr(data: bytes) -> bytes:
    if data.startswith(b"NES\x1a"):
        if len(data) < 16:
            raise ValueError("Truncated iNES header")
        prg_banks = data[4]
        chr_banks = data[5]
        flags6 = data[6]
        if chr_banks == 0:
            raise ValueError("ROM declares CHR-RAM, not CHR-ROM; supply a raw CHR dump")
        offset = 16 + (512 if flags6 & 0x04 else 0) + prg_banks * 16384
        size = chr_banks * 8192
        if len(data) < offset + size:
            raise ValueError("ROM is truncated before its declared CHR-ROM data")
        return data[offset:offset + size]
    if len(data) == 0 or len(data) % 16:
        raise ValueError("Raw CHR input must contain a non-zero multiple of 16 bytes")
    return data


def decode_tile(tile: bytes) -> list[int]:
    if len(tile) != 16:
        raise ValueError("A NES 2bpp tile is exactly 16 bytes")
    pixels: list[int] = []
    for y in range(8):
        plane0 = tile[y]
        plane1 = tile[y + 8]
        for x in range(7, -1, -1):
            bit0 = (plane0 >> x) & 1
            bit1 = (plane1 >> x) & 1
            pixels.append(bit0 | (bit1 << 1))
    return pixels


def write_atlas(chr_data: bytes, output: Path, columns: int = 16) -> tuple[int, int, int]:
    if columns < 1:
        raise ValueError("columns must be at least 1")
    tile_count = len(chr_data) // 16
    if tile_count == 0:
        raise ValueError("No complete CHR tiles found")
    rows = math.ceil(tile_count / columns)
    width, height = columns * 8, rows * 8
    shades = (0, 85, 170, 255)
    rgb = bytearray(width * height * 3)
    for index in range(tile_count):
        tile = decode_tile(chr_data[index * 16:(index + 1) * 16])
        tile_x, tile_y = (index % columns) * 8, (index // columns) * 8
        for p, value in enumerate(tile):
            x, y = p % 8, p // 8
            dst = ((tile_y + y) * width + tile_x + x) * 3
            shade = shades[value]
            rgb[dst:dst + 3] = bytes((shade, shade, shade))
    output.write_bytes(f"P6\n{width} {height}\n255\n".encode("ascii") + rgb)
    return tile_count, width, height


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="iNES ROM or raw CHR byte dump")
    parser.add_argument("output", type=Path, help="output PPM atlas")
    parser.add_argument("--columns", type=int, default=16, help="tiles per row (default: 16)")
    args = parser.parse_args()
    try:
        chr_data = extract_chr(args.input.read_bytes())
        count, width, height = write_atlas(chr_data, args.output, args.columns)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    print(f"Decoded {count} tiles into {width}x{height} diagnostic atlas: {args.output}")
    print("Grayscale only: original palette, attributes, and runtime upload are not applied.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
