# Port assets

Place converted 3DS-ready assets here: textures, track data, sprites, and audio. Do not commit generated build output or copyrighted ROM dumps. Record the origin and license of imported assets.

## Source-derived graphics inspection

Use `tools/nes_chr_to_ppm.py` to decode an iNES ROM with CHR-ROM or a raw CHR dump into a grayscale PPM tile atlas:

```sh
python3 3ds-port/tools/nes_chr_to_ppm.py Excitebike.nes /tmp/excitebike-chr.ppm
```

This atlas shows the four 2bpp pixel indices only. It does not apply original palettes, attribute tables, background/sprite selection, scrolling, transparency, or PPU upload timing. It is diagnostic output, not a faithful in-game texture pack.

## Remaining work

- [ ] Identify the exact source CHR data and record its provenance/hash without committing a ROM dump.
- [ ] Trace the original title/menu nametable writes, palette/attribute writes, and PPU timing.
- [ ] Map source tile indices to exact palette and background/sprite use.
- [ ] Add a native 3DS rendering path and validate orientation, transparency, and palette colors.
- [ ] Compare menu screenshots and selection states against the NES reference before claiming fidelity.
