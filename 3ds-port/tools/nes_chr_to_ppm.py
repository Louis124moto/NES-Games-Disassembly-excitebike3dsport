#!/usr/bin/env python3
"""Extract NES 2bpp CHR tiles to a readable PNG atlas and optional tile files.

Supports iNES/NES 2.0 files with CHR-ROM, raw 16-byte-per-tile CHR dumps, and
explicit byte ranges in arbitrary files. It does not decompress PRG graphics
streams or reconstruct PPU nametables/attribute tables automatically.
"""
import argparse
import json
import math
import struct
import zlib
from pathlib import Path

# Common NTSC NES palette. Exact perceived colors vary by display/emulator.
NES_RGB = (
    (84, 84, 84), (0, 30, 116), (8, 16, 144), (48, 0, 136),
    (68, 0, 100), (92, 0, 48), (84, 4, 0), (60, 24, 0),
    (32, 42, 0), (8, 58, 0), (0, 64, 0), (0, 60, 0),
    (0, 50, 60), (0, 0, 0), (0, 0, 0), (0, 0, 0),
    (152, 150, 152), (8, 76, 196), (48, 50, 236), (92, 30, 228),
    (136, 20, 176), (160, 20, 100), (152, 34, 32), (120, 60, 0),
    (84, 90, 0), (40, 114, 0), (8, 124, 0), (0, 118, 40),
    (0, 102, 120), (0, 0, 0), (0, 0, 0), (0, 0, 0),
    (236, 238, 236), (76, 154, 236), (120, 124, 236), (176, 98, 236),
    (228, 84, 236), (236, 88, 180), (236, 106, 100), (212, 136, 32),
    (160, 170, 0), (116, 196, 0), (76, 208, 32), (56, 204, 108),
    (56, 180, 204), (60, 60, 60), (0, 0, 0), (0, 0, 0),
    (236, 238, 236), (168, 204, 236), (188, 188, 236), (212, 178, 236),
    (236, 174, 236), (236, 174, 212), (236, 180, 176), (228, 196, 144),
    (204, 210, 120), (180, 222, 120), (168, 226, 144), (152, 226, 180),
    (160, 214, 228), (160, 162, 160), (0, 0, 0), (0, 0, 0),
)


def nes_rom_sizes(header: bytes) -> tuple[int, int]:
    """Return PRG and CHR byte sizes for iNES 1.0 or NES 2.0 headers."""
    prg_lsb, chr_lsb = header[4], header[5]
    nes2 = (header[7] & 0x0C) == 0x08

    def size(lsb: int, msb_nibble: int, unit: int) -> int:
        if not nes2 or msb_nibble != 0x0F:
            count = ((msb_nibble << 8) | lsb) if nes2 else lsb
            return count * unit
        exponent = lsb >> 2
        multiplier = (lsb & 0x03) * 2 + 1
        return (1 << exponent) * multiplier

    if nes2:
        msbs = header[9]
        prg_size = size(prg_lsb, msbs & 0x0F, 16384)
        chr_size = size(chr_lsb, (msbs >> 4) & 0x0F, 8192)
    else:
        prg_size = prg_lsb * 16384
        chr_size = chr_lsb * 8192
    return prg_size, chr_size


def extract_chr(data: bytes, offset: int | None = None,
                size: int | None = None) -> tuple[bytes, int]:
    """Return CHR bytes and their input-file offset.

    Explicit offsets are useful for uncompressed CHR-like data in a PRG ROM.
    Such ranges must still be verified as raw NES 2bpp tiles by the caller.
    """
    if offset is not None:
        if offset < 0 or offset > len(data):
            raise ValueError("CHR offset is outside the input file")
        end = len(data) if size is None else offset + size
        if end > len(data) or end <= offset:
            raise ValueError("CHR range is empty or extends beyond the input file")
        raw = data[offset:end]
        if len(raw) % 16:
            raise ValueError("Selected CHR range must be a multiple of 16 bytes")
        return raw, offset

    if data.startswith(b"NES\x1a"):
        if len(data) < 16:
            raise ValueError("Truncated iNES header")
        prg_size, chr_size = nes_rom_sizes(data[:16])
        trainer_size = 512 if data[6] & 0x04 else 0
        chr_offset = 16 + trainer_size + prg_size
        if chr_size == 0:
            raise ValueError(
                "ROM uses CHR-RAM (no CHR-ROM). The graphics may be stored or "
                "compressed in PRG data; provide a verified raw range with "
                "--chr-offset and --chr-size, or extract a runtime CHR-RAM dump."
            )
        if chr_offset + chr_size > len(data):
            raise ValueError("ROM is truncated before its declared CHR-ROM data")
        return data[chr_offset:chr_offset + chr_size], chr_offset

    if not data or len(data) % 16:
        raise ValueError("Raw CHR input must be a non-zero multiple of 16 bytes")
    return data, 0


def decode_tile(tile: bytes) -> list[int]:
    if len(tile) != 16:
        raise ValueError("A NES 2bpp tile is exactly 16 bytes")
    pixels = []
    for y in range(8):
        plane0, plane1 = tile[y], tile[y + 8]
        pixels.extend(
            ((plane0 >> bit) & 1) | (((plane1 >> bit) & 1) << 1)
            for bit in range(7, -1, -1)
        )
    return pixels


def load_palette(path: Path | None, subpalette: int, sprite_palette: bool) -> list[tuple[int, int, int]]:
    if path is None:
        return [(0, 0, 0), (85, 85, 85), (170, 170, 170), (255, 255, 255)]
    raw = path.read_bytes()
    if len(raw) not in (16, 32):
        raise ValueError("Palette file must contain 16 background bytes or 32 PPU palette bytes")
    if not 0 <= subpalette <= 3:
        raise ValueError("--subpalette must be 0, 1, 2, or 3")
    base = (16 if sprite_palette and len(raw) == 32 else 0) + subpalette * 4
    if sprite_palette and len(raw) == 16:
        raise ValueError("--sprite-palette requires a 32-byte PPU palette dump")
    indexes = raw[base:base + 4]
    if len(indexes) != 4:
        raise ValueError("Selected palette is incomplete")
    if any(index > 0x3F for index in indexes):
        raise ValueError("Palette entries must be NES color indices 0..63")
    return [NES_RGB[index] for index in indexes]


def render_pixels(chr_data: bytes, columns: int, scale: int,
                  palette: list[tuple[int, int, int]]) -> tuple[int, int, bytes, list[dict]]:
    if columns < 1 or scale < 1:
        raise ValueError("columns and scale must be positive")
    count = len(chr_data) // 16
    if count < 1:
        raise ValueError("No complete CHR tiles found")
    rows = math.ceil(count / columns)
    width, height = columns * 8, rows * 8
    rgb = bytearray(width * height * 3)
    metadata = []
    for index in range(count):
        raw = chr_data[index * 16:(index + 1) * 16]
        tile = decode_tile(raw)
        ox, oy = (index % columns) * 8, (index // columns) * 8
        for p, value in enumerate(tile):
            x, y = ox + p % 8, oy + p // 8
            dst = (y * width + x) * 3
            rgb[dst:dst + 3] = bytes(palette[value])
        metadata.append({
            "tile": index,
            "byte_offset": index * 16,
            "pattern_planes_hex": raw.hex(),
            "pixels_8x8": [tile[row * 8:(row + 1) * 8] for row in range(8)],
        })
    if scale > 1:
        enlarged = bytearray(width * scale * height * scale * 3)
        scaled_width = width * scale
        for y in range(height):
            row = rgb[y * width * 3:(y + 1) * width * 3]
            for sy in range(scale):
                start = ((y * scale + sy) * scaled_width) * 3
                for x in range(width):
                    color = row[x * 3:x * 3 + 3]
                    for sx in range(scale):
                        dst = start + (x * scale + sx) * 3
                        enlarged[dst:dst + 3] = color
        rgb = enlarged
        width *= scale
        height *= scale
    return width, height, bytes(rgb), metadata


def write_png(path: Path, width: int, height: int, rgb: bytes) -> None:
    if width <= 0 or height <= 0 or len(rgb) != width * height * 3:
        raise ValueError("Invalid RGB image dimensions or pixel buffer")

    def chunk(kind: bytes, payload: bytes) -> bytes:
        body = kind + payload
        return struct.pack(">I", len(payload)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    scanlines = b"".join(
        b"\x00" + rgb[y * width * 3:(y + 1) * width * 3]
        for y in range(height)
    )
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(scanlines, 9))
           + chunk(b"IEND", b""))
    path.write_bytes(png)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="iNES ROM, raw CHR, or file containing an explicit raw CHR range")
    parser.add_argument("output", type=Path, help="PNG atlas output (.ppm is also supported)")
    parser.add_argument("--columns", type=int, default=16, help="tiles per row (default: 16)")
    parser.add_argument("--scale", type=int, default=4, help="nearest-neighbour output scale (default: 4)")
    parser.add_argument("--chr-offset", type=lambda x: int(x, 0), help="verified raw CHR byte offset in the input file")
    parser.add_argument("--chr-size", type=lambda x: int(x, 0), help="raw CHR range size in bytes (used with --chr-offset)")
    parser.add_argument("--palette-file", type=Path, help="16-byte background or 32-byte PPU palette dump (values 0..63)")
    parser.add_argument("--subpalette", type=int, default=0, help="palette group 0..3 (default: 0)")
    parser.add_argument("--sprite-palette", action="store_true", help="use sprite palette group from a 32-byte palette dump")
    parser.add_argument("--json", dest="json_path", type=Path, help="write per-tile pixel/index metadata as JSON")
    parser.add_argument("--tiles-dir", type=Path, help="also export each 8x8 tile as an individual PNG")
    args = parser.parse_args()
    if args.chr_size is not None and args.chr_offset is None:
        parser.error("--chr-size requires --chr-offset")
    try:
        source = args.input.read_bytes()
        chr_data, source_offset = extract_chr(source, args.chr_offset, args.chr_size)
        palette = load_palette(args.palette_file, args.subpalette, args.sprite_palette)
        width, height, pixels, tiles = render_pixels(chr_data, args.columns, args.scale, palette)
        if args.output.suffix.lower() == ".ppm":
            args.output.write_bytes(f"P6\n{width} {height}\n255\n".encode("ascii") + pixels)
        else:
            write_png(args.output, width, height, pixels)
        if args.json_path:
            args.json_path.write_text(json.dumps({
                "input": str(args.input),
                "source_chr_offset": source_offset,
                "chr_bytes": len(chr_data),
                "tile_count": len(tiles),
                "atlas": {"width": width, "height": height, "columns": args.columns, "scale": args.scale},
                "palette_file": str(args.palette_file) if args.palette_file else None,
                "subpalette": args.subpalette,
                "tiles": tiles,
            }, indent=2) + "\n", encoding="utf-8")
        if args.tiles_dir:
            args.tiles_dir.mkdir(parents=True, exist_ok=True)
            for tile in tiles:
                tile_rgb = bytearray()
                for row in tile["pixels_8x8"]:
                    for value in row:
                        tile_rgb.extend(palette[value])
                write_png(args.tiles_dir / f"tile_{tile['tile']:03d}.png", 8, 8, bytes(tile_rgb))
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    print(f"Extracted {len(chr_data) // 16} tiles from byte offset 0x{source_offset:X}.")
    print(f"Atlas: {args.output} ({width}x{height}); scale={args.scale}; palette={'NES palette dump' if args.palette_file else 'diagnostic grayscale'}.")
    if args.json_path:
        print(f"Tile index: {args.json_path}")
    if args.tiles_dir:
        print(f"Individual tiles: {args.tiles_dir}")
    print("PNG/PPM output decodes raw 2bpp patterns only; it does not infer compressed PRG streams, nametables, attributes, or PPU timing.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
