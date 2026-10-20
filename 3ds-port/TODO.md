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


- [x] Translate `sub_E456` with reverse object-order traversal, `$70-$8F` cleanup gate, `$F0/$F8` thresholds, and the exact opponent-reset helper.
- [x] Verify `sub_E456` regression tests and the Nintendo 3DS build in CI (host suite run `39504720602` and 3DS build run `39504884005` succeeded).


- [x] Re-check the literal `CPY #$0C` loop in `sub_E4AD`; the correct model writes exactly three OAM Y bytes at offsets 0, 4, and 8.
- [x] Re-run host regression after restoring the literal E4AD loop bound (run `39504720602` succeeded).

- [x] Model `sub_E26A`'s gate and sprite-command parameters from the literal instructions and `$D90B` three-byte table.
- [x] Validate `sub_E26A` host test and 3DS build (host run `39504865406` and 3DS build run `39504884005` succeeded).


## Whole-project status snapshot (2026-10-20)

- The native app architecture and repeatable `.3dsx`/`.elf` CI build work, and the user previously confirmed the scaffold launches on 3DS.
- The current source scan detects 51 C routine/helper definitions against 177 original `sub_*` labels. Inventory status is not a completion percentage: helpers and state models count as definitions, and several translations are not integrated into gameplay. A translated label is not automatically fidelity-verified.
- The port is still **not a playable faithful Excitebike implementation**. Bike physics, course data/rendering, opponent/race flow, menus, and audio need verified translation and integration. Do not claim a global completion percentage based only on the routine count.

- [x] Translate `sub_E359` temperature arithmetic and OAM output using the literal `$D8F7/$D8FB` table bytes.
- [x] Validate `sub_E359` host regression and 3DS build in CI (host run `39505218389`; 3DS build run `39505237364`).

- [x] Complete `sub_E2F6` through its OAM writes, including tile swap, attribute bit, wrapped coordinates, and conditional hide of the second wheel.
- [x] Validate the full `sub_E2F6` host regression and 3DS build (host run `39505508682`; 3DS build run `39505373018`).

- [x] Translate `sub_EA44`'s four-slot sprite dispatch, thresholds, table lookup, and shared-SBC path; pass inherited zero-page scratch explicitly.
- [x] Validate `sub_EA44` host regression and 3DS build (host run `39505657843`; 3DS build run `39505675306`).

- [x] Replace the partial `sub_E1F9` model with the complete branch/return logic, including `$0036,X` timer semantics and `$039F` side effect.
- [x] Validate expanded `sub_E1F9` regression and 3DS build (host run `39505926103`; 3DS build run `39506181543`).

- [x] Combine the `$E2AF-$E2F3` countdown state path into one model with explicit low/high borrow, finish blink, and render-command output.
- [x] Validate the combined countdown regression and 3DS build (host run `39506181462`; 3DS build run `39506181543`).

- [x] Translate `sub_E1A3`'s `$D892/$D8A9` lookups, `$16` special pointer/attribute adjustment, and conditional overlay-tile loop.
- [x] Validate `sub_E1A3` host regression and 3DS build (host run `39506455129`; 3DS build run `39506426121`).


## Batch 2026-10-20 — 10 priority routines/entries reviewed

- [x] Add or complete `sub_E4C8`/`sub_E52C`, `sub_E456`, `sub_E26A`, `sub_E359`, `sub_E2F6`, `sub_EA44`, the `$E2AF` countdown path, and `sub_E1A3` command setup.
- [x] Replace the partial `sub_E1F9` approximation with its full branch-level logic and correct the literal `$E4AD` loop to its three writes at Y=`$00/$04/$08`.
- [x] Host regression run `39506455129` and Nintendo 3DS build run `39506455082` both passed at code/test revision `b0d89905`; subsequent commits in this section change documentation only.


## Audit pass — 2026-10-20

- [x] Re-scan `nes_routines.c` and reconcile inventory rows that were still marked `Pending` despite having a matching C definition.
- [x] Keep source translation status separate from fidelity/test status; compilation alone does not prove instruction-level equivalence.
- [ ] Compare every existing routine against its exact ASM control flow, flags, indexed RAM effects, and dependencies; correct discrepancies before claiming faithful coverage.
- [ ] Translate remaining routines in dependency order, prioritizing long gameplay/state routines before audio/data decoders where practical.

- [x] Remove contradictory historical TODO entries that incorrectly said `sub_E4AD` writes twelve OAM bytes; the literal ASM loop bound and regression test establish three writes.


### Audit snapshot details

- ASM inventory: 177 original `sub_*` entries.
- C source: 51 function/helper definitions detected by source scan.
- Inventory after reconciliation: 134 rows remain marked pending, 17 have a matching C definition but need their status/test coverage reconciled, and 26 have other statuses. These are tracking categories, not a percentage of playable game completion.
- Largest unported gameplay/state candidates by ASM instruction count include `sub_EB17` (~135 instructions), `sub_E96C` (~106), `sub_D260` (~88), `sub_DFD5` (~85), `sub_D018` (~76), and `sub_D1C7` (~73). Exact counts are a rough source scan, not an estimate of implementation effort.
- The first full audit pass found at least one real documentation/implementation mistake in the E4AD sprite-hide loop: the literal `CPY #$0C` loop writes three bytes (offsets 0, 4, 8), not twelve. The C code and regression expectation were corrected, and the subsequent host test run passed.
- Next work should translate the larger object/race-state routines with explicit RAM state and tests for branch conditions and byte wraparound; then integrate the helpers into the native frame loop. Audio/PPU-heavy routines are later because they depend on memory-mapped hardware behavior.


## Fidelity gate update — 2026-10-20

- [x] Disable the native placeholder speed/temperature increments and arbitrary track-position clamp; they were not justified by the NES source.
- [x] Inspect and document `sub_E733` / `sub_E73B` directly from `bank_FF.asm`; distinguish the object/track-event path from player acceleration.
- [ ] Trace the actual player-slot movement, acceleration, braking, heat, jump/landing, and crash state path from callers and all relevant RAM writers before enabling native physics.
- [ ] Translate the traced state path with explicit 8-bit/carry/branch behavior and independent regression tests before reconnecting it to `gameUpdate`.


## Verified routine fragment — 2026-10-21

- [x] Translate the literal `$E96E-$E97D` block from `sub_E96C` into `nesLocE96EApplyVerticalDelta` in `nes_routines.c`, which is part of the native 3DS codebase. Preserve the `$14/$15` state skips, four-slot traversal, and 8-bit Y-plus-delta wrap; do not describe this fragment as a complete `sub_E96C` port.
- [x] Add an independent exhaustive host reference test for all 16,777,216 `(state, Y, delta)` inputs for slot 0, while checking the remaining slots' expected effects.
- [ ] Confirm the new exhaustive host test and native 3DS build pass in GitHub Actions before calling this fragment validated.
- [ ] Translate and test the rest of `sub_E96C`, including the exact `sub_DD06` side effects, `$E53D` threshold loop, and `$EA07-$EA43` controller branch, before integrating this fragment into live gameplay.


## Batch of exact RAM helpers — 2026-10-21

- [x] Translate `sub_E84F` ($E84F): store A to RAM `$00BC,X` and `$00E4,X`.
- [x] Translate `sub_E86F` ($E86F): store `$00E4,X - A` to `$00BC,X`, modulo 256.
- [x] Translate `sub_E893` ($E893): store A to `$00D4,X` and `$00B4,X`; return `$00C8,X - $00D0,X`.
- [x] Translate `sub_E73B` ($E73B): add `$0060,X` to X-low with 8-bit wrap and no high-byte carry.
- [x] Translate `sub_E733` ($E733): execute the X-low update for slots 3, 2, 1; preserve the original `BNE` loop's omission of slot 0.
- [x] Add host regression tests for these five routines. Test results are pending CI; no gameplay integration is claimed.
- [x] Host regression workflow passed after correcting the initial compile failure; the native 3DS build and output checks passed on the corresponding code commit.
- [ ] Continue tracing and translating additional self-contained ASM routines; only count routines as verified after the source mapping, tests, and CI pass.


## Second exact-helper batch — 2026-10-21

- [x] Translate `sub_E7FC` and `sub_E7FF` data-index loads.
- [x] Translate `sub_E80D`'s six-entry pointer low/high tables exactly (`$E54E-$E559`).
- [x] Translate `sub_E803` indexed RAM read with 6502 8-bit Y/page-crossing semantics represented over RAM `$0400-$07FF`.
- [x] Translate the decision/write behavior of dispatch entry `ofs_002_E818_07`, including `CMP #$07` followed by `BPL` sign-flag semantics.
- [x] Add regression tests for these helpers; CI validation remains pending.
- [ ] Do not mark either batch verified until the newest host test workflow and 3DS build both pass.


## Batch of 21 exact RAM-level routines / dispatch entries — 2026-10-21

- [x] `sub_E84F`, `sub_E86F`, `sub_E893`, `sub_E73B`, `sub_E733` translated into the native C routine module.
- [x] `sub_E7FC`, `sub_E7FF`, `sub_E80D` pointer-byte table, and `sub_E803` indexed RAM read translated; existing pointer resolver kept under its original non-conflicting name.
- [x] Dispatch entries `ofs_002_E818_07`, `ofs_002_E845_04`, `ofs_002_E854_05`, `ofs_002_E85D_02`, `ofs_002_E86A_03`, and `ofs_002_E879_09` translated.
- [x] Dispatch entries `ofs_002_E8BF_0E`, `ofs_002_E8C6_0F`, `ofs_002_E8E3_01`, `ofs_002_E8F5_14`, `ofs_002_E8FA_10`, and `ofs_002_E963_00` translated.
- [x] Added host regression coverage for the new helpers, including byte-wrap arithmetic, slot isolation, lookup-table values, branch flags, and state-write behavior.
- [x] Host regression workflow passed on code commit `1e290e35`; the native 3DS build, output-location check, and artifact upload also passed on that same code commit. Final workflow cleanup was still running at last poll.
- [ ] These helpers are not yet wired into live gameplay; full caller state and call-graph effects must be traced before integration.


## Batch — 2026-10-21: command-stream and PPU-write fragments

- [x] Translate the pre-dispatch setup of `sub_D0F4`: copy seven bytes from RAM `$03D9-$03DF` into `$03D1-$03D7`, then apply the literal writes `$03D0=$17`, `$03D2=$FB`, `$03D5=$FB`; expose source pointer `$2356` and data pointer `$03D0`.
- [x] Model `sub_D1A9` as an explicit status-read count and the two `$2005` writes of zero.
- [x] Model `sub_D1B3` as explicit `$2000` control and `$2005` scroll write effects in source order; no direct PPU access is performed by the portable helper.
- [ ] Validate these new tests in host CI and compile the native 3DS target before marking this batch verified.
- [ ] These are portable source fragments only; the generic command decoder at `loc_D2A3` and PPU backend are not completed by this batch.


## Batch — 2026-10-21: 20 exact bank_FF.asm helpers

- [x] Added 20 source-level translations: `sub_C333`, `sub_C3AF`, `sub_C3B1`, `sub_C621`, `sub_C68A`, `sub_C971`, `sub_CA1B`, `sub_CB13`, `sub_CFE3`, `sub_CF8B`, `sub_C62D`, `sub_D24F`, `sub_ECBF`, `sub_F676`, `sub_F6FB`, `sub_F826`, `sub_F82E`, `sub_F839`, `sub_F871`, and `sub_F885`.
- [x] Added batch regression coverage including all 256 accumulator values for the carry/wrap helpers and full 256-byte page-copy verification.
- [x] Refined `sub_F871`/`sub_F885` to model the `Y`-indexed ROM reads, the zero-byte early exit, and 16-bit address wrap when `Y=$FF`.
- [ ] Host tests and native 3DS compilation must pass before this batch is marked validated.
- [ ] PPU/APU/controller register accesses are modeled as explicit output effects; these helpers do not directly emulate NES hardware or connect all effects to live gameplay.


## Batch — 2026-10-21: 20 additional ASM-to-C translations

- [x] Added 20 new translations: `sub_C1FD`, `sub_C229`, `sub_C32F`, `sub_C53A`, `sub_C53E`, `sub_C716`, `sub_C726`, `sub_CB06`, `sub_DC97`, `sub_EC25`, `sub_EC3B`, `sub_EC5D`, `sub_EC75`, `sub_F681`, `sub_F821`, `sub_F86B`, `sub_F889`, `sub_C94B`, `sub_C64E`, and `sub_C653`.
- [x] Added regression cases for RAM initialization, exact copy order, indexed reads/writes, 8-bit subtraction carry, command-buffer writes, and audio-register effects.
- [ ] Host regression tests and native 3DS build must pass before this batch can be marked validated.
- [ ] Delay-loop helpers preserve final RAM/register effects but do not emulate CPU cycle timing. These helpers remain effect-level translations and are not yet fully connected to live gameplay.


## New exact ASM translations
- [x] `sub_EC99` (`$EC99`): translated the nonzero-delta path, exact `$08` threshold, `$00B9` reset, and calls to the existing `$F6FB` / `$ECBF` helpers.
- [ ] Run host regression and native 3DS builds for the `sub_EC99` addition before counting it as validated.
