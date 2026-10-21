#!/usr/bin/env python3
"""Decode NES 2bpp CHR tiles into a grayscale PPM atlas for source inspection."""
import argparse
import math
from pathlib import Path


def extract_chr(data: bytes) -> bytes:
    if data.startswith(b"NES\x1a"):
        if len(data) < 16:
            raise ValueError("Truncated iNES header")
        prg_banks, chr_banks, flags6 = data[4], data[5], data[6]
        if chr_banks == 0:
            raise ValueError("ROM declares CHR-RAM; provide a raw CHR dump")
        offset = 16 + (512 if flags6 & 0x04 else 0) + prg_banks * 16384
        size = chr_banks * 8192
        if len(data) < offset + size:
            raise ValueError("ROM is truncated before declared CHR data")
        return data[offset:offset + size]
    if not data or len(data) % 16:
        raise ValueError("Raw CHR input must be a non-zero multiple of 16 bytes")
    return data


def decode_tile(tile: bytes) -> list[int]:
    if len(tile) != 16:
        raise ValueError("A NES 2bpp tile is exactly 16 bytes")
    out = []
    for y in range(8):
        p0, p1 = tile[y], tile[y + 8]
        out.extend(((p0 >> x) & 1) | (((p1 >> x) & 1) << 1) for x in range(7, -1, -1))
    return out


def write_atlas(chr_data: bytes, output: Path, columns: int) -> tuple[int, int, int]:
    if columns < 1:
        raise ValueError("columns must be positive")
    count = len(chr_data) // 16
    if count < 1:
        raise ValueError("No complete CHR tiles found")
    rows = math.ceil(count / columns)
    width, height = columns * 8, rows * 8
    shades = (0, 85, 170, 255)
    pixels = bytearray(width * height * 3)
    for i in range(count):
        tile = decode_tile(chr_data[i * 16:(i + 1) * 16])
        ox, oy = (i % columns) * 8, (i // columns) * 8
        for p, value in enumerate(tile):
            x, y = p % 8, p // 8
            dst = ((oy + y) * width + ox + x) * 3
            pixels[dst:dst + 3] = bytes((shades[value],) * 3)
    output.write_bytes(f"P6\n{width} {height}\n255\n".encode("ascii") + pixels)
    return count, width, height


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="iNES ROM or raw CHR dump")
    parser.add_argument("output", type=Path, help="output PPM atlas")
    parser.add_argument("--columns", type=int, default=16)
    args = parser.parse_args()
    try:
        count, width, height = write_atlas(extract_chr(args.input.read_bytes()), args.output, args.columns)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    print(f"Decoded {count} tiles into {width}x{height} diagnostic atlas: {args.output}")
    print("Grayscale diagnostic only; NES palettes, attributes and rendering order are not applied.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
