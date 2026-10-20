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


## First native exact-translation unit: `nesSubDA15DerivedY`

Implementation: `3ds-port/include/nes_routines.h` and `3ds-port/source/nes_routines.c`.

- The function `nesSubDA15DerivedY(ram0390, ram0080, ramY)` translates the instruction sequence at CPU `$DA15` byte-by-byte.
- It explicitly preserves the no-borrow carry from the first `SBC` for the second `SBC #$08`, and the carry from `ADC ramY` for the final `ADC #$04`.
- Each accumulator write is reduced to 8 bits. It does not use floating-point or unconstrained integer arithmetic to approximate the routine.
- The function is intentionally not wired into `bike_physics.c`: the native scaffold does not yet model the original four object slots or their RAM layout, and the meaning/lifetime of the source fields still needs to be established. Integrating it into the current placeholder model would imply a relationship that has not been proven.

Verification status: source translation committed; CI build and a byte-for-byte exhaustive comparison against an independent 6502 reference calculation are still required. No fidelity claim is made until those checks pass.


Automated regression test: `3ds-port/tests/test_nes_routines.c` implements a separate instruction-by-instruction 6502 reference and compares all `256 × 256 × 256 = 16,777,216` possible input triples. Workflow: `.github/workflows/test-nes-routines.yml`. CI run [39495633829](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39495633829) succeeded; the 3DS build workflow [39495627228](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39495627228) also succeeded.

## Writer inventory: object-state and position RAM

Source scanned: `Excitebike/bank_FF.asm` on branch `nes-game`. This inventory records direct write instructions and their source locations; it is not yet a complete semantic interpretation. Indexed writes use X as the original object slot unless the instruction explicitly uses a fixed address. The surrounding branches and call paths still need to be traced before these fields can be named as physics variables.

| RAM symbol | Direct writers found | Initial interpretation / caution |
|---|---|---|
| `ram_0390_obj,X` | `$D93D` INC; `$D9A7` DEC; `$DA98` STA; `$DE10` STA | State-dependent value is incremented/decremented and also assigned in collision/transition paths. It participates in `sub_DA15`'s derived Y calculation; its gameplay meaning is not proven. |
| `ram_0080_obj` / `,X` | `$C5E3` STA fixed slot 0; `$C8BD` STA indexed; `$DA8C` STA indexed; `$DAF1` STA indexed | Byte is initialized and updated in object setup/movement routines. Do not assume it is bike speed: the source uses it in several object-position and state comparisons. |
| `ram_0094_obj,X` | `$CE37` INC; `$CE55` STA; `$CE7D` DEC; `$DB3E` STA; `$DBAA` clear/reset; `$DD02` INC | Appears in object update and movement paths, and is checked by state `$05`; semantic role remains unresolved. |
| `ram_00DC_obj,X` | `$CF85`, `$D96B`, `$D973`, `$D9ED`, `$DA05`, `$DBBC`, `$E9E1`, `$EA03` STA; fixed-slot stores at `$DDBA`, `$E0DA`, `$EA3B` | Also added to `ram_obj_pos_Y_lo,X` at `$E97B`; can be a signed byte/delta in that path, so do not label it as a generic flag. |
| `ram_00BC_obj,X` | `$DB2A`, `$DB9B`, `$E7E2`, `$E84F`, `$E876`, `$E95E`, `$E9AF`, `$EC87` STA; shared/fixed-slot fields `+$01`/`+$02` also initialized and changed in setup/control paths | Used with Y-position and object-state logic; slot 1–3 entries overlap with shared setup/control fields, so array interpretation must be checked against the RAM map. |
| `ram_0370_obj,X` | `$C89D` fixed slot 0 initialized from X; `$DBC0` indexed STA; `$D9CF`, `$DBF8` fixed slot 3 STA | Gates state `$01`/`$03` handlers and other branches. Its values appear to be lifecycle/state gates, but exact meanings are not assigned yet. |
| `ram_009C_obj,X` | `$D94E`, `$D975`, `$D9C4`, `$D9F3` INC; `$DA12`, `$DBAE`, `$DE1C` STA; fixed slot 0 `$DDC9`; fixed slot 3 `$E0DD` | Confirmed dispatch index in `sub_D924`, selecting the state-handler pointer tables. The writers increment transitions, reset a slot, and initialize selected states; not every write is part of the same race path. |

### Trace conclusions and next analysis gate

- The state index `ram_009C_obj,X` is the strongest currently confirmed state-machine field because `sub_D924` indexes the low/high dispatch tables with it.
- State transitions are written from multiple subsystems (object update, initialization/reset, and race logic); following only the state handlers would miss valid transitions.
- `ram_00DC_obj,X` is directly accumulated into object Y at `$E97B`; preserve 8-bit signed/two's-complement behavior when its producer is translated.
- This scan is instruction-based, not a full control-flow/data-flow proof. Next trace each writer's containing routine and caller, then trace initialization paths and identify when slot 0 is the player versus opponent slots. Do not wire these fields into native bike physics until that work is documented.

## Follow-up trace: position update, opponent reset, and vertical correction

### `sub_DA7A` — CPU `$DA7A`, ROM offset `$001A8A`

- Starts with `X = $03`; processes slots 3, 2, and 1. Slot 0 is deliberately excluded by the loop's `DEX / BNE` exit condition.
- For each slot with `ram_00A8_obj,X != 0`, it adds `ram_0060,X` to `ram_0080_obj,X`, increments `ram_0084_obj,X` on carry, then subtracts the global `ram_0060` and decrements the high byte on borrow.
- In the same loop, `ram_0390_obj,X` is reduced by the global `ram_0060` using 8-bit subtraction.
- This proves that `ram_0080_obj,X` and `ram_0084_obj,X` form a low/high-byte pair updated with carry/borrow in this opponent-only routine. The routine applies a scroll-relative coordinate correction. Do not substitute ordinary signed arithmetic: carry and wrap are observable.
- Since slot 0 is excluded here while slots 1–3 are processed, slot 0 has a distinct update path. This supports—but by itself does not prove for every game state—the hypothesis that slot 0 is the player and slots 1–3 are opponents.

### `sub_DB95_clear_opponent_bike_data` — CPU `$DB95`, ROM offset `$001BA5`

- Explicit comment says `X = 01 02 03`; this routine clears per-slot fields for opponent slots, including `ram_obj_pos_Y_lo,X`, `ram_00BC_obj,X`, `ram_0094_obj,X`, `ram_009C_obj,X`, `ram_00DC_obj,X`, and `ram_0370_obj,X`, among others.
- This is a verified reset path for slots 1–3. It is called by `sub_DAE3`, which then repopulates position/appearance fields for a selected opponent. It explains several direct writers in the RAM inventory as reset/setup writes rather than frame-to-frame physics.
- `sub_DBC4` uses this reset path for slot 3 when its state/position conditions allow it; otherwise its alternate branch respawns slot 3 via `sub_DAE3`.

### `sub_E96C` — CPU `$E96C`, ROM offset `$00297C`

- Iterates slots 3 through 0. Except for object animation states `$14` and `$15`, it adds `ram_00DC_obj,X` to `ram_obj_pos_Y_lo,X` and stores the low byte back.
- The value in `ram_00DC_obj,X` is therefore a per-frame vertical-position delta in this path. The code later changes/clears that value based on Y bounds, object state, and the race-start flag; values `$01` and `$FF` can act as upward/downward one-byte deltas depending on the branch.
- Preserve modulo-256 arithmetic and branch order. It is unsafe to replace this with a continuous floating-point velocity model without proving the equivalent state transitions.

### Updated confidence

- **Confirmed:** `ram_009C_obj,X` selects the `$D924` state-dispatch handler; `ram_00DC_obj,X` is added to Y in `sub_E96C`; `sub_DA7A` performs scroll-relative low/high-byte correction for slots 1–3; `sub_DB95` resets opponent slots 1–3.
- **Strong inference, not yet a global proof:** slot 0 is the player slot and slots 1–3 are opponents. The fixed-slot initialization and multiple opponent-only loops support this, but all race/menu initialization paths still need tracing.
- **Not yet established:** the full gameplay meaning of `ram_0390_obj,X`, the full semantics of `ram_0094_obj,X`, and the timing/conditions connecting these routines in each race state.
### Caller placement and frame-order evidence

- `sub_DA7A` is called at CPU `$DA3E` from the `sub_DA2C` loop, after `sub_DA6A` updates the scroll and before `sub_DBFE` adjusts related scroll state. The caller reaches this section through the `ram_03F2` branch at `$C9BB`; therefore the opponent-coordinate correction is conditional, not an unconditional per-frame update.
- `sub_E96C` is called from the race-update path at `$C9DB`, after `sub_DDD1` and `sub_CD1F`, and before `sub_DE31`, `sub_E836`, and `sub_EA44`. Its vertical delta application has a defined place in the update sequence; translating it elsewhere could change interactions with the following routines.
- The RAM declaration file `Excitebike/bank_ram.inc` lists contiguous byte symbols for the object arrays (for example `$0080`–`$0083`, `$009C`–`$009F`, `$00BC`–`$00BF`, `$00DC`–`$00DF`, `$0370`–`$0373`, and `$0390`–`$0393`). This confirms the array-shaped layout, but the assembly's fixed-address stores still need to be interpreted in the context of which slots are active.



## Translation progress — timer routine

The disassembly contains 177 labels named sub_* in Excitebike/bank_FF.asm. The native C code does not yet translate all of them; this is a multi-stage translation. Each label must be classified as executable behavior, data/table helper, hardware-specific PPU/APU logic, or an alias/branch label before porting.

### Exact translation added: sub_D310 at CPU $D310

- Decrements ram_0020_0A_frm_timer as an 8-bit value.
- Uses the 6502 Negative flag (bit 7 of the decremented result) for BPL. If bit 7 is set, resets the frame timer to $0A and processes all 29 timer bytes at RAM $0021–$003D; otherwise processes the first 15 bytes at RAM $0021–$002F.
- Each selected timer is decremented only if nonzero.
- Native helper: nesSubD310DecreaseAllTimers in source/nes_routines.c.
- A separate reference test compares 65,536 frame-timer/value vectors with generated timer-array patterns. CI must pass before this is considered validated.

This helper models the RAM/timing routine only; it does not recreate the NES's interrupt cadence or call schedule.


## Master routine index

A full index of all 177 `sub_*` labels found in `Excitebike/bank_FF.asm` is maintained in [`ASM_FUNCTION_INVENTORY.md`](ASM_FUNCTION_INVENTORY.md). It is a checklist, not a claim that every label is a standalone routine or already ported. The current exact native helper set is small; each subsequent function will be implemented in dependency order and tested against a separate 6502 reference wherever practical.

### Latest exact routine: `sub_D310`

Host regression workflow [39496864809](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39496864809) passed after adding the byte-level timer helper and 65,536 input vectors. The matching 3DS build is still running at the time of this documentation update.


### Exact translation added: `sub_D326` at CPU `$D326`

- The routine compares bit 1 of RAM `$0018` and `$0019`; if those bits differ, it sets the carry flag.
- It then performs `ROR` in sequence across RAM `$0018–$001F`, with each byte's low bit feeding the next byte's high bit.
- Native helper: `nesSubD326RotateInputBits`.
- Regression test compares the implementation against an independent carry-chain reference over all 65,536 combinations of the first two bytes and generated patterns for the remaining six bytes. CI must pass before marking the helper validated.


### Exact RAM-level translation added: `sub_D35B` at CPU `$D35B`

- The physical controller reads from `$4016,X` remain platform-specific and are not copied into the 3DS port.
- The portable helper accepts the eight sampled bytes and reproduces the 6502's `LSR` / `ORA` / `LSR` carry extraction followed by eight `ROL` operations.
- It writes the reconstructed NES button mask to hold and press state. If A is set in both the new and previous hold masks, it clears A only from the press mask, matching the source branch.
- Native helper: `nesSubD35BDecodeController`. Regression test covers every decoded 8-bit button pattern, every previous hold mask, and three active-sample encodings. CI must pass before marking it validated.


### Latest validation snapshot

- `sub_D310`: host regression passed in [run 39496864809](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39496864809).
- `sub_D326`: host regression passed in [run 39497063113](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39497063113).
- `sub_D35B`: host regression passed in [run 39497203936](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39497203936).
- The 3DS build workflow for the newest source changes is still running; do not mark the new helpers fully validated until it completes.


### Exact RAM-level translation added: `sub_DA7A` at CPU `$DA7A`

- The assembly initializes X to `$03` and loops downward until X reaches zero, so only slots 3, 2, and 1 are processed; slot 0 is excluded.
- A slot is skipped when `ram_00A8_obj,X` is zero.
- For an active slot, it adds `ram_0060,X` to the low coordinate byte `ram_0080_obj,X`. The low-byte addition wraps at 8 bits and increments `ram_0084_obj,X` when the addition carries.
- It then subtracts the global byte `ram_0060` from that low-byte result. If this subtraction borrows, it decrements `ram_0084_obj,X`. This order matters: the increment and decrement are separate operations and may cancel.
- Finally, it subtracts the same global `ram_0060` byte from `ram_0390_obj,X`, with 8-bit wraparound.
- Native helper: `nesSubDA7AUpdateScrollSlots`. Its regression test compares it with a separate instruction-level reference over 65,536 deterministic patterns. This helper does not claim to implement the full race/scroll system; callers and global state remain separate work.


## Additional exact native translations added on 2026-10-20

These helpers preserve the original byte operations. They do not replace callers or imply that the whole race system is implemented.

- **`sub_DBFE` (`$DBFE`)**: performs `LSR ram_0060`, three consecutive `ROR` operations on the accumulator, `CLC/ADC ram_03BF`, then `ADC ram_0050_scroll_X` using the carry from the previous addition. It toggles bit 0 of `ram_004E_base_nametable` only when the final scroll addition carries. Native helper: `nesSubDBFEUpdateScroll`.
- **`sub_DC97` / `sub_DC9A` (`$DC97/$DC9A`)**: the entry at `$DC97` loads `ram_03F1`; the shared arithmetic at `$DC9A` executes `SEC; SBC ram_00BC_obj,X; SBC ram_obj_pos_Y_lo,X`. The second SBC consumes the first SBC's carry/no-borrow flag. Native helper `nesSubDC9A` models the combined load-and-arithmetic path.
- **`sub_DCDE` (`$DCDE`)**: if `ram_0374_obj,X` is nonzero, decrements it first; then, only when `ram_005C_obj,X & $C0` is nonzero, clears the timer. Native helper: `nesSubDCDE`.
- **`sub_DD38` (`$DD38`)**: sets `ram_00B0_obj,X=$02` and `ram_0380_obj,X=$0F`; adds `$00AF` to the two-byte value `ram_0094_obj,X:ram_0090_obj,X` with 6502 carry propagation; when `ram_0388_obj,X==$02`, shifts the two-byte result right with LSR/ROR; for X=0 only, computes `ram_00FF` from four ASLs of `ram_0094_obj`, EOR `$30`, and the zero-result fallback `$08`. Native helper: `nesSubDD38SetupObject`.
- **`sub_DD6F` (`$DD6F`)**: preserves the incoming carry as an explicit argument because the first ADC has no preceding CLC. It updates `ram_0380_obj,X`, propagates carry through `ram_0384_obj,X`, performs the SBC against `ram_037C_obj,X`, then the final ADC. Native helper: `nesSubDD6F`.
- **`sub_DB95_clear_opponent_bike_data` (`$DB95`)**: clears exactly the 20 RAM fields written by the assembly, for X=1,2,3 only; X=0 is intentionally untouched. Native helper: `nesSubDB95ClearOpponentBikeData`.

Regression tests compare these helpers against independent instruction-level references or exact RAM-field expectations. Hardware-specific calls and higher-level caller state remain outside these helpers.


## Additional routine translations — 2026-10-20

- **`sub_DA6A` (`$DA6A`)**: adds RAM[$0060] to `ram_scroll_X` with 8-bit wrap. It toggles bit 0 of `ram_004D_base_nametable` only on carry.
- **`sub_DB50` (`$DB50`)**: table-driven object setup. Uses the original `tbl_D8D5` and `tbl_D8CD` bytes supplied by the caller. The X=3 special path preserves the exact bit-1 test on `ram_0084_obj,X`, the player/object position comparison, and the early RTS before the final increment.
- **`sub_DCA0` (`$DCA0`)**: chooses the indexed `tbl_D88B` value, except for the exact `track_finished == 1` and all-zero player/object-index condition; `ram_00A4_obj,X == 1` overrides the result with `$06`.
- **`loc_DCC7` (`$DCC7`)**: shared non-label-prefixed entry takes target in A and duration in Y. If the indexed timer is nonzero, it returns without changes. Otherwise it stores the duration and moves `ram_00AC_obj,X` one unsigned byte step toward the target, with the original DEC/DEC/INC sequence retained for the greater-than case.

The host tests exercise byte boundaries and branch cases. These helpers remain isolated translations; the complete caller paths and game-state integration are still in progress.


- **`sub_DA58` (`$DA58`)**: copies `ram_0094_obj,X` to `ram_0060_obj,X`, adds `ram_0090_obj,X` to `ram_0394_obj,X` modulo 256, and increments the copied `ram_0060_obj,X` byte only when that addition carries. The X index is limited to 0–3 in the C helper.
- **`sub_E091` (`$E091`)**: returns A unchanged for A >= `$E7`; otherwise adds `$0C`. The CMP carry is clear on the ADC path, so the addition is exactly +12 modulo 256.
- **`sub_E098` (`$E098`)**: same threshold as `sub_E091`, but adds `$02` when A < `$E7`.


- **`sub_DFA4` (`$DFA4`)**: stores incoming X as the divisor, clears X, then repeatedly increments X and executes `SEC; SBC divisor` until the subtraction result has N=1 (the `BPL` loop condition). It then decrements X and performs `ADC divisor` using the carry left by the terminating SBC. The C helper exposes both final X and A and deliberately preserves byte wrapping and the N-flag termination condition; divisor zero is excluded because the original routine would not terminate for ordinary inputs.
- **`sub_DFA2` (`$DFA2`)**: loads X with `$0A` and falls through to `sub_DFA4`; represented by the same exact helper with divisor `$0A`.


- **`sub_D0AB` (`$D0AB`)**: processes X=3 down to 0; subtracts `$10` from each Y-position byte, then repeatedly decrements Y and subtracts `$08` while the resulting N flag is clear, stopping if Y reaches zero. Stores the final Y in the corresponding `ram_0360` slot.
- **`sub_DFB2` (`$DFB2`)**: if `ram_009C_obj,X` is nonzero, stores zero. Otherwise scans `tbl_D913` using the 6502 CMP result's N bit, resets Y to zero after five entries, and adds two to a nonzero Y only when `ram_00A4_obj,X == 1`. The table is passed in as the original five bytes.


## Batch 2026-10-20 — additional routine translations

- `sub_D000` ($D000): exact gate for the jump to `sub_DB50`; returns whether the call occurs and the Y value, including INY only when `ram_001A,X == $C0`.
- `sub_D11F` ($D11F): exact byte writes to RAM $0300-$0312, including the X countdown and its final zero.
- `sub_D255` ($D255): writes $F8 to every byte of OAM; the 8-bit Y loop covers all 256 bytes.
- `sub_D2FF` ($D2FF): models INX/CMP $3F and the overflow path's RAM clear and two stack-byte discards.
- `sub_E70B` ($E70B): reproduces SBC result N/Z branches and the six-bit wrap of `ram_00E0,X`.
- `sub_E80D`, `sub_E803`, `sub_E7FF`, `sub_E7FC`: the six-page pointer lookup and each successive entry point, with the original Y/data-index behavior.
- `sub_E836` ($E836): processes X=3 down to 0, skips nonzero `ram_00B0,X`, and delegates the subtraction to the shared `sub_DC97/sub_DC9A` model.


## Lot suivant — 10 entrées ASM modélisées

- `sub_E16C`: writes OAM at offsets 6, 5, 2 and 1.
- `sub_E3F4`: sort four object indices by unsigned Y, with the slot-3 Y override.
- `sub_E4AD`: hide sprite Y bytes at strides of four.
- `sub_E49B`: bit-0-gated sprite-group hiding.
- `sub_E2AF`: low-byte subtraction with borrow into high byte.
- `loc_E2BD`: finish blink toggles only for finish flag 1 and zero timer.
- `loc_E2D0`: DEC-Y equality test.
- `sub_E2F6`: wheel-sprite gate conditions.
- `sub_E1F9`: state/return selection and temporary side effect.
- `loc_E3AA`: heat thresholds; prior Y is an explicit input because ASM preserves it.


## Object overlap analysis — $E4C8 / $E52C

- `sub_E4C8` scans candidate slots X=1 then X=0, and considers Y slots above X only. Eligibility follows the original LSR carry / nonzero branches and the `$F8` Y-position boundary. It counts overlapping Y coordinates and packs their indices into scratch byte `$000B`; that byte is intentionally modeled as input/output because the ASM comments explicitly identify it as uninitialized by this routine.
- `sub_E52C` computes the wrapped 8-bit difference `ram_008C[X] - ram_008C[Y]` after SEC/SBC. It returns that difference only for values `$EC-$FF` or `$00-$11`; the middle interval returns zero.
- The C helper is a byte/register-level translation. Its test covers the wrapped-difference boundary, a two-candidate packed pair, and the no-candidate path. This does not imply that the entire sprite renderer is integrated into the game loop.


**Correction from instruction-level review:** `sub_E4AD` writes `$F8` at offsets 0, 4, and 8, then stops when Y reaches `$0C`; despite its label, it writes three Y bytes per call.


## Sprite culling — $E456 / $E49B / $E4AD

- `sub_E456` iterates sorted object slots in reverse order, skips slot zero, seeds three culling flags from `ram_0084,X`, and either clears opponent-bike RAM for a Y position in `$70-$8F` when bit 0 is clear or adjusts group flags at Y thresholds `$F0/$F8`.
- `sub_E49B` tests bit 0 of each of three group flags; each clear flag invokes `sub_E4AD`.
- `sub_E4AD` forms the OAM byte offset by adding the selected `tbl_D8DC_spr_index` entry to the OAM base and writes `$F8` at offsets 0, 4, and 8, then exits when Y reaches `$0C`. The routine label says “hide 4 sprites”, but the instruction loop writes exactly three Y bytes per call.


## Special object sprite preparation — $E26A

- Models the original early-return gates: `$039C+$03` nonzero, `ram_0084,X != $02`, `ram_0390,X < $F8`, and `ram_0390,X >= ram_0080,X`.
- On the taken path, preserves the 8-bit `SEC/SBC` result `$A8 - ram_006C,X`, table byte from the three-entry `$D90B` table, `$22` attribute, OAM base, OR of the existing `$000B` scratch value with `$20` when `ram_00A4,X` is nonzero, and the Y-index XOR `$03` when Y is nonzero. The final `loc_D1C7` renderer remains a separate dependency.


## Validation clarification

The latest batch CI confirms the host regression suite and native 3DS build pass. This proves compilation and the covered test cases only. `sub_E1F9`, `sub_E2F6`, and `loc_E3AA` remain partial/state-level models; they must not be described as complete instruction-by-instruction ports until their remaining branches, memory writes, and call interactions are implemented and tested.


## Temperature meter — $E359

- Implements the `$D8F7/$D8FB` table-driven fractional/coarse temperature update, including byte carry/borrow, cooling subtraction `$0B`, the finish/latch blink paths, overheat timer/latch writes, `$00FC` flag OR, and the four sprite records written to OAM `$02F0-$02FF`.
- The helper exposes the source RAM values as parameters; it does not claim to reproduce the rest of the full game loop or the surrounding PPU timing.


## Wheel sprites — $E2F6

- Expanded the previous precondition-only model into the full OAM-writing path. It preserves the gate order, byte-wrapped X offsets `$F9/$F8`, Y offsets `$08/$08`, tile swap controlled by `ram_003F & $04`, attribute bit `$02`, and the second-wheel hide write `$F8` when `ram_0094,X` is zero.


## Wheel and overheating sprites — $EA44

- Implements the X=3..0 dispatch and its exact `$F4/$F6` position thresholds, bit tests on `ram_0084,X`, the `$B0,X` special-sprite gate, `$B6`-based chained SBC through `sub_DC9A`, the seven-byte `$E69A` tile table, and fallback to the complete `$E2F6` wheel renderer.
- The scratch operand consumed by the shared SBC helper is passed explicitly as `ramY[X]`, because the original routine inherits it from shared zero-page state rather than initializing it locally.


## Object sprite selection — $E1F9

- Replaced the earlier partial approximation with the literal branch sequence through RTS: resets `$000E` and `$039F`, handles the `$009C` fast path and timer-dependent return, finish-state special case, `$0098/$00B0` gate, `$0058/$00B8` gate, `$0090|$0094` temporary increment, and `$00AC == $06` orientation/flag selection. The return value uses `$0036,X - 1` when the timer is nonzero, matching the actual `LDY ram_0036_timer,X / DEY / TYA` sequence.


## Finish countdown — entry $E2AF / labels $E2BD and $E2D0

- Adds a combined state transition for the code beginning at `$E2AF`: low-byte subtract and borrow into `$03BE`, finish-flag/timer blink behavior, exact early RTS for finish values other than 0/1, and the `$03BE == 1` countdown sprite command (`$50`, `$32`, OAM base `$C0`, and `$D8E5` tile). Return code 2 explicitly represents the original branch into the shared `$E2F6` tail rather than pretending it is an independent RTS helper.


## Object sprite setup — $E1A3

- Translates the caller-side parameter setup around `sub_E1F9`: loads `$0080,X/$008C,X`, performs the 23-entry `$D892/$D8A9` lookups, applies the `$16` special case (save OAM base to `$039F`, add `$0C` to OAM pointer and `$08` to Y, change attribute `$33` to `$23`), and writes the exact conditional overlay tile bytes for `$000E` values 1 or 2.
- The call into `loc_D1C7` remains represented by the returned render-type and command fields; the renderer itself is still a separate dependency.
