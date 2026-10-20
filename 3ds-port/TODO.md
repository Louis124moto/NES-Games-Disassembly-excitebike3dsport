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
- [x] Resolve the exact pointer values for dispatch states `$00-$05` from `tbl_D8FF_lo`/`tbl_D905_hi`; document only state behavior directly supported by the visible instructions.
- [x] Trace visible branch and byte-operation behavior for dispatch states `$02-$05` and `sub_DA15`; record unresolved variable semantics separately.
- [x] Add a standalone byte-exact C translation of `sub_DA15` in `nes_routines.c`, preserving 6502 carry/borrow semantics; it is not yet connected to placeholder physics.
- [x] Build the new routine into the 3DS homebrew (run `39495476753` succeeded and uploaded `.3dsx`/`.elf`).
- [x] Add a host-side exhaustive comparison test and CI workflow for all 256³ input combinations.
- [x] Confirm the exhaustive-test workflow passes (run `39495633829`, conclusion `success`; compile and all 16,777,216 input comparisons passed).
- [ ] Trace every writer of `ram_009C_obj,X` and the producers of `ram_0370_obj,X`, `ram_0390_obj,X`, `ram_0080_obj,X`, `ram_0094_obj,X`, `ram_00DC_obj,X`, and `ram_00BC_obj,X`; establish player-slot identity before implementing gameplay.

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

## Source-analysis checkpoint (2026-10-20)

- [x] Scan `bank_FF.asm` for direct write instructions affecting `ram_0390_obj`, `ram_0080_obj`, `ram_0094_obj`, `ram_00DC_obj`, `ram_00BC_obj`, `ram_0370_obj`, and `ram_009C_obj`; record writer addresses and interpretation limits in `REFERENCE_MAPPING.md`.
- [x] Confirm the exhaustive `sub_DA15` test passed in CI run [39495633829](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39495633829) and the native 3DS build passed in run [39495627228](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39495627228).
- [ ] Trace control flow and callers around every writer above; establish slot identity and initialization/reset sequences before translating any stateful behavior.
- [ ] Check indirect or alias writes to these RAM addresses and cross-check the RAM symbol declarations/memory map.

- [x] Trace `sub_DA7A`'s opponent-only scroll-relative coordinate correction, `sub_DB95`'s opponent-slot reset path, and `sub_E96C`'s per-frame Y-delta application; record the confirmed facts and remaining uncertainties in `REFERENCE_MAPPING.md`.
- [x] Record caller placement and surrounding frame-order for `sub_DA7A` and `sub_E96C`, including the `ram_03F2` gate and the adjacent update calls.

- [x] Translate the byte-level logic of sub_D310 into nesSubD310DecreaseAllTimers, preserving 8-bit wraparound, BPL's negative-flag condition, selected timer ranges, and nonzero-only decrements.
- [ ] Verify the expanded host regression test and 3DS build in GitHub Actions before marking the timer helper validated.
- [x] Translate `sub_DA7A` as a byte-exact helper and add instruction-level reference tests.
- [x] Translate six additional RAM-level routines: `sub_DBFE`, `sub_DC9A` (with its `$DC97` load entry), `sub_DCDE`, `sub_DD38`, `sub_DD6F`, and `sub_DB95`; add regression tests.
- [x] Host regression suite passed and Nintendo 3DS build passed for the six-routine code revision `730a1048`; later commits only update documentation/inventory.
- [x] Translate `sub_DA6A`, `sub_DB50`, `sub_DCA0`, and shared entry `loc_DCC7`; add regression coverage.
- [x] Host regression suite passed on code revision `a7537aff`; the Nintendo 3DS compile and artifact checks also passed on that revision.
- [ ] Continue translating the remaining `sub_*` routines systematically, preserving incoming flags, branch behavior, indexed slot bounds, and byte wraparound.

- [x] Create a full checklist of all 177 `sub_*` labels in `ASM_FUNCTION_INVENTORY.md`; distinguish inventory status from actual port completion.
- [x] Validate `sub_D310` host regression against the independent reference: [workflow run 39496864809](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39496864809) passed.
- [x] Confirm host-test and 3DS-build CI for `sub_DA7A`.

- [x] Translate sub_D326's eight-byte ROR carry chain into `nesSubD326RotateInputBits`; host regression and 3DS build are pending CI.

- [x] Translate sub_D35B's portable controller decode and A-button press suppression; host regression and 3DS build pending CI.

- [x] Validate host-side regression tests for `sub_D326` and `sub_D35B`: [D326 test run](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39497063113), [D35B test run](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39497203936).
- [ ] Confirm the newest 3DS build after adding `sub_D35B`; keep all native helper translations separate from gameplay until integrated through verified call order.


- [x] Translate `sub_DA58` with the original add/carry side effect and validate against the byte-level reference.
- [x] Translate `sub_E091` and `sub_E098`, preserving the `CMP #$E7` branch threshold and carry state on the ADC path.
- [ ] Await host regression and native 3DS build for the DA58/E091/E098 batch before marking the batch fully validated.


- [x] Translate `sub_DFA4` with the 6502 N-flag loop condition, byte-wrapped X/A registers, and final ADC carry preserved.
- [x] Translate `sub_DFA2` as the original `LDX #$0A` entry into `sub_DFA4`.
- [ ] Await host regression and native 3DS build for the DFA2/DFA4 batch before marking the batch fully validated.


- [x] Translate `sub_D0AB` with the original SBC sign-bit branches and four-object loop.
- [x] Translate `sub_DFB2` with the exact `CMP` N-flag condition and the `tbl_D913` lookup sequence.
- [ ] Await host regression and native 3DS build for the D0AB/DFB2 batch before marking the batch fully validated.

- [x] Translate and test `sub_D000`, `sub_D11F`, `sub_D255`, `sub_D2FF`, `sub_E70B`, `sub_E80D`, `sub_E803`, `sub_E7FF`, `sub_E7FC`, and `sub_E836` at RAM/register level.
- [ ] Confirm host regression and Nintendo 3DS build for this batch before marking fully validated.

- [x] Add state-level models for E16C, E3F4, E4AD, E49B, E2AF, E2BD, E2D0, E2F6, E1F9 and E3AA.
- [ ] Verify host tests and Nintendo 3DS build for this batch.

- [x] Translate `sub_E4C8` and `sub_E52C` at byte level, including wrapped subtraction, loop bounds, packed index scratch byte, and the source-commented `$000B` initialization bug.
- [ ] Verify the new overlap routine tests and Nintendo 3DS build in CI before marking the batch validated.

- [x] Correct `sub_E4AD` to write all 12 sprite Y bytes at four-byte strides, matching its 6502 loop.
- [ ] Re-run host regression and 3DS build after correcting `sub_E4AD`.

- [x] Translate `sub_E456` with reverse object-order traversal, `$70-$8F` cleanup gate, `$F0/$F8` thresholds, and the exact opponent-reset helper.
- [ ] Verify `sub_E456` regression tests and the Nintendo 3DS build in CI.
