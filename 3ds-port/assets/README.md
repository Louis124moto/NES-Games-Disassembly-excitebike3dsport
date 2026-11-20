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

The extractor decodes raw 2bpp tile patterns and exports source PPU-list snapshots. It does not decompress arbitrary PRG streams or infer dynamic menu updates. The native front-end consumes the exported EBPX tile pack plus the verified static setup-list snapshots; it composes these as native scene data and draws them directly with the 3DS framebuffer, without a runtime PPU or OAM emulator. These three scenes still require screenshot comparison against the original. Run its host checks with `python3 -m unittest discover -s 3ds-port/tools -p 'test_*.py'`.

## Remaining work

- [ ] Identify the exact source CHR data and record its provenance/hash without committing a ROM dump.
- [ ] Trace the original title/menu nametable writes, palette/attribute writes, and PPU timing.
- [ ] Map source tile indices to exact palette and background/sprite use.
- [ ] Add a native 3DS rendering path and validate orientation, transparency, and palette colors.
- [ ] Compare menu screenshots and selection states against the NES reference before claiming fidelity.


## Native 3DS CHR atlas path

The native renderer can display a decoded 2bpp CHR atlas on the top screen when the generated `include/nes_chr_assets.h` contains tiles. Generate that header from an actual ROM with CHR-ROM, or from a verified raw CHR dump:

```sh
python3 3ds-port/tools/nes_chr_to_c.py game.nes 3ds-port/include/nes_chr_assets.h
# or, only for a verified raw 2bpp range:
python3 3ds-port/tools/nes_chr_to_c.py game.nes 3ds-port/include/nes_chr_assets.h --chr-offset 0x4000 --chr-size 0x2000
```

The tool reuses the same CHR-ROM parsing and tile decoder as the PNG extractor. It emits source code, not a ROM dump, and the 3DS app draws the decoded tile indices directly from the generated header. The default checked-in header has zero tiles, so no fabricated art is displayed until a real source asset is supplied. This atlas is a diagnostic texture view only: the title screen still needs the original nametable, attribute/palette data, tile placement, and PPU update sequence before it can be called faithful.


## One-command extraction pipeline

From the repository root, use the orchestrator to create the atlas, per-tile metadata, C header, and optional individual tile images together:

```sh
python3 3ds-port/tools/extract_nes_assets.py game.nes --tiles
```

For a verified raw CHR range in a file with CHR-RAM or another layout:

```sh
python3 3ds-port/tools/extract_nes_assets.py game.nes --chr-offset 0x4000 --chr-size 0x2000 --install-header
```

The offset and size above are examples only; derive them from the actual ROM/disassembly before use. Outputs default to `3ds-port/build/extracted-assets/`, which is generated build data. `--install-header` explicitly installs the generated C header for the native diagnostic renderer. No ROM or dump is embedded in the repository.

The extractor does not reconstruct compressed PRG graphics, nametable/attribute layout, exact game palettes unless supplied, OAM ordering, or PPU timing. Those are still required before the title screen can be called faithful.

## Native front-end asset loading

The normal game startup does not open `rom.nes`. It reads the existing exported files from the `extracted/` directory beside the `.3dsx`:

- `chr_pixels.ebp` — versioned EBPX decoded 8x8 tile pack
- `ppu_snapshot_02.bin` — title setup scene
- `ppu_snapshot_06.bin` + `ppu_snapshot_03.bin` — selector setup scene, merged in source order
- `ppu_snapshot_06.bin` + `ppu_snapshot_15.bin` — design setup scene, merged in source order
- `ppu_palette_18.bin` and `ppu_palette_18_mask.bin` — palette selected by C514

Press **X** in the native front-end to open the optional asset extractor. It can generate/update these files from a source ROM/CHR file. Closing the inspector returns to the native front-end, which reloads the exported pack. The renderer uses native framebuffer pixel composition and a semantic cursor sprite; it does not run the PPU command interpreter, OAM builder or NES scanline renderer.

## Native first-course race scene

The first-course setup list 2 selects the source PPU snapshots in this order:

- `ppu_snapshot_18.bin` — `C551`, course palette/setup selection
- `ppu_snapshot_01.bin` — `C820`, track/background data upload (`D13F`)
- `ppu_snapshot_17.bin` — `C5A3`
- `ppu_snapshot_14.bin` — `C566`

The native loader merges these into scene 6. These files are optional for the front-end; if any are missing, the menu still loads but the first-course scene reports that extraction must be rerun. The extractor exports the full source snapshot set, so generate a fresh pack after updating the project.

`nativeRaceRenderFrame` draws the merged nametable with the original decoded CHR pixels and palette 18 using direct framebuffer composition. It overlays the seven initial HUD glyphs produced by `C875/C935` at the source nametable location. It does not execute the NES PPU command interpreter or OAM renderer. The initial track background/HUD still require screenshot comparison against a real NES capture; bike sprites, camera scrolling and the live race frame loop are not complete yet.
