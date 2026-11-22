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
- [x] Port the state handlers called by `sub_D918` and the source-ordered object-update chain; preserve the exact call order. The remaining C875 setup tail is tracked separately below.
  - [x] Translate and integrate `sub_E70B` as semantic four-bike position smoothing (`$0060/$0064/$00E0/$00D8`); 200,000 randomized full-routine vectors compare against the original ASM.
- [x] Complete the per-bike pass `DA58 → DC1A → DCF2 → DCDE`, then camera/opponent movement and `DBFE`; 50,000 complete `DA26` differential frames match ASM on commit `e80b85b9c03ae61f65e0ecec61a1816938cb8f92`.
- [x] Complete and integrate the core steering, braking, posture, jump/landing, heat/overheat, bike-contact, obstacle-hit, collision, crash/recovery and opponent-decision routines in native C.
- [x] Differential-test the routines and deterministic source-ordered whole-frame scenarios against the host-only original-ASM oracle. This includes 50,000 phase-zero plus 100,000 all-phase E96C/DD06 jump frames; the test caught and fixed the missing E97D Y store before the phase-one early branch.
- [x] Integrate the semantic race update into the native application loop. No NES RAM image or CPU execution is used in the shipped runtime.
- [x] Integrate the semantic `BikeTemperatureState` into E359, including native temperature-bar OAM output; the full race-loop differential test passes after this wiring.
- [ ] Finish the remaining C875 setup tail and confirm the first-course player/opponent slot identity from the complete setup path.
- [ ] Validate obstacle collision inputs against the fully decoded first-course geometry; the collision algorithms are ported, but the complete geometry source is not yet decoded.

### B. Decode first-course data
- [x] Trace the first-course stream/template sources: $ED3A/$ED40 selects stream 0 at $ED46; the next stream begins at $EE59; $F063/$F087 supplies template pointers. Keep palette list $12 separate from geometry.
- [x] Extract the complete raw two-pass stream to extracted/course1_geometry.json: 1,334 ordered columns, six source pages per column, raw tile IDs, ring/PPU coordinates, stream offsets and template addresses. Stop on the original $F638 / $03BC completion signal.
- [x] Add 3ds-port/tools/read_course.py: strict complete-file validation, six-band hex map, diagnostic glyph map, per-column inspection and CSV export.
- [ ] Differentially replay the exact first-course initial state frame-by-frame against F4FF through $F638; the test has been added and awaits CI confirmation.
- [ ] Interpret raw tile IDs into semantic surface/elevation/ramp/obstacle/collision structures and connect those structures to the native runtime.
- [ ] Keep later courses out of the first implementation.

### C. Native presentation
- [x] Identify and use the extracted source CHR/tile/sprite assets and course-1 palette for the native race scene.
- [x] Feed the source F4FF 14-tile vertical column packet into the native scrolling renderer, using the verified race-scene CHR/palette and vertical nametable layout; no NES PPU runtime.
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
- [x] Port the complete first-course `E96C` vertical path (`E96E-EA05`): exact anchor and bound tables, all `$00A4` phase branches (including phase-one `DD06` jump creation, phase-two/four descent and phase-three hold), the `DD38/DD1A/DD6F` arc initialization/update, and the `EA07-EA43` player vertical-input handshake. The differential suite now runs 50,000 phase-zero frames plus 100,000 randomized all-phase four-bike frames against the original ASM. It caught and fixed a missing `$00B8` Y store before the phase-one early branch.
- [x] Implement native menu setup list 2 (`C551,C869,C820,C5A3,C566,C875`) and test each command against the original ASM. The host oracle bypasses only `D13F`'s PPU artwork upload when testing `C820`; it executes and checks the CPU track-page fills, scroll values and sprite marker.
- [x] Integrate `NativeRaceSession` into `game.c` after setup list 2 completes, using the menu's actual `$0041/$0046/$0034/$03F8` values and the native RNG seed. It initializes gameplay state but deliberately does not run an incomplete frame loop.
- [x] Add the native first-course background/HUD path: merge extracted snapshots 18, 01, 17 and 14, draw original CHR tiles with palette 18, and overlay the seven C875/C935 HUD glyphs directly into the framebuffer. No PPU/OAM emulator is used at runtime.
- [x] Finish the rest of `CD59`, including `loc_CDBD` steering/timer side effects (`CE83`, `DCC7`); 240,000 tail vectors + 120,000 whole-routine vectors match ASM (2026-11-20). Verified domain: `ram_00B0` 0..3.
- [x] Port the six-entry `D924` dispatch and all five non-idle state handlers (states 1–5), plus the `D91A → D924 → DA15` four-bike order. `bike_state.c` is semantic C; `test_bike_state.c` compares 50,000 randomized four-bike frames against the original `D918` ASM oracle. This covers the currently valid state domain 0–5; posture/jump/landing effects outside these handlers remain separate work.
- [x] Port the source-derived first-course `C875` path (`C87F-C8DC`, `C522/CB13`, `C90E/DF7F`, and `C92C/C53E`) into native semantic state; preserve the `$0018` zero-page wrap and selected `$0078` half-lock operation. `C905` confirms the first-course setup index is `$02`, so the `$08+` `C53A` branch is not taken. Host tests and New 3DS builds passed through commit `bfa160ca3fbc1f70674315e2afeb652532bda6a9`.
- [ ] Trace and port the remaining `C875` tail and opponent initialization/AI: `C522/C53A`, `DA9F/DAE3`, `DB50`, `DB95`, `D000`, and every table they consume. Confirm which slots are active and how player identity is assigned.
- [x] Port the semantic state transitions of `sub_DD8D` for the countdown/start handshake; its OAM-only `CCDD/CD13` work is represented as native renderer events. Host regression passed in CI.
- [x] Integrate heat/overheat into E359 using the semantic temperature state and native OAM output; verified by the full race-loop ASM differential suite.
- [x] Integrate bike-to-bike contacts (DFD5), obstacle-hit checks (E09F), bike collision checks (E4C8), crash recovery (DDD1) and collision sprite hiding (E42B) at their source-ordered race-loop call sites; the race-frame and full-loop ASM differential tests pass.
- [ ] Complete source-derived first-course obstacle/terrain geometry and validate every obstacle-hit input against it.
- [ ] Port the source-faithful finish/results state transition (kept separate from the physics milestone).
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
1. [x] Implement `RaceWorld` and connect the source-ordered race stages, including D918, DD8D, CD1F, DA26, F4FF, E4C8, E456 and E17F. The composed loop is stage-by-stage differential-tested; the app calls it through `race_session_world.c`.
2. [ ] Trace and port the remaining unimplemented/unsupported handlers and validate long consecutive-frame scenarios, not only isolated routines.
3. [ ] Trace the first-course terrain/obstacle data and replace the scene-only presentation with source-derived geometry; pass the course-stream output to the renderer.
4. [ ] Complete heat, opponent AI, obstacle interaction, finish/results, physical controls and native menu-to-race return flow.

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

## Raw first-course extraction milestone — 2026-11-22

- Source stream 0: $ED46; next stream pointer: $EE59; source stream segment: 275 bytes.
- The first stream byte supplies repeat count $02. The decoder reaches the source-defined $F638 branch twice; when $0057 reaches zero, the original code sets $03BC.
- Generated artifact: extracted/course1_geometry.json — 1,334 columns × 6 vertical bands. Each column keeps raw bytes and provenance; semantic names are annotations, not a claim that ramps/collision bounds have been fully interpreted.
- Reader: python3 3ds-port/tools/read_course.py. Use --format glyph for a compact diagnostic map or --column N for the full metadata of one column.
- The generation workflow stores the JSON in extracted/; host CI checks reproducibility against the disassembly.
- Remaining: exact initial-state full-sequence differential replay, semantic tile decoding, collision mapping, and native runtime integration.

## Course stream reader (`sub_F4FF`)

- [x] Port `sub_F4FF` with `sub_F82E`, `sub_F676`, `sub_F681`, `sub_F64E` and `sub_ECBF` as `raceCourseStreamUpdate` (`race_course_stream.c`). The ROM-derived stream/template tables ($ED3A-$F4FE) are constant data in `race_course_stream_data.c`; no ROM is loaded at runtime. The six course pages, `ram_00E0_obj[0]` and `ram_03A8` stay in `RaceSpawnState`.
- [x] 6502 quirks kept: the `sub_F64E` descending scan that stores the wrapped counter when the buffer is exhausted, and the `sub_F681` copy loop. Out-of-domain states (stream index 5 pointing at RAM $06E0, template byte count > $0E, reads outside the embedded data) set `unsupported` instead of guessing.
- [x] 300,000 randomized vectors plus 150,000 sequential frames over all five streams × ten course ids match the original ASM; 16/17 mutants killed (`y < 0x58` vs `<=` is equivalent: template 9 record boundaries are $57 and $61).
- [x] Wire `raceCourseStreamUpdate` at its `C99A` slot (after `sub_EA44`, before `sub_E4C8`) in `race_loop.c`; the decoded column buffer/pages are now available in `RaceWorld`. Feeding them into a native track renderer remains open.

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

## Unified native race frame loop (race_world / race_loop)

- [x] `RaceWorld` (126 named blocks keyed by source RAM addresses, generated by `tools/gen_race_world_parse.py` + `tools/gen_race_world.py`) holds the whole race state natively; no NES RAM image is shipped.
- [x] `raceLoopStep` runs the source frame order (CA08 … E42B) and is compared against the original ASM stage by stage (300,000 frames, `tests/test_race_loop.c`). Bugs found and fixed through it: opponent acceleration index in CD59/CE29 (Y = X, +1 for bit $40, no cap), `$039F` export from E1F9/E1BE, D9A7 path in state 3 of D918, `$03AA` base of DEBB cue flags.
- [x] DA44 tail (E0E6 banner, E2AF finish sign) ported in `source/race_tail.c`.
- [ ] Still open: D347 input layer, D19B frame sync, menu↔race transitions, native course renderer, audio, 3DS devkitARM build and hardware validation.
- [x] `race_session_world.c` seeds the world from the composed session and steps native frames; `game.c` now runs one native race frame per update after setup (host-tested only; 3DS build/hardware not verified here). `$0400-$0446` is the NES PPU packet buffer (ECE4/D13F/D0F4 are presentation-only and intentionally not reproduced; the native renderer draws from semantic state).


## App-loop integration correction — 2026-11-22

- [x] `game.c` now steps the native `RaceWorld` once per application update after first-course setup.
- [x] Freeze menu timers, menu input handling and setup-command sequencing once the race session is active; previously those menu updates continued underneath the race. Fix: commit `500ed1521566dd127ad346e42cf98e5c4818561b`.
- [x] Update README status to distinguish an integrated native frame scheduler from a fully playable/accepted race.
- [ ] Run host CI and the exact-commit devkitARM build after these integration changes.
- [ ] Physical New 3DS validation remains outstanding.


### Native race-world and course-render integration — 2026-11-22

- [x] Seed the unified RaceWorld with the exact C820 CPU course pages ($3B,$3B,$3D,$3D,$3D,$3D), the D0AB vertical bands, D0C6 terrain codes/samples, the C1CE RNG bytes and the setup transition flag $0046.
- [x] Keep the F4FF output packet ($EA/$E9 address, $8E control and fourteen tile bytes) in the native session whenever the source column index advances.
- [x] Apply each decoded course column to the native race nametable and render with extracted CHR/palette assets, source horizontal scroll and vertical nametable selection.
- [x] Forward HID held and pressed button states separately; the original pause handler consumes the press latch, not held input.
- [x] Add host integration assertions for course pages, D0AB/D0C6 projections, RNG/transition seeding and Start press behavior.
- [ ] Compare dynamic course captures at fixed frames against the original NES reference and verify on a physical New Nintendo 3DS.
- [ ] Finish trigger, result presentation and return-to-menu state transition are still not end-to-end integrated. Do not mark the first course playable until those pass.


- [x] Latch the first $0041 transition out of race state 2 so CA9B/C3AF is not called repeatedly by the app after the original dispatcher would have left the race loop.
- [x] Add a temporary development escape from the frozen finish frame back to the native title with B.
- [ ] Replace the temporary finish hold with the source-faithful results presentation, score/rank calculation and original post-race menu transition.


### Vertical physics completion — 2026-11-22

- [x] Differentially exercise all first-course vertical phases and action-state gates through the real E96C → DD06 → DD38 → DD1A/DD6F path.
- [x] Fix the missing E97D-equivalent store of updated object Y before the phase-one jump branch. Without it, the ASM wrapped the Y coordinate correctly and entered jump setup, while native C kept the pre-addition value.
- [x] Validate 50,000 phase-zero and 100,000 all-phase randomized four-bike vectors against the host-only original-ASM oracle.
- [x] Verify the New 3DS devkitARM build after the fix.


### Full native physics integration — 2026-11-22

- [x] Fix E96C phase-one jump Y wrap: commit `8248314e768368d31b16cf73d04e8d028a9849c1` stores the E97D result before the jump branch.
- [x] Extend E96C/DD06 differential coverage to 100,000 randomized all-phase frames in addition to 50,000 phase-zero frames.
- [x] Route E359 through `BikeTemperatureState` and `bikePhysicsUpdateTemperature`; generate the four temperature-bar sprites natively in `race_loop.c`.
- [x] Verify race-loop, AI, race-frame collision/crash/obstacle, vertical recovery, DA26 movement and all-phase jump tests against the original ASM.
- [x] Verify the devkitARM/New 3DS target build.
- [ ] Remaining physics dependency: decode the complete first-course obstacle/terrain geometry and validate the already-ported obstacle-hit algorithm with real course inputs.
