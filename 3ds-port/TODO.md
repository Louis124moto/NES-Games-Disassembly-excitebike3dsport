# Excitebike 3DS — roadmap and task tracker

Tasks are checked only after completion and evidence. The native 3DS port must match the original NES game's behaviour as closely as practical; the original 6502 assembly is a reference to trace and translate, not code that can be compiled directly for ARM11.

## Phase 1 — Build foundation

- [x] Create the `3ds-port/` project structure.
- [x] Add an initial C entry point and devkitARM Makefile.
- [x] Add a GitHub Actions workflow for native 3DS builds and artifact upload.
- [x] Verify CI produces both `.3dsx` and `.elf` under `3ds-port/build/` (run 39493444147 succeeded; artifact `excitebike3ds-homebrew` uploaded).
- [x] Launch the build on real 3DS hardware (user-confirmed: it launches and works; gameplay remains the architecture/debug scaffold, not faithful Excitebike).
- [x] Add a `.gitignore` for generated build output and temporary files.

## Phase 2 — Application structure and source analysis

- [x] Define module interfaces for game state, input, rendering, track, bike physics, collision, audio, and menus.
- [x] Add a central game lifecycle and deterministic module update order.
- [x] Add a basic top-screen debug/status view and bottom-screen console initialization.
- [x] Document local build requirements and commands.
- [x] Document the fidelity target, technical constraints, and original-source-to-native-module mapping.
- [x] Compile the modular architecture successfully after the output-directory change (CI run 39493444147; `.3dsx` and `.elf` artifacts uploaded).
- [x] Verify the application and controls/screen layout on real hardware (user-confirmed working; this does not validate NES gameplay fidelity).

- [x] Record the first verified temperature-meter/overheat routine trace from `sub_E359_display_temperature_meter_with_sprites` (CPU `$E359`, ROM offset `$002369`): update is skipped when `ram_0052_track_finished_flag` is nonzero; it derives an index from `ram_005C_obj & $C0` and race state, compares `ram_03B6` against `tbl_D8FB`, and adjusts the fractional/coarse pair `ram_03B5`/`ram_03B6` using `tbl_D8F7` or subtracts `$0B` on the cooling path. At coarse value `$1A`, a state-dependent branch can set the danger indicator. This is a partial trace, not a complete model of heat input/cooling.
- [x] Trace the NES controller sampling routine and held/press storage (`sub_D35B`, CPU `$D35B`, ROM offset `$00136B`); record its NES-specific bit and repeat behavior.
- [x] Trace the exact outer object-state dispatch loop (`sub_D918`/`sub_D924`) and the observed branch logic of dispatch target `$D933`; record that target meanings remain partially unresolved.
- [x] Trace the exact outer loop and sequence-table access in `sub_E733`/`sub_E73B`; explicitly distinguish object-path handling from player acceleration.
- [ ] Trace dispatch tables and all reachable state handlers, then map persistent RAM data flow for acceleration, heat/cooling, jumping/landing, collisions, and course progression before implementing those systems.

## Phase 3 — Playable prototype

Do not start until Phase 2's source-analysis gate is complete. This phase uses an explicitly labelled temporary prototype model, not guessed claims of NES fidelity.

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
- Distinguish confirmed source facts from hypotheses; record source labels, ROM offsets, and confidence.
- Mark fidelity tasks complete only after comparing behaviour with the original.
- The current modules are scaffolding, not a playable or faithful port.
