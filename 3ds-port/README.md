# Excitebike 3DS — native port scaffold

This directory is the starting point for a **native Nintendo 3DS homebrew port**. It is separate from `../Excitebike/`, which contains the original NES assembly/disassembly project.

## Requirements

- devkitPro with devkitARM
- libctru
- 3DS development tools provided by devkitPro

On Arch Linux, install and configure devkitPro using its official instructions: https://devkitpro.org/wiki/Getting_Started

## Build

From this directory, with the devkitPro environment configured:

```sh
make
```

The build is expected to produce `excitebike3ds.3dsx` (and a `.elf` build artifact). Copy the `.3dsx` to your SD card to run it with a homebrew launcher.

## Current status

This is only the initial application scaffold: it initializes the 3DS graphics/input system and displays a status screen. **It is not yet a playable Excitebike port.** Gameplay, physics, track rendering, audio, menus, and asset conversion still need to be implemented.

## Planned layout

- `source/main.c` — application entry point and main loop
- `include/` — project headers
- `assets/` — converted graphics, sound, and track data
- `build/` — generated files (not committed)
- `Makefile` — devkitARM/libctru build configuration

The original NES source remains in the parent repository. Reuse or adapt it only where licensing and technical compatibility permit; NES assembly cannot be compiled directly into a native 3DS application.
