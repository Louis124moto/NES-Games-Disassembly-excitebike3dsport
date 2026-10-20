# Excitebike 3DS: fidelity and source mapping

## Fidelity target
Reproduce the original NES game's observable behaviour as closely as practical: acceleration, braking, overheating/cooling, ramps, airborne posture, landing, crashes/recovery, opponents, course layouts, menus, scoring, and timing.

## Technical constraint
The original `../Excitebike/` source is a 6502/NES disassembly. Its CPU instructions, memory map, PPU/APU registers, interrupt assumptions, and NES-specific graphics routines cannot be compiled directly into a native ARM11/libctru application. A native port must translate the original logic and data to 3DS-side code. Embedding a cycle-accurate NES emulator is a separate approach, not a direct native port.

## Confirmed disassembly observations

- `bank_FF.asm` begins at NES CPU address `$C000`; its `tbl_C000` at `$C000` is a table of 16-bit routine pointers, including `ofs_000_C346_00`, `ofs_000_C395_01`, `ofs_000_C99A_02`, `ofs_000_CB2C_03`, and other state-specific targets. This is evidence of table-driven dispatch, but the routine meanings must not be guessed from the labels alone.
- `bank_val.inc` defines NES controller masks: Right `$01`, Left `$02`, Down `$04`, Up `$08`, Start `$10`, Select `$20`, B `$40`, A `$80`. The native 3DS HID key masks are different, so inputs must be translated semantically, not copied numerically.
- `bank_ram.inc` documents extensive zero-page scratch aliases such as `ram_0000_temp` and `ram_0001_temp`; the same addresses are reused for different temporary roles at different ROM offsets. Do not map these aliases directly to persistent C fields without tracing their lifetimes.
- `ram_00F3_bike_speed_sound` is explicitly described as an engine-sound state that rises from `$00` toward `$2F` while not heated, jitters at `$2E/$2F` at maximum speed, and is set to `$30` at ROM offset `$003ACF` when B is held while the engine is heated. This is an audio-related state, not confirmed as the bike's primary speed variable.
- The initial routine-pointer table and RAM comments alone do **not** establish exact acceleration, heat, jump, collision, or course logic. Those routines must be traced in the disassembly before a faithful implementation is claimed.

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
The code is architecture scaffolding, not playable or faithful gameplay. The source analysis has identified input constants, a table-driven routine dispatcher, and an audio-related speed/heat state, but has **not** yet mapped the core gameplay routines and persistent RAM variables with enough confidence to finish Phase 2. Do not start implementing supposedly authentic physics from guesses.
