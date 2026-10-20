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


## Detailed trace — temperature-meter update and frame call order

Primary source: `Excitebike/bank_FF.asm`, `sub_E359_display_temperature_meter_with_sprites`, CPU `$E359`, ROM offset `$002369`. This section records the actual branch conditions and byte operations; labels such as “heat” are not substituted for the underlying arithmetic.

### Entry conditions and index selection

1. Read `ram_0052_track_finished_flag`. If nonzero, skip the accumulator update and go directly to the sprite-bar path at `$E3C6`.
2. Otherwise read `ram_03E0_obj`. If nonzero, read `ram_003C_timer`; shift it right once. If the shifted-out low bit is 1, branch to `$E3C6`. If it is 0, write `$FC` to the temporary sprite tile value and jump to `$E3C8`. This is a timer-controlled alternate display path; the code does not update `$03B5/$03B6` on this branch.
3. If `ram_03E0_obj == 0`, compute `ram_005C_obj & $C0`. If the result is zero, the table index is 0. Otherwise load `ram_004F_race_started_flag`; if that value is zero, replace the accumulator with `$80`; then execute `ASL; ROL; ROL` and transfer the result to Y. The precise index follows these 6502 flag operations; a C translation must emulate 8-bit carry semantics rather than replace them with an intuitive button mapping.

### Accumulator update

Tables at CPU `$D8F7` and `$D8FB` contain these bytes:

| Index | `tbl_D8F7` increment | `tbl_D8FB` comparison threshold |
|---:|---:|---:|
| 0 | `$3F` | `$08` |
| 1 | `$0F` | `$20` |
| 2 | `$07` | `$11` |
| 3 | `$07` | `$11` |

The routine compares `ram_03B6` with `tbl_D8FB,Y` as an unsigned byte:

- If `ram_03B6 < threshold`, add `tbl_D8F7,Y` to `ram_03B5` modulo 256. If this addition carries, increment `ram_03B6` by one.
- If `ram_03B6 == threshold`, do not change either accumulator in this section.
- If `ram_03B6 > threshold`, subtract `$0B` from `ram_03B5` modulo 256. If the subtraction does not borrow, leave `ram_03B6` unchanged. If it borrows and `ram_03B6` is nonzero, decrement `ram_03B6`; if it is zero, leave it at zero.

### Danger state and display

After the accumulator branch, the routine reads `ram_03B6`:

- At values `>= $20`, it copies that value into `ram_003C_timer` and `ram_03E0_obj`.
- At values `>= $1A`, it decrements Y; only if that makes Y zero does it set X to `$B0`. It ORs X into `ram_00FC`. The assembly comment identifies the `$E3BF` breakpoint as “overheat danger while holding B”; the exact branch depends on the computed Y, so it must not be generalized to every state at that threshold.
- The bar sprites are then generated in a loop from X=`$0C` down by four to zero. Each iteration writes Y=`$CF`, the current tile value, attribute `$20`, and X position to the OAM sprite slots at `$02F0+X` through `$02F3+X`; the horizontal position increases by 8 per iteration. The initial horizontal coordinate is `ram_03B6 + $50`. The disassembly comment says the bar remains full size and moves behind the HUD background.

### Confirmed call order in the race update path

At CPU `$C99A` (ROM offset `$0009AA`), the routine calls `sub_CA08`, then returns early through `$C997` if pause flag `ram_pause_flag` or `ram_pause_timer` is nonzero. Otherwise it calls, in order, `sub_D918`, `sub_DD8D`, `sub_E733`, `sub_E927`; checks `ram_0052_track_finished_flag`; when the track is not finished it may call `sub_DA9F`, `sub_DBC4`, `sub_DEBB` depending on `ram_03F2`, then calls `sub_DF30`, `sub_EAC5`, `sub_DFD5`, `sub_E09F`. The shared tail calls `sub_DDD1`, `sub_CD1F`, `sub_E96C`, `sub_DE31`, `sub_E836`, `sub_EA44`, `sub_F4FF`, `sub_E4C8`, `sub_DEE2`, `sub_E359`, `sub_E17F`, `sub_E456`, `sub_DA26`, `sub_E70B`, `sub_D19B`, `sub_CED0`, and jumps to `loc_E42B`. These names and ordering are observed; the purpose of every called routine is not yet fully established.

### Fidelity constraints

- `ram_03B5` and `ram_03B6` are the two bytes directly updated by this routine. Do not replace the byte arithmetic with floating-point temperature values.
- The routine itself does not establish a complete acceleration or cooling model, nor does it prove the exact frame in which a crash occurs. Those require tracing callers, all writers of these RAM bytes, and related state transitions.
- The current native `bike_physics.c` is still placeholder code and has not been changed to pretend to implement this trace. Implement only after input-to-index mapping, all relevant writes, and reproducible NES comparison cases are established.


## Detailed trace — per-object state dispatch and horizontal object update

### `sub_D918` / `sub_D924` (CPU `$D918` / `$D924`, ROM `$001928` / `$001934`)

Confirmed control flow:

- `sub_D918` initializes X to `$03`; for X = 3, 2, 1, 0 it calls `sub_D924`, then `sub_DA15`, and decrements X. Thus these two operations are performed for all four object slots, descending.
- `sub_D924` loads `ram_009C_obj,X` into Y, loads the low byte from `tbl_D8FF_lo,Y` and high byte from `tbl_D905_hi,Y`, stores the pointer in zero-page scratch `ram_0000_t06_jmp`/ `ram_0001_t06_jmp`, then performs an indirect JMP. This is a state-indexed dispatch table, not a conventional C switch with yet-known state meanings.
- The entry at dispatch target `$D933` checks `ram_0370_obj,X`. If zero, it increments `ram_009C_obj,X` and continues into the state-transition setup at `$D97E`. If nonzero, it checks bit 0 of `ram_003F`; when that bit is set it returns without the per-frame increment. Otherwise it increments `ram_0390_obj,X`, looks up `tbl_D90E` using `ram_03E4_obj,X`, adds `ram_0080_obj,X`, and compares that sum with `ram_0390_obj,X`; it returns if the sum is greater than or equal to the latter, else it advances `ram_009C_obj,X`.
- The state number alone is insufficient to name this behaviour: the state-pointer table and every target must be mapped before translating all states.

### `sub_E733` / `sub_E73B` (CPU `$E733` / `$E73B`, ROM `$002743` / `$00274B`)

Confirmed control flow:

- `sub_E733` initializes X=`$03`, calls `sub_E73B`, decrements X, and repeats while X is nonzero. It therefore calls the routine for slots 3, 2, and 1; slot 0 is not processed by this loop.
- `sub_E73B` first performs 8-bit addition `ram_obj_pos_X_lo,X += ram_0060,X`.
- If `ram_0058_obj,X` is nonzero, execution enters the table-driven sequence handler at `$E762`.
- If `ram_0058_obj,X == 0`, it calculates `(ram_00C0_obj,X - $40)` as an 8-bit subtraction. If the result has the negative flag set, it returns. Otherwise it shifts right twice; if the result is `>= $16`, it returns. For values below `$16`, it stores the result in `ram_0058_obj,X`, increments that state byte, clears `ram_00C4_obj,X`, sets `ram_obj_pos_X_lo,X` to `ram_0064,X - 1` (8-bit), then enters the table-driven handler.
- The handler chooses low/high pointers from `tbl_E55A_lo - 1,Y` and `tbl_E56F_hi - 1,Y`, where Y is `ram_0058_obj,X`; reads the sequence byte at index `ram_00C4_obj,X`; and terminates the sequence when the byte is `$FF`. Otherwise it compares the sequence coordinate with `ram_obj_pos_X_lo,X`. Depending on that comparison and the sequence byte's flag bits, it either returns for this frame, dispatches through `tbl_E6B7`, or updates object-state fields such as `ram_00AC_obj,X` and `ram_00D4_obj,X`. A full interpretation of those sequence encodings requires tracing the target handlers and table data.
- These are object-position/state-sequence operations. They are not yet proof of the player's acceleration formula, nor grounds to reuse them as native bike physics without mapping object indices and callers.

### Analysis gate

The next exact-analysis targets are the dispatch pointer tables `tbl_D8FF_lo`/`tbl_D905_hi`, every state handler reachable from them, the consumers/writers of the player slot's `ram_0060`/ `ram_0090_obj`/ `ram_0094_obj`, and the transition logic around `ram_005C_obj`, `ram_00BC_obj`, and `ram_0370_obj`. Until that data flow is traced, acceleration, jumping, and collision implementation remains blocked.

## Dispatch table values — `tbl_D8FF_lo` / `tbl_D905_hi`

The two tables are parallel arrays of low/high address bytes. Reading both directly from the disassembly yields:

| `ram_009C_obj,X` | Target CPU address | Label |
|---:|---:|---|
| `$00` | `$D923` | `ofs_003_D923_00_RTS` |
| `$01` | `$D933` | `ofs_003_D933_01` |
| `$02` | `$D953` | `ofs_003_D953_02` |
| `$03` | `$D983` | `ofs_003_D983_03` |
| `$04` | `$D9D3` | `ofs_003_D9D3_04` |
| `$05` | `$D9F6` | `ofs_003_D9F6_05` |

These are confirmed pointer values, not semantic names for the states. The index is used directly as Y in both lookup tables; no range check is performed in `sub_D924`, so the valid range must be established from every writer of `ram_009C_obj,X` and the surrounding state machine before a native translation is considered safe.

Known details from the visible handlers:
- State `$00` jumps to an immediate RTS.
- State `$01` uses `ram_0370_obj,X`, `ram_003F`, `ram_0390_obj,X`, `ram_03E4_obj,X`, `ram_0080_obj,X`, and `tbl_D90E` as described above.
- State `$02` compares `ram_obj_pos_Y_lo,X` with `tbl_D8C4,Y`; depending on the comparison it writes `$01` or `$FF` to `ram_00DC_obj,X`. On equality it clears `ram_0398_obj,X` and `ram_00DC_obj,X`, increments `ram_009C_obj,X`, and sets an 8-frame timer in `ram_002A_timer,X`; for object slot 0 it also sets `ram_00FD` to `$02`.
- States `$03-$05` still need their full entry-to-exit data-flow trace before assigning higher-level meanings.

Do not map these object states to “jump”, “crash”, “landing”, or other gameplay concepts solely from their state number or neighboring code.

## Expanded state-handler trace — states `$02-$05`

Source: `Excitebike/bank_FF.asm`, dispatch from `sub_D924`, handlers at CPU `$D953`, `$D983`, `$D9D3`, `$D9F6`.

### State `$02` — target `$D953`

- Set Y to X. If X is nonzero and `ram_00BC_obj,X` is nonzero, replace Y with `$04`.
- Compare `ram_obj_pos_Y_lo,X` with `tbl_D8C4,Y`.
- If unequal, write `$01` to `ram_00DC_obj,X` when the Y position is below the table value; otherwise write `$FF`. Return immediately.
- If equal, clear `ram_0398_obj,X` and `ram_00DC_obj,X`, increment `ram_009C_obj,X`. If X is zero, set `ram_00FD = $02`. Set `ram_002A_timer,X = $08`, then return.

This documents a comparison/state-transition handler. The disassembly alone in this block does not establish a semantic name such as landing or collision.

### State `$03` — target `$D983`

- If `ram_0370_obj,X == 0`, branch to the transition tail at `$D9C0`.
- Otherwise, if `ram_002A_timer,X != 0`, return.
- Compute `X OR ram_03F7`. If nonzero, skip the button-sampling branch. If zero, save the previous `ram_0049` in Y, compute `ram_btn_hold AND $C0`, and update `ram_0049` according to the branch at `$D998-$D99F`. If the prior value was zero in the qualifying branch, decrement `ram_0390_obj,X`.
- Read `ram_003F AND $0F`. If zero, decrement `ram_0390_obj,X`. Then test its low three bits; if zero, toggle bit 0 of `ram_0398_obj,X`.
- Compute the 8-bit value `ram_0390_obj,X - $08` and compare it with `ram_0080_obj,X`. If the result is greater than or equal to that value under the 6502 CMP/BCS semantics, return.
- Otherwise, or if the initial `ram_0370_obj,X` was zero, set `ram_0036_timer,X = $03` and increment `ram_009C_obj,X`. For X=3 only, also clear `ram_03A8` and `ram_0370_obj+3`.

The precise signed/unsigned meaning of these variables is not inferred here; branches and byte operations are what the code proves.

### State `$04` — target `$D9D3`

- If `ram_0036_timer,X` is nonzero, return.
- For X=0: if `ram_03E0_obj` is nonzero, clear `ram_009C_obj,0` and return via `$DA10`. Otherwise, if `ram_03F7` is zero, increment `ram_009C_obj,0`; if nonzero, jump to `loc_CA85_clear_0021_00FF`.
- For X nonzero: if `ram_0022_timer` is nonzero, return. Otherwise set `ram_00DC_obj,X = $01`, set `ram_0022_timer = $18`, increment `ram_009C_obj,X`, and return.

### State `$05` — target `$D9F6`

- If `ram_0094_obj,X OR ram_00DC_obj,X` is nonzero, enter the state-reset path.
- If both are zero, compare `ram_obj_pos_Y_lo,X` with `tbl_D8C4,X`; equality returns without changing the state.
- On the reset path, set `ram_00DC_obj,X = $01`. If X=0, clear `ram_009C_obj,0` immediately. For X nonzero, if `ram_obj_pos_Y_lo,X < $08`, return with the state unchanged; otherwise clear `ram_009C_obj,X`.

### Shared derived-position routine — `sub_DA15`

Called immediately after the dispatch handler for each X from 3 down to 0. Its arithmetic is:

1. `A = ram_0390_obj,X - ram_0080_obj,X - $08`, using 6502 SEC/SBC carry semantics (the second SBC consumes the carry from the first subtraction).
2. `ASL A` (8-bit shift, carry out discarded by the subsequent CLC).
3. `A = A + ram_obj_pos_Y_lo,X + $04`, with 8-bit result and the carry from the addition to `ram_obj_pos_Y_lo,X` included by ADC.
4. Store the final byte in `ram_006C_obj,X`.

A faithful C translation must model every intermediate as an 8-bit value and preserve the carry/borrow between the two SBC instructions. Rewriting this as an unconstrained integer expression would not be equivalent for underflow/overflow cases.

### Remaining proof obligations

The handlers above still depend on variables whose writers and meaning are not completely mapped. Before turning them into native gameplay, trace the producers of `ram_0370_obj,X`, `ram_0390_obj,X`, `ram_0080_obj,X`, `ram_0094_obj,X`, `ram_00DC_obj,X`, and `ram_00BC_obj,X`, and confirm which object slot is the player under each race state. Do not add guessed labels or physics behavior.
