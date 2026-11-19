# Excitebike 3DS — faithful native port

> **Architecture rule (2026-11-19):** the shipped game is a full native Nintendo 3DS port written primarily in C. It must not execute NES code, boot a ROM, interpret/recompile 6502 instructions, or emulate NES hardware. No NES CPU, bus, PPU, APU, mapper, scanline renderer, or ROM file is part of the final game runtime. The original assembly is the behavioral specification; host-only reference execution is permitted solely for differential tests.

## Project goal

Recreate the original NES Excitebike as faithfully as possible on Nintendo 3DS hardware:

- original menus, navigation, course selection, editor and save/load behavior;
- course layouts, ramps, terrain and scrolling;
- bike handling, acceleration, braking, heat/cooling, jumps, posture, landing, crashes and recovery;
- opponents, collisions, race timing, scoring and results;
- original visual assets, animation, sound events and music, implemented through native 3DS systems.

The goal is behavioral fidelity, not merely a similar-looking game. No approximate physics, invented artwork, generic AI or guessed sound behavior may be presented as faithful.

## Non-negotiable architecture

- **Game logic:** native, deterministic C modules with semantic game state. Do not keep a live 64 KiB NES RAM image or 6502 register file as the game's state model.
- **Graphics:** native 3DS rendering using verified extracted assets and semantic scene/sprite data. No runtime PPU or NES register model.
- **Audio:** native 3DS playback/mixing from verified source sound data. No runtime APU/register emulation.
- **Input:** libctru HID mapped to game actions; do not copy NES controller bitmasks into hardware input code.
- **Platform:** devkitARM, libctru and native 3DS filesystem/timing/audio/graphics APIs.
- **Tests:** host-side deterministic regression and differential tests against the original ASM. A 6502 oracle or external emulator may exist under host test tooling only and must never be linked into the 3DS executable.
- **Assets:** original ASM and source assets under `Excitebike/` remain unchanged. Verified graphics, palettes, tables, course data and sound data may be extracted and converted to native formats.
- **Branches:** `main` is the faithful baseline. Do not create `enhanced` until the faithful game is complete, validated and accepted; create it from that exact accepted commit.

See [NATIVE_PORT_ARCHITECTURE.md](3ds-port/NATIVE_PORT_ARCHITECTURE.md) for the runtime boundary and module design, [PORTING_RULES.md](3ds-port/PORTING_RULES.md) for translation and verification rules, and [TODO.md](3ds-port/TODO.md) for the controlling roadmap.

## Current status — 2026-11-19

<details>
<summary>Evidence-based status</summary>

- **Stage 1 — asset extraction:** accepted by owner decision on 2026-11-17, with verification debt. The extractor/asset inspector is a development utility, not the final game and not a prerequisite that the released game must run at startup.
- **Stage 2 — native menus/front-end:** in progress and not accepted. Some source-derived native menu/input/timer/transition functions exist and have differential tests, but the complete menu, renderer, editor, save/load flow and application integration are not done.
- **Stage 3 — first playable race:** blocked until Stage 2 is accepted. Native C slices exist for player speed control, acceleration/deceleration, terrain gating, temperature/overheat and movement. They are not a complete physics loop or playable race.
- **Runtime boundary:** the emulator/recompiler gameplay runtime has been removed from the active 3DS build. The host-only 6502 oracle is test tooling. The current 3DS application is still an asset inspector, not a playable native Excitebike game.
- **Differential evidence:** the recorded native physics/movement slices total 1,798,992 reference vectors. The verified code revision `f214e5bbc96473896ca3abbbbf9e8cb6c8cc05f8` passed host differential CI [44585477913](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44585477913) and the New 3DS build [44585478080](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44585478080). These results validate only the tested slices, not a complete game.
- **Latest documentation commit before this rewrite:** `639554501e839b8758776bbd01b691f58eb9cc99`.

</details>

## Development stages

Stages are sequential. Do not start a later stage until the current one meets its acceptance gate.

1. **Stage 1 — source asset/data extraction.** Keep the accepted extractor and close its relevant verification debt. Extract only source-confirmed data into documented native formats. Keep the inspector separable from the game.
2. **Stage 2 — native menus and front-end.** Implement title, course selector, design menu/editor, save/load, transitions, menu state flow, visuals, animation and sound in native C. Integrate them into the actual game loop. No PPU/NMI/register emulation.
3. **Stage 3 — first complete race.** Implement one original course end-to-end: track, player bike, opponents, input, physics, collisions, heat, race timing, finish, scoring and results. Compare per-frame behavior against original ASM.
4. **Stage 4 — remaining original game.** Port the other courses, modes, editor interactions and all remaining original flows in source-confirmed order.
5. **Stage 5 — faithful release acceptance.** Validate complete sessions on New Nintendo 3DS and, where supported, original 3DS; verify visuals, audio, input, stability and frame pacing; publish the faithful `.3dsx`.
6. **After acceptance — enhanced branch.** Only then create `enhanced` from the accepted `main` commit. Keep optional improvements out of the faithful baseline.

## Repository map

- `Excitebike/` — original NES disassembly, source assets and assembly scripts. Preserve unchanged.
- `3ds-port/source/` — native 3DS application and native game systems.
- `3ds-port/include/` — native interfaces.
- `3ds-port/tests/` — host regression and differential tests.
- `3ds-port/tests/oracle/` — host-only reference CPU/oracle; never part of the 3DS link.
- `3ds-port/TODO.md` — controlling roadmap, status and acceptance record.
- `3ds-port/NATIVE_PORT_ARCHITECTURE.md` — runtime boundary and module design.
- `3ds-port/PORTING_RULES.md` — source tracing, fidelity and test policy.
- `3ds-port/ASM_FUNCTION_INVENTORY.md` — original routine inventory and translation tracking.
- `3ds-port/REFERENCE_MAPPING.md` — source-to-native observations and unresolved interpretations.
- `3ds-port/STAGE2_TRACE.md`, `3ds-port/STAGE3_TRACE.md` — source traces and historical implementation evidence.
- `.github/workflows/` — host tests and 3DS build workflows.

## Build

Install devkitPro with devkitARM and libctru using the [devkitPro getting-started guide](https://devkitpro.org/wiki/Getting_Started). From the repository root:

```sh
make -C 3ds-port
```

The intended homebrew artifact is:

- `3ds-port/build/excitebike3ds.3dsx`

A successful compile proves only that the target builds. It does not prove that the game is playable or faithful.

The original NES disassembly can be assembled separately for source/reference work:

```sh
sh Excitebike/assemble.sh
```

This is not part of the 3DS runtime or a release dependency.

## Acceptance standard

A subsystem is not complete merely because a C function exists or a unit test passes. Record all of the following as applicable:

1. source trace and dependency map;
2. native implementation using semantic game state;
3. regression coverage for normal paths, branches, boundaries and reset/transition cases;
4. differential results against original ASM behavior;
5. integration into the actual 3DS game loop;
6. exact-commit host tests and New 3DS build;
7. real-hardware validation where relevant;
8. known deviations, limitations, commit and CI links.

Never claim full-game fidelity until a complete end-to-end session has been compared and accepted.
