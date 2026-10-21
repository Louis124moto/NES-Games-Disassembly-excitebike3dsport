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

The application now has separate modules for shared game state, input, rendering, bike physics, collision, track, audio, and menus. The current physics and display are scaffolding for testing module boundaries; there is no playable or faithful Excitebike implementation yet.

See [TODO.md](TODO.md) for the staged roadmap and [REFERENCE_MAPPING.md](REFERENCE_MAPPING.md) for the fidelity target and plan for mapping original NES routines to native code.

## Fidelity approach

The NES source is 6502 assembly and relies on NES-specific hardware. It cannot be linked directly into a native ARM11/libctru executable. The port will translate verified gameplay logic and data into native modules, then compare behaviour against the original. Any subsystem remains marked incomplete until validated.

## Menu port status (2026-10-21)

The current implementation is a native state/input trace, not a completed faithful menu. ASM states 0, 1, and 8 have partial setup command lists and input branches. State 2 and destination states 9, 10, and 12 still need their exact command lists and side effects. State 13 now uses the exact shared two-command setup from state 8, but its state handler is still unported. The app intentionally stops rather than inventing gameplay. Numeric option indices are not verified translations of the original option labels. Original title graphics also require verified CHR data plus nametable, attribute/palette, sprite/OAM, and PPU timing reconstruction.

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

Latest verification on `e3459d1e518603b73375aaa9ff06215358c7e8a6`: Python extractor tests, host C routine regression tests, and runtime iNES decoder tests passed ([CI](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39524481254)). The matching 3DS build and artifact checks also passed ([CI](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39524481329)). These checks do not establish that the menu is visually or behaviorally complete.

### Built-in runtime CHR extractor (on the 3DS)

The homebrew now includes a runtime inspector; the Python scripts are optional PC-side tools, not a runtime dependency. Copy a legally obtained **classic iNES** ROM with CHR-ROM to `sdmc:/3ds/excitebike3ds/rom.nes` on the SD card (create the `3ds/excitebike3ds` folders if needed), then launch the homebrew and press **Y**.

- **Y**: open/close the extractor
- **X**: reload the ROM from the SD card
- **B**: close the extractor and return to the menu

The inspector validates the iNES magic and header lengths, accounts for the optional 512-byte trainer, checks the PRG/CHR ranges against the file size, and decodes up to 960 raw 2bpp CHR tiles for a grayscale atlas. It reports errors instead of reading beyond the ROM. This first runtime version does not yet support NES 2.0 headers, CHR-RAM snapshots, compressed graphics in PRG, PPU palette/nametable mapping, or original menu reconstruction. Do not rename a random file to `rom.nes` and assume it is compatible.

### Latest menu routine batch

The setup prelude now models the side effects visible in the source: all 256 OAM bytes become `$F8`, both `$2000` and `$2400` nametable blocks are initialized to `$FC` for 960 bytes and zero for the 64-byte attribute region, scroll resets to zero, PPU mask is disabled, and PPU control becomes `$10`. The host regression suite has a test for these exact memory effects. The state-2 six-command setup list and states 9, 10, and 12 remain incomplete; state 13 has its shared setup list but no handler. The main menu is not yet fidelity-complete.