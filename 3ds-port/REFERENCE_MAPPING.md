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

## New source trace — temperature meter (partial)

Source: `Excitebike/bank_FF.asm`, `sub_E359_display_temperature_meter_with_sprites`, CPU address `$E359`, ROM offset `$002369`.

Confirmed directly from the instructions:

- If `ram_0052_track_finished_flag` is nonzero, the routine bypasses its normal temperature-meter update path.
- Otherwise, the routine forms a table index from `ram_005C_obj & $C0` (the NES A/B bits in the game's button representation), with a branch that also considers `ram_004F_race_started_flag`.
- It compares `ram_03B6` against `tbl_D8FB,Y`. One path adds `tbl_D8F7,Y` into `ram_03B5`, carrying into `ram_03B6`; the other subtracts `$0B` from `ram_03B5` and may decrement `ram_03B6`. This establishes a fractional/coarse accumulator pair used by the displayed temperature meter, but not yet the complete gameplay heat model.
- When `ram_03B6 >= $1A`, the routine stores timer/state values and may set the danger indicator depending on the selected state. The comment at CPU `$E3BF` explicitly calls this the overheat-danger breakpoint while holding B.
- It then builds the meter's sprite tiles from `ram_03B6 + $50`; the comment says the bar is fixed-size and moves behind the HUD background.

Not yet established: the full semantic meaning of each table index, exact frame-by-frame relation to speed, all cooling conditions, the crash/lockout transition, and the bike's acceleration values. Do not translate this routine into the current placeholder physics until the caller/state flow and related RAM writes are traced.

## New source trace — NES controller sampling

Source: `Excitebike/bank_FF.asm`, `sub_D35B`, CPU address `$D35B`, ROM offset `$00136B`.

- The routine reads the NES controller serial register `$4016,X` eight times, assembles the sampled bits, and stores the result into `ram_btn_hold,X` and `ram_btn_press,X` (CPU `$D376-$D378`).
- `bank_val.inc` defines the NES button bit assignments: Right `$01`, Left `$02`, Down `$04`, Up `$08`, Start `$10`, Select `$20`, B `$40`, A `$80`. These values are NES-side protocol/state bits, not 3DS HID masks.
- If the current sample has bit `$80` set and the previous held sample also had it set, the routine clears `$80` from `ram_btn_press,X` (`$D37C-$D384`). This is a confirmed special repeat/edge rule for A; do not assume all buttons share that exact rule.
- The native 3DS layer must translate physical HID controls into a separate NES-compatible logical bitfield if later code is to mirror the original routines. Preserve held and press semantics separately; do not pass raw `hidKeysHeld()` masks to code expecting the NES masks.

Limit: this routine explains input sampling and one special A-button rule; it does not identify which buttons trigger every race-state action. That requires tracing the consumers of `ram_btn_hold` and `ram_btn_press` in context.
