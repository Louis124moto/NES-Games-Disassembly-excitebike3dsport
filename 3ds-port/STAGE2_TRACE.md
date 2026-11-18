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
