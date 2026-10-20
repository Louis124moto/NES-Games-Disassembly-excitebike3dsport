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
