#!/usr/bin/env python3
"""Run the CHR inspection and native-header extraction tools as one asset pipeline.

This pipeline only decodes verified raw NES 2bpp CHR bytes. It does not infer
compressed graphics, nametable layout, attribute tables, or PPU upload timing.
"""
import argparse
from pathlib import Path
import subprocess
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="iNES/NES 2.0 ROM or raw 16-byte-per-tile CHR dump")
    parser.add_argument("--output-dir", type=Path, default=Path("3ds-port/build/extracted-assets"),
                        help="output directory (default: 3ds-port/build/extracted-assets)")
    parser.add_argument("--chr-offset", type=lambda value: int(value, 0),
                        help="verified raw CHR offset in a file that is not a standard CHR-ROM image")
    parser.add_argument("--chr-size", type=lambda value: int(value, 0),
                        help="verified raw CHR range size; requires --chr-offset")
    parser.add_argument("--palette-file", type=Path,
                        help="optional 16-byte background or 32-byte full PPU palette dump")
    parser.add_argument("--subpalette", type=int, default=0, help="palette group 0..3 (default: 0)")
    parser.add_argument("--sprite-palette", action="store_true",
                        help="use sprite palette group from a 32-byte PPU palette dump")
    parser.add_argument("--scale", type=int, default=4, help="PNG atlas scale (default: 4)")
    parser.add_argument("--columns", type=int, default=16, help="tiles per row (default: 16)")
    parser.add_argument("--tiles", action="store_true", help="also export each tile as a PNG")
    parser.add_argument("--install-header", action="store_true",
                        help="also replace 3ds-port/include/nes_chr_assets.h for the native renderer")
    args = parser.parse_args()
    if args.chr_size is not None and args.chr_offset is None:
        parser.error("--chr-size requires --chr-offset")
    if args.chr_offset is not None and args.chr_offset < 0:
        parser.error("--chr-offset must not be negative")
    if args.chr_size is not None and args.chr_size <= 0:
        parser.error("--chr-size must be positive")

    tools_dir = Path(__file__).resolve().parent
    out = args.output_dir
    out.mkdir(parents=True, exist_ok=True)
    atlas = out / "chr-atlas.png"
    metadata = out / "chr-tiles.json"
    header = out / "nes_chr_assets.h"
    common = []
    if args.chr_offset is not None:
        common.extend(["--chr-offset", hex(args.chr_offset)])
    if args.chr_size is not None:
        common.extend(["--chr-size", hex(args.chr_size)])

    atlas_cmd = [sys.executable, str(tools_dir / "nes_chr_to_ppm.py"),
                 str(args.source), str(atlas), "--json", str(metadata),
                 "--scale", str(args.scale), "--columns", str(args.columns), *common]
    if args.palette_file is not None:
        atlas_cmd.extend(["--palette-file", str(args.palette_file), "--subpalette", str(args.subpalette)])
    elif args.subpalette != 0 or args.sprite_palette:
        parser.error("--subpalette/--sprite-palette require --palette-file")
    if args.sprite_palette:
        atlas_cmd.append("--sprite-palette")
    if args.tiles:
        atlas_cmd.extend(["--tiles-dir", str(out / "tiles")])

    header_cmd = [sys.executable, str(tools_dir / "nes_chr_to_c.py"),
                  str(args.source), str(header), *common]
    try:
        subprocess.run(atlas_cmd, check=True)
        subprocess.run(header_cmd, check=True)
        if args.install_header:
            target = Path(__file__).resolve().parents[1] / "include" / "nes_chr_assets.h"
            target.write_bytes(header.read_bytes())
            print(f"Installed generated header: {target}")
    except (OSError, subprocess.CalledProcessError) as exc:
        print(f"Asset extraction failed: {exc}", file=sys.stderr)
        return 1

    print(f"CHR extraction pipeline complete: {out}")
    print("The atlas/header contain decoded CHR tiles only; title-screen placement and PPU behavior remain separate tasks.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
