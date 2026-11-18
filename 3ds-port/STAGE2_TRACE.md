# Stage 2 — original menu source trace

Status: **IN PROGRESS / REOPENED 2026-11-18**. Stage 3 is blocked until this stage is accepted.

This document is based on the unmodified `Excitebike/bank_FF.asm`. The native menu implementation is partial. The owner has reported that the current menu responds to controls on New 3DS, but the original artwork is incorrect. No NES capture comparison or complete hardware acceptance is recorded.

## Source dispatch and setup tables

- `tbl_C000` at CPU `$C000` dispatches `ram_0041` to the menu/game state handlers.
- `tbl_C01C_index` at `$C01C` maps each state to a setup-list index. It is not a direct state-to-list mapping:
  `[0,1,2,3,4,2,3,4,6,5,2,3,7,8]` for states `$00-$0D`.
- `tbl_C02A_lo` / `tbl_C033_hi` point to setup lists at `$C03C-$C07A`.
- `sub_C2A9` selects and runs one command per NMI/setup step. `bra_C2ED` first disables rendering and runs `sub_C318` when the old `ram_0044` value is zero.
- `sub_C318` calls `sub_D255` (hide all sprites), `sub_D23B` (clear nametables/attributes), and `sub_D1A9` (reset scroll), then writes PPUCTRL `$10`.

## Relevant state and command paths

| State | Handler | Setup list | Current native coverage |
|---|---|---:|---|
| `$00` | `ofs_000_C346_00` title menu | 0 | Input/cursor state and list 0 partially translated |
| `$01` | `ofs_000_C395_01` selector | 1 | Input/cursor state and list 1 partially translated |
| `$02` | `ofs_000_C99A_02` race | 2 | Not a menu completion target; Stage 3, blocked |
| `$03` | `ofs_000_CB2C_03` | 3 | Not fully classified; trace required |
| `$04` | `ofs_000_C465_04` | 4 | Partial transition/timer and setup-list support |
| `$05` | `ofs_000_C99A_05` race | 2 | Stage 3, blocked |
| `$06` | `ofs_000_CB2C_06` | 3 | Not fully classified; trace required |
| `$07` | `ofs_000_C465_07` | 4 | Partial transition/timer and setup-list support |
| `$08` | `ofs_000_C3BD_08` design menu | 6 | Input/cursor/confirm branches and list 6 partially translated |
| `$09` | `ofs_000_C97C_09` | 5 | Not fully classified; trace required |
| `$0A` | `ofs_000_C99A_0A` race | 2 | Stage 3, blocked |
| `$0B` | `ofs_000_CB2C_0B` | 3 | Not fully classified; trace required |
| `$0C` | `ofs_000_C43D_0C` tape save | 7 | Host file-backend substitute; native path not equivalent to a physical recorder |
| `$0D` | `ofs_000_C41C_0D` tape load | 8 | Host file-backend substitute; native path not equivalent to a physical recorder |

The state names above are address/table observations, not claims that every associated screen or transition is complete. The exact menu scope must include every user-visible menu and return path, while race handlers remain Stage 3.

## Full state and setup-list inventory (source audit pass 1)

The state dispatcher and setup-list selector are two distinct layers. The following is the exact mapping read from `tbl_C000` and `tbl_C01C_index` in the original ASM:

| `ram_0041` | Handler at `tbl_C000` | Setup list (`tbl_C01C_index`) | Stage 2 classification |
|---:|---|---:|---|
| `$00` | `ofs_000_C346_00` | 0 | Title menu; in scope |
| `$01` | `ofs_000_C395_01` | 1 | Course/track selector; in scope |
| `$02` | `ofs_000_C99A_02` | 2 | Race runtime; Stage 3, out of Stage 2 |
| `$03` | `ofs_000_CB2C_03` | 3 | Shared transition dispatcher; in scope only for menu transitions |
| `$04` | `ofs_000_C465_04` | 4 | Shared timed transition; in scope |
| `$05` | `ofs_000_C99A_05` | 2 | Race runtime; Stage 3 |
| `$06` | `ofs_000_CB2C_06` | 3 | Same shared transition dispatcher as state 3 |
| `$07` | `ofs_000_C465_07` | 4 | Same shared timed transition as state 4 |
| `$08` | `ofs_000_C3BD_08` | 6 | Design/options menu; in scope |
| `$09` | `ofs_000_C97C_09` | 5 | Calls game/race update routines; exact role/entry/exit still to be established |
| `$0A` | `ofs_000_C99A_0A` | 2 | Race runtime; Stage 3 |
| `$0B` | `ofs_000_CB2C_0B` | 3 | Same shared transition dispatcher as states 3/6 |
| `$0C` | `ofs_000_C43D_0C` | 7 | Tape save menu/transfer; in scope (3DS file backend is a platform replacement) |
| `$0D` | `ofs_000_C41C_0D` | 8 | Tape load menu/transfer; in scope (3DS file backend is a platform replacement) |

### Setup-list contents from `tbl_C02A_lo`, `tbl_C033_hi`

| List | Source table | Ordered commands |
|---:|---|---|
| 0 | `off_C03C_00` | `ofs_001_C2BD_00_01`, `ofs_001_C514_00_02` |
| 1 | `off_C040_01` | `ofs_001_C55E_01_01`, `ofs_001_C3F3_01_02`, `ofs_001_C514_01_03` |
| 2 | `off_C046_02` | `ofs_001_C551_02_01`, `ofs_001_C869_02_02`, `ofs_001_C820_02_03`, `ofs_001_C5A3_02_04`, `ofs_001_C566_02_05`, `ofs_001_C875_02_06` |
| 3 | `off_C052_03` | `ofs_001_C55E_03_01`, `ofs_001_C56A_03_02`, `ofs_001_C455_03_03`, `ofs_001_C5A7_03_04`, `ofs_001_C485_03_05` |
| 4 | `off_C05C_04` | `ofs_001_C55E_04_01`, `ofs_001_C571_04_02`, `ofs_001_C5AC_04_03`, `ofs_001_C514_04_04` |
| 5 | `off_C064_05` | `ofs_001_C551_05_01`, `ofs_001_C869_05_02`, `ofs_001_C820_05_03`, `ofs_001_C5A3_05_04`, `ofs_001_C562_05_05`, `ofs_001_C5C2_05_06` |
| 6 | `off_C070_06` | `ofs_001_C55E_06_01`, `ofs_001_C575_06_02`, `ofs_001_C514_06_03` |
| 7 | `off_C07A_07` | `ofs_001_C869_07_01`, `ofs_001_C586_07_02`, `ofs_001_C518_07_03` |
| 8 | `off_C076_08` | `ofs_001_C582_08_01`, `ofs_001_C518_08_02` |

Lists 2 and 5 are assigned to the three race states. Lists 3, 4, 7 and 8 are not interchangeable with ordinary title/menu lists: their side effects and indirect transitions must be traced before calling them complete.

### Proven transition behavior

- Title state `$00`: `sub_C7DE` returns the selected entry in Y. The three entries branch to state `$01` (course selector) or state `$08` (design menu); the title's idle timer path enters state `$02` (race path). The current C model has a source-derived title/selector/design navigation path, but it does not yet implement every downstream transition.
- Selector state `$01`: LEFT/RIGHT are remapped to the vertical selector input, and confirmation stores `ram_0042` into `ram_0043`, increments `ram_0041` to state `$02`, then resets the setup sequencer. This is a race-entry transition, not a further menu screen.
- Design state `$08`: selection values 0/1 advance to state `$09`/related setup path; value 2 remains/returns through the state-3 transition path; value 3 enters save state `$0C`; value 4 enters load state `$0D`; value 5 clears `$0021-$00FF` and returns to title state `$00`. These branches are visible in `$C3C5-$C3E7`; each branch still needs end-to-end validation.
- States `$03`, `$06`, `$0B` share the indirect dispatcher at `$CB2C`, selecting `ofs_CB3B_00`, `ofs_CB77_01`, `ofs_CBA4_02`, or `ofs_CC6B_03` by `ram_0044`. They are a shared transition state machine, not three independent screens.
- States `$04` and `$07` share `$C465): wait on `ram_0034_timer`, update a table tuple through `sub_C92C`, then increment state and normalize to `$02` when the old state is below `$08`; otherwise return to `$08`.
- State `$09` calls `sub_CA1B`, `sub_EB17`, `sub_DA6A`, `sub_DBFE`, `sub_E70B`, `sub_F4FF`, `sub_F68D`, `sub_F755`, and `sub_C229`, then jumps to `loc_D19B`. It must not be treated as a simple menu state; its exact place in the user-facing flow needs further call-graph tracing.
- Save/load states `$0C/$0D` use `ram_0048` as the main-thread transfer handshake. State `$0D` validates the four-byte `tbl_C180` header before reading the 256-byte data block; state `$0C` writes the 64-byte header/data block plus 256-byte payload. The file backend is a deliberate hardware-interface replacement, not an emulation of analog tape signals.

## Ordered completion plan — work one item at a time

The boxes below are the complete known Stage 2 work queue. A box is checked only when the corresponding source audit, implementation, and regression evidence exist. "Menu state flow" excludes race simulation itself, but must correctly hand off to the race state.

### A. State graph and source trace
- [x] A1. Inventory all 14 state handlers, setup-list selector values, and all nine setup-list command sequences.
- [x] A2. Identify title, selector, design, save/load, race-entry, and shared transition paths from the state handlers.
- [x] A3. Trace every branch and return from `ofs_CB3B_00`, `ofs_CB77_01`, `ofs_CBA4_02`, and `ofs_CC6B_03`; record exact conditions and state writes (see the transition-dispatcher trace below).
- [ ] A4. Trace the complete state-$09 path and prove whether it is menu transition, pre-race, post-race, or gameplay support.
- [ ] A5. Trace each reachable setup command's full transitive calls and RAM/PPU/APU side effects; include `$30-$3D` timers and one-command-per-NMI sequencing.
- [ ] A6. Audit reset/cold-start, return-to-title, cancel/back, and all menu-to-race handoffs against the actual ASM call order.

### B. PPU, OAM, and visual reconstruction
- [ ] B1. Trace all menu PPU stream indices selected through `ram_0045`, including dynamic stream `$00`, zero-page stream `$1A`, and palette streams `$12-$19`; prove the CHR bank/pattern-table selection for each screen.
- [ ] B2. Verify the native PPU command parser against every stream reachable from menu setup lists; test terminators, lengths, PPU address bounds, palette mirroring, and unsupported writes.
- [ ] B3. Model the full two-nametable PPU address space and nametable mirroring selected by the source; current renderer reads a fixed 960-byte background plus one attribute table and does not yet prove correct base nametable selection.
- [ ] B4. Integrate source scroll writes (`$2005/$2000`), split/scanline behavior, and NMI ordering; do not replace raster effects with a guessed static scroll.
- [ ] B5. Trace every menu OAM writer and all cursor/decoration sprites, sprite priority, flips, size, scanline limits, and visibility; render all supported source attributes.
- [ ] B6. Reconstruct authentic title, selector, design, save/load, and transition artwork from actual CHR + PPU writes; remove diagnostic text/placeholder rendering from the normal menu path.
- [ ] B7. Verify menu palettes and palette transitions using the actual PPU palette RAM writes; account for palette mirroring and PPUMASK grayscale/emphasis if used.

### C. Menu input and transitions
- [ ] C1. Compare `sub_C7DE` byte-for-byte with native input: repeat guard, timer, Select/Start priority, Up/Down, and exact wrap rules.
- [ ] C2. Complete title and selector navigation/confirmation and the transitions into all destination states.
- [ ] C3. Complete all six design-menu choices, including the paths through states 9, 12 and 13 and return to title.
- [ ] C4. Complete the shared state-3/6/0B and state-4/7 transition sequencers with correct frame/timer behavior.
- [ ] C5. Complete save/load error, retry, header mismatch, missing file, corrupt file, and successful return behavior; clearly document differences from the original recorder hardware.

### D. Validation and acceptance
- [ ] D1. Add host regressions for every menu state, setup list, input edge case, transition, PPU/OAM write, and tape backend branch.
- [ ] D2. Create deterministic NES reference captures for each menu screen and transition (ROM identity/hash, emulator/version, frame, PPU/OAM state, and screenshot).
- [ ] D3. Compare native frame output and PPU/OAM state against those references; fix all visible/source-state mismatches.
- [ ] D4. Run full host test suite and exact-commit devkitARM/New 3DS build; retain CI links and commit SHA.
- [ ] D5. Verify cold boot, every menu route, save/load and return on a physical New Nintendo 3DS; record evidence.
- [ ] D6. Accept Stage 2 only after A–D pass. Keep Stage 3 blocked until then.

### Shared transition dispatcher: completed branch trace (`$CB2C-$CC7C`)

All three state handlers `$03/$06/$0B` enter the same code. `ram_0044` is an index into the four-entry pointer tables `tbl_C12C_lo`/`tbl_C130_hi`:

| `ram_0044` | Target | Source behavior |
|---:|---|---|
| 0 | `ofs_CB3B_00` | Copies `ram_03F2` to selection `ram_0042` when `ram_0041 < 8`. If `ram_0053 >= 4`, or state `>=8`, it skips the selection update. Otherwise, when `ram_0046` is set, it advances `ram_0043` with a special blink/half-rate path for selection 4 and `ram_03F8`; then sets `ram_0046=1`, `ram_004B=1`, and starts timer `$0033 = 7` (`$04 + $03`). |
| 1 | `ofs_CB77_01` | Waits until `ram_0033_timer=0`. Sets table index 4, copies seven bytes from `ram_03D9,Y` to dynamic PPU buffer `ram_0304,X`, calls `sub_C94B`, writes `ram_03AD=$20` if `ram_004B != 0` else `$31`, then sets `ram_0033_timer = ram_03AD + 3` and increments `ram_0044`. |
| 2 | `ofs_CBA4_02` | If timer is zero, starts the next timer and increments `ram_0044`. Otherwise waits until timer reaches `ram_03AD`. At that point, if `ram_004B=0` and no A/B/Start/Select hold is active, calls `sub_CC15` then `loc_CC7D`; otherwise calls `sub_CC6B`. The latter increments `ram_0041`; when `ram_004B=0`, it clamps states `>=9` to state 8. |
| 3 | `ofs_CC6B_03` | Calls `sub_C3AF` (increments `ram_0041`, clears `ram_0040/$0044`). If `ram_004B != 0`, returns. Otherwise keeps the new state below 9, or sets it to 8 if it reached/exceeded 9. |

Additional effects in the transition's timed phase:

- `sub_CC15` compares `ram_0033_timer` with `ram_03AD - $10`. At the matching frame and when the `$20` frame divider is zero, it posts PPU list `$0D`, `$0B`, or `$0C` depending on `ram_0052_track_finished_flag` and `ram_004B`.
- Otherwise `sub_CC15` builds a dynamic PPU command from `tbl_C134` into `ram_0301+`; unless frame flag `ram_003F & $10` is set, it uses `sub_C971` and `sub_DFA2` to append values derived from `ram_0053`.
- `loc_CC7D` chooses a table entry using `ram_0053-1`, `tbl_C0DA`, `tbl_C0DE`, and `tbl_C0E8`, then jumps to `loc_D1C7` to update OAM/data. When `ram_002F_timer` is nonzero it writes fixed object fields before the update; otherwise `sub_DD6F` derives a value and may start a ten-tick timer.
- `ofs_CBA4_02`'s ten-frame branch uses `tbl_C0DA/C0DE/C0E2/C0E5`, calls `sub_CCCA` for three entries, derives `ram_00FB`, `ram_0056`, `ram_0054/$0055`, and may post PPU list `$0A`. This branch's rendering dependencies (`sub_CCCA` -> `loc_D1C7`) remain part of A5/B5, not a completed renderer claim.

**Interpretation:** these shared handlers implement a timed selection/transition animation and PPU/OAM updates, not a static independent screen. They are in Stage 2 because title/design selections pass through them. The exact visual result remains blocked on the called PPU/OAM routines and source asset mapping.

## PPU/NMI path

- `vec_C23B_NMI` calls `sub_C32F`, conditionally enables `$2001` when `ram_0045 == 0`, performs OAM DMA from `$0200`, calls `sub_C27F`, handles scroll-related calls when `ram_0047 != 0`, decrements timers, reads controller registers, dispatches the state handler, and waits for vblank.
- `sub_C27F` resolves `ram_0045` through `tbl_C0F6_lo` / `tbl_C111_hi`, calls `sub_D3C1`, clears `$0300/$0301`, and clears `ram_0045`.
- The current native renderer has a source-derived static PPU command interpreter and a CHR/nametable/palette compositor. It is not yet a full live PPU model. Dynamic lists, all OAM update paths, scroll split/timing, and every menu-specific stream selection still require audit.
- The title setup list 0 points to `ofs_001_C2BD_00_01` and `ofs_001_C514_00_02`. The title handler uses `sub_C7DE` and `tbl_C080_spr_Y`.
- State 1 uses `ofs_001_C55E_01_01`, `ofs_001_C3F3_01_02`, and `ofs_001_C514_01_03`; its cursor X positions come from `tbl_C083_spr_X`.
- State 8 uses setup list 6 (`ofs_001_C55E_06_01`, `ofs_001_C575_06_02`, `ofs_001_C514_06_03`) and cursor Y positions from `tbl_C08B_spr_Y`.

## Known gaps to close

1. Trace every state reachable from the title menu, identify which are genuine menu screens versus race/result flow, and follow all return/cancel branches.
2. Trace every menu setup command and all transitive routines; do not silently omit side effects or model unknown lists as complete.
3. Determine the exact CHR source and PPU pattern-table selection used by each menu state. Confirm every nametable and attribute byte has valid source data before drawing.
4. Trace all menu OAM writes, sprite priority/visibility, scroll writes, PPUCTRL/PPUMASK changes, NMI ordering, and frame/timer dependencies.
5. Implement and test all missing menu state handlers and setup commands in original call order.
6. Replace diagnostic/placeholder text with source-derived artwork. Do not draw guessed boxes or text.
7. Create reproducible reference captures for each menu state and transition and compare pixels plus relevant PPU/OAM state.
8. Run the host test suite, 3DS build on the exact tested commit, and physical New 3DS validation. Record evidence in TODO before acceptance.

## Acceptance gate

Stage 2 is complete only when every menu screen and transition in scope is source-traced, integrated into the actual 3DS runtime, covered by host regressions, built for 3DS on the same commit, and compared against the original NES behavior. A successful menu launch or controller response alone is insufficient.
