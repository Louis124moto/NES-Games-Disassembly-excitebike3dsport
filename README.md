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

## Current status — 2026-11-22

<details>
<summary>Evidence-based status</summary>

- **Stage 1 — asset extraction:** accepted by owner decision on 2026-11-17, with verification debt.
- **Stage 2 — native menus/front-end:** the title, selector and design baseline was accepted by the owner for race-engine work on 2026-11-19. Optional editor and tape save/load screens are deferred; some transitions and visual/hardware validation remain.
- **Stage 3 — first playable race:** in progress. Multiple isolated native C systems now have host regression or ASM differential coverage, including bike movement, opponent logic, course-stream decoding, and pause/finish control. The complete race-frame scheduler, integration, track rendering, and end-to-end play are still unfinished.
- **Runtime boundary:** the 3DS target uses native C modules. The 6502 core is confined to host-side differential tests and is not part of the 3DS source glob or runtime.
- **Playability:** the application is not yet a playable race. The native race session and initial scene can be prepared/rendered, but the complete per-frame race update and bike/track presentation are not integrated.
- **Assembler dependencies:** the legacy Excitebike/assemble.sh script is optional. Native builds and host differential tests do not require cc65 or Lua; the host workflow reconstructs the reference PRG with 3ds-port/tools/asm_to_prg.py.

</details>

## Development stages

Stages are sequential. Do not start a later stage until the current one meets its acceptance gate.

1. **Stage 1 — source asset/data extraction.** Keep the accepted extractor and close its relevant verification debt. Extract only source-confirmed data into documented native formats. Keep the inspector separable from the game.
2. **Stage 2 — native menus and front-end.** The title, course selector and design/options baseline is accepted for race-engine work. Optional editor and tape save/load screens are deferred; complete the remaining race-entry transitions and visual/hardware validation in native C. No PPU/NMI/register emulation.
3. **Stage 3 — first complete race.** In progress. Implement one original course end-to-end: track, player bike, opponents, input, physics, collisions, heat, race timing, finish, scoring and results. Existing isolated C routines do not count as a playable race until integrated and compared per frame against original ASM.
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

This legacy helper is optional and is **not** part of the native 3DS build, host differential tests, or release dependency chain. It requires the original helper scripts plus **cc65 2.19** (`ca65` and `ld65`) and **Lua 5.3.x**. Its setup scripts may offer interactive dependency installation, so it is not suitable for unattended CI.

The host differential workflow does **not** invoke that assembler: `3ds-port/tools/asm_to_prg.py` reconstructs the 16 KiB reference PRG from the byte annotations already present in `Excitebike/bank_FF.asm`, then the host-only 6502 oracle executes reference routines. Consequently, installing cc65 or Lua is not required to build the native 3DS application or run the differential suite.

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
