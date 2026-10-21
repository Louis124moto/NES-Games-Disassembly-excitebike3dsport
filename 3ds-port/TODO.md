# Excitebike 3DS — roadmap and task tracker

This file is the active project checklist. The original NES disassembly is the reference implementation and must remain unchanged. The port is native C for Nintendo 3DS; 6502 code and NES hardware registers cannot be reused directly as an ARM11/libctru executable.

**Overall status:** not yet a playable faithful port. The application scaffold exists; routine translations are in progress. A translated routine is not automatically tested, build-verified, integrated, or fidelity-verified.

## Use of AI assistance

AI tools have been used to assist with repository analysis, technical documentation, assembly-to-C translation work, and test planning. AI-generated output is treated as a proposal, not as proof of correctness. Verify translations against the original NES instructions and record test execution, build success, gameplay integration, and fidelity comparisons only when supported by reproducible evidence. AI use does not mean a routine or feature is complete or validated; human review remains required.

## Status definitions

- **Analyzed:** source instructions and relevant dependencies inspected; confirmed facts separated from hypotheses.
- **Translated:** native C implementation exists.
- **Tested:** relevant host regression tests have actually run and passed.
- **Build-verified:** the same revision successfully built the 3DS target and produced the expected artifacts.
- **Integrated:** connected through a verified game update/call path.
- **Fidelity-verified:** observable behavior was compared with the original NES reference using documented tests or scenarios.

Do not promote a task to a later state without evidence. Keep the relevant CI run, revision, and known limits beside the task.

## Progress dashboard

| Measure | Current status |
|---|---|
| ASM inventory | 177 labels beginning with `sub_` are listed in `ASM_FUNCTION_INVENTORY.md`; some may be aliases, helpers, or non-standalone labels. |
| Latest documented batch | 20 entry translations added to `nes_routines.c` and `nes_routines.h` on 2026-10-21. |
| Latest batch test wiring | A dedicated regression group is present in `tests/test_nes_routines.c`. |
| Latest batch host test execution | **Pending confirmation.** |
| Latest batch 3DS build | **Pending confirmation.** |
| Latest batch gameplay integration | Not demonstrated. |
| End-to-end game fidelity | Not achieved; no complete scenario comparison is documented. |

A project-wide total for translated/tested/integrated/fidelity-verified routines is **not published yet** because the routine inventory, helper definitions, state models, test history, and gameplay integration are not reconciled into one evidence-backed count. Do not calculate a completion percentage from C function counts or label-name matches.

## Phase 1 — Build foundation

- [x] Create the `3ds-port/` project structure.
- [x] Add a native C entry point and devkitARM Makefile.
- [x] Add GitHub Actions workflows for the 3DS build and host-side regression tests.
- [x] Produce `.3dsx` and `.elf` artifacts in earlier successful CI runs, including run [39493444147](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39493444147).
- [x] Confirm the scaffold launches on real 3DS hardware (user-reported). This confirms launch only, not faithful gameplay.
- [x] Add ignore rules for generated build output.
- [x] Document build requirements and commands.
- [ ] Re-run a clean 3DS build on the latest code revision and record the exact CI run.

## Phase 2 — Source analysis and architecture

- [x] Define module interfaces for game state, input, rendering, bike physics, collision, track, audio, and menus.
- [x] Add a central lifecycle and deterministic module update order.
- [x] Add a debug/status view and bottom-screen console initialization.
- [x] Create `ASM_FUNCTION_INVENTORY.md` for the 177 `sub_*` labels.
- [x] Create `REFERENCE_MAPPING.md` to document routine traces, RAM relationships, and uncertainties.
- [x] Trace the controller sampling/held/press path around `sub_D35B` (CPU `$D35B`, ROM offset `$00136B`).
- [x] Trace selected object-state dispatch and table paths, including `sub_D918/sub_D924`, `sub_D933`, and `sub_E733/sub_E73B`; some state meanings remain unresolved.
- [x] Translate `sub_DA15` as a byte-level helper and run an exhaustive comparison for all 16,777,216 three-byte input combinations in CI run [39495633829](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39495633829).
- [x] Disable unsupported placeholder movement/physics updates in the fidelity-target build.
- [ ] Trace all direct, indirect, and alias writers for `ram_009C_obj,X`, `ram_0370_obj,X`, `ram_0390_obj,X`, `ram_0080_obj,X`, `ram_0094_obj,X`, `ram_00DC_obj,X`, and `ram_00BC_obj,X`.
- [ ] Establish player-slot identity, initialization/reset behavior, and caller order before implementing stateful player behavior.
- [ ] Reconcile `ASM_FUNCTION_INVENTORY.md` with actual C entry points, tests, CI evidence, and gameplay integration.
- [ ] Record a confirmed baseline for the current branch's host tests and 3DS build.

## Phase 3 — Player physics and faithful gameplay core

**Priority:** identify the original player update path first. Do not infer player physics from routines that update other objects.

- [ ] Trace player controller input into the player state/update path.
- [ ] Identify and document the actual acceleration routine, including its callers, input conditions, flags, RAM effects, and frame order.
- [ ] Translate acceleration and speed limits from verified ASM behavior.
- [ ] Translate braking and deceleration.
- [ ] Complete the overheating/cooling model and temperature meter path. `sub_E359` has a documented translation, but its surrounding game-state integration must be verified.
- [ ] Trace and translate jumping, posture, landing, and crash behavior.
- [ ] Trace collisions and recovery/reset behavior.
- [ ] Add branch and boundary regression tests against independent instruction-level references where practical.
- [ ] Compare deterministic movement/heat/jump scenarios with the original NES game and record any differences.
- [ ] Integrate each validated batch only after dependencies and call order are known.

### Player-path source-analysis gate

- [ ] Resolve the player slot from RAM writers, initialization, input handling, and update callers.
- [ ] Document which routines are player-only, opponent-only, or shared.
- [ ] Trace indirect writes and shared scratch variables that affect the player state.
- [ ] Record CPU address, ROM offset when known, entry/exit state, branch conditions, memory effects, and unresolved hypotheses for each priority routine.

## Phase 4 — Track, opponents, race flow, and presentation

- [ ] Reconstruct course selection and original track/course data.
- [ ] Reproduce terrain, ramps, track layout, and track-dependent effects.
- [ ] Reproduce opponent behavior and opponent state transitions.
- [ ] Reproduce race progression, timers, finish conditions, scoring, and results.
- [ ] Reproduce original menus and pause/unpause behavior.
- [ ] Reproduce sprite/OAM command generation and rendering behavior through an explicit 3DS renderer.
- [ ] Reproduce sound effects and music with technically and legally suitable source assets/data.
- [ ] Compare representative complete races and menu flows against the original NES version.

## Phase 5 — Original and New Nintendo 3DS compatibility

- [ ] Verify controls and screen layout on original 3DS hardware.
- [ ] Verify controls and screen layout on New Nintendo 3DS hardware.
- [ ] Check frame pacing, memory use, rendering limits, and audio timing on both targets.
- [ ] Keep enhancements specific to the 3DS optional and separate from the faithful baseline.
- [ ] Verify that optional features do not change baseline gameplay when disabled.

## Phase 6 — Release

- [ ] Perform a clean build from documented prerequisites.
- [ ] Run all host regression tests and the 3DS build on the same commit.
- [ ] Record known limitations and any differences from the NES reference.
- [ ] Produce a versioned `.3dsx` release and release notes.
- [ ] Verify the distributed artifact launches on supported hardware.
- [ ] Do not claim a faithful release until core gameplay, menus, courses, opponents, scoring, and audio have passed documented comparison.

## Verified history and routine-level notes

This section records selected evidence already present in the repository. It does not imply that every routine is integrated into the game.

- [x] Earlier native build and artifact upload succeeded in CI run [39493444147](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39493444147).
- [x] `sub_DA15` exhaustive comparison passed in CI run [39495633829](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39495633829).
- [x] Earlier host regression and 3DS build checks are recorded for batches including `sub_DA7A`, `sub_D310`, `sub_D326`, `sub_D35B`, and multiple object/sprite helpers. Consult `REFERENCE_MAPPING.md` and the linked workflow runs for the exact scope and revision.
- [x] `sub_E359` has a documented temperature-meter/overheat translation and prior host/build CI evidence. Full gameplay-level heat behavior remains subject to integration and scenario comparison.
- [x] A block-level translation of `loc_E96E` from within `sub_E96C` is documented and has exhaustive host regression coverage for the modeled byte inputs. The whole `sub_E96C` path is not thereby complete or integrated.
- [ ] Reconcile older checklist entries that still say tests/builds are pending against the actual latest workflow runs before changing their status.

## Batch log — 2026-10-21: 20 ASM entry translations

**Source/header/test wiring is present; execution and build verification remain pending.** The latest batch has not been counted as test-passing or gameplay-integrated.

- [x] `sub_CFCB` — unsigned object-position comparison and flag-mask branches.
- [x] `sub_CFDB` — conditional upper-nibble clear and zero early return.
- [x] `sub_CFEA` — comparison flags and zero/equal exits.
- [x] `sub_CE58` — object-index equality result.
- [x] `loc_CAFA` — seven `$FE` command bytes with 8-bit X wrap.
- [x] `loc_CD13` — eight-byte sprite-template copy to OAM `$C0-$C7`.
- [x] `loc_D0A6` — indexed object-field store and scratch-A return.
- [x] `loc_D197` — explicit PPU scroll-register write effect.
- [x] `loc_EC47` — fixed four-byte sprite write.
- [x] `loc_CA85` — RAM clear over `$0021-$00FF`, preserving `$0042` and incrementing `$0049` after reset.
- [x] `sub_C339` — vblank polling represented with a finite status trace.
- [x] `sub_D19B` — PPU-status bit-6 polling represented with a finite status trace.
- [x] `sub_D347` — controller strobe writes and two decoded controller sample streams.
- [x] `loc_C4FC` — branch-specific 8-bit arithmetic and shift loop.
- [x] `sub_C92C` — table-index setup and selection path through `sub_C53E/sub_CB06/sub_CB13`.
- [x] `sub_C935` — decimal conversion path and seven-byte buffer copy.
- [x] `sub_ECC9` — four-byte sprite-table comparison.
- [x] `bra_C411` — sprite-X store, attribute clear, and `ram_0046` clear.
- [x] `loc_C3EF` — sprite-Y store.
- [x] `sub_C409` — table-indexed sprite-X and fixed tile/attribute/flag writes.

Next steps for this batch:

- [ ] Run the dedicated host regression group; record the command/workflow, result, and commit.
- [ ] Fix any discrepancies against the original ASM and extend tests for uncovered branches/boundaries.
- [ ] Run the Nintendo 3DS build on the same revision and verify `.3dsx` and `.elf` artifacts.
- [ ] Review PPU/controller polling abstractions to ensure their modeled behavior and timing assumptions are explicit.
- [ ] Trace callers/dependencies and integrate only after validation.
- [ ] Compare relevant behavior with the NES reference before marking fidelity verified.

## Per-routine completion checklist

For each routine or ASM block, record the following in `ASM_FUNCTION_INVENTORY.md` or `REFERENCE_MAPPING.md`:

- [ ] ASM source label, CPU address, and ROM offset where known.
- [ ] Exact entry point and translated instruction/block range.
- [ ] Inputs and outputs, including A/X/Y, carry/negative/zero flags, stack, and scratch state where relevant.
- [ ] RAM reads/writes, aliases, table bytes, indexed bounds, and side effects.
- [ ] NES hardware effects and the chosen 3DS representation.
- [ ] Explicitly labelled hypotheses and unresolved dependencies.
- [ ] Tests for every branch and relevant boundary; independent reference comparison where practical.
- [ ] Host tests executed and passing.
- [ ] 3DS build passed on the same code revision.
- [ ] Gameplay integration verified through real callers/update order.
- [ ] Fidelity evidence recorded against the original NES behavior.

## Project-wide invariants

- Preserve the original disassembly and assets; do not modify original NES source files as part of porting.
- Use consistent technical English for source comments, identifiers, and documentation.
- Do not describe a state model or partial block translation as a complete routine.
- Do not treat a function count or matching label name as a measure of game completion.
- Keep hypotheses explicitly marked until verified from instructions, tables, callers, or repeatable tests.
- Prefer completing the original game's features before optimization.
- Keep optional 3DS enhancements isolated from the faithful baseline.


## Main-menu source trace — 2026-10-21

- [x] Trace the shared input/cursor helper `sub_C7DE` at CPU `$C7DE` (ROM offset `$0007EE`): reads `ram_btn_press`, masks `$3C`, sets `ram_0031_timer=$5D` when a relevant input is present, uses `ram_0049` as a repeat guard, and applies one-based cursor wrap behavior.
- [x] Add a native C model of those helper side effects and host tests for up/down/select, wrapping, repeat suppression, idle reset, and Start's four-frame timer effect.
- [ ] Run the host suite and confirm the new helper tests pass.
- [x] Trace the exact selection branches in `ofs_000_C3BD_08`: choices 1/2 reach state 9, choices 3/4 reach states 12/13, and choice 5 maps selection 5 to 2 before clearing RAM $0021–$00FF.
- [x] Model the choice-5 RAM-clear side effects that affect the menu trace, then model state 0's timer-zero transition into state 2 from `ofs_000_C346_00`.
- [ ] Trace every caller and fully validate state transitions, especially the initialization path after states 9/12/13.
- [ ] Reconstruct the original menu visuals from the original tile data, palette/attribute writes, sprite coordinates, and PPU upload path. Do not invent menu labels or pretend a diagnostic overlay is the original menu.
- [ ] Connect the verified state model to a pixel renderer and compare selection/confirmation timing against the NES reference.
- [ ] Run a clean 3DS build after integration and record evidence for the exact commit.

**Current menu status:** one shared ASM input helper has been translated; the full title/main-menu state machine and authentic visual reconstruction are still incomplete. The new helper is not yet wired to a playable menu.


## Main-menu implementation update — 2026-10-21

- [x] Added a native C model of `sub_C7DE` cursor input and repeat-guard behavior, with host tests wired.
- [x] Added a native C model of the confirmation branches from `ofs_000_C3BD_08`: selection 1/2 increments the state, selection 3/4 maps to states 12/13 and clears the selection, and selection 5 requests the source's RAM-clear path and sets the selection to 2.
- [x] Connected the state-8 input and confirmation model to the native menu trace screen. The current screen displays option indices only; it does not invent menu labels or claim to reproduce original NES artwork.
- [x] Upgraded `tools/nes_chr_to_ppm.py` to write PNG atlases, optionally export per-tile PNGs and JSON pixel metadata, decode iNES/NES 2.0 CHR-ROM, accept explicit verified raw CHR ranges, and apply a supplied NES palette dump.
- [x] Added Python smoke tests for bitplane order, raw input, and PNG signature.
- [ ] Run the Python extractor tests and host C suite; new tests are wired but not yet confirmed passing.
- [ ] Run the host suite; the new tests are wired but not yet confirmed passing.
- [ ] Build the 3DS target on this same revision and verify both `.3dsx` and `.elf`.
- [ ] Trace the exact menu option meanings and title-screen PPU/nametable/palette path before replacing index labels with authentic content.
- [ ] Implement and test destination states 9, 12, and 13 and their complete callers/dependencies; only the state-8 choice branches and state-0 timer-zero path are currently modeled.
- [ ] Verify menu navigation, confirm behavior, sound, timing, and title visuals against the NES reference.
