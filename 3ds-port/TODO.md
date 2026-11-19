# Excitebike 3DS — native port roadmap and acceptance tracker

> **Controlling specification — 2026-11-19.** The final product on `main` must be a complete, faithful Nintendo 3DS port implemented in native C/C++. It must not execute 6502 instructions, load or boot `rom.nes`, or emulate NES hardware. Any 6502 oracle/emulator is permitted only in host-side differential tests and must be excluded from the 3DS executable. This rule overrides all historical notes below and in trace documents.

## 1. Product requirements

- [x] Native-only runtime architecture approved by owner.
- [x] No NES CPU, generated 6502 recompiler/interpreter, NES bus, PPU, APU, mapper, scanline renderer or ROM boot path in the final game runtime.
- [x] Original `Excitebike/` disassembly and assets remain unchanged and serve as the behavioral authority.
- [x] Host-only reference execution is allowed for offline deterministic differential tests.
- [x] Use native ARM11/libctru input, graphics, audio, storage and timing.
- [x] Main branch is the faithful baseline. Do not create `enhanced` until faithful acceptance; then branch from the exact accepted main commit.
- [ ] Full original game reproduced: menus, editor, courses, physics, opponents, collisions, heat, scoring, sound, transitions and results.
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

## 4. Current verified baseline — 2026-11-19

### Repository/runtime state

- [x] The live game loop no longer initializes or steps an NES CPU/system/APU or PRG-backed menu recompiler.
- [x] Generated 6502 recompiler, CPU call glue, NES system/APU/game wrappers, old emulator menu, ROM boot path, emulator-only tests and generators were removed from the active source tree.
- [x] 3DS Makefile compiles the native C source glob directly.
- [x] Host-only 6502 reference CPU is retained under `tests/oracle/`, outside the 3DS source glob.
- [x] The normal game path now loads the existing exported EBPX CHR pack and source scene snapshots without opening or requiring rom.nes.
- [x] The title/selector/design front-end is wired to the native menu state and setup sequencer; its tilemap and cursor are drawn directly to the 3DS framebuffer by native code.
- [x] The native asset loader now reads source snapshots for editor state 9 and tape save/load states 12/13, and loads all seven palette presets 12–18. Renderer support is static-background-only; it is not editor/save/load functionality.
- [ ] The diagnostic asset inspector and its static PPU preview/compositor helpers still need a strict utility-only boundary from the final game renderer.
- [ ] The application is now a native front-end prototype, not yet a complete playable Excitebike game; editor/save/load and several transitions remain incomplete.
- [ ] Native menu and physics modules must be integrated into a real game loop; isolated helper coverage is not gameplay completion.

### Existing native code — verified slices only

| Area | Present native code/evidence | Not yet proven or implemented |
|---|---|---|
| Input | `inputMapNesButtons` maps libctru buttons to semantic/original input bits; all 256 combinations tested. | Full game action handling/integration. |
| Menu logic | `nativeMenuUpdateSelection` (`sub_C7DE`), title/selector/design state logic, timer bank (`sub_D310`), transition phase 0 (`CB3B`), phase 3 (`CC6B`) and timer-expired `C465` branch have source-derived tests. | Remaining transition phases/branches, setup command semantics, complete editor/save/load, native visual/audio presentation, app integration and reference capture comparison. |
| Bike speed/physics | Native four-slot speed-control prefix (sub_CD59) and active-bike dispatch (sub_CD1F), plus acceleration/deceleration/terrain gate and temperature/overheat slices. New all-slot tests compare 120,000 sub_CD59 vectors and 12,000 sub_CD1F frame vectors against the ASM oracle. | Remaining sub_CD59 steering/posture side effects, heat state integration, jump/landing, crash/recovery, opponents and complete frame ordering. |
| Movement | Native fixed-point movement deltas, player scroll and opponent-relative position slices. | Full object update, collision, track event and race-frame integration. |
| Track | Module/interface scaffolding. | Verified course geometry, ramps, obstacles, scrolling and native rendering. |
| Opponents | Some movement primitives only. | Full initialization, AI/state decisions, collision and race behavior. |
| Collision | No faithful complete implementation. | Trace and implement all original collision paths. |
| Graphics | CHR extraction/decoding, exported EBPX/snapshots, and direct-framebuffer native rendering for title/selector/design plus static editor/save/load source-snapshot backgrounds. | Validate all scenes/palettes against reference captures; implement editor objects/cursor, save/load UI, transition sprites and animations. |
| Audio | Interface/scaffolding only. | Source-derived sound/music import, native playback/mixing and event timing. |
| Runtime/game flow | Native app scaffold currently launches asset inspection. | Native title-to-results game loop and all transitions. |

### Existing differential evidence

- Native player speed-control/acceleration/terrain/temperature slices: 1,224,704 ASM reference vectors recorded in the current tracker. The new four-slot speed-control test adds 120,000 sub_CD59 vectors and 12,000 sub_CD1F frame-dispatch vectors.
- Native movement slices: 574,288 ASM reference vectors recorded in the current tracker.
- Combined recorded slice count: **1,798,992 vectors**. This is not a complete physics or race validation.
- Verified code revision: `f214e5bbc96473896ca3abbbbf9e8cb6c8cc05f8`.
- Host differential CI: [44585477913](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44585477913).
- New 3DS build CI: [44585478080](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44585478080).
- Latest documentation commit before this roadmap rewrite: `639554501e839b8758776bbd01b691f58eb9cc99`.

These CI results validate only the code/tests included in that revision. They do not validate the current HEAD after later changes, nor prove a playable game.

## 5. Sequential milestone plan

No stage may be marked accepted until all its required native integration, differential, build and hardware evidence is recorded.

### Stage 1 — source asset/data extraction

**Status: owner-accepted 2026-11-17; verification debt remains.**

- [x] Native CHR extraction/decoding and asset export path exists.
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

**Status: IN PROGRESS by owner instruction. Some isolated native physics slices and the first four-bike speed-control frame pass are now implemented and differential-tested.**

Goal: one original course playable from menu entry through finish/results, with source-faithful game behavior.

- [ ] Complete source trace of state-2 race entry, setup sequence and exact per-frame update order. Initial evidence: dispatcher state 2 enters ofs_000_C99A_02; sub_CD1F runs from loc_C9D5 and dispatches active bike slots 3..0 through sub_CD59.
- [ ] Establish player/opponent identity and all persistent semantic state from initialization, writers, input flow and callers.
- [ ] Convert race setup results into semantic native track/race structures; do not use a NES RAM image as live state.
- [ ] Decode/import the first course's verified geometry, ramps, terrain, obstacles and visual data.
- [x] Translate and differential-test the active-bike speed-control pass (sub_CD1F) and its sub_CD59 speed/terrain prefix for all four object slots.
- [ ] Complete player acceleration, braking, speed limits, heat/cooling, steering, posture, jumps, landing, collisions, crashes and recovery; speed-control translation is only one subsystem, not a complete bike controller.
- [ ] Complete opponent initialization, movement, AI and collision behavior.
- [ ] Implement race timers, lap/finish detection, scoring, result flow and return-to-menu behavior.
- [ ] Implement native rendering for track, bike, opponents, HUD and animation.
- [ ] Implement native audio events and music for the race.
- [ ] Differential-test deterministic frame-by-frame scenarios against the original ASM, including all branches and boundary conditions.
- [ ] Integrate the complete race into the native runtime.
- [ ] Run end-to-end on physical New 3DS and compare captures/behavior against the original reference.
- [ ] Exact-commit host tests and New 3DS build pass.
- [ ] Stage 3 acceptance report complete.

Existing speed, temperature and movement vectors are reusable evidence. They do not waive any item above.

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
