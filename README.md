# Excitebike 3DS — faithful port in progress

A Nintendo 3DS homebrew project aiming to reproduce the original NES **Excitebike** as faithfully as practical. The original 6502 disassembly is the reference for translating game logic into native C code for the 3DS's ARM11 processor.

> **Status: work in progress.** The 3DS application and porting infrastructure exist, and a subset of NES routines has C implementations and host-side regression tests. This is **not yet a complete or faithful playable port**. The current application still contains scaffolding; matching a routine's address/name does not prove its implementation is correct.

## Current progress

The detailed live checklist is in [`3ds-port/TODO.md`](3ds-port/TODO.md), and source-to-RAM/routine notes are in [`3ds-port/REFERENCE_MAPPING.md`](3ds-port/REFERENCE_MAPPING.md).

The current routine inventory reports:

- 165 unique `sub_XXXX` labels scanned in `Excitebike/bank_FF.asm`.
- 85 labels with a C function at the same address/name (about **51.5% nominal label coverage**).
- 80 labels without a matching C function in that inventory.

**These numbers measure name/address matching only. They are not a correctness score, do not mean 51.5% of the game is implemented, and do not establish test coverage or integration into gameplay.** Each translation still needs instruction-by-instruction review, side-effect checks, regression tests, and game-level comparison.

## Repository layout

- `Excitebike/` — original NES disassembly, assets, and assembly scripts.
- `_scripts/` — shared build helpers used by the NES assembly scripts.
- `3ds-port/source/` — native C homebrew and translated NES routines.
- `3ds-port/include/` — native port interfaces.
- `3ds-port/tests/` — host-side routine regression tests.
- `3ds-port/TODO.md` — roadmap, remaining routine inventory, and verification gates.
- `3ds-port/REFERENCE_MAPPING.md` — notes connecting NES routines, RAM locations, and native code.
- `.github/workflows/` — automated build and test workflows.

## Build the original NES disassembly

Run from the repository root.

**Linux / macOS**
```sh
sh Excitebike/assemble.sh
```

**Windows**

Run `Excitebike/assemble.bat`.

These commands assemble the NES version; they do not build the Nintendo 3DS application.

## Build the Nintendo 3DS homebrew

Install devkitPro with devkitARM and libctru, then run from the repository root:

```sh
make -C 3ds-port
```

The build output is placed under `3ds-port/build/`, including `excitebike3ds.3dsx` and `excitebike3ds.elf` when the build succeeds.

## Fidelity requirements

A faithful translation must preserve the original 6502 behavior, including:

- 8-bit wraparound and carry/borrow propagation;
- status-flag behavior where it affects later instructions;
- register and RAM side effects;
- branches, calls, lookup tables, and 16-bit address behavior;
- routine-specific edge cases and interactions with the game state.

Host-side tests help validate routines in isolation. They do not replace building the 3DS application, testing on hardware/emulator, or comparing complete gameplay scenarios against the original NES game. Work is marked complete only when the relevant evidence is recorded in the task tracker.

## Upstream source and credits

The original disassembly originates from [NES Games Disassembly](https://github.com/cyneprepou4uk/NES-Games-Disassembly). Preserve relevant upstream credits and follow the original project's applicable terms when reusing or redistributing its work.

## Contributing

Please focus contributions on the Excitebike 3DS port. When translating a routine, include the source label/address, preserve observable 6502 behavior, add regression tests for edge cases, and document what was actually built and tested. Clearly distinguish confirmed source behavior from hypotheses.
