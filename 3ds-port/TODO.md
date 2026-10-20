# Excitebike 3DS — roadmap and task tracker

Tasks are checked only after completion and evidence. The native 3DS port is intended to match the original NES game's behaviour as closely as practical; the original 6502 assembly is a reference, not code that can be compiled directly for ARM11.

## Phase 1 — Build foundation

- [x] Create the `3ds-port/` project structure.
- [x] Add an initial C entry point and devkitARM Makefile.
- [x] Add a GitHub Actions workflow for native 3DS builds and artifact upload.
- [ ] Obtain a successful CI build that produces both `.3dsx` and `.elf`.
- [ ] Launch the build on a real 3DS or compatible emulator and record the result.
- [x] Add a `.gitignore` for generated build output and temporary files.

## Phase 2 — Application structure

- [x] Define module interfaces for game state, input, rendering, track, bike physics, collision, audio, and menus.
- [x] Add a central game lifecycle and deterministic module update order.
- [x] Add a basic top-screen debug/status view and bottom-screen console initialization.
- [x] Document local build requirements and commands.
- [x] Document the fidelity target, technical constraints, and original-source-to-native-module mapping.
- [ ] Compile and fix warnings/errors for the modular architecture.
- [ ] Verify controls and screen layout on hardware/emulator.
- [ ] Analyse the original Excitebike disassembly and map core routines/state before implementing authentic gameplay.

## Phase 3 — Playable prototype

- [ ] Draw a test track and ground.
- [ ] Add a controllable bike placeholder with visible position.
- [ ] Implement a restartable test course and finish condition.
- [ ] Replace temporary input/physics with an explicitly documented prototype model.

## Phase 4 — Faithful Excitebike implementation

- [ ] Map original disassembly routines, RAM variables, timing, and track data to native code.
- [ ] Reproduce acceleration, braking, overheating/cooling, jumping, posture, landing, and crashes from verified source behaviour.
- [ ] Reproduce track layouts, ramps, terrain, and course selection.
- [ ] Reproduce opponents, race progression, scoring, and results.
- [ ] Reproduce menus and sound/music using technically and legally suitable assets.
- [ ] Compare scenarios against the original NES game and document known differences.

## Phase 5 — 3DS polish and release

- [ ] Optimize rendering, memory use, and frame pacing for original 3DS hardware.
- [ ] Test controls and layout on original 3DS and New 3DS where available.
- [ ] Test clean builds and document known limitations.
- [ ] Produce a versioned `.3dsx` release and release notes.

## Fidelity and progress rules

- Preserve the original NES disassembly on its existing branch.
- Translate logic and data for the 3DS; do not assume 6502 assembly can be linked into an ARM11 homebrew.
- Mark fidelity tasks complete only after comparing behaviour with the original.
- The current modules are scaffolding, not a playable or faithful port.
