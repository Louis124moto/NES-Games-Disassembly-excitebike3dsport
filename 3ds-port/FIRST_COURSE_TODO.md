# Stage 3 — first course implementation TODO

Scope: **only the first original course** (the title-menu selector's first race choice). Do not start courses 2–5 or optional editor/tape-save features until this course is accepted.

## Source facts established (2026-11-19)

- Race state 2 enters `ofs_000_C99A_02`.
- For the title-menu route, `ram_0043` carries the selected course index and `ram_03F2=1`.
- Setup command 1 writes `ram_0045 = $12 + ram_0043` while `ram_0041 < 8`. Thus the first course selects setup/palette list **$12**.
- `tbl_C0F6/$C108` maps list $12 to `_off_000_D3D6_12`. That list contains palette writes to PPU $3F00 and $3F10. It is **palette data, not course geometry**; do not mistake it for the track layout.
- The race frame order in `ofs_000_C99A_02` is source-defined. Before the main object pass it calls `sub_D918`, `sub_DD8D`, `sub_E733`, `sub_E927`; after the optional `ram_03F2` branch it calls `sub_DDD1`, `sub_CD1F`, `sub_E96C`, `sub_DE31`, `sub_E836`, `sub_EA44`, `sub_F4FF`, `sub_E4C8`, `sub_DEE2`, `sub_E359`, `sub_E17F`, `sub_E456`, `sub_DA26`, `sub_E70B`, `sub_D19B`, `sub_CED0`, then `loc_E42B`.
- `sub_DA26` calls, for X=3 down to 0, `sub_DA58`, `sub_DC1A`, `sub_DCF2`, `sub_DCDE`; then `sub_DA6A`, `sub_DA7A`, `sub_DBFE`, followed by a timer-selected branch. The already-tested DA58/DA6A/DA7A helpers are only slices of this caller.

## Implementation order

### A. Complete the minimum native race simulation
- [ ] Build semantic per-bike state from the complete first-course setup path, including the C875 tail and player/opponent identity.
- [ ] Port the remaining state handlers called by `sub_D918` and the object-update chain; preserve the exact call order above.
- [x] Complete the per-bike pass `DA58 → DC1A → DCF2 → DCDE`, then camera/opponent movement and `DBFE`; 50,000 complete `DA26` differential frames match ASM on commit `e80b85b9c03ae61f65e0ecec61a1816938cb8f92`.
- [ ] Complete steering, braking, posture, jump/landing, heat/overheat, collision/crash/recovery and opponent decisions.
- [ ] Differential-test each routine and deterministic whole-frame scenarios against the host-only original-ASM oracle.
- [ ] Integrate the semantic race update into the native application loop. No NES RAM image or CPU execution in the shipped runtime.

### B. Decode first-course data
- [ ] Trace the first-course geometry/data source and every table/pointer it consumes; distinguish palette list $12 from course data.
- [ ] Convert the course into a compact native structure (segments, surface, elevation, ramps, obstacles and collision bounds) using source-derived values.
- [ ] Add host tests for decoded segment count, boundaries, wrap/termination and source-to-native correspondence.
- [ ] Keep later courses out of the first implementation.

### C. Native presentation
- [ ] Identify the exact source CHR/tile/sprite assets and palette for course 1.
- [ ] Implement native 3DS course/background rendering and horizontal scrolling; no PPU/nametable emulator.
- [ ] Draw bikes, opponents, ramps/obstacles and HUD from semantic state.
- [ ] Compare fixed-frame captures with the original reference and record any known platform-specific differences.

### D. Acceptance gate
- [ ] First course can be entered from the native menu, raced from start to finish, and reaches the correct result/return flow.
- [ ] Physics, course events, opponent behavior and scoring pass deterministic differential scenarios.
- [ ] Host tests and exact-commit New 3DS build pass.
- [ ] Physical New 3DS run and visual comparison recorded.
- [ ] Only after acceptance, begin course 2.

## Guardrails

- A translated helper is not automatically an integrated game feature.
- Do not draw guessed course geometry or tune physics by feel and label it faithful.
- Host-side 6502 oracle use is test-only. The New 3DS runtime remains native C/C++ only.
- Do not claim playable status or hardware verification until the acceptance gate is met.


## Exact dependency plan — refreshed 2026-11-19 21:50 CET

This is the execution order for completing the first playable race. A checked item means only the named deliverable is complete; it does not imply the next layer is done.

### 0. Freeze the baseline and keep the runtime native
- [x] Keep the 6502 oracle under `tests/oracle/` only; the 3DS Makefile compiles `source/*.c` and never links the oracle.
- [x] Preserve the native title/selector/design baseline and first-course-only scope.
- [ ] Verify the latest host-test and New 3DS build workflows after every code commit; record exact commit SHAs.

### 1. Finish the per-bike movement chain (blocking all race-frame integration)
- [x] `DA58`: speed fixed-point fraction → per-frame movement delta.
- [x] `DBFE`: secondary/parallax scroll accumulator.
- [x] `DCDE`: per-object lock-timer decay helper.
- [x] `DC1A`: source-derived vertical recovery, branch thresholds and rider/object state updates; 50,000 randomized ASM differential vectors.
- [x] `DCF2`: source-derived vertical movement, `DD38` initialization, `DD6F` fixed-point/carry chain and `D868` direction table; 50,000 randomized ASM differential vectors.
- [x] Add semantic native bike/rider/movement state; the shipped runtime does not use a live NES RAM image.
- [x] Add direct ASM-oracle differential tests for `DC1A` and `DCF2`.
- [x] Integrate exact per-bike order `DA58 → DC1A → DCF2 → DCDE` for slots 3,2,1,0; test the complete `DA26` pass across 50,000 randomized states.
- [x] Integrate `DA6A → DA7A → DBFE` and compare the complete movement/camera state against `sub_DA26`.
- [ ] Integrate the verified movement pass into the actual native race frame loop; the app currently remains a front-end prototype.

### 2. Complete player and opponent simulation
- [x] Native semantic projection of `C875` initial per-bike fields (`C87F-C8DC`), first-course `C522/CB13` parameters, and `C90E/DF7F` HUD glyph data into native structures; `object_class` and `$0078` start-lock timer are separate from the later `$0374` lock timer. The dynamic PPU command-list wrapper is deliberately not carried into gameplay.
- [ ] Finish the rest of `CD59`, including `loc_CDBD` steering/timer side effects; retain existing speed-control differential coverage.
- [ ] Trace and port every state branch reached from `D918/D924` and `D91A` (bike state machine, timers, rider posture, jumps and landings).
- [x] Port the source-derived `C875` entry and per-bike initialization loop plus the first-course `C522/CB13` parameter selection into native semantic setup; preserve the `$0018` zero-page wrap and selected `$0078` half-lock operation. Host test and New 3DS build passed at commit `808021c5a5e82271cd72b0d1ea993e0339e4c409`.
- [ ] Trace and port the remaining `C875` tail and opponent initialization/AI: `C522/C53A`, `DA9F/DAE3`, `DB50`, `DB95`, `D000`, and every table they consume. Confirm which slots are active and how player identity is assigned.
- [ ] Port heat/overheat integration and the race-start handshake; the current temperature and speed helpers are isolated slices.
- [ ] Port collision detection, crash/recovery, obstacle interaction and finish-state transitions from their actual callers.
- [ ] Build deterministic race-frame fixtures and compare all semantic bike state against the original ASM oracle over long frame sequences.

### 3. Recover the actual first-course data (do not guess geometry)
- [ ] Trace state-2 setup command 3 completely, including `D13F/ECE4` uploads and their source data.
- [ ] Trace command 6 from `C875` through its tail and identify the exact first-course data pointers.
- [ ] Follow every first-course geometry/terrain/obstacle table read from the race update and renderer paths. The `D3D6_12` palette list is not geometry.
- [ ] Decode course segments, height/surface, ramps, obstacles, bounds, wrap and finish marker into a compact semantic structure.
- [ ] Add source-to-native data tests (counts, values, pointers, boundaries and terminators).

### 4. Make the native race visible and playable
- [ ] Implement course 1 background/ground/terrain drawing using verified extracted assets and native 3DS rendering; no PPU emulation.
- [ ] Implement bike and rider sprites, opponent sprites, ramps/obstacles, animation and source-derived palette selection.
- [ ] Implement camera scroll, HUD, heat meter, countdown, finish/results and native audio events.
- [ ] Add a race state to the app loop: native menu confirm → race setup → countdown → per-frame simulation → finish/results → return flow.
- [ ] Bind physical controls to throttle, braking/steering/posture exactly as traced; ensure pause and frame timing are deterministic.
- [ ] Add replayable integration scenarios: start line, acceleration, terrain transitions, jump/landing, opponent interaction, finish and result.

### 5. Acceptance — stop only here
- [ ] The first course starts from the native menu without `rom.nes` and without any emulator/recompiler in the runtime.
- [ ] All four bikes update through the complete source-derived frame order; player controls, opponent AI, collisions, heat and course events work.
- [ ] The player can finish the entire course and reach the correct result/return flow.
- [ ] Differential frame tests, host CI and exact-commit New 3DS build pass.
- [ ] The build is exercised on a physical New Nintendo 3DS and visual differences are documented.
- [ ] Only then mark Stage 3 accepted and begin another course.

### Immediate next implementation slice
1. Trace and port the remaining first-course `C875` setup dependencies: `C53A` conditional branch and `C92C/C53E` table lookup. Keep the `C935` PPU command wrapper out of runtime; only its source-derived HUD glyph values are represented natively.
2. Resolve the first-course setup command's remaining source-derived inputs and build a complete native race-start state without a live NES RAM image.
3. Differential-test the setup result against the host-only original-ASM oracle.
4. Integrate the initialized semantic state with the complete `D918` / `DA26` native frame update.
5. Continue upward through the remaining race-frame calls; do not stop at a helper or a successful build.


## Verified movement milestone — 2026-11-19 21:09 CET

- Commit: `e80b85b9c03ae61f65e0ecec61a1816938cb8f92`.
- Native routines: `DC1A`, `DCF2`, their `DD38/DD6F` dependencies, and the complete `DA26` movement/camera pass.
- Differential tests: 50,000 `DC1A` vectors, 50,000 `DCF2` vectors, and 50,000 complete `DA26` frames.
- CI: [host differential tests](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44615749414) and [New 3DS build](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44615749343) passed.
- Important boundary: this is a verified simulation subsystem, not yet connected to the app's native menu/race state machine. Course data, full frame order, AI, collisions, race rendering and finish/results remain outstanding.
