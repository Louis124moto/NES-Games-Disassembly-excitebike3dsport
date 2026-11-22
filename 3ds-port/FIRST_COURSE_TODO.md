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
  - [x] Translate `sub_E70B` as semantic four-bike position smoothing (`$0060/$0064/$00E0/$00D8`); 200,000 randomized full-routine vectors compare against the original ASM. This is a standalone verified routine, not yet integrated into the race-frame scheduler.
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
- [ ] Integrate the verified movement pass and the D918 state dispatcher into a complete native race frame loop; the app still deliberately stops at the first-course scene because several intervening source-ordered systems are not yet integrated.

### 2. Complete player and opponent simulation
- [x] Native semantic projection of the first-course `C875` setup path, including `C522/CB13`, `C53E`, `C90E/DF7F` HUD glyph data and all initialized per-bike/global fields. `C905` guards the first-course `$02` path; `object_class`, `$0064` origin, `$0074` setup parameter and `$0078` start-lock timer are separately modeled. The `C935` PPU command-list wrapper is replaced by native HUD data.
- [x] Port `sub_D326` as an eight-byte native RNG state (`$0018-$001F`), seeded from `C1CE`; the setup initializer now consumes the native RNG state rather than fabricated per-bike inputs. Its startup-loop call site is not yet wired into the application.
- [x] Port the first-course CPU track lookup profile from `C820` plus `D0AB/D0C6/E7FC`: six 64-byte pages are `$3B,$3B,$3D,$3D,$3D,$3D`; native code computes each bike's vertical band and four source samples. For these initial values, `E733`'s `$40` gate is never reached, so first-course opponent event dispatch is a no-op.
- [x] Port the gameplay-field reset performed by `C869` (`$004D-$00EF`) and `C820` (`$0340-$03EF`) into `nativeRaceSetupClearFirstCourseState`; preserve bytes outside those ranges.
- [x] Port the phase-zero first-course path of `E96C` (`E96E-EA05`): exact anchors `$0E,$1A,$26,$32` (the fourth indexed byte is labeled at `$E540`), `$08/$3A` bounds, and the source-specific direction-update branches (in-range Y does not toggle; player slot 0 has the low-bound bypass). The `EA07-EA43` player vertical-input handshake is also ported and covered by the four-bike differential test. Nonzero `$00A4` jump phases and their `DD06` initialization remain separate work.
- [x] Implement native menu setup list 2 (`C551,C869,C820,C5A3,C566,C875`) and test each command against the original ASM. The host oracle bypasses only `D13F`'s PPU artwork upload when testing `C820`; it executes and checks the CPU track-page fills, scroll values and sprite marker.
- [x] Integrate `NativeRaceSession` into `game.c` after setup list 2 completes, using the menu's actual `$0041/$0046/$0034/$03F8` values and the native RNG seed. It initializes gameplay state but deliberately does not run an incomplete frame loop.
- [x] Add the native first-course background/HUD path: merge extracted snapshots 18, 01, 17 and 14, draw original CHR tiles with palette 18, and overlay the seven C875/C935 HUD glyphs directly into the framebuffer. No PPU/OAM emulator is used at runtime.
- [x] Finish the rest of `CD59`, including `loc_CDBD` steering/timer side effects (`CE83`, `DCC7`); 240,000 tail vectors + 120,000 whole-routine vectors match ASM (2026-11-20). Verified domain: `ram_00B0` 0..3.
- [x] Port the six-entry `D924` dispatch and all five non-idle state handlers (states 1–5), plus the `D91A → D924 → DA15` four-bike order. `bike_state.c` is semantic C; `test_bike_state.c` compares 50,000 randomized four-bike frames against the original `D918` ASM oracle. This covers the currently valid state domain 0–5; posture/jump/landing effects outside these handlers remain separate work.
- [x] Port the source-derived first-course `C875` path (`C87F-C8DC`, `C522/CB13`, `C90E/DF7F`, and `C92C/C53E`) into native semantic state; preserve the `$0018` zero-page wrap and selected `$0078` half-lock operation. `C905` confirms the first-course setup index is `$02`, so the `$08+` `C53A` branch is not taken. Host tests and New 3DS builds passed through commit `bfa160ca3fbc1f70674315e2afeb652532bda6a9`.
- [ ] Trace and port the remaining `C875` tail and opponent initialization/AI: `C522/C53A`, `DA9F/DAE3`, `DB50`, `DB95`, `D000`, and every table they consume. Confirm which slots are active and how player identity is assigned.
- [x] Port the semantic state transitions of `sub_DD8D` for the countdown/start handshake; its OAM-only `CCDD/CD13` work is represented as native renderer events. Host regression passed in CI.
- [ ] Port heat/overheat integration beyond the countdown handshake; the current temperature and speed helpers remain isolated slices.
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
- [x] Implement and differentially verify the native E17F sprite path: E3F4 sorting, E1F9 dynamic pose selection, E1A3/D1C7 OAM generation, E1D7 temp overlays and E26A rider overlays for all four bike slots.
- [x] Port E456 off-screen sprite hiding and DB95 opponent-bike clearing; 200,000 randomized routine vectors are checked by `test_e456`. Multi-frame OAM transition integration and remaining ramp/obstacle sprites remain open.
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
1. Implement a semantic frame-state aggregate and connect the already-tested D918 state machine, DD8D countdown, DA26 movement pass, E70B smoothing, E4C8 collision, E456 culling and E17F sprite build in source order. Keep unported calls explicitly gated rather than silently skipping them.
2. Trace and port the remaining handlers called between D918 and E96C, then differential-test consecutive frames (not only isolated routine calls).
3. Trace the first-course terrain/obstacle data and replace any scene-only presentation with source-derived track geometry.
4. Continue with heat, opponent AI, obstacle interaction, finish/results, and native menu-to-race input flow.

### Physics validation update — 2026-11-20
- `E836` calls `DC97`, which is exactly `SEC; SBC $00BC,X; SBC $00B8,X`; native `bikePhysicsUpdateScreenY` preserves the first subtraction borrow and skips slots with `$00B0 != 0`.
- Added a 50,000-vector differential test that invokes the original `E836` entrypoint in the host-only 6502 oracle and compares all four `$008C` screen-Y bytes with the native C implementation.
- The native E17F path now passes 50,000 randomized full OAM differential frames across four bikes, including E3F4 sorting, E1F9 state/approach selection, all D892/D8A9 pose-table entries, D1C7 tile/coordinate generation, E1D7 temporary overlays, and E26A rider overlays. E456 off-screen hiding and DB95 opponent-bike clearing remain unported; OAM is still rebuilt from a clean `$F8` baseline, so consecutive-frame persistence must be tested after E456 is integrated.


## Verified movement milestone — 2026-11-19 21:09 CET

- Commit: `e80b85b9c03ae61f65e0ecec61a1816938cb8f92`.
- Native routines: `DC1A`, `DCF2`, their `DD38/DD6F` dependencies, and the complete `DA26` movement/camera pass.
- Differential tests: 50,000 `DC1A` vectors, 50,000 `DCF2` vectors, and 50,000 complete `DA26` frames.
- CI: [host differential tests](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44615749414) and [New 3DS build](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44615749343) passed.
- Important boundary: this is a verified simulation subsystem, not yet connected to the app's native menu/race state machine. Course data, full frame order, AI, collisions, race rendering and finish/results remain outstanding.


### Native routine batch — 2026-11-20 (E70B)

- [x] Add semantic state/API: `RacePositionSmoothingState` / `raceSmoothBikePositions`.
- [x] Translate the four-slot descending loop exactly: skip target adjustment when movement delta is zero; otherwise subtract in 8-bit arithmetic, advance target by eight and increment the six-bit phase on equality/negative result, and clear each transient byte.
- [x] Add 200,000 randomized full-entrypoint comparisons against original `sub_E70B`.
- [ ] Integrate E70B at its exact `C99A` frame-order position and validate long deterministic race sequences.
- [ ] Continue the remaining five-routine batch with the next source-ordered race-frame dependency; do not mark this helper as gameplay integration.

## Course stream reader (`sub_F4FF`)

- [x] Port `sub_F4FF` with `sub_F82E`, `sub_F676`, `sub_F681`, `sub_F64E` and `sub_ECBF` as `raceCourseStreamUpdate` (`race_course_stream.c`). The ROM-derived stream/template tables ($ED3A-$F4FE) are constant data in `race_course_stream_data.c`; no ROM is loaded at runtime. The six course pages, `ram_00E0_obj[0]` and `ram_03A8` stay in `RaceSpawnState`.
- [x] 6502 quirks kept: the `sub_F64E` descending scan that stores the wrapped counter when the buffer is exhausted, and the `sub_F681` copy loop. Out-of-domain states (stream index 5 pointing at RAM $06E0, template byte count > $0E, reads outside the embedded data) set `unsupported` instead of guessing.
- [x] 300,000 randomized vectors plus 150,000 sequential frames over all five streams × ten course ids match the original ASM; 16/17 mutants killed (`y < 0x58` vs `<=` is equivalent: template 9 record boundaries are $57 and $61).
- [ ] Wire `raceCourseStreamUpdate` at its `C99A` slot (after `sub_EA44`, before `sub_E4C8`) and feed the column buffer/pages to the native track renderer.

## Pause/input and finish flow (`sub_CA38`, `sub_CA1B`, `sub_CA9B`)

- [x] Port pause toggle (Start latch, `$4015` enable output, sound request bytes), `ram_005C` input copy, the `CA13` tail of `sub_CA08`, and the finish palette packet / `sub_C3AF` flow as `race_frame_control.c`; 200,000 vectors per entry match the original ASM (one `LDA ram_03B0` reload quirk found and fixed). `ram_03F7` != 0 restart paths are reported unsupported.
- [ ] Wire `sub_CA08` (DCA0/DFB2 loop + `raceFrameInputStep`) and `sub_CA9B` into the native race loop.


## Small integration and maintenance fixes — 2026-11-22

- [x] Guard public race-frame control entry points against null state; add host test coverage.
- [x] Guard the course-stream entry point against null state/spawn projection; null spawn is reported as unsupported and covered by a host test.
- [x] Exercise the existing session-level E836/E17F sprite update wrapper from the native race-session test and verify repeated OAM generation is deterministic.
- [x] Fix the 3DS link-map output path so Make expands the resolved `$(OUTPUT)` rather than freezing an empty automatic `$*` during immediate assignment.
- [x] Fix literal backslash-n sequences in the native scene asset-error message.
- [ ] Run the updated host CI and an exact-commit devkitARM/New 3DS build; these checks are not claimed until Actions reports success.
- [ ] Keep the complete source-ordered race-frame scheduler, remaining simulation dependencies, and course renderer assigned to the main gameplay implementation work.
