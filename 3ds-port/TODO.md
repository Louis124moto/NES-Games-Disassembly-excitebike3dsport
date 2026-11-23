# Excitebike 3DS — native port roadmap and acceptance tracker

> **Controlling specification — 2026-11-19.** The final product on `main` must be a complete, faithful Nintendo 3DS port implemented in native C/C++. It must not execute 6502 instructions, load or boot `rom.nes`, or emulate NES hardware. Any 6502 oracle/emulator is permitted only in host-side differential tests and must be excluded from the 3DS executable. This rule overrides all historical notes below and in trace documents.

## 1. Product requirements

- [x] Native-only runtime architecture approved by owner.
- [x] No NES CPU, generated 6502 recompiler/interpreter, NES bus, PPU, APU, mapper, scanline renderer or ROM boot path in the final game runtime.
- [x] Original `Excitebike/` disassembly and assets remain unchanged and serve as the behavioral authority.
- [x] Host-only reference execution is allowed for offline deterministic differential tests.
- [x] Use native ARM11/libctru input, graphics, audio, storage and timing.
- [x] Main branch is the faithful baseline. Do not create `enhanced` until faithful acceptance; then branch from the exact accepted main commit.
- [ ] Full original game reproduced: menus, editor, all courses, physics, opponents, collisions, heat, scoring, original soundtrack, transitions and results.
- [ ] Complete end-to-end session validated on New Nintendo 3DS.
- [ ] Faithful `.3dsx` release accepted.

## 2. Runtime boundary — required architecture

### Forbidden in the final 3DS runtime

- 6502 CPU core, interpreter, generated recompiler or execution glue.
- NES ROM loading/booting as the source of game execution.
- NES bus, CPU memory map as a live game-state model, PPU, APU, mapper, NMI/IRQ simulation or cycle/scanline renderer.
- Any dependency on `rom.nes` being present beside the homebrew.
- Emulator-specific menu, gameplay, sound or rendering paths.

### Allowed outside the final runtime

- Original 6502 assembly and ROM/data files as immutable source/reference material.
- A host-only 6502 oracle or external emulator used to generate/verify deterministic reference vectors.
- Asset extraction and conversion tools used during development.
- Source-address and NES-RAM annotations in trace documents, provided the shipped game uses semantic native state instead.

### Required native design

- Game state is expressed as semantic C structures (bike, rider, track, opponent, menu, race and result state), not as a live NES RAM dump.
- Gameplay update functions are deterministic and independent of libctru.
- Platform adapters translate HID input into game actions and submit native render/audio events.
- Graphics use native 3DS rendering and verified imported assets; audio uses native 3DS playback/mixing.
- Asset inspector/extractor remains a separate diagnostic mode or utility and is not the game's required startup path.

See [NATIVE_PORT_ARCHITECTURE.md](NATIVE_PORT_ARCHITECTURE.md) and [PORTING_RULES.md](PORTING_RULES.md).

## 3. Evidence and status vocabulary

Use these labels precisely:

1. **Traced** — source routine, callers, branches, data and state effects documented.
2. **Translated** — a native C implementation exists; this does not imply correctness.
3. **Unit-tested** — host regression tests pass for the stated inputs.
4. **Differential-tested** — deterministic outputs/state match the original ASM oracle for the documented vectors.
5. **Integrated** — the native implementation is called by the actual game flow.
6. **3DS-build-verified** — the exact same commit builds for the 3DS.
7. **Hardware-verified** — the exact build has been exercised on physical hardware.
8. **Accepted** — every relevant gate is satisfied and evidence is recorded.

A function, passing test, successful build or screenshot alone does not mean the subsystem is integrated or accepted.

## 4. Current verified baseline — 2026-11-23

### Repository/runtime state

- [x] The shipped 3DS target is a native C application. The host-only 6502 oracle remains outside the 3DS source glob.
- [x] Removed the retail ROM file from the current public tree in commit `dc970b9e6289b0d239c8ebe8cdb4bd8a006e5eb3`; `.gitignore` already excludes `*.nes`. **Security cleanup remains incomplete:** earlier commits still contain the blob and the repository is still public. The available GitHub connection cannot change repository visibility or delete refs/rewrite all history.
- [x] Reset `s_raceUnsupportedMask` when a first-course session starts and when returning to the selector (commit `05d849ba90d58bc8c13734ab8a9bc85dc71325f6`).
- [x] Upgraded workflow actions from Node 20-era versions to Node 24-compatible releases in all four workflows (checkout/setup-python/upload-artifact v7 where used).
- [ ] Remove stale branch refs. Six branches have zero commits ahead of main; `faithful-audit-menu-assets` has 15 unique commits and must be reviewed before deletion. The available GitHub connection exposes no delete-ref operation.
- [x] The native title, course selector and design baseline are integrated into the app loop; the asset inspector remains an optional diagnostic mode.
- [x] The native race session, semantic RaceWorld and source-ordered race-frame scheduler are integrated into the app loop.
- [x] The first-course simulation has reached the original source finish transition in a full host session: 34,370 frames, 13,177 live horizontal-scroll updates and zero unsupported-stage bits. The test was introduced on commit `8f05fa7af9086759dec496f664b2e19c4c864bf1`; the successful full host CI run 45088718550 is attached to commit `e900fcef9269d85da7ef82e8aff994d3ea15d49d`.
- [x] The app latches that finish state and B returns to the native course selector while preserving the selected race mode. This is a scoped first-course exit, not the original next-course/results flow (game.c commit f847add1a635de3e18212659da4061ff8844b251).
- [x] The first-course F4FF stream is extracted as 1,335 source records × six vertical-band values, with the full 18-byte packet and source provenance.
- [x] The native race renderer draws the scrolling course map, HUD and native OAM bike/effect sprites from extracted CHR and palette assets.
- [ ] The final race renderer still needs fixed-frame comparison against the original and physical New 3DS validation.
- [x] Native NDSP backend generates procedural engine-sputter and source-event effects from `$00FB/$00FC/$00FD/$00FE/$00FF`; original music is not included.
- [x] First-course result times, persistent best-time record, podium OAM blocks and timed result phases are implemented. [ ] Final automatic next-course transition and all source `$CC15/$CC7D` presentation side effects remain.
- [x] F4FF page data gates source obstacle spawning; spawned coordinates feed E09F obstacle collision. An end-to-end regression now exercises the actual first-course pages. [ ] Complete ramp/surface/elevation interpretation for all bike-physics cases remains.
- [ ] Full first-course acceptance and faithful .3dsx release remain open.

### Native subsystem status

| Area | Integrated state | Remaining work |
|---|---|---|
| Input/menu | Native HID mapping, title, selector, design baseline and first-course entry. | Remaining menu/editor/tape transitions, visual comparison and hardware validation. |
| Setup/world | C869/C820/C875 setup translated into semantic state; RaceWorld generated and seeded. | Complete remaining source state transitions. |
| Physics | Player speed, acceleration/braking, steering, vertical phases, jumps, heat, recovery and four-bike movement integrated. | Verify all first-course terrain/event cases in real deterministic runs. |
| Opponents/collisions | Native opponent AI/state handlers, bike contacts, obstacle-hit checks and crash recovery integrated. | Validate complete course-specific interactions against the original. |
| Course stream | F4FF and dependencies integrated; exact first-course source pass is differentially tested through F638. | Verify live scroll/render behavior and source-defined terrain/event semantics. |
| Graphics | Native scrolling nametable framebuffer renderer, HUD and OAM sprite rendering. | Fixed-frame visual parity, transitions and hardware performance. |
| Audio | Native NDSP procedural event/sputter effects. | Original soundtrack and complete source sound sequencing. |
| Finish/flow | Best/current time rows, persistent record, podium sprites and timed result phases; returns to selector. | Complete `$CC15/$CC7D` and automatic next-course transition. |

### Current build/test evidence

- Exact first-course session result: 34,370 frames; finish flag $0052 asserted; source state $0041 left race state 2; 13,177 live scroll updates; unsupported mask zero. Full host suite: [GitHub Actions 45088718550](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/45088718550).
- Native framebuffer regression test passed: the source scene draws, F4FF $8E column packets update the native nametable, and invalid PPU addresses are rejected. Verified in the same host run, commit e900fcef9269d85da7ef82e8aff994d3ea15d49d.
- Exact-commit New 3DS devkitARM build passed at commit e900fcef9269d85da7ef82e8aff994d3ea15d49d: [GitHub Actions 45088718674](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/45088718674).
- These host/build results are not physical-hardware validation and do not make Stage 3 accepted.


## 5. Sequential milestone plan

No stage may be marked accepted until all its required native integration, differential, build and hardware evidence is recorded.

### Stage 1 — source asset/data extraction

**Status: owner-accepted 2026-11-17; verification debt remains.**

- [x] Native CHR extraction/decoding and asset export path exists.
- [x] The existing on-device asset extractor now exports the first course alongside CHR/PPU assets as `extracted/course1_geometry.ebc` (EBCG v1; 1,335 columns, six raw track bands and per-column stream metadata).
- [x] Source PPU command/palette/nametable snapshots and static preview exports exist for supported source lists.
- [x] On-device atlas/inspector controls and SD-card export exist.
- [ ] Audit supported ROM/header/CHR cases and malformed inputs.
- [ ] Validate exported data, dimensions, palette mapping and output paths on host and physical New 3DS.
- [ ] Trace active-state asset selection and any compressed/generated source graphics.
- [ ] Document remaining unsupported inputs and inspector limitations.
- [ ] Keep the inspector independent from the final game startup/runtime.

Owner acceptance allows Stage 2 work to proceed; it does not mean every extractor verification item is complete. The extractor is a developer utility, not the final game's mandatory startup flow.

### Stage 2 — native menus and front-end

**Status: MAIN-MENU BASELINE ACCEPTED BY OWNER FOR ENGINE WORK (2026-11-19); optional editor/tape submenus deferred.**

Goal: retain the usable native title, selector and design/options flow needed to enter a race. The owner explicitly deferred the course editor and tape save/load menus so work can move to the race engine. Any transition or setup behavior required to enter/play a race remains in scope; optional submenus do not block Stage 3.

- [x] Trace the original state dispatcher and setup-list mapping. Evidence: `STAGE2_TRACE.md`.
- [x] Trace the shared transition dispatcher and timer behavior.
- [x] Translate/test menu selection, title/selector/design state slices, timer bank and some shared transition phases.
- [x] Translate/test libctru-to-game input mapping for all 256 controller-byte combinations.
- [ ] Finish every remaining menu transition branch and setup-list side effect as semantic native state changes.
  - [x] Translate the title, selector and design setup-list commands in lists 0, 1 and 6 (C2BD/C514/C55E/C3F3/C575) into semantic native state, including cursor sprite fields and source visual-asset keys; differential checks pass on the host.
  - [ ] Extend setup-command translation to the remaining transition, editor, save and load lists; integrate the sequencer into the real application loop.
- [ ] Translate shared transition phases 1 and 2 into semantic native menu state and renderer events; see the 2026-11-19 source trace in `STAGE2_TRACE.md`. Do not reproduce the dynamic PPU command buffer or OAM algorithm in the game runtime.
  - [x] Add an isolated native phase-1 timing/state translation and 64 deterministic ASM-oracle differential vectors (not integrated; seven-byte visual payload intentionally not rendered or guessed).
  - [ ] Trace and translate the seven-byte dynamic visual payload into a native scene update from its actual producer/caller context.
  - [x] Expand the source trace for phase-2 timer/input branches, the three table-driven sprite records, and the shared sprite-layout routine; see the 2026-11-19 refinement in STAGE2_TRACE.md. This is trace-only, not native implementation.
  - [x] Trace sub_DD6F's fixed-point object update and return-value path used by phase 2; see STAGE2_TRACE.md.
  - [ ] Trace the initialization and semantic meaning of the object fields consumed by sub_DD6F, and identify the source graphics/visible meaning of phase-2 sprite groups.
  - [ ] Translate phase-2 timing and branch decisions to semantic native state, with differential tests for every branch.
  - [ ] Implement the corresponding native transition sprite events and integrate them into the renderer.
- [x] Load the source-derived editor, tape-save and tape-load background snapshots and palette presets into the native renderer.
- [ ] Implement native title, selector, design/options and editor entry/return screens from verified assets; current editor/save/load support is only static background rendering.
- [ ] Implement all cursor/selection movement, confirmation, cancel/back and state transitions.
- [ ] Implement the 3DS SD-card save/load format and interactive UI, using the source-defined 256-byte course payload and header/metadata. Static tape-screen snapshots are not a working save/load backend. Preserve original user-visible behavior where possible; document unavoidable platform differences instead of pretending they are identical.
- [ ] Implement menu animation, scrolling and sprite composition in the native renderer; no PPU/OAM/nametable emulation.
- [ ] Implement source-confirmed menu sounds using native audio.
- [x] Integrate title/selector/design state handling and setup-command sequencing into the actual app loop; keep the asset extractor optional and open it with X.
- [ ] Integrate all remaining menu/editor/save/load transition states into the actual app/game loop.
- [ ] Compare fixed-frame screenshots and input/replay scenarios against the original NES reference.
- [ ] Run the integrated menu on physical New 3DS and record evidence.
- [ ] Exact-commit host tests and New 3DS build pass.
- [x] Owner authorized starting Stage 3 with the title/selector/design baseline; editor and tape save/load are deferred. This is a development-gate override, not a claim that every original menu subsystem is complete.

Do not carry forward emulator-specific goals such as scanline accuracy, PPU register emulation, NMI simulation, sprite-0 hit simulation or a NES RAM-backed live menu. Translate only the observable menu behavior into native scene/state logic.

### Stage 3 — first complete playable race

**Status: IN PROGRESS.** The first-course native simulation now reaches the original source finish transition in a complete host session. It is not yet accepted as a faithful playable race.

Goal: one original course playable from native menu entry through source-faithful finish/results/return behavior, with verified visuals and audio.

- [x] Trace the state-2 race dispatcher and source-ordered per-frame call sequence.
- [x] Establish player/opponent identity and initialize the first-course semantic race state from C869/C820/C875.
- [x] Build the semantic RaceWorld and integrate raceLoopStep through nativeRaceSessionStepFrame into the actual app loop.
- [x] Translate and integrate player acceleration, braking, speed control, steering, vertical phases, jump setup, heat/overheat, movement and camera scroll.
- [x] Translate and integrate opponent state handlers/AI, bike contacts, obstacle-hit checks, collision handling and crash recovery.
- [x] Port the F4FF course stream and dependencies; compare the exact 1,335-record first-course source pass against the original ASM through F638.
- [x] Integrate the F4FF packets into the native scrolling course renderer; render the native OAM bikes/effects and HUD using extracted assets.
- [x] Drive a complete native first-course session to the source finish transition. The host test reached $0052=1 and $0041!=2 after 34,370 frames, with 13,177 live scroll updates and no unsupported-stage bits.
- [x] Add a first-course-only B exit from the finish latch back to the native course selector, preserving the selected race mode. Course 2 remains outside this milestone.
- [x] Implement source best/current time rows, persistent record comparison, podium OAM and timed result phases. [ ] Complete the final automatic next-course transition and remaining `$CC15/$CC7D` effects; selector return remains the fallback.
- [ ] Complete source-backed terrain/event and collision validation using the actual first-course run, not only randomized routine vectors.
- [ ] Implement source-derived race sound effects and music through native 3DS audio. audio.c is currently empty.
- [ ] Compare fixed-frame course, bike, opponent, HUD and finish captures against the original NES reference.
- [ ] Validate rendering, controls, timing, frame pacing and full first-course completion on a physical New 3DS.
- [x] Run the complete host differential suite and exact-commit devkitARM build at commit e900fcef9269d85da7ef82e8aff994d3ea15d49d.
- [ ] Stage 3 acceptance report complete.

The 1,335 value is the count of extracted F4FF source records. The full runtime session's 13,177 value is the number of live horizontal-scroll column updates; these counters describe different things and must not be conflated. The current host session proves that the native simulation reaches the source finish transition, not that visual/audio parity or the original result flow is complete.


### Stage 4 — remaining original game

- [ ] Port remaining courses and source-defined differences.
- [ ] Complete all original modes, course editor interactions, saved-track flows and results.
- [ ] Complete every menu/game transition and reset path.
- [ ] Complete remaining opponent, collision, audio and visual behaviors.
- [ ] Differential-test all newly implemented behavior.
- [ ] Validate complete original game sessions on hardware.

The exact Stage 4 sub-stages must be derived from the disassembly after Stage 3 is accepted; do not invent an implementation order before the source flow is understood.

### Stage 5 — faithful release

- [ ] Verify no ROM is required or loaded by the shipped game.
- [ ] Verify no 6502/PPU/APU/NES-system emulator code is linked into the 3DS executable.
- [ ] Verify extractor is optional/separate from game startup.
- [ ] Complete end-to-end regression and differential scenario suite.
- [ ] Test on New 3DS; test original 3DS where supported.
- [ ] Verify frame pacing, memory use, stability, controls, graphics and audio.
- [ ] Produce reproducible `.3dsx` build and release notes.
- [ ] Owner accepts the faithful baseline.

### After Stage 5 — optional enhanced branch

- [ ] Create `enhanced` from the exact accepted faithful `main` commit.
- [ ] Keep all non-original features and visual/gameplay improvements on `enhanced`.
- [ ] Never let enhanced behavior silently alter the faithful baseline.

## 6. Rules for each implementation batch

1. Read the relevant original ASM and trace callers, branches, tables and state effects.
2. Record exact source labels/addresses and distinguish facts from hypotheses.
3. Design semantic native state and APIs before writing the translation.
4. Implement a small coherent native slice; do not create speculative placeholder behavior.
5. Add host tests, including differential vectors against the host-only oracle.
6. Test boundary values, branch paths, reset and transition behavior; use mutation checks where useful.
7. Integrate the slice into the actual native game flow before calling it integrated.
8. Build the 3DS target on the exact same commit as the host tests.
9. Test on physical hardware when the change affects platform behavior.
10. Update this TODO, relevant trace/inventory docs and README with exact commit, test command/results and CI links.

## 7. Source and evidence references

- Original source: `Excitebike/bank_FF.asm`, `bank_ram.inc`, `bank_val.inc`, original tables and CHR data. Never modify these source files for port implementation.
- Routine inventory: [ASM_FUNCTION_INVENTORY.md](ASM_FUNCTION_INVENTORY.md).
- Source mapping: [REFERENCE_MAPPING.md](REFERENCE_MAPPING.md).
- Native architecture: [NATIVE_PORT_ARCHITECTURE.md](NATIVE_PORT_ARCHITECTURE.md).
- Porting policy: [PORTING_RULES.md](PORTING_RULES.md).
- Menu source trace and historical implementation evidence: [STAGE2_TRACE.md](STAGE2_TRACE.md).
- First-race source trace and native physics evidence: [STAGE3_TRACE.md](STAGE3_TRACE.md).

Historical trace entries may describe old emulator/recompiler experiments. Treat those as source-analysis/history only; never treat them as permission to restore NES execution or hardware emulation to the final runtime.


### First-course execution scope — 2026-11-19

The next playable milestone is restricted to course 1. See [FIRST_COURSE_TODO.md](FIRST_COURSE_TODO.md) for the source-backed implementation order and acceptance gate. In particular, setup list `$12` (`_off_000_D3D6_12`) is the first course's palette setup, not its geometry. The first course's F4FF stream and template sources are now traced and extracted as raw editable data; semantic terrain/ramp/obstacle decoding and collision integration remain before the geometry can be considered gameplay-complete.


### E456 off-screen sprite pass — 2026-11-20

- Native implementation: `raceHideOffscreenOpponentSprites` in `source/race_frame.c`; semantic interface in `include/race_frame.h`.
- Source trace: `E456-E4C7` uses `D8C0` OAM block bases and `D8DC` group offsets. It copies each mapped opponent's `0084` high byte into three masks; the low-byte `F8/F0` increments execute on both carry paths, while the `70-8F` clear window is gated by the initial high-byte bit. The clear operation delegates to the already-differential-tested `raceSpawnClearSlot` (`DB95`).
- Differential test: 200,000 randomized vectors compare all OAM bytes, the `03A6` event flag and every DB95-cleared semantic field against the original ASM.
- Host CI and New 3DS build both passed at `8f301eab1088b0fc7682ed2616d14deabb76e2ad`: [host test run](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44788058666), [3DS build](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44788058703).
- Boundary: this is a verified routine, not yet wired into a complete race frame; OAM persistence across consecutive frames and full semantic session integration remain unverified.
