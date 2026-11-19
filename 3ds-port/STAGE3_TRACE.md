# Stage 3 trace — first course entry (started 2026-11-18)

Status: **partial native physics slices implemented and host-differential-tested; no playable race yet.** The trace below is read from `Excitebike/bank_FF.asm`; nothing is inferred from slot numbers or generic physics.

## Entry
- Track selector (state 1) confirm → `sub_C3AF` INC state → state 2 (`tbl_C000` → `ofs_000_C99A_02`), with `ram_0043` = selected track, `ram_03F2` = 1 for the selection from the title menu (design-menu path uses states 8→0A).
- Setup list for state 2 (`tbl_C01C_index` = 2 → `tbl_C02A_lo/tbl_C033_hi`), six commands run by `sub_C2A9` one per frame (`ram_0044`):
  1. `ofs_001_C551_02_01`: `ram_0045 = $12 + ram_0043` when `ram_0041 < 8` (PPU list $12–$16, palette per track), else $12.
  2. `ofs_001_C869_02_02`: clear `$004D–$00EF` (X from $EF down to $4D).
  3. `ofs_001_C820_02_03`: `sub_D13F` (`sub_ECE4` for X=0,4,8), `ram_0045=1`, `ram_004B=0`, clear `$0340–$03EF`, fill `$0400–$047F=$3B`, `$0480–$057F=$3D`, sprite 0 `Y=$BF,T=$FF,X=$F7`, `ram_00E8=$3F, 00E9=$1F, 00EB=$0A, 00EA=$25`.
  4. `ofs_001_C5A3_02_04`: `ram_0045 = $17`.
  5. `ofs_001_C566_02_05`: `ram_0045 = $0E`.
  6. `ofs_001_C875_02_06`: `INC ram_0047, INC ram_0040`, `ram_0034_timer` default 5, `E8=$34, E9=$14`, `03F1=$A0`, `03B4=03B0=$80`, `03BD=$FF`, `0370=1`, `03BE=2`, then per-object init loop from `tbl_C0B4/C0B0/C0B8` (X=3, or 0 when `ram_03F2==0`) — remainder not yet read.
- Race frame handler `ofs_000_C99A` (states 2/5/0A): `sub_CA08` (4× `sub_DCA0`+`sub_DFB2`, pause logic, `ram_005C = ram_btn_hold`), then unless paused: `sub_D918, sub_DD8D, sub_E733, sub_E927`, finish test `ram_0052`, then `sub_DA9F/DBC4/DEBB` (when `ram_03F2≠0`), `sub_DF30, sub_EAC5, sub_DFD5, sub_E09F`, `sub_DDD1, sub_CD1F, sub_E96C, sub_DE31, sub_E836, sub_EA44, sub_F4FF, sub_E4C8, sub_DEE2, sub_E359, sub_E17F, sub_E456, sub_DA26, sub_E70B, sub_D19B, sub_CED0, loc_E42B`.

## Native bike-speed translation — first source-verified slice (2026-11-19)

This is a narrow native implementation, not a complete physics port.

### Confirmed source behavior

- `sub_CA08` updates four object/control slots through `sub_DCA0` and `sub_DFB2`, then stores the controller's held byte from `ram_btn_hold` at `ram_005C_obj` (zero-page address `$005C`, object slot 0).
- `sub_CD1F` iterates object slots X=3 down to X=0 and calls `sub_CD59`. The X=0 path is the player-controlled path; it receives the controller byte stored at `$005C`.
- `sub_D14E` sets `ram_004C = ram_003F & $03`. Therefore `ram_004C` is a four-phase update selector, **not** the player object index. The player speed step at X=0 is eligible only on phase 0.
- The player's fixed-point speed is stored as low/fraction byte `ram_0090_obj` and whole byte `ram_0094_obj` (X=0 means `$0090/$0094`). `sub_DA58` later transfers the whole speed byte into the per-frame movement accumulator and folds the fraction into `ram_0394_obj`.
- `sub_CD59` selects the acceleration-table index from the two upper controller bits: bit 7 takes the no-increment path after ASL (Y=0); bit 6 alone increments the saved index (Y=1). If both are held, bit 7 wins. This records the exact bit-path; the control labels are not inferred from game feel.
- `sub_CE29` first compares X with `ram_004C`; on the matching phase it adds `tbl_C0BC[Y]` to the fractional byte, increments the whole byte on carry, and for X=0 clamps against `tbl_C0CE[Y]` and `tbl_C0D1[Y]`.
- `sub_CE58` gates the deceleration path on the same phase match, then falls through to `sub_CE5C`. For player X=0, if the whole speed byte is zero it computes `Y = ram_00B0_obj >> 1` and compares the fraction with `tbl_C0CC[Y]`. It then subtracts `tbl_C0C1[Y_decel]`; a borrow decrements the whole byte, or clamps the fraction to zero when the whole byte is already zero.
- Source table values used by the player path: increments `$18/$3F/$28` (`$C0BC-$C0BE`), fractional caps `$20/$40/$7F` (`$C0CE-$C0D0`), whole caps `$03/$03/$01` (`$C0D1-$C0D3`).

### Native temperature / overheat state

The gameplay-state portion of `sub_E359` is translated as `bikePhysicsUpdateTemperature`. Its source-derived behavior is:

- The two controller bits `$40/$80` map to heat modes 1/2; both map to 3; no throttle maps to 0. Before the race-start flag is set, any held throttle is treated as mode 2.
- Increment table `tbl_D8F7`: `[$3F,$0F,$07,$07]`; whole-byte thresholds `tbl_D8FB`: `[$08,$20,$11,$11]`.
- Below the mode threshold, add the table increment to `ram_03B5` and increment `ram_03B6` on carry. Above the threshold, subtract `$0B` from the fraction and decrement the whole byte only on borrow when it is nonzero. At exact equality, leave the meter unchanged.
- At whole value `$20` or above, copy the whole byte into both `ram_03E0` and `ram_003C`. At `$1A` or above, mode 1 ORs `$B0` into `ram_00FC` (the source's B-button overheat-warning path).
- A finished race skips meter-state updates. An active overheat state also pauses updates and selects `$FC`/ `$FE` from the overheat timer parity. The native function returns that source tile selector; the 12-sprite OAM drawing is deliberately left to the native renderer.

`tests/test_bike_speed.c` compares the state bytes and tile selector against the original `sub_E359` routine for **266,240** deterministic combinations of controller, race flags, overheat state/timer, threshold-edge whole values and fractional boundaries. It does not claim that the original HUD sprites have been ported.

### Native player speed-control branch

The player-only control path from `sub_CD59` is now implemented as `bikePhysicsUpdatePlayerSpeed`. It covers the source branch conditions that decide which speed helper runs:

- `ram_0374_obj` / `ram_0098_obj` locks and `ram_009C_obj` states other than 0 or 5 select deceleration mode 0.
- After those early gates, a nonzero track-finished flag selects mode 2 and its lower speed cap.
- A nonzero `ram_00B0_obj` selects deceleration mode 4 when no direction bits are held, otherwise `(controller & 3) + 1`.
- With `ram_00B0_obj == 0`, no A/B throttle bits select deceleration mode 0. Bit 7 selects acceleration mode 0; bit 6 alone selects mode 1; bit 7 wins if both are set.
- The native `sub_CDEE` terrain gate is evaluated before throttle. If it returns nonzero Z, or whole speed is zero, the acceleration path continues. Otherwise the player decelerates with mode 0.
- The source cap comparison is three-way: below cap calls `sub_CE29`; exactly at cap does nothing; above cap increments Y and calls `sub_CE58` with the next deceleration-table mode. This applies to A/B modes and the post-finish mode 2.

The integration test runs the original `sub_CD59` routine directly on the host-only 6502 reference CPU with 8,192 deterministic combinations (all 256 controller bytes × four update phases × four `$00B0` status values × both track-finished states), randomized speed/lock/terrain state, and compares the player speed bytes plus `$036C`/ `$00FD` terrain side effects. It deliberately keeps the later `loc_CDBD` steering timer path inert; that path is not claimed as ported.

### Native movement integration

The movement core is now translated into three platform-independent C functions:

- `sub_DA58` → `bikePhysicsBuildMovementDeltas`: for all four slots, copy the whole speed byte to the frame movement delta, add the fractional speed byte to the carried fractional accumulator, and increment the movement delta on carry.
- `sub_DA6A` → `bikePhysicsAdvancePlayerScroll`: add player movement delta to the horizontal scroll byte; toggle bit 0 of the base-nametable selector only when the 8-bit addition carries.
- `sub_DA7A` → `bikePhysicsAdvanceOpponentPositions`: iterate opponent slots 3, 2, 1; active opponents move by their own delta minus the player's delta, with 16-bit carry/borrow into the high coordinate byte. Inactive slots are skipped. The player itself is camera-relative and is represented by scroll rather than an opponent coordinate.

`tests/test_bike_movement.c` compares these routines directly with the original ASM: 262,144 `sub_DA58` vectors, 262,144 `sub_DA6A` vectors, and 50,000 deterministic `sub_DA7A` vectors (**574,288 total**). Exact-commit validation passed on `81380f81f263b9899fb34395873da5c3b7aec80f`: [host workflow 44577215677](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44577215677) and [New 3DS build 44577215802](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44577215802).

This is movement integration only. The full `sub_DA26` caller also runs object updates/collisions/timers around these helpers; those routines and the complete race-frame order still need to be ported.

### Native implementation

- `source/bike_physics.c::bikePhysicsAcceleratePlayer` translates `sub_CE29` for X=0, preserving byte wraparound and the original phase gate.
- `source/bike_physics.c::bikePhysicsDeceleratePlayer` translates the `sub_CE58/sub_CE5C` player path. The player speed-control wrapper resolves `ram_00B0_obj >> 1` through `tbl_C0CC` ($01 or $B0) before calling this low-level helper.
- `tests/test_bike_speed.c` executes the original `$CE29` and `$CE58` routines directly on the host-only 6502 reference CPU and compares the speed bytes with native C. It checks all 3 acceleration modes × 4 phases × 256 whole × 256 fractional states, plus 2 deceleration thresholds × 7 table modes × 4 phases × 256 whole values × 10 boundary fractions, plus 20,480 terrain-gate states: **950,272 vectors**. The 8,192-vector full player-control comparison and 266,240 temperature/overheat vectors bring the speed/temperature suite to **1,224,704 vectors**. The temperature/overheat test adds 266,240 more.
- The test consumes a PRG rebuilt from the unmodified `Excitebike/bank_FF.asm`. No generated recompiler, NES system, PPU, or APU is used by this differential test.
- Exact-commit validation for the helper slice passed on `3e6d5da164d872afcdb1081c61cc048050b81188`: [host workflow 44574307056](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44574307056) and [New 3DS build 44574307046](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44574307046). The player-control integration passed **958,464 total vectors** on `e3f00d26cb4bdfa476b00ac7f979b156104f7f6d`: [host workflow 44575202175](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44575202175) and [New 3DS build 44575202241](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44575202241). Adding temperature/overheat passed **1,224,704 vectors** on `dd9c3bd4d39b115b351354f88f9a8039e173d041`: [host workflow 44575918494](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44575918494) and [New 3DS build 44575918042](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44575918042). The movement core then passed **574,288 more vectors** on `81380f81f263b9899fb34395873da5c3b7aec80f`: [host workflow 44577215677](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44577215677) and [New 3DS build 44577215802](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44577215802).
- `bikePhysicsUpdate` remains deliberately empty. This one helper does not implement throttle selection conditions, braking/coasting, temperature, jumps, posture, collisions, AI, or per-frame integration.

### Still required before this is accepted as player physics

Finish the remaining `loc_CDBD` steering/timer side effects, trace the initialization values for all player fields, port speed movement, heat accumulation/cooling, posture/jump/landing/crash state, and all relevant timers. Add state-vector tests for each branch and compare after each original game frame. The tests now also compare the player speed-control branch of `sub_CD59`, but the later `loc_CDBD` steering/timer side effects and the rest of the bike physics are still outside this implementation.

## Not yet traced (required before any gameplay is accepted)
Complete player-slot initialization (commands 6/`sub_E1A3`/`sub_EC25`/`loc_EC47`, all RAM writers), the full `ram_005C` input-to-speed path beyond `sub_CE29`, every subroutine in the frame handler above, PPU lists $01/$12–$17 contents, `sub_D14E` raster-split wait (sprite-0 hit), track data decoding, opponents, timers, scoring, crashes/collisions.

## Ported so far (2026-11-18) — `source/nes_race_setup.c`, test `tests/test_nes_race_setup.c`
Earlier helpers were translated line by line and host-tested with hand-derived expectations (including mutation checks on zero-page wrap). The newly added ECE4 helper has regression vectors wired, but project CI execution is still pending:
- cmd 1 (C551), cmd 2 (C869), cmd 4 (C5A3), cmd 5 (C566): complete.
- cmd 3 (C820): everything after `JSR sub_D13F` (clear $0340-$03EF, fill $0400-$057F, sprite 0, $E8-$EB).
- cmd 6 (C875): head up to `LSR ram_0078,X` at bra_C8DB, including the `LDA ram_0019,X` with X=$FF reading $0018 (zero-page wrap).
- `sub_ECE4` buffer builder: the three `tbl_E6A1` records at offsets 0/4/8, both 32-byte dynamic command payloads, terminator, pointer `$0400`, and the `$0502/$0525` page-crossing writes. PPU execution via `loc_C289/sub_D3C1` is not part of this helper.

NOT ported: the complete `sub_D13F` sequence and its three ordered dynamic PPU uploads through `loc_C289/sub_D3C1`; cmd 6 tail from bra_C8DE (`ram_0046` test, `tbl_C158`, `sub_DFA2`, `sub_C522`, `sub_C53A`, `sub_C935`/`sub_DF7F`, `ram_03F8` adjust); PPU lists $12-$17 and $01 contents; and the wiring into `menus.c` (list index 2 still reports "not ported"). The isolated helpers are not called by the game yet.

## Next slice
Model `loc_C289/sub_D3C1` and integrate the three `sub_D13F` buffer builds in source order; then trace and port the C875 tail (`ram_0046`, `tbl_C158`, `sub_DFA2`, `sub_C522`, `sub_C53A`, `sub_C935/sub_DF7F`, `ram_03F8` adjustment). Only after the complete six-command list is represented should it be wired into the setup sequencer; then trace `sub_E1A3` and the player object (`ram_00B0`/`ram_00B4` writers).


## Source trace and native translation — sub_ECE4 / loc_ECE4 (2026-11-18)

Source: `Excitebike/bank_FF.asm`, CPU `$ECE4`, ROM offset `$002CF4`; data table `tbl_E6A1` at CPU `$E6A1`, ROM offset `$0026B1`.

- `sub_D13F` invokes this routine with X = `$00`, then `$04`, then tail-jumps with X = `$08`. These are three source-defined 4-byte records in `tbl_E6A1`: `22 64 62 C0`, `22 74 63 E0`, and `21 83 FC C0`.
- The routine copies each record in reverse order into zero-page scratch `$02-$05`: `$05=record[0]`, `$04=record[1]`, `$03=record[2]`, `$02=record[3]`.
- It builds a dynamic PPU command at `$0400`: first header bytes are record[0], record[3], `$20`; the 32 payload bytes alternate record[2], record[1]. A second command starts at `$0423`: record[0] XOR `$04`, record[3], `$20`, followed by the same 32-byte payload. `$0446` is the zero terminator.
- The paired absolute-indexed stores are significant: when X reaches `$00`, the second `STA $0403,X` addresses `$0502`; the corresponding second block writes `$0525`. These page-crossing side effects are preserved rather than clamped.
- The routine sets the dynamic PPU data pointer to `$0400` and jumps to `loc_C289`, which invokes `sub_D3C1`. The native helper intentionally models the buffer construction and the two page-crossing writes only; it does **not** claim to execute the PPU command stream or reproduce the hardware timing/PPU side effects of `sub_D3C1`.

Native implementation: `source/nes_race_setup.c::nesRaceSetupECE4`; declaration in `include/nes_race_setup.h`; regression vectors in `tests/test_nes_race_setup.c` cover all three table offsets, both 32-byte payloads, the terminator, pointer bytes, untouched guards, invalid offsets, and the `$0502/$0525` page-crossing writes.

Verification status: source-derived translation and test code are committed; host execution and exact-commit 3DS build are pending CI. This helper is not yet integrated into `sub_D13F` or the state-2 setup sequencer. Command 3 and the first race remain incomplete.
