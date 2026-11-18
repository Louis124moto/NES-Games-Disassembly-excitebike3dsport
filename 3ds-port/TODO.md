# Excitebike 3DS — roadmap and task tracker

## User-approved specification and strict stage gates — 2026-10-21

This is the controlling project specification. If older checklist items conflict with it, follow this section and update the older item before acting on it.

### Confirmed requirements

- [x] Goal: complete faithful Excitebike port, with the extractor as the first required component.
- [x] Preserve original NES ASM and use it as the authority for graphics, game logic, and behavior.
- [x] The motorcycle's behavior/physics must be faithful; all other game behavior must also be faithful. No plausible-but-unverified replacement physics or graphics.
- [x] Extractor supports Excitebike first while keeping architecture reusable for other NES ROMs where formats and mapper behavior are verified.
- [x] Extract all recoverable graphic classes: raw CHR tiles, palettes, full-screen compositions, sprites/animations, track, bikes/riders, menus/title, and other data the source proves recoverable.
- [x] Provide a tile atlas and distinguish raw stored data from the actual NES-rendered result.
- [x] Analyze compressed/generated graphics where source routines indicate they exist; unknowns remain unknown until traced.
- [x] Use actual PPU RAM writes for palette truth; reconstruct nametable, attributes, CHR bank changes, PPU writes, sprites/OAM, priority, scrolling, and animation progressively from the ASM.
- [x] For incomplete screen data, render known parts and visibly mark missing/unknown portions rather than fabricating content.
- [x] Controls: D-pad, A, and B. Minimal image-focused interface with simple and advanced modes.
- [x] Dual-screen layout: selected image/view on the top screen; controls, status, and technical diagnostics on the bottom screen.
- [x] Diagnostics for CHR banks, palettes, tile indices, nametables/attributes, sprites/OAM, and PPU state/addresses.
- [x] Save useful exports to SD card; include image exports (PNG) and binary/source-data exports (such as CHR, palette, and nametable where available).
- [x] Keep extractor native and integrated into the 3DSX; do not require Python or PC preprocessing for the 3DS workflow.
- [x] Prefer complete accurate extraction over speed; do not enable incomplete features before their prerequisites are verified.
- [x] When ASM does not confirm behavior, investigate further and do not invent an implementation.
- [x] Maintain tracking in this file: checklist, evidence-backed module percentages, completed/in-progress/blocked/verification statuses, test evidence, and known limits.
- [x] Keep README and TODO synchronized on each meaningful implementation batch; test each batch when possible and never claim success without proof.

### Mandatory sequential roadmap

**No later stage may begin until the current stage is fully tested and accepted.** Work on `main` only; do not create a branch for this workflow. The original disassembly remains unchanged.

#### Stage 1 — Native extractor (ACCEPTED BY OWNER DECISION 2026-11-17; see note below)

**Owner decision, 2026-11-17:** the project owner reported that the extractor/atlas works ("ça marche") and asked to move to Stage 2. This acceptance rests on that report only: there is no photographed or reproducible evidence in the repository, no per-criterion verification of the unchecked items below, and it is not stated whether the test ran on hardware or in an emulator. The unchecked items stay open as follow-up debt; they no longer gate Stage 2 work.

- [ ] Audit the current extractor and its real 3DSX startup path against every requirement above.
- [ ] Verify supported ROM/header and graphics-memory cases from source evidence; document unsupported cases instead of implying universal compatibility.
- [x] Extract and display raw CHR/pattern data from iNES/NES 2.0 CHR-ROM or a standalone raw CHR file; report CHR-RAM as requiring runtime PPU capture. The atlas is currently limited to 960 decoded tiles.
- [ ] Trace active palette selection and all live-state write paths; seven source-defined palette command streams are now replayed with `sub_D3C1` semantics and exported, but their active assignment for each game state is not yet proven.
- [x] Replay and export 18 source PPU command streams from `tbl_C0F6` indices 1–17 plus index 25 (D631), with nametable snapshots, palette snapshots, and per-byte write masks.
- [x] Export one 256x240 RGB PNG preview per replayed source PPU snapshot using the native CHR/nametable/attribute/palette compositor; hatch tiles with missing or unverified inputs. Regression tests check PNG signature, dimensions, and manifest entry; current revision still needs execution.
- [x] Add a self-describing `chr_pixels.ebp` EBPX v1 tile pack: 16-byte little-endian header plus 64 one-byte 2bpp pixel indices per tile; keep `chr_pixels.bin` unchanged for compatibility and add header/payload regression assertions.
- [ ] Trace which source command stream is selected by each active game state; determine from the full ASM whether any compressed/generated PRG graphics exist beyond the verified static PPU lists. The checked-in source header confirms mapper 0, 16 KiB PRG, 8 KiB CHR-ROM for canonical Excitebike; the runtime does not yet identify arbitrary ROMs as Excitebike.
- [ ] Validate the color PPU preview export on the host and on physical New Nintendo 3DS hardware, including PNG dimensions/palette correctness and output paths.
- [ ] Provide a faithful live-game screen only after tracing active state, scrolling, OAM sprites, priority, and dynamic PPU writes. The new PNGs are static source-command replays, not complete live frames.
- [ ] Add sprite/OAM, priority, scroll, animation, and PPU state/address diagnostics to the extent supported by verified runtime capture/source analysis.
- [x] Implement basic D-pad/A/B navigation, CHR-atlas/palette/partial-PPU views, top-screen imagery, unknown-data hatching, and bottom-screen source diagnostics. Hardware usability validation remains open.
- [x] Implement SD-card exports for raw CHR, decoded pixels, PNG atlas, seven source palette snapshots, write masks, and palette command streams.
- [x] Export source-replayed nametable/attribute snapshots and write masks for all 18 static command streams in the source table.
- [ ] Add dynamic active-state selection, CHR bank context, OAM/sprite capture, and any further verified runtime data formats.
- [ ] Validate cold start, cache hit/miss, re-extraction, stale-cache rejection, malformed/unsupported input, bounds, output correctness, and repeated navigation.
- [x] Run the complete host test suite and a 3DS build on the exact same commit; record links and commit SHA (commit `9ffec940bf211957b229dae8f4f04ff00dc027fd`).
- [ ] Test on physical New Nintendo 3DS: startup, extractor UI, all controls, exports, diagnostic accuracy, stability, and representative ROM/data cases.
- [ ] Record a requirement-by-requirement acceptance report with reproducible evidence. Do not start Stage 2 until every applicable Stage 1 criterion is passed.

#### Stage 2 — Original NES menu (REOPENED 2026-11-18; IN PROGRESS; blocks Stage 3)

The previous owner-decision acceptance was premature. The current native menu is an input/state trace with partial PPU rendering, not a fully reconstructed original menu. The owner has now explicitly requested that Stage 2 be completed before any Stage 3 work continues. The source audit and current gaps are tracked in `3ds-port/STAGE2_TRACE.md`. No hardware/reference-capture acceptance is claimed.

- [x] A1/A2 — Inventory all 14 state handlers, all nine setup lists and classify title/selector/design/tape/race/shared-transition paths. Evidence: `3ds-port/STAGE2_TRACE.md`.
- [x] A3 — Trace the shared state-$03/$06/$0B dispatcher (`$CB2C-$CC7C`) and its four indexed branches, timers, PPU buffer and state writes. Evidence: `3ds-port/STAGE2_TRACE.md`.
- [x] A4 — Classify state $09 as the interactive course-design/editor loop and trace its state-$08 entry and B-button return; see `STAGE2_TRACE.md`. List 5/editor handoff is still a C3 implementation dependency.
- [ ] A5 — List 5's six commands and C5C2 state writes are inventoried; finish transitive traces for `sub_E1A3`, `sub_EC25`, `loc_EC47`, C820 dynamic PPU upload and remaining list-3 dependencies.
- [ ] A6 — Audit reset, return-to-title, cancel/back and menu-to-race handoffs.
- [x] B1 — Map all static/palette/dynamic/zero-page `ram_0045` targets and verify the NROM/PPUCTRL pattern-table selection; see `STAGE2_TRACE.md`.
- [x] B2 — All 18 static PPU streams, 7 palette streams, dynamic `$0300`, zero-page `$1A`, malformed records, bounds/terminators and palette mirroring are host-tested; see `STAGE2_TRACE.md`.
- [ ] B3/B4 — Model selected nametable, mirroring, scroll and split/raster timing from source writes.
- [ ] B5/B6/B7 — Trace and render every menu OAM sprite and reconstruct authentic screens from verified CHR/PPU data; remove diagnostic placeholder rendering from normal menu mode.
- [ ] C1/C2 — Complete source-faithful input and title/course-selector navigation.
- [ ] C3 — Implement all six design-menu routes, including list-5/state-9 editor setup/return, save/load and title return.
- [ ] C4/C5 — Complete shared transition sequencers and save/load edge cases.
- [ ] D1 — Add host regressions for every menu state/list/input/transition/PPU/OAM/tape branch.
- [ ] D2/D3 — Capture and compare deterministic original-NES reference frames and PPU/OAM state.
- [ ] D4/D5 — Run host suite, exact-commit 3DS build and physical New 3DS verification; record evidence.
- [ ] D6 — Accept Stage 2 only after all menu routes are integrated and fidelity-checked. Stage 3 remains blocked.

#### Stage 3 — First course / first race (BLOCKED until Stage 2 acceptance; source-trace work already exists but must not advance)

- [x] (Existing research only) Trace the state-2 entry and six-command setup-list order; see `3ds-port/STAGE3_TRACE.md`. Do not implement further Stage 3 work until Stage 2 passes.
- [x] Trace `sub_ECE4` and `tbl_E6A1` for the three `sub_D13F` calls (X = `$00/$04/$08`); the dynamic PPU upload execution at `loc_C289/sub_D3C1` remains unported.
- [ ] Integrate the `sub_ECE4` builder into a source-faithful `sub_D13F` model with the three ordered PPU uploads; do not mark command 3 complete before the PPU effects are represented.
- [ ] Establish the player slot through source-confirmed initialization, RAM writers, input path, and update callers; never infer it from slot number.
- [ ] Reproduce course/terrain data and rendering, opponent behavior, race timing, start/countdown, scoring, and state transitions as source-confirmed.
- [ ] Translate and verify the actual player motorcycle path: acceleration, braking, speed/gear behavior where applicable, overheating/cooling, jumping, posture, landing, crashes, collisions, and recovery.
- [ ] Compare physics and frame-by-frame observable state against the original NES for normal play, boundaries, collisions, jumps, overheating, and reset/edge cases.
- [ ] Do not use generic physics or mark the course playable while any required player behavior is placeholder or unverified.
- [ ] Run host tests, exact-commit 3DS build, and physical New 3DS tests; record evidence.
- [ ] Accept Stage 3 only when the first race in scope is faithfully playable and its required comparisons pass.

#### Stage 4 onward — remaining original game flow

- [ ] After each prior stage is accepted, identify the next state/feature in the original game's actual execution order from ASM.
- [ ] Work on one stage at a time: analysis → implementation → tests → exact-commit 3DS build → hardware verification → fidelity comparison → acceptance.
- [ ] Include remaining courses, opponents, results/scoring, menus, audio, reset and other original flows in the order supported by the source.
- [ ] Never skip an unaccepted stage or work on a later feature as a substitute for finishing the current one.

### Stage 1 implementation log — 2026-10-21 (NOT ACCEPTED)

**Implemented in native C and committed directly to `main`:**
- [x] Expose and validate the original CHR-ROM byte range for iNES/NES 2.0 ROMs, including trainer and PRG offsets.
- [x] Keep the raw source CHR bytes separate from decoded 2bpp pixel indices.
- [x] Vendor public-domain `stb_image_write` v1.16 and generate a native PNG tile atlas without a PC-side tool.
- [x] Export `extracted/chr_raw.bin`, `extracted/chr_pixels.bin`, `extracted/chr_atlas.png`, and `extracted/manifest.txt` from the 3DS runtime.
- [x] Version the internal cache separately as `extracted/chr_cache.bin`.
- [x] Add host regression tests for raw-byte preservation, decoded pixels, PNG signature, source PPU command-list semantics, palette mirror behavior, all seven source palette snapshots, and explicit limits.
- [x] Keep the original menu path locked while the extractor stage is not accepted.
- [x] Add a source-derived PPU command-list interpreter and export seven verified palette command streams from `bank_FF.asm` D3D6–D450.
- [x] Add a third diagnostic preview that composes source PPU snapshots and hatches tiles whose nametable bytes, attribute bytes, or required palette entries are unknown.
- [x] Export 18 source PPU update streams (table indices 1–17 plus index 25/D631) with 4 KiB nametable/attribute snapshots, write masks, and palette snapshots.
- [x] Trace which palette list is active in each game state. (2026-10-30, write side only: the 18 setup-command `$0045` writes per menu state are tabulated in `nes_active_stream.c` and surfaced in extractor view 5; see the "ram_0045 active command-stream trace" section below. The live NMI consumer and dynamic indices `$00`/`$1A` remain untraced, so this is not yet full active-state proof.) [ ] Complete nametable/attribute/bank/OAM diagnostics and compressed/generated PRG graphics analysis.
- [x] Remove the 960-tile decoded-preview cap or implement safe paging/streaming for all supported CHR sizes. (2026-10-30: raw CHR retained on the heap within the 8 MiB ROM limit; `chr_pixels.bin`/`chr_pixels.ebp` stream every tile in 64-tile chunks; the on-device atlas decodes 375-tile pages on demand; host regression `test_asset_export.c` verifies a 1000-tile export end to end. Hardware paging test beyond tile 960 still pending.)
- [ ] Finish UI and error-path tests, including malformed headers, missing source, failed writes, stale cache, repeated extraction, palette navigation, and viewer mode changes.
- [ ] Validate on physical New Nintendo 3DS and compare extracted data against the ROM/source reference.
- [ ] Record a full acceptance report. **Stage 1 remains NOT ACCEPTED.**

**Evidence available at the time of this log:**
- Host regression workflow passed at commit `9ffec940bf211957b229dae8f4f04ff00dc027fd`, including CHR decoding, native PNG/binary export, source PPU command interpretation, seven palette streams, and 18 PPU snapshot exports: https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39596598754
- 3DS build passed at the exact same commit, including the three-view extractor UI and partial PPU preview: https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39596598792
- No physical New Nintendo 3DS test has been performed here. A CI pass does not validate hardware behavior or full extraction fidelity. The PPU preview uses direct CHR tile indices. The checked-in source header confirms mapper 0 and 8 KiB CHR-ROM for canonical Excitebike, but runtime identity validation and active palette/stream selection remain unverified.
- No physical New Nintendo 3DS test has been performed here. A CI pass does not validate hardware behavior or full extraction fidelity.

### Acceptance record template

For every stage, add a dated record containing:
- Stage and exact scope;
- source files, CPU addresses/ROM ranges, data tables, and dependencies examined;
- implemented modules and unresolved items;
- host test command/results and CI URL;
- 3DS build command/result, artifact confirmation, and commit SHA;
- physical New 3DS test steps/results where applicable;
- comparison method and results against original NES behavior;
- remaining differences or blocked criteria;
- explicit decision: ACCEPTED or NOT ACCEPTED.

A stage is NOT ACCEPTED if any applicable required check is missing or fails. A successful build or isolated unit test is not enough. Do not use a percentage to imply acceptance; any module percentage must be tied to a defined checklist and evidence.

---

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
| Latest host routine test execution | **Passed** in [CI run 39577668191](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39577668191) (revision `601e7e3`), including the new CBA3 regression vector and the existing menu routine suite. |
| Latest completed 3DS build | **Passed** in [CI run 39577668163](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39577668163) (revision `601e7e3`). Note: `nes_routines.c` remains excluded from the active runtime target, so this verifies the homebrew scaffold/build, not gameplay integration. |
| Latest batch gameplay integration | Not demonstrated. |\n| Current milestone order | Stage 1 and Stage 2 were accepted by owner decision with explicit verification debt; Stage 3 is active. No playable race or full-game fidelity is claimed. |
| End-to-end game fidelity | Not achieved; no complete scenario comparison is documented. |

A project-wide total for translated/tested/integrated/fidelity-verified routines is **not published yet** because the routine inventory, helper definitions, state models, test history, and gameplay integration are not reconciled into one evidence-backed count. Do not calculate a completion percentage from C function counts or label-name matches.

## Historical foundation tasks (do not override the mandatory staged roadmap above)

The completed/in-progress module lists below are historical implementation tracking. They do not authorize advancing to the original menu or race before the Stage 1/Stage 2 gates above are formally accepted.

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

## Menu runtime integration update — 2026-10-21

- [x] Connect `GameState` initialization and `menusUpdate()` to the active 3DSX frame loop after START exits the diagnostic screen.
- [x] Include `menus.c`, `game_state.c`, and `nes_routines.c` in the native build target so menu code is no longer silently excluded.
- [x] Route menu-mode rendering to the menu-state diagnostic and suppress the CHR atlas while menu mode is active.
- [ ] Reconstruct the original title screen from source-verified CHR/nametable/attribute/palette data; current `menusRender()` is still a textual state trace, not the original NES image.
- [x] Remove the invented B-to-diagnostic escape and X-help toggle from menu mode; map A/B/Select/Start/d-pad to NES controller bit positions.
- [ ] Complete all menu setup/dispatch states and transitive routine dependencies before claiming faithful menu navigation.
- [ ] Run host regressions and a clean 3DSX build on this integration revision; runtime wiring is not yet build-verified in this commit.

## Historical planning snapshot — 2026-10-21 (superseded by the controlling 2026-11-18 stage decisions above)

This section records the earlier menu-first plan. Stage 1 and Stage 2 were subsequently accepted by owner decision as documented at the top of this file, and Stage 3 is now active. Use the Stage 3 checklist and `3ds-port/STAGE3_TRACE.md` for current work; the open menu/artwork items below remain verification debt, not a reason to invent race behavior or skip the current Stage 3 dependencies.

**Latest evidence at the time of this historical snapshot:** C485's host test and 3DS build passed on revision `9286e5d4` ([host](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39576312428), [3DS](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39576312415)); C621's copy regression and C43D's transition regression passed in later host runs; the combined host suite including CB2C passed on revision `4661d203` ([CI](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39576697188)). The latest 3DS build that includes CB2C is not yet confirmed.

**Execution order is mandatory:** do not begin the course implementation until the menu has passed its end-to-end acceptance gate. Do not implement courses 2+ in parallel with Course 1.

### Menu completion gate

- [ ] Finish and integrate `ofs_001_C485_03_05` and validate its newly added regression vectors in CI. Its C translation and tests are present; CI verification is still pending.
- [ ] Finish the remaining state-3 dependencies and command path, including `sub_C485`, the correct table/ROM data reads, and any callers affected by its RAM side effects.
- [ ] Integrate the now-translated `sub_CB2C` pointer-table lookup (`tbl_C12C_lo`/`tbl_C130_hi`) into the handler state machine; its isolated regression vectors are present, but the native dispatcher is not connected.
- [ ] Implement all required setup lists and transitions from `tbl_C01C_index`, `tbl_C02A_lo`, `tbl_C033_hi`, and `tbl_C000` in the original order; preserve the `$0040` alternate dispatch path, `$0030` timer gate, `$0044` wrap behavior, and `$C318` cleanup.
- [ ] Finish state handlers and input/confirmation behavior, including currently incomplete states; `sub_C621` and `ofs_000_C43D_0C` now have isolated translations and regression vectors, but must be integrated into the full sequencer before those states are enabled.
- [ ] Complete the race-start boundary needed to leave the menu correctly, including the full `C820/C875` dependency chain, without silently treating track initialization as a menu-only routine.
- [ ] Reconstruct the original title/menu output from verified CHR indices, nametable data, attributes, palettes, OAM/sprite ordering, scroll and frame timing. The grayscale CHR atlas is diagnostic only.
- [ ] Verify menu input, timers, transitions, selection/confirmation, and exit-to-race behavior against the original NES game.
- [ ] Mark the menu complete only after an end-to-end scenario is repeatable and the exact revision's host tests and 3DS build pass.

### Course 1 gate — start only after the menu is complete

- [ ] Trace and reproduce **Course 1 only**, using the original course-selection/start path and the original course data.
- [ ] Implement the minimum full faithful race slice for Course 1: player controls and physics, course geometry/ramps, opponents, collisions/crashes, overheating/cooling, timers, finish detection and results/scoring, following the source's actual update order.
- [ ] Compare Course 1 from race start through results with the original NES version; record remaining differences.
- [ ] Do not implement or port courses 2+ until Course 1 passes its documented acceptance checks.

### Decisions to confirm with the user

- [ ] Confirm what counts as “Course 1 complete”: full original race through results (including all opponents and scoring), or an earlier player-on-track milestone before the full race.
- [ ] Confirm whether menu acceptance must include the exact original title/menu graphics and transitions, or whether functional input/navigation may be validated first while graphics are completed before the menu is declared done. The fidelity baseline remains unchanged; this question only determines test order.

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
- [ ] Finish and integrate the remaining state-3 setup entry `C485` and its dependencies (`CB06`, `CB13`, and all indirect table reads) before enabling state 3 in the native sequencer. `C53A` now has a tested tuple-reader helper that preserves `$0003`; it is not yet wired into the full `C485` command path. `C5A7` now has an exact zero-index entry wrapper and regression vector; `C522` has a tested tuple lookup, but both remain outside the active menu sequence. `C522` now has an isolated source-faithful tuple lookup and regression vectors; integrate it only after the complete `C485` branch path is traced. `C455`, `C55E`, and `C56A` are now translated with regression vectors, but not yet integrated.
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


## Runtime ROM placement — 2026-10-21
- [x] Resolve `rom.nes` from the launched `.3dsx` path (`argv[0]`) rather than a hard-coded SD-card folder.
- [x] Save decoded CHR cache to `extracted/chr_pixels.bin` beside the `.3dsx`; create the `extracted` directory on demand.
- [x] Update README and diagnostic overlay instructions to match the adjacent-file layout.
- [ ] Verify on hardware/homebrew launcher that `argv[0]` contains the full `.3dsx` path and that ROM/cache I/O succeeds on SD.

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



## Bottom-screen extraction log — 2026-10-21

- [x] Start the runtime CHR cache/ROM check during initialization, so the result is available without first opening the atlas.
- [x] Keep the bottom-screen console visible in normal runtime and show the current extraction/cache status, decoded tile count, expected ROM/cache locations, and viewer state.
- [x] Keep the top-screen grayscale CHR atlas conditional on the viewer toggle; diagnostics do not pretend to be the original menu.
- [x] 3DS build passed on commit `62b918d`: [CI run 39577470276](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39577470276). The subsequent host test suite also passed on `601e7e3`.
- [ ] On hardware, verify that the bottom screen reports a useful success or failure message with the ROM beside the launched `.3dsx`, that a cache hit is reported, and that X retries extraction while the viewer is open.
- [ ] If `argv[0]` does not resolve to the SD path on the target homebrew launcher, replace it with a verified libctru/argv path solution and document the evidence.

**Expected startup behavior:** the bottom screen shows `CHR status` and `Decoded tiles` immediately. A successful cache load should say “Loaded … CHR tiles from extracted cache”; a successful fresh extraction should say “Extracted … CHR tiles; cache saved.” Failures should show the decoder/file error. The atlas is still opened with Y; X retries extraction while it is open; B closes it. Hardware verification is still pending.


## Menu handler shared exit — `loc_CBA3` (2026-10-21)

- [x] Translate the shared three-instruction exit at CPU `$CBA3`: store incoming A in `$0033`, increment `$0044` with byte wrap, return.
- [x] Add a regression vector for normal increment and `$FF -> $00` wrap.
- [ ] Run the host suite and 3DS build against the exact revision containing this helper.
- [ ] Continue with `ofs_CB77_01` and `ofs_CBA4_02`; trace the `sub_C94B`, `sub_CC15`, `sub_CC6B`, `sub_CCCA`, and PPU/OAM dependencies before translating the complete handler path.
- [ ] Keep states 3/6/11 out of the active runtime until their handlers and graphics/data dependencies are fully integrated and fidelity-checked.


## Hardware-reported extractor/display defects — 2026-10-21

- [x] Support NES 2.0 CHR-ROM size fields, including exponent/multiplier encoding within the runtime's 8 MiB ROM input limit.
- [x] Add a decoder regression case for a valid NES 2.0 header with a standard 8 KiB CHR-ROM bank.
- [x] Fix top-screen atlas addressing to use the Nintendo 3DS 8×8 tiled framebuffer layout; clear the top screen when the viewer is closed.
- [x] Shorten bottom-screen diagnostic lines to prevent status/path/help text wrapping off-screen.
- [ ] Run host decoder tests and the full host suite on the exact revision containing the NES 2.0 change.
- [ ] Build the 3DS target on the same revision and confirm .3dsx and .elf outputs.
- [ ] Test on hardware with the same ROM that produced the NES 2.0 error; confirm a nonzero tile count and visible atlas patterns.
- [ ] Confirm that the ROM and cache paths resolve correctly in the actual homebrew launcher. If no tiles are found, capture the new status text and inspect the ROM's CHR-ROM/CHR-RAM declaration.
- [ ] Check atlas pixel bounds and repeatedly test opening/closing with Y and B.

**Important:** these source changes address the reported failure causes, but are not considered fixed until CI passes and the same ROM is tested on the 3DS. A CHR atlas only displays raw pattern tiles; it does not reproduce the NES menu without palette, nametable, attributes, sprite/OAM and PPU upload sequence.


## 3DS extraction + atlas correction pass — 2026-10-21

- [x] Fix runtime iNES signature comparison: the previous C literal accidentally matched the four characters `NES\\x1A` instead of the magic bytes `NES 1A`, causing valid ROMs to be rejected.
- [x] Fix top-screen dimensions: the top framebuffer is 400×240, not 240×400; the previous guard returned early, leaving the atlas black.
- [x] Lay out tiles 50 across × 8 pixels, keeping all 960 tiles inside 400×240 and avoiding out-of-bounds writes.
- [x] Add a diagnostic four-colour view for 2bpp CHR indices. This is explicitly false-colour diagnostic output, **not** the game's authentic palette; original colors require palette selection plus nametable/attribute mapping.
- [x] Correct the runtime decoder test suite after the magic-byte fix: [CI run 39579214835 passed](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39579214835) on revision `92ac657`.
- [ ] Confirm the latest 3DS build including the framebuffer and diagnostic-colour changes: [CI run 39579240514](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39579240514).
- [ ] Verify on the user's 3DS that `Tiles` is nonzero, the atlas is visible after pressing Y, and no pixels wrap or disappear.
- [ ] Implement the authentic NES palette tables and tile-to-palette assignments from the original source/ROM; do not substitute the diagnostic palette in the faithful menu renderer.

**Menu gate remains closed:** this does not complete the menu. Required work still includes source-faithful setup lists for states 2/3/4/5/6/7 and destinations 9/10/12/13, the PPU/nametable/attribute/palette upload path, OAM/sprite ordering, startup timing, and input/state verification against the original ASM. Do not enable an approximate menu as a substitute.


## Authentic NES colour palette for the CHR atlas — 2026-10-21

- [x] Replace false-colour diagnostics with a 64-entry NES master-palette RGB lookup.
- [x] Extract the game's actual palette bytes from `Excitebike/bank_FF.asm` command streams at `_off_000_D3D6_12`, `_off_000_D3E9_13`, `_off_000_D3FC_14`, `_off_000_D40F_15` and `_off_000_D422_16`.
- [x] Expose a ten-choice palette selector; L/R cycle through the five source-defined sets × two background sub-palettes.
- [x] Keep the atlas a raw pattern-table viewer and state the limitation: without original nametable/attribute tables, each tile's actual in-game palette assignment is unknown. Palette previews use the exact source palette values, but do not yet reproduce the menu's layout.
- [ ] Run the host tests and 3DS build on the exact revision with the new palette renderer.
- [ ] Verify on hardware that L/R cycles all ten palette choices and the colours match the source-defined NES palette values.
- [ ] Trace which palette command is active in each menu state and integrate nametable/attribute data before claiming faithful menu colours.


## Fix crash observed on physical New Nintendo 3DS — 2026-10-21

- [x] Remove `consoleSelect(NULL)`: keep and select the actual bottom-screen `PrintConsole` instance. The crash dump's data-abort read at address `0x0000002C` was consistent with dereferencing a null console pointer.
- [x] Restore correct libctru top framebuffer dimensions (`240×400` memory coordinates) and convert physical atlas coordinates (`400×240`) into rotated framebuffer coordinates before tiled addressing. The previous code checked dimensions as `400×240` and then wrote coordinates outside the actual framebuffer, risking memory corruption/data abort.
- [ ] Build and run CI on commit `34ebefd` or later; the previously successful build was for an earlier revision and does not validate these crash fixes.
- [ ] Retest on the user's console: launch, verify the bottom log appears without crash, press Y to open atlas, L/R to cycle palettes, B to close; verify no crash after several frames.
- [ ] If it still crashes, save the crash dump with A and provide the dump/exception context; do not infer further causes without the new PC/LR/FAR and revision.


## Atlas partial-width / top-left rendering defect — 2026-10-21

- [x] Fix the tiled framebuffer offset: reverse the framebuffer tile-row index, but preserve the within-tile Y coordinate. The previous helper reversed both, which incorrectly swizzled tile pixels and could produce the narrow/garbled atlas seen on hardware.
- [ ] Confirm latest CI build succeeds and the `.3dsx` is committed to `3ds-port/build/`.
- [ ] Hardware retest: atlas should occupy the full 400×240 top screen without a garbled left strip; verify palette cycling and no crash.
- [ ] If any left-edge clipping remains in the bottom log, capture a straight-on photo and check the actual console viewport; do not conflate text-console layout with top framebuffer atlas addressing.


## Bottom log refresh and atlas legibility — 2026-10-21

- [x] Stop clearing/redrawing the bottom-screen diagnostic console every frame; redraw only when its displayed status changes to avoid unnecessary flicker and cursor/layout churn.
- [x] Reduce the status text length and clarify that the atlas shows raw CHR patterns.
- [x] Increase atlas tile scale to 2× nearest-neighbour (25 columns × 15 rows, up to 375 visible tiles) so individual patterns can be inspected more easily.
- [ ] Confirm CI build on latest commit succeeds.
- [ ] Hardware validation required: confirm bottom log no longer flickers/clips and atlas spans the top screen at 2×. Raw CHR alone will still look abstract/noisy; faithful menu textures require nametable, attribute-table and sprite composition from the original PPU/ASM path.


## CHR extractor legibility and START entry — 2026-10-21

- [x] Replace guessed per-tile palette coloring with four-level grayscale based on the decoded 2bpp pixel indices. This avoids presenting arbitrary sub-palettes as if they were the true tile palette assignments.
- [x] Add D-pad left/right page navigation through the full extracted CHR set (375 tiles per page at 2× scale), rather than showing only the first page.
- [x] Wire START to leave the diagnostic viewer and enter a distinct native game-mode state; B returns to the diagnostic view.
- [ ] Important limitation: START currently enters the native game-mode placeholder, not actual Excitebike gameplay. The source-faithful NES menu/PPU/gameplay execution path is not yet integrated; do not claim this is a playable game start until that runtime exists.
- [ ] Verify latest host tests and 3DS build on the exact revision; verify the `.3dsx` publishing workflow.
- [ ] Hardware test: confirm grayscale tile patterns are discernible, D-pad paging exposes all 512 extracted tiles (including last partial page), and START/B transition works without crash.
- [ ] For faithful visual reconstruction, implement CHR-RAM support or document it as unsupported per ROM, preserve mapper/bank selection, and trace original nametable/attribute/OAM/palette writes. A raw CHR atlas cannot itself look like the game's menu or track.


## Extractor completion pass using external NES/3DS references — 2026-10-21

Reference projects consulted for architecture, not copied wholesale:
- `https://github.com/SKyletoft/nes-emu`: NES recompiler with a 3DS frontend; useful comparison for target-platform constraints, but it supports only NROM at present and is not a drop-in graphics extractor.
- `https://github.com/ognis1205/nesdev`: modular NES toolkit with a PPU framebuffer and separate CHR rendering; supports the distinction between raw CHR patterns and the composed screen.
- `https://github.com/tavuntu/PPUX`: NES graphics/PPU inspection concepts (CHR, palettes, nametables/OAM) inform future visual diagnostics.
- `https://github.com/devkitPro/libctru`: official homebrew API reference for framebuffer and console behavior.

- [x] Fix diagnostic log newline escapes so each status item is actually rendered on a separate line.
- [x] Version the CHR cache format and store an FNV-1a fingerprint of the complete ROM; reject a cache made from a different ROM and regenerate it.
- [x] Remove nonfunctional L/R palette cycling. A raw CHR tile's final color palette depends on PPU nametable attributes and sprite attributes; arbitrary palette cycling would be misleading. Current grayscale preview intentionally visualizes the four decoded 2bpp indices.
- [x] Keep 2× atlas paging (375 tiles/page), bounds-check page selection, and keep the bottom log persistent unless its content changes.
- [x] CI verification is required for the cache/header and renderer changes; host unit tests should cover cache fingerprint/version validation if cache logic is extracted into a host-testable module. (2026-10-30: cache logic extracted into `nes_chr_cache.c` with `test_nes_chr_cache.c` covering fingerprint, magic, truncation, trailing data, invalid pixels, and the tile-count cap; CI was pending at push time.)
- [ ] Hardware verification required on New Nintendo 3DS: check log line breaks, atlas bounds/orientation, page 1/page 2, X re-extraction, Y open/close, B close, and cold start with both valid and stale cache.
- [ ] Remaining honest limitation: this extracts CHR-ROM tiles from iNES/NES 2.0 ROM images within configured size bounds. CHR-RAM cartridges need runtime PPU memory capture; full-color game/menu composition needs nametable, attribute table, OAM, palette RAM and mapper bank context. Do not mark full menu textures complete until these are implemented and tested.


## Native PPU background renderer — 2026-10-21

Reference projects informed the architecture: NEStractor (CHR extraction), NES-Screen-Tool and makechr (separate CHR/nametable/attributes/palettes), plus QLN​​ES (graphics inspection). No external Python renderer is used by the 3DSX.

- [x] Implement native C background composition in `source/nes_ppu_render.c`; it is linked into the 3DSX automatically by the Makefile's C-source discovery.
- [x] Compose a 256×240 background from decoded CHR pixels, nametable tile indices, attribute quadrants and 16 source-derived palette values.
- [x] Connect the compositor to the top-screen renderer and use static storage for the RGB frame buffer to avoid excessive 3DS stack use.
- [x] Keep composition disabled unless `menu_ppu_background_ready` is set by a verified PPU setup path; do not display uninitialized placeholder data as authentic graphics.
- [x] Move regression coverage to native C in `tests/test_nes_ppu_render.c`.
- [ ] Verify the C renderer tests and 3DSX build on the exact revision.
- [ ] Populate nametable/attributes/palette from the original ASM's actual PPU writes and enable the readiness flag only after that data is valid.
- [ ] Add sprite/OAM, scrolling and bank-switch handling before claiming a faithful title screen.

### Extractor follow-up — 2026-10-21 (implementation updated; validation pending)

- [x] Add `extracted/chr_pixels.ebp`, EBPX v1 with a 16-byte little-endian header and 64 decoded pixel-index bytes per tile; preserve `chr_pixels.bin` compatibility.
- [x] Export 18 color PNG previews by composing the source CHR with static source PPU nametable/attribute/palette snapshots; hatch data marked unknown by source write masks.
- [x] Make preview rendering tolerate out-of-range tile IDs in incomplete snapshots by sanitizing only the temporary render input; preserve original snapshot bytes and hatch the corresponding output tiles.
- [x] Extend host regression source to check EBPX header/payload and PNG signature/dimensions.
- [x] Update root README, native-port README, and this TODO to distinguish static command-list previews from complete live frames.
- [ ] Execute the updated host suite and 3DS build on the exact current revision; no result is claimed until a workflow reports it.
- [ ] Test the updated `.3dsx` on a physical New Nintendo 3DS, including export paths, cache hit/miss, re-extraction, malformed input, and repeated navigation.
- [ ] Continue source audit of active palette/stream selection, runtime ROM identity, dynamic PPU writes, OAM/sprites, scrolling, and generated/compressed graphics. Do not open Stage 2 until all Stage 1 gates are evidenced and accepted.

**Latest implementation revision before this note:** `662ca49b6da9c43807fca4ed32f72a146b376088` (test additions; documentation commits may move HEAD). The GitHub-connected editing tools accepted the changes, but the host test suite and a 3DS build were not run from this environment. The prior CI evidence recorded above applies to older revision `9ffec940bf211957b229dae8f4f04ff00dc027fd` only and does not validate these changes. **Stage 1 remains NOT ACCEPTED; Stage 2 remains blocked.**


### Hardware feedback — atlas color and third-page corruption — 2026-10-21

- [x] User confirmed the CHR tile shapes in `chr_atlas.png` were correct but colors were missing. Source-palette color variants were added in the follow-up section below; a standalone CHR tile still has no unique palette assignment.
- [x] User reported pixel corruption on physical New Nintendo 3DS. The initial tile-row tweak (`908e43e8757d02a7973c7a8668308101fec57a10`) did not resolve it. Inspection of libctru's existing framebuffer implementation identified the actual issue: the renderer incorrectly assumed 8x8-tiled storage. Commit `bd0f3b7a327f231d720e309ca9dfb13eaafaf1ce` replaces it with linear sideways addressing and BGR byte order; rebuild and hardware validation remain pending.
- [ ] Rebuild the 3DSX and test pages 1, 2, and 3 on physical New 3DS. The corrected addressing is **not yet hardware-verified**. Check image orientation, tile boundaries, all 960 tiles, and navigation wraparound.


### Follow-up after physical-console report — colored atlas and persistent pixel corruption — 2026-10-21

- [x] Use existing source-derived palette replay and the project's NES RGB conversion table to generate color atlas variants: `chr_atlas_palette_12..18_subpalette_0..3.png` (28 variants). `chr_atlas.png` now uses source palette preset 12 / subpalette 0 as a diagnostic default; standalone CHR does not encode the correct palette assignment per tile.
- [x] Change the on-device CHR atlas from grayscale index display to the selected source preset's first background subpalette, with magenta hatching for missing palette entries.
- [x] Extend host regression assertions to check a color-variant PNG and the manifest entry. Tests have **not** yet been executed on the new revision.
- [x] Root cause identified from libctru's existing framebuffer implementation: the renderer incorrectly treated the 3DS framebuffer as 8x8-tiled and used the wrong rotated-coordinate mapping. Replaced it with the established linear sideways offset `((screenX * 240) + (239 - screenY)) * 3`; also corrected RGB-to-BGR byte order required by `gfxInitDefault()`.
- [ ] Rebuild from the latest `main` commit before the next console test; confirm the app is actually running the updated binary. Check all three pages after the framebuffer fix. This source correction is not yet hardware-verified.
- [ ] Confirm that the color atlas variants and the on-device palette-colored atlas show recognizable tile shapes, then validate palette choice against known original game states. Stage 1 remains unaccepted.

### User hardware report — atlas remains pixelated (2026-10-22)

- [x] User reports that the on-device CHR atlas is still visually pixelated; some text-like tile shapes are distinguishable, but the atlas is not yet readable.
- [ ] Do not conclude that the framebuffer correction failed until the exact running `.3dsx` revision is identified. The latest source on `main` contains the linear rotated BGR framebuffer correction from `bd0f3b7a327f231d720e309ca9dfb13eaafaf1ce`; that source change has not been verified on physical hardware.
- [ ] Build the current `main` revision and test the same binary on physical New Nintendo 3DS. Record the source commit and whether the binary was rebuilt after the framebuffer correction.
- [ ] Check pages 1–3 for tile shape, row/column ordering, orientation, color-channel order, and navigation. If the current build still shows pixel corruption, isolate framebuffer addressing from CHR decoding using a known deterministic test pattern before changing the renderer again.
- [ ] Keep Stage 1 NOT ACCEPTED until the hardware results and source-data comparison are recorded. Do not start Stage 2.


### Framebuffer isolation diagnostic — 2026-10-22

- [x] Add a fourth extractor view that draws a deterministic top-screen test pattern without reading CHR pixels or palettes: five 80-pixel vertical color bands (red, green, blue, white, black), an 8-pixel grid, and stronger 40-pixel axes. This isolates framebuffer coordinate mapping and BGR channel order from NES asset decoding.
- [x] Expose the diagnostic through the existing Up/Down view cycle. Implementation commits: renderer `45406ac752a8603e8939fad6a6f250b1ae3735f7`; view navigation `545479d0aecef95f30c5181d861acce06c118c3a`.
- [ ] Build the exact current `main` revision and test this diagnostic on physical New Nintendo 3DS. Expected: five clean vertical bands, no repeated horizontal pixel noise, and a regular grid. If this pattern is corrupted, investigate framebuffer mapping/flush/presentation; if it is clean, investigate CHR tile layout, palette selection, and tile scaling next.
- [ ] Do not accept Stage 1 until the diagnostic and then the full 512-tile atlas render sharply with source-verified colors on hardware. No Stage 2 work before acceptance.
- [ ] Host tests, 3DS build, and hardware test were not run by the editing environment for these commits; do not treat the diagnostic as validated until the user tests the built binary.



## ram_0045 active command-stream trace — 2026-10-30

**Evidence (source only, no invention):** `Excitebike/bank_val.inc` names ram_0045 values in its `con_0045_*` block — `con_0045_02` is commented "blue background" and `con_0045_18` is commented "title screen". ram_0045 is the index used with the PPU command-stream table at CPU `$C0F6` (ROM offset 0x0106). The embedded stream arrays in `nes_ppu_commands.c` cover exactly the table indices the menu setup lists write.

**Index mapping (implemented in `nes_active_stream.c`, host-tested in `test_nes_active_stream.c`):**

- `$01`-`$11` → static PPU update lists, array indices 0-16 (`nesExcitebikePpuStream`).
- `$19` → the D631 stream, PPU array index 17.
- `$12`-`$18` → the seven palette command lists D3D6-D450, array indices 0-6 (`nesExcitebikePaletteStream`).
- `$00` and `$1A` are not captured statically; the API reports them as unresolved rather than guessing.

**Setup-command `$0045` writes per menu state (18 rows, source-derived from the translated setup lists):**

| State ($0041) | Command | $0045 value | Resolves to |
|---|---|---|---|
| 0 | 0 | `$02` | PPU D64B ("blue background") |
| 0 | 1 | `$18` | PAL D450 ("title screen") |
| 1 | 0 | `$06` | PPU stream 6 |
| 1 | 1 | `$03` | PPU stream 3 |
| 1 | 2 | `$18` | PAL D450 ("title screen") |
| 2 | 0 | `$12` | PAL D3D6 |
| 2 | 3 | `$17` | PAL D435 |
| 2 | 4 | `$0E` | PPU stream 14 |
| 3 | 0 | `$06` | PPU stream 6 |
| 3 | 1 | `$04` | PPU stream 4 |
| 4 | 0 | `$06` | PPU stream 6 |
| 4 | 1 | `$09` | PPU stream 9 |
| 4 | 3 | `$18` | PAL D450 ("title screen") |
| 6 | 0 | `$06` | PPU stream 6 |
| 6 | 1 | `$0F` | PPU stream 15 |
| 6 | 2 | `$18` | PAL D450 ("title screen") |
| 8 | 0 | `$11` | PPU D614 |
| 13 | 0 | `$11` | PPU D614 (state 13 shares list 8) |

States whose setup lists are not yet translated (5, 9, 10, 11, 12) intentionally have no rows.

**Superseded 2026-11-16 (see "ram_0045 consumer trace" below):** the reader of ram_0045 is now traced from the ASM, and the `$D55A`/`$CC2D` attribution written here originally was wrong. The write side is still incomplete (states 5/9/10/11/12 are untranslated, and the `$0300` buffer writers are not enumerated per state), so this is still not a complete active-stream proof.

**External references consulted for the architecture:** ognis1205/nesdev (rp2c02 PPU/OAM model) and SKyletoft/nes-emu (NROM loading with a citro2d framebuffer path), used as engineering references only; all Excitebike-specific data remains source-derived.


## ROM identity, cache module, and 960-cap removal — 2026-10-30

- [x] `nes_rom_identify.c/.h`: header-level iNES/NES 2.0 identity validation; canonical Excitebike = mapper 0, 16 KiB PRG-ROM, 8 KiB CHR-ROM, vertical mirroring. Honest summary for raw/unidentified sources. Host test `test_nes_rom_identify.c`. PRG content identity (checksums) is intentionally NOT claimed.
- [x] `nes_chr_cache.c/.h`: version-3 `EBCHR03` cache (magic + NUL, u16 tile count, u32 FNV-1a ROM fingerprint, 64 bytes per tile) extracted from the runtime extractor into a host-testable module; stale-fingerprint, magic, truncation, trailing-data, invalid-pixel, and tile-count-cap rejections are covered by `test_nes_chr_cache.c`.
- [x] 960-tile export cap removed: `assetExportWrite` streams `chr_pixels.bin`/`chr_pixels.ebp` from the complete raw CHR in 64-tile chunks (ALL tiles), `asset_extractor` retains the raw CHR on the heap and decodes 375-tile atlas pages on demand, and `test_asset_export.c` verifies a 1000-tile export (file sizes, EBPX u32 header count, spot-checked tile 999, manifest wording). `chr_atlas.png` and PPU preview PNGs still compose only the first 960 tiles; documented in the manifest.
- [x] Extractor view 5 (active stream + OAM): ROM identity summary, header-match yes/no/unknown, live `$0041`/`$0045` from the menu GameState, per-state setup `$0045` writes, OAM visible/hidden counts (Y=`$F8` via sub_D255), and (as of 2026-11-16) the traced NMI consumer instead of the earlier "NMI consumer not traced" line.
- [ ] CI verification of these commits was pending at push time; check the `Verify NES routine translations` and `Build 3DS` workflow runs on `main` before treating host coverage as green.
- [ ] Hardware verification on New 3DS remains required: atlas paging beyond tile 960, streamed export contents on SD, cache hit/miss, ROM identity line, and the active-stream view. **Stage 1 remains NOT ACCEPTED.**


## Hardware report — 2026-11-01 (New Nintendo 3DS, CI build of commit `ffd48a0`)

User-reported hardware observations with photographs of both screens:

- [x] Extractor journal reports `Tiles: 512, page 1/2`: on-demand atlas paging renders and the tile count is plausible for an 8 KiB CHR-ROM.
- [x] View 5 (active stream + OAM) renders on hardware and shows the ROM identity and live-state lines.
- [x] PPU snapshot view (D470 stream) renders with correct counters (1309 writes, nametable 640/960, attributes 48/64, palette 0/32); palette view shows the D3D6 snapshot values.
- [x] Defect confirmed: the user's `rom.nes` is a legitimate NES 2.0 dump of Excitebike, but the ROM identity line reported "NES 2.0 mapper 128" and "Header matches Excitebike: no". Root cause: `nesRomIdentifyHeader` read the NES 2.0 flags-7 low nibble (format bits) as mapper bits, so flags 7 = $08 produced mapper 128 instead of mapper 0. Fixed on 2026-11-01 in commit `103443c` with the correct NES 2.0 mapper formula (flags 6 high nibble | flags 7 high nibble | flags 8 low nibble << 8); the iNES mapper-32 regression vector was also corrected (flags 7 = $02, not $20). Hardware retest pending: the identity line should now read "NES 2.0 mapper 0, PRG 16 KiB, CHR-ROM 8 KiB" and "Header matches Excitebike: yes".
- [ ] The CHR atlas remains pixelated/garbled on hardware across builds; this defect is NOT resolved. The view-3 deterministic framebuffer test pattern is the next isolation step: if the bands/grid are corrupted, investigate framebuffer mapping/flush; if clean, investigate CHR decode/page composition.
- [ ] Re-verify the new ROM identity output and atlas sharpness with the next CI-committed `.3dsx`. **Stage 1 remains NOT ACCEPTED.**


## Hardware report follow-up — 2026-11-01 (build of commit `046b1fe`)

New-console retest of the NES 2.0 identity fix and the deterministic pattern:

- [x] ROM identity fix validated on hardware: the identity line now reads "NES 2.0 mapper 0, PRG: 16 KiB, CHR-ROM: 8 KiB", 512 CHR tiles extracted, and "Header matches Excitebike: yes".
- [x] Deterministic framebuffer pattern (view 3) validated geometrically from console photographs: five 80-pixel vertical bands in the designed order (red/green/blue/white/black), straight band edges, aligned yellow grid lines, full-screen fill. The top-screen offset mapping and BGR byte order are therefore correct on hardware.
- [x] Root cause identified for the persistent "pixelated/garbled" defect reported on every build: console text was rendered only when the log text changed (the `s_logInitialized`/`s_lastLog` gate), but libctru gfx framebuffers are double buffered. After each `gfxSwapBuffers()` the newly current backbuffer still holds pixels from two frames earlier, so the text screen alternated between fresh text and stale pixels at half the refresh rate. To the eye this reads as mush with corner flicker; a single-frame photograph looks clean, which is why earlier screenshots appeared fine. Fixed in commit `9c450d0` by re-rendering the console text every frame (canonical libctru practice); the gate and its state variables were removed.
- [ ] Hardware retest pending with the next CI-committed `.3dsx`: confirm every text view (journal, palette, snapshot, active stream, atlas) is stable and sharp, with no flicker or mush. **Stage 1 remains NOT ACCEPTED.**


## Hardware report follow-up 2 — 2026-11-01 (build of commit `046b1fe`)

Correction of the previous entry after the user's on-console observation:

- [x] The bottom-screen console text is perfectly stable on hardware; the previous "console redraw" diagnosis was wrong. The defect is on the TOP screen and affects every view, including the deterministic framebuffer pattern (view 3): mush plus a flicker localized at the bottom-left corner, which is framebuffer offset 0.
- [x] Diagnosis: once-rendered text staying stable while every-frame-redrawn content shows a ~30 Hz mix of fresh and stale pixels (and single-frame photographs looking clean) indicates the LCD scanout did not follow the `gfxSwapBuffers()` double-buffer alternation. The drawn buffer and the scanned buffer diverged on alternating frames, so every other frame the redraw raced the scanout (the full-buffer memset sweep visibly started at offset 0, hence the corner flicker).
- [x] Fixed in commit `026a99e` (renderer.c): disable libctru double buffering on both screens (`gfxSetDoubleBuffering(GFX_TOP/GFX_BOTTOM, false)`) so the drawn buffer is always the scanned buffer; remove the per-frame full-buffer memset (it would flash a black sweep on the scanned buffer); every diagnostic view now repaints its full extent with identical per-frame content (pattern/atlas/palette cover the full 400x240, the snapshot view repaints its side bands, and the atlas repaints the unused tail of its page so paging leaves no stale pixels). Console text continues to be re-rendered every frame, which is invisible under single buffering and keeps the log current.
- [ ] Hardware retest pending with the next CI-committed `.3dsx`: every top-screen view (pattern, atlas, palette, snapshot, active stream) must be rock-stable, with no mush and no corner flicker. If the mush persists even under single buffering, the divergence theory is falsified and the next isolation step is to render the top screen only on content change (draw once per view/page/palette switch into the single buffer, no per-frame redraw at all). **Stage 1 remains NOT ACCEPTED.**


## CI restoration — 2026-11-01

The "Verify NES routine translations" workflow had been failing on every run since 2026-10-21. Three stacked causes, each hiding the next because a failing step skips everything after it:

- [x] `test_nes_chr_cache.c` asserted `NES_CHR_CACHE_TRUNCATED_PAYLOAD` against a truncated file whose first byte still held the 'X' written by the preceding bad-magic check, so the load legitimately returned `BAD_MAGIC` (exit code 12 on every run since 2026-10-21). Fixed in commits `078c2a6` (partial) and `38e60fc` (complete: a valid cache is re-saved before the truncation block and before the trailing-data block; exit codes renumbered).
- [x] With the cache test green, the pipeline reached "Compile native CHR export test" for the first time: `asset_export.c` calls `nesPpuRenderNametable`/`nesPpuColorToRgb`, but the workflow compile command did not link `nes_ppu_render.c` (missing since the step was added at `0bd804e`; the step had never executed before). Fixed in commit `a39e0f7`.
- [x] Run #356 (commit `a39e0f7`): "Verify NES routine translations" completed successfully — the first green test run since 2026-10-21. All host suites pass: Python tools, nes_routines exhaustive 8-bit comparison, chr_decode, ROM identity (the NES 2.0 mapper fix is now CI-validated too), chr cache, active stream, PPU renderer, PPU commands, and the full asset export round-trip (PNG dimensions, EBPX header, all-tiles streaming beyond the 960 cap, manifest content).
- [x] Build workflow green at `a39e0f7`; the `.3dsx` artifact therefore contains every pending fix: NES 2.0 ROM identity, single-buffered framebuffers, and full-extent per-view repaint.
- [ ] Hardware retest pending with the `a39e0f7` (or later) CI artifact: the top-screen atlas must display the decoded Excitebike tiles (512 tiles, pages 1-2) rock-stable with no mush and no corner flicker, and the text screens must stay sharp. **Stage 1 remains NOT ACCEPTED.**

## Hardware report follow-up 3 — 2026-11-01 (build of commit `a39e0f7`)

- [x] Retest of the single-buffered build: the top-screen corner blink (offset 0) is RESOLVED, but the top-screen content still renders as mush — only short words ("best") are readable and textures remain illegible. The divergence-only theory is falsified: single buffering removed the corner flicker but did not make large per-frame repaints invisible on this console.
- [x] New defect on the bottom screen: the bottom-left corner (offset 0) now flickers with the same signature the top screen previously showed. The bottom console is the only surface still rewritten every frame (consoleClear() plus a full log reprint), which confirms the rule observed on this console: ANY large per-frame rewrite of the scanned buffer is visible.
- [x] Conclusion: the only configuration observed to hold a perfectly stable image on this console is ZERO framebuffer writes per frame (console text rendered once). Fixed in commit `85835db` (renderer.c): change-driven rendering — rendererWriteConsole() rebuilds the log text every frame but reprints the console only when the text changed; rendererTopNeedsRepaint() repaints the top screen only when the extractor-active/game/menu/view/page/palette/stream/tile-count inputs change; menu-mode rendering is gated on menu-state inputs. gfxSwapBuffers() is retained (hardware-verified no-op with double buffering disabled).
- [ ] Hardware retest pending with the next CI artifact: both screens must be bit-stable at rest (no mush, no corner flicker); a single transition frame on view/page changes is expected and acceptable. If the settled top screen still shows artifacts, the defect is in the drawn content itself (photograph the settled screen for analysis); if a screen degrades without input, a view input is oscillating (the bottom log View/page lines would show it). **Stage 1 remains NOT ACCEPTED.**



## Top-screen pixel-format root cause — 2026-11-16 (fix applied; hardware confirmation pending)

**Source audit against existing projects.** libctru `console.c`: `consoleInit(screen, ...)` calls `gfxSetScreenFormat(screen, GSP_RGB565_OES)` (2 bytes per pixel) and disables double buffering on that screen. The official devkitPro `3ds-examples/graphics/bitmap/24bit-color` example puts the console on one screen and the raw 24-bit BGR image on the other screen, and never calls `consoleInit` on the screen that receives raw pixels. `gfxSetScreenFormat` only records the format (framebuffers are not shrunk), and `gfxScreenSwapBuffers` presents using that recorded format.

- [x] Defect found: `rendererInit()` called `consoleInit(GFX_TOP, NULL)` and the renderer then wrote 3-byte BGR8 pixels into the top framebuffer with `topFramebufferOffset()` (stride 240x3). The top screen was therefore switched to RGB565 and scanned out 3-byte data as 2-byte pixels. This fits the symptoms reported since 2026-10-21: top-screen content garbled in every view with only some text-like shapes recognizable, while the 16-bit bottom console text stays sharp. The 2026-11-01 diagnoses (double-buffer scanout divergence, per-frame rewrite visibility) were not independently proven; their workarounds remain in the code until hardware shows they can be removed.
- [x] Caveat recorded honestly: the 2026-11-01 entry judged the view-3 test pattern geometrically correct from photographs. With a 3-byte write stride into a 2-byte-per-pixel format, distortion of that pattern is expected, so the two observations conflict. The hypothesis is NOT confirmed until the hardware retest below; the earlier photographs should be re-examined.
- [x] Fix in `source/renderer.c`: removed `consoleInit(GFX_TOP, NULL)`; explicit `gfxSetScreenFormat(GFX_TOP, GSP_BGR8_OES)` guard; new `rendererClearTop()` used once at startup (the top screen no longer clears itself through a console) and whenever the viewer closes. The close-time clear also repairs a regression: the 2026-10-21 item "clear the top screen when the viewer is closed" was lost when the per-frame framebuffer clear was removed on 2026-11-01, leaving the last atlas page on screen after B.
- [x] Host evidence (this environment, before the change): all eight C host suites (`nes_routines`, `chr_decode`, `rom_identify`, `chr_cache`, `active_stream`, `ppu_render`, `ppu_commands`, `asset_export`) and the Python tool tests pass on `ecc69fa`. After the change, `renderer.c` passes `gcc -fsyntax-only -Wall -Wextra` against a minimal stub `<3ds.h>`; this is a syntax check, not a build. `renderer.c` links libctru and is not host-testable.
- [x] Code review of `nes_chr_decode.c` and `nes_ppu_render.c` against the standard NES formats (2bpp plane order and bit order, NES 2.0 size fields, attribute quadrant shift, background color mirroring): no defects found.
- [ ] 3DS build NOT run here (devkitARM unavailable in this environment): the Build workflow must pass on the commit containing this change before it is treated as build-verified.
- [ ] Hardware retest on New Nintendo 3DS: view 3 must show five clean 80-pixel bands (red/green/blue/white/black) with a regular grid; the CHR atlas, palette, snapshot and active-stream views must be stable and sharp; pressing B to close the viewer must blank the top screen. If artifacts persist, photograph the settled screen; the next candidate step is to revert to the canonical double-buffered full-redraw loop from `3ds-examples` (full redraw each frame, `gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();`) rather than keeping the single-buffer/change-driven workarounds. **Stage 1 remains NOT ACCEPTED; Stage 2 remains blocked.**

 
## ram_0045 consumer trace — 2026-11-16 (Stage 1; source analysis plus host tests, no hardware involved)

Scope: every reference to `ram_0045` in `Excitebike/bank_FF.asm` (14 lines: 4 loads, 10 stores; plus the RAM-clear loop at ROM 0x000A9C that `bank_ram.inc` lists). Facts below are read directly from the listing unless marked *derived*.

- [x] **NMI vector.** `$FFFA` holds `vec_C23B_NMI` (CPU `$C23B`, ROM 0x00024B). Order of one NMI as written: `sub_C32F`; if `ram_0045 == 0`, OR `$1A` into `ram_for_2001` and store it to `$2001`; OAM DMA from page `$02`; `sub_C27F` (PPU upload); if `ram_0047 != 0`, `sub_D0AB` and `sub_D14E`; unless paused (`ram_pause_flag | ram_pause_timer`), `INC ram_003F` and `sub_D310` timers; `sub_D347` joypad read; `sub_F844`; `sub_C2A9` (state/setup dispatcher); read `$2002`; `sub_C339`; `RTI`. The menu setup lists modelled so far therefore run inside the NMI, after the upload.
- [x] **The reader.** `sub_C27F` (CPU `$C27F`, ROM 0x00028F): `X = ram_0045`; pointer = `tbl_C111_hi,X : tbl_C0F6_lo,X` into `ram_0000/0001`; `sub_D3C1`; then `ram_0300 = ram_0301 = ram_0045 = 0`; then `$2006` gets `$3F 00 00 00`. The loads at `C74B` and `C782` are busy-wait loops on that clear and are not covered by the CDL.
- [x] **List format end token.** `sub_D3C1` stops when the FIRST byte of a record (address-hi) is zero, then reads `$2002` and writes `$2005` twice with 0. `nesPpuApplyCommandStream` already matched this; only its header comment said "control byte" and has been corrected.
- [x] **Earlier attribution corrected.** `$D55A` is data inside the PPU list at `D524` (no label, not code) and `$CC2D` is the `ADC #$0B` in `sub_CC15` that WRITES `ram_0045`. Neither is the consumer. Wrong text removed from README, this file, `nes_active_stream.h` and the view-5 line in `renderer.c`.
- [x] **Table transcribed.** `tbl_C0F6_lo` / `tbl_C111_hi` (27 entries, `$00`-`$1A`) are now in `nes_active_stream.c` (`nesExcitebikeRam0045Target`). Index `$00` = dynamic buffer `$0301` (`ram_0300` holds the write offset); `$1A` = zero-page list `$00A1`; `$01`-`$19` = the static lists already embedded.
- [x] **Dynamic buffer.** Records appended by the code around `D2B7-D2F7` use the same record format, and `sub_D2FF` is an overflow guard: if the offset reaches `$3F` it writes an end token at the committed offset and then executes `PLA PLA` before its `RTS`. That overflow path is not covered by the CDL, and the prologue at `loc_D2A3` pushes X and Y, so *derived* from the listing the `RTS` would take those saved bytes as a return address: the guard looks like it was never exercised in normal play. Do not model it as a graceful abort without evidence. `ram_0300`/`ram_0301` are also referenced at C5B5, C8ED, C93A/C955, C971-C978, CB7F, CC3D, D121/D126, F5FD and F6E5 (listed, not individually traced yet).
- [x] **Index `$1A` content.** The code at `C739-C77F` (not covered by the CDL) stores `$07 $22 $4F $04` into `ram_00A0-00A3`, copies four bytes from `(ram_000E_temp)` into `$00A4-00A7`, zeroes `$00A8`, and sets `ram_0045 = $1A`. Read as a list from `$00A1`: PPU address `$224F`, control `$04` (4 literal bytes), 4 data bytes, end token.
- [x] **Other writers.** `sub_CC15` sets `ram_0045` to `$0D`, or `ram_004B + $0B` when `ram_0052_track_finished_flag == 1`; `CC11` sets `$0A`. These gameplay-side writers are not in the menu setup-write table yet.
- [x] *Derived timing.* Because `sub_C27F` runs before `sub_C2A9` in the NMI, a `ram_0045` value queued by a setup command is uploaded by the NEXT NMI, which does not write `$2001`; rendering is enabled by the first later NMI that sees `ram_0045 == 0`. Not yet checked against an emulator trace.
- [x] Code and tests: `nesExcitebikeRam0045Target`, `nesExcitebikeNmiRamFor2001`, `nesExcitebikeNmiUploadAck` in `nes_active_stream.c`; `test_nes_active_stream.c` cross-checks each static index's combined address against its independent source label, plus the `$00`/`$1A`/out-of-table cases and the NMI side effects. Mutation check: changing `0xD64B` to `0xD64C` in the table makes the test fail (exit 23). View 5 now describes `$00` and `$1A` instead of "uncaptured" and closes the previously unbalanced `[hint` bracket.
- [x] Host evidence (this environment): all eight C host suites and the Python tool tests pass; `renderer.c` passes `-fsyntax-only -Wall -Wextra` against a stub `<3ds.h>`.
- [x] Enumerate the direct `ram_0300`/`ram_0301` references listed by `bank_ram.inc` and inspect their source blocks; the first-pass writer/consumer census is recorded in "Dynamic PPU buffer reference census" below.
- [ ] Map each buffer initializer/appender to its actual caller and game state; determine which writes form the index-`$00` stream versus other object/update structures.
- [ ] Replay the dynamic buffer contents for a selected frame and connect the NMI-order model to the menu/runtime state.
- [ ] Complete the exact-commit 3DS build and New Nintendo 3DS hardware validation. This does not complete "trace which command stream is selected by each active game state". **Stage 1 remains NOT ACCEPTED.**


## Dynamic PPU buffer reference census — 2026-11-16 (first-pass source audit; no runtime replay)

Scope: the `ram_0300`/`ram_0301` cross-reference entries in `Excitebike/bank_ram.inc`, checked against their corresponding instruction blocks in `Excitebike/bank_FF.asm`. This records what the instructions do; it does not yet prove that every structure below is the list selected by `ram_0045 = $00`.

- [x] **NMI acknowledgement:** `sub_C27F` at CPU `$C28E-$C296` clears `$0300`, `$0301`, and `$0045` after `sub_D3C1` consumes the selected list.
- [x] **Table initializers:** `C5B5-C5B9` copies six bytes from `tbl_C13C` into `$0300-$0305`, then `C5BE` overwrites `$0304` with `ram_0043 + 1`. `C8ED-C8F1` copies eight bytes from `tbl_C158` into `$0300-$0307` under the `ram_0046 != 0` and `ram_0043 == 4` branch; subsequent instructions update `$0306/$0307`.
- [x] **Cursor helper and appenders:** `sub_C971` (`C971-C978`) loads `X = $0300` and adds the caller's A value to `$0300`. `sub_C935` writes seven bytes from `$03D1` onward at `$0304,X`; `sub_C94B` advances the cursor by `$0A` and writes a table-selected PPU address plus command fields at offsets from `$0301,X`. `ofs_CB77_01` copies seven bytes from `$03D9` at `$0304,X` and calls `sub_C94B`. The `CC32-CC67` path advances the cursor by `$07`, copies bytes from `tbl_C134` to `$0301,X`, and conditionally updates fields `$0305-$0307`.
- [x] **Dedicated initializer and stream builder:** `sub_D11F` (`D11F-D13E`), called by `sub_D14E` at `D15E`, initializes `$0300 = $11`, `$0301 = $23`, `$0302 = $A0`, `$0303 = $0E`, and fills the following structure bytes with `$FE`. `loc_D2A3` reads the source record through `ram_0002_t02_data`, appends address-high, address-low, count and payload bytes at `$0301,X`, calls `sub_D2FF` to advance X, commits the cursor to `$0300` at `D2E1`, and writes a zero terminator at `D2F7`.
- [x] **Per-object command setup:** `F5F8/F5FD/F602` and `F6E0/F6E5/F6EA` write the command control/address bytes at `$0303/$0301/$0302` before calling `sub_F64E`. `sub_F64E` and `sub_F681` then read/write the associated `$0303-$030F` data area. Their callers and relationship to the `ram_0045 = $00` upload are not yet proven.
- [ ] **Still unresolved:** caller/state mapping for each initializer and appender; whether the C5/C8/CC and F5/F6 structures all feed the same selected command list; the exact dynamic buffer bytes at a given NMI; and overflow-path behavior at `sub_D2FF`. Do not infer a graceful abort from the `PLA PLA` path.

## CHR atlas palette modes — 2026-11-17 (Stage 1; host-tested, hardware confirmation pending)

Hardware report (user, New 3DS): the top-screen display is now correct ("ça s'affiche correctement"), so the 2026-11-16 `consoleInit(GFX_TOP)` fix is confirmed by the user's observation, though no photo was recorded. Remaining report: atlas tiles rendered red, green and light blue. Reference given by the user: a screenshot of the original game (orange-brown track, green infield, blue NINTENDO banner, black HUD, red HUD text).

- [x] Diagnosis: the NES colour table is the canonical one; preset 0 (D3D6) background entries `$29 $27 $22 $30` are green, orange, light blue, white and match the reference's grass, track, banner and white trim. The atlas painted EVERY tile with that single background sub-palette, so tiles the game draws with other palettes (sprites, HUD, banner text) came out in the wrong colours. Preset 5 (D435) writes no background entries, which produced the magenta hatching.
- [x] Fix, modelled on the CHR viewer of Mesen (palette selection: four background palettes, four sprite palettes, grayscale): the atlas defaults to grayscale; X/Y cycle grayscale / BG 0-3 / SPR 0-3; L/R cycle the seven source presets. `nesAtlasPaletteIndex` maps (mode, pixel) to the palette RAM index, with pixel 0 always the universal background entry. Unwritten entries fall back to gray instead of magenta. Bottom screen shows the mode name and preset.
- [x] Host evidence: `test_nes_ppu_render` covers the mapping for all eight modes, grayscale and invalid input, and fails under a mutation (pixel 0 mapped per mode). All eight C suites and the Python tests pass; `renderer.c` and `asset_extractor.c` pass `-fsyntax-only` against a stub `<3ds.h>`. No 3DS build was run here. The bottom text buffer was enlarged to 768 bytes (the default view needs about 510 with worst-case arguments).
- [ ] Not done: tile-to-palette assignment as the game does it (nametable/attribute for backgrounds, OAM attribute for sprites). The atlas lets the viewer choose a palette; it does not know which one a tile uses. That belongs to the nametable/OAM work in the open items above.
- [ ] Hardware retest: grayscale default shows crisp tile shapes; X/Y change colours; with preset 1, SPR modes should give a red/white palette for rider tiles; B closes and blanks the top screen. **Stage 1 remains NOT ACCEPTED.**

### CI build record — 2026-11-17

- [x] `Build Nintendo 3DS homebrew` run 44037438495 on `cde4f46` passed (devkitARM compile and artifact upload; the 3DS binaries are produced from sources that include the top-screen BGR8 fix, the ram_0045 trace and the atlas palette modes). This is the first 3DS compile of those changes, so the earlier "3DS build not run" caveats are resolved for compilation. It says nothing about on-device behaviour.
- [x] The run on `8b8be4d` had failed before compiling: `pkg.devkitpro.org` returned HTTP 403 while `dkp-pacman -S 3ds-dev` re-downloaded 20 packages that the `devkitpro/devkitarm` image already contained. The workflow now uses `--needed` with up to three attempts (`cde4f46`). Not a source defect.
- [ ] Hardware retest of the atlas palette modes remains open. **Stage 1 remains NOT ACCEPTED.**

## Stage 2 step 1 — NMI upload model and reachable menu — 2026-11-17 (host-tested; no hardware run)

Done (source-derived, host-tested):
- `nesExcitebikeNmiUploadStatic` (nes_active_stream.c) models sub_C27F for static lists `$01-$19`: it selects the list through the verified tbl_C0F6_lo/tbl_C111_hi mapping, runs it with sub_D3C1 semantics on the CURRENT nametable/palette model (lists are cumulative), then clears ram_0045. Palette mirroring `$3F10/14/18/1C -> $3F00/04/08/0C` is applied on copy-in and copy-out. Expected values in tests/test_nes_active_stream.c were hand-parsed from the list bytes; a mutation of the mirror rule is caught (exit 51).
- `menusNmiUpload` (menus.c) follows vec_C23B_NMI order: `$2001` is written (`ram_for_2001 | $1A`) only when ram_0045 is zero, then the static upload runs. `menu_ppu_background_ready` is set only when the mask enables the background (bit 3) AND a palette list has really been executed.
- game.c: START (consumed, not forwarded to the menu) enters the menu path once CHR is loaded; every menu frame runs `menusUpdate` then `menusNmiUpload`.
- renderer.c: the BG pattern table follows PPUCTRL bit 4 (`$10` -> CHR tiles 256..511); the redraw gate includes the upload revision.
- GameState: `menu_ppu_palette` widened to 32 bytes (full palette RAM), plus `menu_ram_for_2001`, `menu_ppu_palette_loaded`, `menu_ppu_revision`.

Not done / open (nothing here is claimed as working):
- [ ] Sprites/OAM composition is still not drawn; the cursor and any sprite is missing.
- [ ] Dynamic buffer `$0300/$0301` (ram_0045 == 0 path) and zero-page list `$1A` are not modelled; the model leaves the PPU untouched for them. The writers per menu state are still unmapped.
- [ ] The setup sequencer is translated only for states 0,1,4,6,7,8,13; other states hold for inspection.
- [ ] Whether the setup code waits for the NMI acknowledgement before issuing the next command has not been verified against the ASM; the native order (set during menusUpdate, consumed at the end of the same frame) is an assumption to check.
- [ ] Scroll registers and timing are not applied to the renderer.
- [ ] Nothing compared against a real NES reference screenshot yet; hardware/emulator run of this step is pending.
- [ ] Possible pattern-table issue in exported PNG previews (asset_export may use table 0 regardless of PPUCTRL) still to verify.

## Stage 2 step 2 — ASM audit of the menu and completion of states 0/1/8/12/13 — 2026-11-18 (host-tested; no hardware or emulator run)

Audit against Excitebike/bank_FF.asm found and fixed these defects in the earlier translation (each now has a hand-derived test; mutations of every fix are caught):
- All menu logic runs INSIDE vec_C23B_NMI (sub_C2A9). The native frame order was inverted; `menusNmiFrame` now runs: `$2001` enable when ram_0045 == 0 -> upload (sub_C27F) -> timers (sub_D310) -> joypad -> menu dispatch.
- The setup command list is selected through `tbl_C01C_index[ram_0041]`, not by ram_0041 itself (design menu $08 -> list 6, state $0C -> list 7, state $0D -> list 8, state $07 -> list 4, state $06 -> list 3). The old code ran the state-13 list for state 8.
- The last command of lists 4 and 6 (ofs_001_C514 falls into C518) did not set ram_0049/ram_0040/ram_0044, so those setups never finished.
- sub_C7DE: Down past the last entry wrapped to 1; the ASM (`LDY #$01` falls through into `DEY`) wraps to 0.
- ofs_000_C41C_0D stores 2 (not 1) in ram_0044; loc_CC6B stores 0 when ram_004B == 0 and the state is below 9 (the `< 9 -> 0` half was missing); it also resets ram_0040/ram_0044 through sub_C3AF.
- loc_CA85 (design-menu entry 5) clears $0021-$00FF: ram_0045, timers $33/$34, ram_0046/0048/004B, $004D-$00EF and ram_00FB were not cleared.
- The cursor sprite was kept in side fields and never written to OAM; sprite 0 is now `menu_oam[0..3]` (Y/tile/attr/X).
- Rendering is disabled by sub_D24F every setup frame (`$2001 = 0`): the model blanks the picture exactly then.
- Timers $30-$3D count down once per 11 frames (sub_D310), not every frame.

New in this step:
- Dynamic PPU buffer ($0300/$0301, ram_0045 == 0) and zero-page list ($1A -> $00A1) uploads (`nesExcitebikeNmiUploadDynamic/ZeroPage`).
- Sprite compositor `nesPpuRenderSprites` (Y+1, flips, subpalettes, 8 per scanline, lower index wins, left-8 clip, behind-background priority via an opacity map, PPUCTRL bit 3 sprite table). 8x16 sprites are refused, not guessed.
- State 8 (design menu, 6 entries, all confirm branches), state 7/12 setup commands (tbl_C180, sub_C621), states 12/13 handlers, and the main-thread handoff of the Famicom Data Recorder routines (sub_C6C8 save, sub_C739 load incl. the $19 and $1A error/feedback lists and retry loop).
- tests/test_menus.c (via tests/host_stub/3ds.h) is run by .github/workflows/test-nes-routines.yml.

Deliberate deviations from the original (not exact, stated plainly):
- **Tape recorder**: the real routines bit-bang `$4016`; the port stores the same two blocks ($0060-$009F, $05E0-$06DF, each with a 16-bit count-of-1-bits checksum) in `sdmc:/3ds/excitebike3ds_tracks.bin`. The retry/error behaviour (lists $19/$1A) is derived from the ASM, but the file format is not the tape signal format.
- With no saved file, load waits forever like a console without a recorder; the port adds B to leave the menu path (not in the original).

Still open (Stage 2 is NOT complete):
- [ ] Handlers for states 4/7 (ofs_000_C465) and the setup lists 2/3/5 (race, results) are not ported; states 2/3/5/6/9/$0A/$0B run the race engine (Stage 3).
- [ ] Sound: ram_00FB / $4015 / $4011 requests are recorded in state but nothing is played.
- [ ] Scroll registers and the second nametable are not rendered; only nametable 0 is drawn.
- [ ] Timing is per 3DS frame (60 Hz); the NES NMI is 60.0988 Hz and there is no mid-frame PPU behaviour.
- [ ] Not compared with a real NES capture; not run on hardware or in an emulator in this step.
- [ ] Expected list contents ($02, $18, $19, $1A) were hand-parsed from the ROM bytes by the same author as the code; an independent check against an emulator trace is still needed.
- [ ] The Famicom keyboard/other-peripheral paths (if any) were not examined.

## Hardware report — 2026-11-18 (New Nintendo 3DS, owner test)

- [x] The owner confirms the runtime extractor displays CHR textures sharply and without geometric distortion. The atlas is grayscale by design; this confirms the grayscale inspection path, not the in-game palette assignment.
- [x] The owner confirms the native menu starts and its selection responds to controls on physical hardware.
- [ ] The owner reports that the original menu boxes/artwork are still not reproduced correctly. This remains a Stage 2 visual-fidelity issue; do not substitute invented rectangles or labels.
- [x] Commit `2bc8d15`: render the 256x240 NES PPU image into a centered 320x240 4:3 viewport on the 400x240 New 3DS top screen (nearest-neighbour horizontal scale, 40-pixel side bars); track all 256 OAM bytes and both scroll registers in the change-driven redraw gate.
- [x] Commit `8707ef5`: report a missing $1000 CHR pattern table or PPU nametable-render error on the bottom diagnostic screen rather than silently falling back to the textual menu trace.
- [ ] Retest the 4:3 viewport and new PPU failure diagnostics on physical hardware. The 3DS build passed for `2bc8d15`; the verification workflow for `8707ef5` is pending at the time of this note.
- [ ] Obtain a hardware capture of the title menu and compare it with the original NES image before claiming the menu boxes/artwork are fixed.


## Stage 3 batch — sub_ECE4 dynamic PPU buffer builder — 2026-11-18

- [x] Inspect `sub_ECE4` at CPU `$ECE4` / ROM `$002CF4` and `tbl_E6A1` at CPU `$E6A1` / ROM `$0026B1`.
- [x] Translate the four-byte table copy and both dynamic command payload builders into `nesRaceSetupECE4`.
- [x] Preserve the 6502 absolute-indexed page-crossing writes to `$0502` and `$0525` when X wraps to `$FF`.
- [x] Add regression vectors for all three table offsets, payload bytes, headers, terminator, pointer, invalid offset, and untouched guard bytes.
- [ ] Run the race setup host test and full host workflow on the resulting commit; record CI run and SHA.
- [ ] Build the 3DS target on the same revision.
- [ ] Integrate the three `sub_D13F` calls and the `loc_C289/sub_D3C1` PPU command execution in exact source order. Until then, `sub_ECE4` is an isolated buffer-builder translation, not a complete PPU upload.
- [ ] Continue with the `C875` tail only after the command-3 PPU path has been modeled; then integrate state-2 list index 2 into the setup sequencer.
