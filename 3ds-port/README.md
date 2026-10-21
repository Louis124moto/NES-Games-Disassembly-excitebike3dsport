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

## Menu port status (2026-10-21)

The partial native menu trace is no longer executed by the homebrew. Setup-command effects are partially translated for states 0, 1, 4, 6, 8, and 13; state 7 blocks at its unported `sub_C621` dependency. Transition lists 3, 5, 9, 10, 11, and 12 and the complete race-start path still need source-faithful implementations. State 13 uses the exact shared two-command setup from state 8; its entry wait (`ram_0048=2`, `ram_0044=1`) is translated and tested, but the later `ram_0048==$FF` branch through `sub_EC3B`/`loc_CC6B` is not ported. The app intentionally stops rather than inventing gameplay. Numeric option indices are not verified translations of the original option labels. Original title graphics also require verified CHR data plus nametable, attribute/palette, sprite/OAM, and PPU timing reconstruction.

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

The homebrew now includes a runtime inspector; the Python scripts are optional PC-side tools, not a runtime dependency. Copy a legally obtained **classic iNES** ROM with CHR-ROM named `rom.nes` into the **same SD-card folder as `excitebike3ds.3dsx`**. Launch the homebrew and press **Y**. The extractor derives this path from the launched `.3dsx` path, so it does not require a fixed `sdmc:/3ds/excitebike3ds/` folder. The decoded cache is saved beside it at `extracted/chr_pixels.bin`.

- **Y**: open/close the extractor
- **X**: reload the ROM from the SD card
- **B**: close the extractor and return to the menu

The inspector validates the iNES magic and header lengths, accounts for the optional 512-byte trainer, checks the PRG/CHR ranges against the file size, and decodes up to 960 raw 2bpp CHR tiles for a grayscale atlas. It reports errors instead of reading beyond the ROM. This first runtime version does not yet support NES 2.0 headers, CHR-RAM snapshots, compressed graphics in PRG, PPU palette/nametable mapping, or original menu reconstruction. Do not rename a random file to `rom.nes` and assume it is compatible.

### Latest menu routine batch

The setup prelude now models the side effects visible in the source: all 256 OAM bytes become `$F8`, both `$2000` and `$2400` nametable blocks are initialized to `$FC` for 960 bytes and zero for the 64-byte attribute region, scroll resets to zero, PPU mask is disabled, and PPU control becomes `$10`. The host regression suite has a test for these exact memory effects. State 2's six-command list is still incomplete; commands C820 and C875 have deep data-buffer/race-start dependencies and must not be treated as ordinary menu rendering. States 3, 5, 7 (blocked at sub_C621), 9, 10, 11, and 12 remain incomplete. State 13's setup, entry wait, and `$FF` completion branch (`sub_EC3B`/`loc_CC6B`) are translated and host-tested. The main menu is not yet fidelity-complete.