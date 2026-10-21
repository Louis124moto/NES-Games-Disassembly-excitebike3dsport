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

The current implementation is a native state/input trace, not a completed faithful menu. ASM states 0, 1, and 8 have partial setup command lists and input branches. State 2 and destination states 9, 10, 12, and 13 still need their exact command lists and side effects; the app intentionally stops rather than inventing gameplay. Numeric option indices are not verified translations of the original option labels. Original title graphics also require verified CHR data plus nametable, attribute/palette, sprite/OAM, and PPU timing reconstruction.

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

Latest verification: the Python extraction tests and host C regression suite passed on `b47057371b09b68451119cf98706654e32243d3e` ([CI](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39523497914)). The corresponding 3DS build also passed ([CI](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39523497853)). These checks do not establish that the menu is visually or behaviorally complete.