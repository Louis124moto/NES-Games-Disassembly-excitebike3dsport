# Excitebike 3DS: fidelity and source mapping

## Fidelity target
Reproduce the original NES game's observable behaviour as closely as practical: acceleration, braking, overheating/cooling, ramps, airborne posture, landing, crashes/recovery, opponents, course layouts, menus, scoring, and timing.

## Technical constraint
The original `../Excitebike/` source is a 6502/NES disassembly. Its CPU instructions, memory map, PPU/APU registers, interrupt assumptions, and NES-specific graphics routines cannot be compiled directly into a native ARM11/libctru application. A native port must translate the original logic and data to 3DS-side code. Embedding a cycle-accurate NES emulator is a separate approach, not a direct native port.

## Confirmed disassembly observations

- `bank_FF.asm` begins at NES CPU address `$C000`; its `tbl_C000` at `$C000` is a table of 16-bit routine pointers, including `ofs_000_C346_00`, `ofs_000_C395_01`, `ofs_000_C99A_02`, `ofs_000_CB2C_03`, and other state-specific targets. This is evidence of table-driven dispatch, but the routine meanings must not be guessed from the labels alone.
- `bank_val.inc` defines NES controller masks: Right `$01`, Left `$02`, Down `$04`, Up `$08`, Start `$10`, Select `$20`, B `$40`, A `$80`. The native 3DS HID key masks are different, so inputs must be translated semantically, not copied numerically.
- `bank_ram.inc` documents extensive zero-page scratch aliases such as `ram_0000_temp` and `ram_0001_temp`; the same addresses are reused for different temporary roles at different ROM offsets. Do not map these aliases directly to persistent C fields without tracing their lifetimes.
- `ram_0052_track_finished_flag` is a named state variable used at multiple ROM offsets, including `$0009C1` (CPU address `$C9B1`) and `$0022CD` (CPU address `$E2BD`). This is a confirmed source-level hook for course completion, but its full state-transition rules still need tracing.
- `sub_DB95_clear_opponent_bike_data` / `loc_DB95_clear_opponent_bike_data` is an explicitly named routine at CPU address `$DB95`; call sites include ROM offset `$001AF3` and `$00248A`. This maps to opponent/race-state handling, not bike physics.
- The audio routine around CPU address `$FA82` reads `ram_00F3_bike_speed_sound`; its branch `bra_FABF_overheat_danger_noise` at `$FABF` writes `$30` to that sound-state byte when the comparison reaches the danger-noise branch. The subsequent lookup in `tbl_F9F4` selects sound data. This is audio behavior associated with overheat, not proof that `$00F3` is the primary speed or heat variable.
- The initial routine-pointer table and RAM comments still do **not** establish exact acceleration, heat/cooling, jump/landing, or collision logic. Those routines and persistent variables must be traced before a faithful implementation is claimed.

## Fidelity workflow
1. Preserve the original disassembly and its history.
2. Trace routine entry points and RAM writes/reads; record source labels, ROM offsets, inferred purpose, and confidence.
3. Translate verified behaviour and data structures rather than blindly copying 6502 instructions into C.
4. Validate each subsystem against the original with repeatable input sequences and recorded results.
5. Document deviations; do not call a subsystem faithful before comparison.

## Module map

| Module | Responsibility | Current status |
|---|---|---|
| `game_state.c` | Shared state and reset | Initial state container |
| `input.c` | Sample 3DS controls | Basic input sampling; NES-to-3DS mapping still needed |
| `bike_physics.c` | Speed, heat, posture, jumps and landing | Temporary placeholder; not faithful |
| `collision.c` | Track bounds, obstacles and crashes | Temporary boundary placeholder |
| `track.c` | Course data, terrain and ramps | Interface only |
| `renderer.c` | 3DS display and debug view | Console output only |
| `audio.c` | Sound events and audio backend | Interface only |
| `menus.c` | Menus, pause and help | Minimal interface only |
| `game.c` | Module lifecycle and update order | Initial orchestration |

## Current status
The code is architecture scaffolding, not playable or faithful gameplay. The source analysis has identified input constants, a table-driven routine dispatcher, a course-finished flag, opponent-bike cleanup, and an audio routine associated with overheating. It has **not** yet mapped the core acceleration/heat/jump/collision routines and their persistent RAM variables with enough confidence to finish Phase 2. Do not start implementing supposedly authentic physics from guesses.
