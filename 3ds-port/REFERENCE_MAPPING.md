# Excitebike 3DS: fidelity and source mapping

## Fidelity target
Reproduce the original NES game's observable behaviour as closely as practical: acceleration, braking, overheating/cooling, ramps, airborne posture, landing, crashes/recovery, opponents, course layouts, menus, scoring, and timing.

## Technical constraint
The original `../Excitebike/` source is a 6502/NES disassembly. Its CPU instructions, memory map, PPU/APU registers, interrupt assumptions, and NES-specific graphics routines cannot be compiled directly into a native ARM11/libctru application. A native port must translate the original logic and data to 3DS-side code. Embedding a cycle-accurate NES emulator is a separate approach, not a direct native port.

## Fidelity workflow
1. Preserve the original disassembly and its history.
2. Map original routines and RAM variables to native modules. Record labels, addresses, inferred purpose, and confidence.
3. Translate behaviour and data structures rather than blindly copying 6502 instructions into C.
4. Validate each subsystem against the original with repeatable input sequences and recorded results.
5. Document deviations; do not call a subsystem faithful before comparison.

## Module map
| Module | Responsibility | Current status |
|---|---|---|
| `game_state.c` | Shared state and reset | Initial state container |
| `input.c` | Sample 3DS controls | Basic input sampling |
| `bike_physics.c` | Speed, heat, posture, jumps and landing | Temporary placeholder; not faithful |
| `collision.c` | Track bounds, obstacles and crashes | Temporary boundary placeholder |
| `track.c` | Course data, terrain and ramps | Interface only |
| `renderer.c` | 3DS display and debug view | Console output only |
| `audio.c` | Sound events and audio backend | Interface only |
| `menus.c` | Menus, pause and help | Minimal interface only |
| `game.c` | Module lifecycle and update order | Initial orchestration |

## Current status
The code is architecture scaffolding, not playable or faithful gameplay. Speed/heat/position updates are temporary test behaviour. Before replacing them, inspect the original disassembly and map the relevant routines and state with evidence.
