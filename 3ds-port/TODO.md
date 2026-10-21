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
| Latest batch host test execution | **Passed** in [CI run 39525039178](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39525039178) (revision `2fb6e14`). |
| Latest batch 3DS build | **Passed** in [CI run 39525039042](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39525039042) (revision `2fb6e14`). |
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
- [x] Re-run a clean 3DS build on revision `2fb6e14`; [CI run 39525039042 passed](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39525039042).

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
- [ ] Implement the source-faithful `sub_C2A9` setup dispatcher: preserve the `$0040` alternate dispatch path, `$0030` timer gate, `tbl_C01C_index` mapping, `$0044` byte increment/wrap, indirect pointer reads, and the `$C318` list-transition cleanup. The command-list trace is now documented; native integration is not yet done.
- [ ] Finish the source-derived menu setup sequencer before expanding gameplay: connect the six tested state-2 setup commands (`C551`, `C869`, `C820`, `C5A3`, `C566`, `C875`) to persistent NES RAM state; do not treat isolated helper tests as menu integration.
- [ ] Trace and translate setup command lists for states 3/4/5/6/7/8/12 and destination states 9/10/12, preserving pointer-table order, timer gating, and the `$0044` command index.
- [ ] Translate and test the remaining state-3 setup entries (`C5A7`, `C485`) and their dependencies (`C53A`, `C522`, `C92C`, and any indirect table reads) before enabling state 3 in the native sequencer. `C455`, `C55E`, and `C56A` are now translated with regression vectors, but not yet integrated.
- [ ] Trace the actual title/menu PPU command streams, nametable/attribute writes, palettes, OAM ordering, scroll, and frame timing; connect extracted CHR patterns to their exact tile indices and palette use.
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
- [x] Trace the exact selection branches in `ofs_000_C3BD_08`: selection indices 0/1 reach state 9, indices 3/4 reach states 12/13, and choice 5 maps selection 5 to 2 before clearing RAM $0021–$00FF.
- [x] Model the choice-5 RAM-clear side effects that affect the menu trace, then model state 0's timer-zero transition into state 2 from `ofs_000_C346_00`.
- [x] Translate the state-0 three-entry selector in `ofs_000_C346_00`, including the `ram_0042`-dependent state 1/8 branch and cursor-Y table `tbl_C080_spr_Y`.
- [x] Translate the state-1 five-entry selector in `ofs_000_C395_01`, including LEFT/RIGHT-to-vertical remapping, cursor-X table `tbl_C083_spr_X`, and `sub_C411` side effects.
- [x] Connect the existing `sub_D310` timer routine to the menu's `$20/$30/$31` timer state, preserving the 10-frame divider.
- [ ] Trace every caller and fully validate the startup/display setup and state transitions, especially destination states 9/12/13.
- [ ] Reconstruct the original menu visuals from the original tile data, palette/attribute writes, sprite coordinates, and PPU upload path. Do not invent menu labels or pretend a diagnostic overlay is the original menu.
- [ ] Connect the verified state model to a pixel renderer and compare selection/confirmation timing against the NES reference.
- [ ] Run a clean 3DS build after integration and record evidence for the exact commit.

**Current menu status:** CPU-side handlers cover states 0, 1, and 8, shared input, cursor coordinate tables, left/right remapping, confirmation branches (including the double increment on the state-8 selections 0/1 path), and the choice-5 RAM-clear path. The native port now runs a source-derived subset of `sub_C2A9` setup sequencing for state 0 (`C2BD/C514`), state 1 (`C55E/C3F3/C514`), and state 8 (`C582/C518`), including the `sub_C318` prelude and the timer-30 wait. Timer projection includes `$0030/$0031/$0034`. Other setup lists, especially states 2, 9, 10, 12, and 13, remain incomplete, so selecting a route/mode does not yet lead into a faithful playable game. Original PPU nametable, attribute/palette, tile placement, and sound register behavior are not reconstructed. A new `tools/nes_chr_to_c.py` converts a supplied verified CHR-ROM/raw CHR range into `include/nes_chr_assets.h`; the 3DS top screen can render a diagnostic grayscale tile atlas when generated data is present. No actual game ROM/CHR dump is included, and no host tests or current 3DS build have been run for these latest commits.


## Main-menu implementation update — 2026-10-21

- [x] Added a native C model of `sub_C7DE` cursor input and repeat-guard behavior, with host tests wired.
- [x] Added a native C model of the confirmation branches from `ofs_000_C3BD_08`: selection 1/2 increments the state, selection 3/4 maps to states 12/13 and clears the selection, and selection 5 requests the source's RAM-clear path and sets the selection to 2.
- [x] Connected states 0, 1, and 8 to the native menu trace screen, including source-derived cursor coordinates. The screen displays option indices only; it does not invent menu labels or claim to reproduce original NES artwork.
- [x] Upgraded `tools/nes_chr_to_ppm.py` to write PNG atlases, optionally export per-tile PNGs and JSON pixel metadata, decode iNES/NES 2.0 CHR-ROM, accept explicit verified raw CHR ranges, and apply a supplied NES palette dump.
- [x] Added Python smoke tests for bitplane order, raw input, and PNG signature.
- [ ] Run the Python extractor tests and host C suite; new tests are wired but not yet confirmed passing.
- [ ] Run the host suite; the new tests are wired but not yet confirmed passing.
- [ ] Build the 3DS target on this same revision and verify both `.3dsx` and `.elf`.
- [ ] Trace the exact menu option meanings and title-screen PPU/nametable/palette path before replacing index labels with authentic content.
- [ ] Implement and test destination states 9, 12, and 13 and their complete callers/dependencies; states 0/1/8 are modeled at CPU-control-flow level, but startup/display setup and destination-state behavior remain incomplete.
- [ ] Verify menu navigation, confirm behavior, sound, timing, and title visuals against the NES reference.


## Menu and asset pipeline status — 2026-10-21

- [x] Add a unified Python extraction entry point: `tools/extract_nes_assets.py` invokes the PNG/JSON extractor and the C-header generator using the same verified source/range arguments.
- [x] Document the one-command extraction workflow, generated files, optional tile PNGs, palette input, and native diagnostic-header installation.
- [x] Run Python extractor unit tests and the host C regression suite on `b47057371b09b68451119cf98706654e32243d3e`; [CI run 39523497914 passed](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39523497914).
- [x] Complete the clean 3DS build on `b47057371b09b68451119cf98706654e32243d3e`; [CI run 39523497853 passed](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39523497853).
- [ ] Complete and verify setup-command tables and their side effects for ASM states 2, 9, 10, 12, and 13 before calling the title/menu flow complete.
- [ ] Recover the original option text/graphics mapping from verified source data; do not infer labels from option indices.
- [ ] Reconstruct menu nametable/attribute/palette writes, sprite/OAM state, scroll and PPU timing; compare screenshots/input traces with the NES reference.

**Current honest status:** PC extraction tests, runtime iNES decoder tests, host C regression tests, and the 3DS build all pass on revision `2fb6e14`. The runtime extractor is integrated. The native menu remains partial; this commit does not claim a completed faithful menu or playable port. The native menu remains partial; this commit does not claim a completed faithful menu or playable port.

## Built-in extractor and menu PPU setup — 2026-10-21

- [x] Integrate a runtime classic-iNES CHR-ROM reader into the native homebrew; no Python interpreter is required on the 3DS.
- [x] Add an in-homebrew grayscale atlas for up to 960 decoded 2bpp CHR tiles, with Y to open/close, X to reload, and B to return.
- [x] Validate iNES signature, optional trainer, PRG/CHR offsets and file bounds; report unsupported NES 2.0 and CHR-RAM inputs explicitly.
- [x] Translate and host-test the `sub_C318` PPU/OAM prelude effects (`sub_D255`, `sub_D23B/sub_D260`, `sub_D1A9`, `sub_C333`).
- [x] Run and record the 3DS build and host suite for revision `2fb6e14`; [host suite 39525039178](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39525039178) and [3DS build 39525039042](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39525039042) passed.
- [ ] Implement the exact six-command setup list `off_C046_02` (state 2) and its dependencies before treating the main menu-to-game transition as complete.
- [x] Port state 13's shared `ofs_001_C582_08_01` / `ofs_001_C518_08_02` setup list.
- [x] Translate and test the state-13 entry wait (`ofs_000_C41C_0D` sets `$0048=2`, `$0044=1`).
- [x] Translate the state-13 `$FF` branch: `sub_EC3B` copies 256 bytes from `$05E0-$06DF` to `$06E0-$07DF`; `loc_CC6B` increments `$0041` and clamps to state 8 only when `$004B==0`. Host regression covers copy, clamp, no-clamp and 8-bit wrap.\n- [ ] Implement the six-command state-2 setup list and setup lists/handlers for states 9, 10, and 12. These paths include gameplay initialization and must not be stubbed as if they were complete.
- [ ] Reconstruct palette/attribute/nametable tile placement and compare a real screenshot against the NES reference.

**Runtime extractor limitation:** the built-in viewer extracts raw CHR-ROM patterns only; it does not yet save PNG files or reconstruct the original menu. The Python PC-side pipeline still produces PNG/JSON/C-header files.

## State-13 completion branch — 2026-10-21

- [x] `sub_EC3B` translated as a full 256-byte copy from `$05E0-$06DF` to `$06E0-$07DF`.
- [x] `loc_CC6B` translated: increment `$0041`; if `$004B` is zero, clamp states at or above 9 to state 8; otherwise preserve the incremented byte, including 8-bit wrap.
- [x] State-13 handler runs this path when `$0048==$FF`, writes `$40` to `$00FB`, and uses the modeled `$004B` value.
- [ ] Overall menu-to-game path remains incomplete: state 2's six-command setup and destination handlers for states 9, 10, and 12 are not ported.


## Runtime fidelity cleanup — 2026-10-21

- [x] Disable the partial menu trace in the homebrew runtime; do not present numbered placeholder options as the original title menu.
- [x] Disable generic bike physics, collision, track, and audio scaffolding from the active runtime; these modules are not yet a verified translation of Excitebike's ASM.
- [x] Keep only the separately labelled diagnostic CHR viewer available from the runtime while the faithful baseline is incomplete.
- [ ] Port and validate the complete ASM setup lists, destination handlers, PPU writes, authentic title assets/layout, gameplay loop, and sound before enabling any game screen.
- [ ] Run host tests and the 3DS build on the exact cleanup commit; do not infer build success from historical runs.

**Runtime behavior after cleanup:** without opening the diagnostic viewer, the application intentionally displays no invented menu or gameplay. This is a temporary fidelity safeguard, not a completed port.


## State-2 setup list — 2026-10-21

- [x] Translate command 1 `ofs_001_C551_02_01` and test its conditional add.
- [x] Translate command 2 `ofs_001_C869_02_02` and test exact inclusive clear range.
- [x] Translate command 3 `ofs_001_C820_02_03` lasting RAM effects, including scratch bytes and fill ranges.
- [x] Translate commands 4 and 5 (`$0045=$17` and `$0045=$0E`).
- [x] Translate command 6 through RTS at `$C970`: conditional `tbl_C158` path, course-table lookups, numeric conversion, and both PPU command-buffer records; add host regression.
- [ ] Connect the six verified state-2 command handlers to the sequencer only after checking the state-2 preconditions and interaction with states 9/10/12.


## ASM menu port — source audit (2026-10-21)

- [x] Re-check the real setup-list pointer tables `tbl_C01C_index`, `tbl_C02A_lo`, `tbl_C033_hi` and state dispatch `tbl_C000` against `Excitebike/bank_FF.asm`.
- [x] Confirm `off_C046_02` contains six commands: `C551`, `C869`, `C820`, `C5A3`, `C566`, `C875`.
- [ ] Do not classify `off_C046_02` as a pure menu list: `C820` initializes PPU/data buffers and `C875` initializes race objects and proceeds into track-data preparation. Its transitive calls (`sub_D13F`, `sub_C522`, `sub_C53A`, `sub_C935`, `sub_DF7F`, and related data tables) must be ported as part of the real race-start path, not replaced with guessed menu behavior.
- [ ] Port remaining menu transition setup lists `off_C052_03`, `off_C05C_04`, `off_C070_06`, `off_C07A_07`, and `off_C076_08`, preserving each command's exact side effects and wait/timer behavior.
- [ ] Port state handlers `ofs_000_CB2C_03`, `ofs_000_C465_04_07`, `ofs_000_C43D_0C` and their called routines before enabling any interactive native menu.
- [ ] Replace CLI/debug cursor output with the original PPU/CHR-backed screen layout only after nametable, palette, attribute, sprite, and timing paths are verified against the ASM.

**Fidelity rule:** the currently active homebrew runtime remains in diagnostic-only mode. A menu or gameplay screen must not be re-enabled just because its input/state trace exists; source-derived rendering and all required dependencies must be present first.

## Source-trace continuation — `sub_E96C` vertical object-state update (2026-10-21)

Source inspected directly in `Excitebike/bank_FF.asm`, CPU `$E96C-$EA43`, ROM offsets `$00297C-$002A55`. This trace narrows the next gameplay-core task; it is **not** a complete native translation.

- The loop initializes X to 3 and decrements through 0, so all four object slots are visited in descending order.
- At `$E96E-$E97D`, states `$14` and `$15` skip the initial Y update. Other states apply `Y = (Y + ram_00DC_obj[X]) mod 256`; the 6502 carry is discarded.
- At `$E97F-$E989`, the updated Y is compared against four entries `tbl_E53D[3..0]`. An exact match clears `ram_00DC_obj[X]` and bypasses the rest of that slot's vertical-state logic.
- Otherwise `ram_00A4_obj[X]` selects behavior. Zero bypasses the initial state-1 transition; value 1 can transition to 4 when Y is below `$20`. Depending on `ram_0058_obj[X]`, the code clears `ram_00BC_obj[X]`, calls `sub_DD06`, and decrements `ram_00A4_obj[X]`. Other values have additional state-specific handling.
- The normal path clamps low Y values below `$08` through `bra_E9E9`, raises Y to `$07` when `ram_009C_obj[X]` is zero, and otherwise preserves Y unless it is below `$02`, in which case it becomes `$01`. Values from `$08` through `$39` continue; values `$3A` or above are clamped to `$39`.
- At `$E9D2-$E9E1`, when both `ram_009C_obj[X]` and `ram_03E0_obj[X]` are zero, the vertical delta is toggled by the source's conditional `LDA #$FF` / `EOR #$FE` sequence. If the old delta is zero it becomes `$01`; otherwise the stored result is `old_delta XOR $FE`. Preserve this exact byte behavior rather than replacing it with a signed negation.
- After all four slots, `$EA07-$EA43` checks the race-start flag and several shared state fields, then handles player-input-related state. The presence of this tail alone does **not** prove which object slot is the player.

### Dependencies and next actions

- [x] Record the exact loop bounds, threshold-table comparison, Y clamps, and byte-level delta update from the ASM.
- [ ] Inspect the full `tbl_E53D` bytes and trace every writer/caller affecting the tested object arrays.
- [ ] Inspect `sub_DD06` and document its complete register, flag, RAM, and call-order effects before translating this stateful path.
- [ ] Resolve player-slot identity from initialization, direct/indirect RAM writers, controller input, and update callers; do not infer it from slot 0 or from this routine's tail.
- [ ] Extend the existing `nesLocE96EApplyVerticalDelta` helper only when the surrounding state transitions and dependencies can be represented faithfully, then add branch/boundary tests against the ASM.
- [ ] Keep this routine out of gameplay integration until the full path and dependencies have evidence-backed tests.

The existing `nesLocE96EApplyVerticalDelta` implementation covers only the first Y-delta block (`$E96E-$E97D`). Its exhaustive test does not establish equivalence for the rest of `sub_E96C`.

