# Native porting rules — faithful Excitebike implementation

These rules govern the final game on `main`. The target is a native Nintendo 3DS game, not a NES emulator.

## 1. Source of truth and implementation

- Treat the unmodified `Excitebike/bank_FF.asm` and its source data as the behavioral authority.
- Translate/reimplement each behavior in maintainable native C (C++ only when justified). Do not execute 6502 opcodes in the final game.
- Do not add a 6502 CPU, generated 6502-to-C runtime, NES bus, PPU, APU, mapper, scanline renderer, or ROM boot path to the final executable.
- The original ROM may be used as an offline test/reference input only. The released game must not require `rom.nes`.
- Reuse verified original graphics, tables, course data, sound data, and constants in native formats. Do not emulate NES hardware to consume them.

## 2. Trace before implementation

For every subsystem or routine group, document:
- original ASM label/address range and relevant source tables;
- entry conditions, caller/update order, and all reachable branches;
- input bytes/state and output state;
- RAM fields read/written, including wraparound and shared scratch effects;
- timing expressed as original game frames where it affects behavior;
- unresolved dependencies and hypotheses, clearly separated from confirmed facts.

Do not infer the player slot or player physics from unrelated object routines. Establish identity from initialization, writers, input flow, and update callers.

## 3. Native module boundaries

Keep gameplay rules independent from 3DS APIs. Prefer small modules with explicit state and deterministic update functions, for example:
- `bike_physics.c`: acceleration, braking, speed, heat/cooling, jumps, posture, landing and crashes;
- `bike.c`: bike/player state and frame update;
- `track.c`: course geometry, ramps, scrolling and track events;
- `opponents.c`: opponent state and AI;
- `collision.c`: source-derived collision rules;
- `menus.c`: title, selector, design, save/load and transitions;
- `game_flow.c`: state machine, race timing, scoring and results;
- `renderer.c`: native 3DS composition using verified source assets;
- `audio.c`: native audio events/playback using verified sound data;
- platform-specific input, graphics, audio and storage backends.

These are suggested boundaries, not permission to create speculative placeholder modules. Match the actual source dependencies and split further when useful.

## 4. Exact behavioral fidelity

Preserve observable original behavior, including:
- 8-bit and 16-bit arithmetic, signedness, overflow, carry/borrow and truncation where they affect game logic;
- lookup-table contents, indexing, bounds, wraparound and branch conditions;
- update/call order, frame counters, timers, state transitions and input sampling;
- bike speed/acceleration/heat/posture/jump/landing/crash state;
- opponent behavior, track events, collision and scoring;
- menu selection, confirmation, cancellation, save/load and result transitions;
- animation, sprite priority, palette choices, scrolling and audio event timing.

Do not "clean up" quirks that change the original game's behavior. Keep the C readable, but preserve the behavior.

## 5. Differential tests — required, offline only

- Use deterministic test vectors generated from source analysis and an independent reference execution of the original ASM.
- The reference 6502/oracle or an external NES emulator may be used by host-side tests/tools only. It must never be linked into the 3DS executable or used as the game's runtime.
- Compare observable state after every original game frame, not only final outcomes.
- For physics, compare at least speed/velocity, heat, bike posture, jump/airborne state, landing/crash/recovery state, player position, relevant timers, and outputs/events.
- Cover normal branches, boundary values, button combinations, reset conditions, and transitions. Explain any constrained/randomized test preconditions.
- Include deterministic replay tests and mutation checks where practical. A test that does not exercise a branch is not evidence for that branch.
- Keep reference captures and vector formats reproducible; document the original game version/ROM identity used to produce them.

## 6. Rendering and audio validation

- Render the game natively with 3DS graphics APIs. Use extracted source art/data; no PPU register emulation.
- Compare screenshots/captures at fixed game frames against the original NES reference. Account for the 3DS screen layout/scaling separately from the NES logical image.
- Play sound through the native 3DS audio backend. Compare event order, pitch, duration and timing against source/reference behavior; do not model APU registers as a runtime substitute.
- Missing source assets must be traced or explicitly reported; never replace them with guessed art or sound.

## 7. Build and integration gates

For each completed subsystem batch:
1. source trace reviewed;
2. native implementation and tests added;
3. host differential/regression tests pass;
4. 3DS build succeeds on the exact same commit;
5. integration path is exercised in the actual game;
6. hardware behavior is validated when relevant;
7. TODO and README record evidence, limits, commit and CI runs.

A translated function, passing isolated test, successful build, or attractive screenshot alone does not establish a faithful port.

## 8. Branch policy

- `main` contains only the faithful native baseline.
- Do not create `enhanced` until all faithful acceptance gates pass and the owner accepts the baseline.
- Create `enhanced` from that exact accepted `main` commit. Optional visual, gameplay, quality-of-life or performance changes belong only on `enhanced`.
- Never back-port enhancements into `main` unless they are required to correct fidelity.
