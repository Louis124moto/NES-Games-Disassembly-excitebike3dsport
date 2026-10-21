# Port assets

Place converted 3DS-ready assets here: textures, track data, sprites, and audio. Do not commit generated build output or copyrighted ROM dumps. Record the origin and license of imported assets.

## Source-derived graphics extraction

The dependency-free `tools/nes_chr_to_ppm.py` now writes a scaled PNG atlas by default, plus optional tile metadata and individual 8×8 PNGs:

```sh
python3 3ds-port/tools/nes_chr_to_ppm.py raw-chr.bin /tmp/excitebike-chr.png --scale 4 --json /tmp/tiles.json --tiles-dir /tmp/tiles
```

For a ROM that has CHR-ROM, pass the iNES file directly:

```sh
python3 3ds-port/tools/nes_chr_to_ppm.py game.nes /tmp/chr.png --scale 4
```

For a verified uncompressed 2bpp graphics range inside a CHR-RAM ROM, specify both the byte offset and size (decimal or `0x` hexadecimal):

```sh
python3 3ds-port/tools/nes_chr_to_ppm.py game.nes /tmp/chr.png --chr-offset 0x4000 --chr-size 0x2000
```

**Do not guess that offset:** inspect the disassembly/ROM first. The tool refuses to treat a CHR-RAM iNES header as if it contained CHR-ROM. If a 16-byte background palette dump or 32-byte full PPU palette dump is available, pass it with `--palette-file palette.bin --subpalette 0`; use `--sprite-palette` to select sprite palettes from a 32-byte dump. Palette bytes must be NES color indices 0–63.

The extractor decodes raw 2bpp tile patterns; it does not decompress arbitrary PRG streams or infer the title screen's nametable, attribute table, tile placement, transparency rules, scrolling, or PPU upload timing. PNG output is a readable inspection atlas, not yet a faithful in-game texture pack. Run its host checks with `python3 -m unittest discover -s 3ds-port/tools -p 'test_*.py'`.

## Remaining work

- [ ] Identify the exact source CHR data and record its provenance/hash without committing a ROM dump.
- [ ] Trace the original title/menu nametable writes, palette/attribute writes, and PPU timing.
- [ ] Map source tile indices to exact palette and background/sprite use.
- [ ] Add a native 3DS rendering path and validate orientation, transparency, and palette colors.
- [ ] Compare menu screenshots and selection states against the NES reference before claiming fidelity.
