# Excitebike 3DS — native homebrew port

This directory contains the native Nintendo 3DS application. The original NES disassembly remains in `../Excitebike/` and is preserved separately.

## Requirements and build

Install devkitPro with devkitARM and libctru using the official instructions: https://devkitpro.org/wiki/Getting_Started

From this directory:

```sh
make
make clean
```

The CI build should produce `build/excitebike3ds.3dsx` and `build/excitebike3ds.elf`. Generated objects, dependency files, and final binaries belong under `build/`. The `.3dsx` can be copied to an SD card for a homebrew launcher, once the build and launch have been verified.

## Architecture status

The application now has separate modules for shared game state, input, rendering, bike physics, collision, track, audio, and menus. The runtime deliberately does not execute the partial menu trace or generic bike/track scaffolding. Until the source-derived menu, PPU rendering, and gameplay dependencies are complete, the homebrew shows no invented game screen; only the explicitly diagnostic CHR viewer can be opened with Y.

See [TODO.md](TODO.md) for the staged roadmap and [REFERENCE_MAPPING.md](REFERENCE_MAPPING.md) for the fidelity target and plan for mapping original NES routines to native code.

## Fidelity approach

The NES source is 6502 assembly and relies on NES-specific hardware. It cannot be linked directly into a native ARM11/libctru executable. The port will translate verified gameplay logic and data into native modules, then compare behaviour against the original. Any subsystem remains marked incomplete until validated.

## Menu port status (updated 2026-10-21)

**The menu is not finished yet.** The active runtime remains diagnostic-only: it does not pretend that partial ASM translations are a working menu. Current source-derived work includes the `sub_C2A9` dispatcher trace, the state-2 command-list trace, isolated state-selection helpers (`C455`, `C55E`, `C56A`), table lookup helpers (`C522`, `C53A`, `C5A7`), and a new native translation of `ofs_001_C485_03_05` with branch/arithmetic regression vectors. The C485 vectors are included in the passing host suite on revision `601e7e3` ([CI run 39577668191](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39577668191)); the helper remains isolated and is not integrated into the active runtime.

The next gate is to finish the **entire interactive menu flow** before starting course implementation: integrate every required setup/transition command in source pointer-table order; preserve timer gates and byte-level state changes; implement the menu's input/confirmation paths; reconstruct title/menu graphics from verified CHR tiles, nametables, attributes, palettes, OAM and timing; then verify the full flow against the original NES version. State 3 and several other transition states remain incomplete. `sub_C621` now has an isolated source-faithful 256-byte copy translation and regression vector, but its callers are not integrated; the state-2 race-start path still has unported data-buffer and track-preparation dependencies. The four-entry `sub_CB2C` indirect handler lookup is now translated and regression-tested, but is not yet wired into a running native menu state machine. Isolated helpers do not count as a complete menu.

**After the menu passes its end-to-end gate, work on Course 1 only.** Finish the first course's track data, player controls/physics, opponents, collisions, overheating, race start/finish and scoring as one verifiable slice. Do not start courses 2+ until Course 1 is validated against the original. See [TODO.md](TODO.md) for the detailed acceptance gates and decisions still needed.

### Extract graphics and prepare the native diagnostic atlas

Run from the repository root with a legally obtained ROM or raw CHR dump:

```sh
python3 3ds-port/tools/extract_nes_assets.py game.nes
```

Outputs go to `3ds-port/build/extracted-assets/`: `chr-atlas.png`, `chr-tiles.json`, and `nes_chr_assets.h`. Add `--tiles` for individual PNGs. For a verified raw 2bpp range inside a CHR-RAM image, add both `--chr-offset 0x... --chr-size 0x...`; do not guess the range. Add `--install-header` to copy the generated header into `3ds-port/include/nes_chr_assets.h` so the native renderer can display the diagnostic atlas on the next build. The atlas is not the original title-screen layout.

### Test and build

```sh
python3 -m unittest discover -s 3ds-port/tools -p 'test_*.py'
make -C 3ds-port
```

Historical verification on `2fb6e14470a604c32785e7e4f6c3bc68392ddfbd` (before disabling the placeholder runtime): Python extractor tests, host C routine regression tests, runtime iNES decoder tests, and the state-13 entry test passed ([CI](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39525039178)). The matching 3DS build and artifact checks also passed ([CI](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39525039042)). These checks do not establish that the menu is visually or behaviorally complete.

### Built-in runtime CHR extractor (on the 3DS)

The homebrew now includes a runtime inspector; the Python scripts are optional PC-side tools, not a runtime dependency. Copy a legally obtained iNES or supported NES 2.0 ROM with CHR-ROM named `rom.nes` into the **same SD-card folder as `excitebike3ds.3dsx`**. Launch the homebrew and press **Y**. The extractor derives this path from the launched `.3dsx` path, so it does not require a fixed `sdmc:/3ds/excitebike3ds/` folder. The decoded cache is saved beside it at `extracted/chr_pixels.bin`.

- **Y**: open/close the extractor
- **X**: reload the ROM from the SD card
- **B**: close the extractor and return to the menu

The inspector validates the iNES magic and header lengths, accounts for the optional 512-byte trainer, checks the PRG/CHR ranges against the file size, and decodes up to 960 raw 2bpp CHR tiles for a grayscale atlas. It reports errors instead of reading beyond the ROM. The runtime decoder now supports standard NES 2.0 ROM-size fields and exponent/multiplier size fields within the 8 MiB runtime input limit. CHR-RAM snapshots, compressed graphics in PRG, PPU palette/nametable mapping, and original menu reconstruction are still unsupported. Do not rename a random file to `rom.nes` and assume it is compatible.

### Latest menu routine batch

The setup prelude now models the side effects visible in the source: all 256 OAM bytes become `$F8`, both `$2000` and `$2400` nametable blocks are initialized to `$FC` for 960 bytes and zero for the 64-byte attribute region, scroll resets to zero, PPU mask is disabled, and PPU control becomes `$10`. The host regression suite has a test for these exact memory effects. State 2's six-command list is still incomplete; commands C820 and C875 have deep data-buffer/race-start dependencies and must not be treated as ordinary menu rendering. States 3, 5, 7 (blocked at sub_C621), 9, 10, 11, and 12 remain incomplete. State 13's setup, entry wait, and `$FF` completion branch (`sub_EC3B`/`loc_CC6B`) are translated and host-tested. The main menu is not yet fidelity-complete.

## Runtime extraction log on the bottom screen

At startup, the homebrew now checks `extracted/chr_pixels.bin` first and falls back to extracting CHR-ROM from `rom.nes` when the cache is missing or invalid. The bottom-screen console remains visible even when the atlas viewer is closed and reports the status, decoded tile count, expected file locations, and viewer state.

- **Y** — open/close the top-screen grayscale CHR atlas.
- **X** — retry ROM extraction and refresh the cache while the atlas is open.
- **B** — close the atlas.
- **ROM location** — `rom.nes` in the same directory as the launched `.3dsx`.
- **Cache location** — `extracted/chr_pixels.bin` beside the `.3dsx`.

A message saying “Loaded … CHR tiles from extracted cache” means the cache passed its format and pixel-index checks. “Extracted … CHR tiles; cache saved” means the current ROM was decoded and the cache write succeeded. Any file/decoder error is shown on the bottom screen. The log is designed to make failures visible, but the `.3dsx` argument path and SD-card I/O still require verification on the target 3DS/homebrew launcher.

This remains a CHR-pattern diagnostic tool, not a reconstructed title screen: authentic menu layout, palettes, nametable/attribute data, sprite ordering, and PPU timing remain menu-completion tasks. See [TODO.md](TODO.md) for the acceptance checklist.


### Fixes for the 3DS hardware report (2026-10-21)

The bottom-screen log is now intentionally short, with one action per line, to avoid wrapping text beyond the visible console area. The top-screen atlas renderer now uses the 3DS 8×8 tiled framebuffer layout rather than treating the framebuffer as linear memory; it also clears the screen when the viewer is closed. NES 2.0 size decoding and a regression case have been added. The latest CI results must be checked before these fixes are considered validated, and a real-console run is still required.
