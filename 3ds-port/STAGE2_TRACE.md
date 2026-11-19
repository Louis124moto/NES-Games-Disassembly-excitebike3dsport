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

### Menu `ram_0045` PPU stream selection: verified mapping pass

The native active-stream table maps the source `tbl_C0F6_lo/tbl_C111_hi` index to the literal lists in `nes_ppu_commands.c`. The following mappings are source-backed (CPU address, not a guessed screen label):

| `ram_0045` | CPU source | Kind | Meaning in menu flow |
|---:|---|---|---|
| `$02` | `$D64B` | Static PPU | Title state 0 first setup command; source hint "blue background" |
| `$03` | `$D6D4` | Static PPU | Selector state 1 second setup command |
| `$04` | `$D717` | Static PPU | Shared state-$03/$06/$0B transition command; sets `ram_0033_timer=$04` |
| `$06` | `$D63A` | Static PPU | Selector/design/shared-transition setup |
| `$07` | `$D59E` | Static PPU | Conditional shared transition stream when `ram_0041<8` and `ram_0046!=0` |
| `$08` | `$D5B4` | Static PPU | Shared transition stream when `ram_0041>=8` |
| `$09` | `$D796` | Static PPU | State 4/7 timed transition list |
| `$0A` | `$D5C8` | Static PPU | Shared transition ten-frame branch |
| `$0B` | `$D5DE` | Static PPU | Shared transition completion branch |
| `$0C` | `$D5EF` | Static PPU | Shared transition completion branch |
| `$0D` | `$D605` | Static PPU | Shared transition completion branch |
| `$0F` | `$D7FA` | Static PPU | Design state 8 second setup command |
| `$10` | `$D623` | Static PPU | Tape-save setup after restoring stored track data |
| `$11` | `$D614` | Static PPU | Tape-load setup |
| `$12-$18` | `$D3D6-$D450` | Palette | Seven palette lists; `$18` is used by title/selector/design setup |
| `$19` | `$D631` | Static PPU | Error/diagnostic PPU list used by tape/transition code |
| `$00` | `$0301` | Dynamic RAM | `sub_D3C1` reads the variable command buffer at `$0301`; epilogue clears `$0300/$0301` |
| `$1A` | `$00A1` | Zero-page RAM | `sub_D3C1` reads the list at `$00A1`; used by the tape-load header display |

### Cartridge mirroring and the two physical nametables

The checked-in `Excitebike/header.bin` is the 16-byte iNES header `4E 45 53 1A 01 01 01 00 00 00 00 00 00 00 00 00`. Header byte 6 is `$01`: mapper 0 with **vertical nametable mirroring** (two physical 1 KiB nametables).

The physical mapping is therefore:

| PPU address range | Physical 1 KiB page |
|---|---|
| `$2000-$23FF` | nametable A (page 0) |
| `$2400-$27FF` | nametable B (page 1) |
| `$2800-$2BFF` | mirror of A |
| `$2C00-$2FFF` | mirror of B |

The existing `GameState.menu_ppu_nametables[2048]` and `nes_active_stream.c` scratch PPU memory already reserve the two physical pages A/B. However, `rendererDrawPpuBackground()` currently passes only page A's first 960 tile bytes and its 64 attribute bytes to `nesPpuRenderNametableOpaque()`. It therefore does not render page B when PPUCTRL/scroll selects it, and the PPU command interpreter does not yet normalize `$2800-$2FFF` writes onto A/B. The source mirroring fact is now established; the renderer/address-mirroring implementation remains open under B3.

### Screen setup paths observed in the current native model

- Title: list 0 posts `$02` (`$D64B`), then `$18` (`$D450`).
- Course selector: list 1 posts `$06` (`$D63A`), `$03` (`$D6D4`), then `$18`.
- Design menu: list 6 posts `$06`, `$0F` (`$D7FA`), then `$18`.
- State 4/7 transition: list 4 posts `$06`, `$09`, then creates a dynamic `$0300` command from `tbl_C13C`; final command posts `$18`.
- Shared transition list 3: posts `$06`, `$04`, then conditionally `$07` or `$08`; later commands use `loc_C92C` and `sub_C485`.
- Tape save: list 7 clears the temporary RAM range, restores the four-byte recorder header and saved track data, copies `$06E0-$07DF` to `$05E0-$06DF`, then posts `$10`.
- Tape load: list 8 posts `$11`, then performs the recorder/file transfer and may post dynamic `$1A` or error `$19` output.

### CHR and PPUCTRL constraints

- The original ROM is mapper 0 / NROM in the checked-in source header, so there is no mapper bank-switch sequence for menu CHR in this project.
- The setup prelude `sub_C318` writes PPUCTRL `$10`: background pattern table is `$1000` (CHR tile indices 256..511); sprite pattern table bit 3 is clear (`$0000`, tile indices 0..255).
- `loc_D19B`/ `sub_D1B3` preserve the upper PPUCTRL nibble and combine it with `ram_004E_base_nametable`; this is part of the active scroll/raster path and must be modelled separately from the static menu setup.
- This verifies stream selection and pattern-table selection, **not** complete menu composition. The native renderer still needs the exact nametable mirroring/base selection, scroll/raster, and all OAM paths before the artwork can be accepted.

### Mirroring implementation checkpoint (commit `79a3f28136bc`)

Implemented in the native PPU command path:

- Added `nesPpuApplyCommandStreamMirrored()` with explicit horizontal/vertical mirroring modes; retained the original flat helper as a compatibility wrapper.
- Added canonical PPU address handling for `$3000-$3EFF` mirrors and palette aliases.
- Routed Excitebike static stream uploads and source PPU snapshots through **vertical** mirroring, matching iNES header flag 6 = `$01`.
- Added host vectors that distinguish vertical from horizontal mirroring and verify the `$3000` alias.

Validation on exact commit `79a3f28136bc1507c42527178addb9ec8dab1eea`:
- Host workflow `Verify NES routine translations`: success (run 44356209040).
- New 3DS/devkitARM build workflow: success (run 44356209037).

Remaining B3 work: render both physical nametables with the selected base nametable and scroll; this code change fixes PPU write addressing, not the on-screen composition. No physical console screenshot/reference comparison was run.

### Additional `sub_D14E` timing and OAM preparation facts

- `sub_D11F` prepares a 17-byte dynamic PPU command: address `$23A0`, fill count `$0E`, then fourteen `$FE` bytes, with `ram_0312=0`. This is issued before the split.
- For state 9, `sub_D14E` keeps X=`$43` (67 outer iterations) because the `CMP #$09 / BEQ bra_D17E` branch skips the non-editor sprite-hide and object-copy path.
- The state-9 delay loop at `$D17E-$D184` is exactly 6,431 6502 CPU cycles: 67 iterations of `LDY #$12`, an 18-count `DEY/BNE` inner loop (89 cycles), `DEX`, and the outer `BNE` (taken 66 times). This is about 56.6 NTSC scanlines, before the surrounding routine overhead.
- The routine first waits while PPUSTATUS bit 6 (sprite-0 hit) remains set. Since the editor setup initially places sprite 0 at Y=`$BF`, the synchronization point is expected to be after that hit clears near the pre-render line; the second scroll pair therefore lands roughly 56.6 scanlines into the following frame plus the exact surrounding overhead. Do not hardcode a guessed split line from the loop alone; capture/measure it.
- Non-state-9 paths use X=`$28` (40 iterations) but first call `sub_D0C6` and `sub_D0F4`: D0C6 refreshes four 4-byte records in `$03C0-$03CF` from the `$03D9` data through `sub_E7FC`; D0F4 copies seven bytes `$03D9-$03DF` to `$03D1-$03D7`, constructs a dynamic PPU command at `$2356`, and jumps to `loc_D2A3`.
- The split routine is therefore state-dependent. A single global scroll value or a fixed unverified split line is not source-faithful.

### Menu/editor OAM writer inventory (source audit)

The menu's visible sprites are not limited to sprite 0. Source writes identified in `bank_FF.asm`:

| Source routine/address | OAM range | Role and data |
|---|---|---|
| `sub_D255` / `$D255-$D25C` | `$0200-$02FF` | Initializes all 64 sprites by filling all 256 OAM bytes with Y=`$F8`. |
| `$C2E6-$C2E9`, `sub_C409` | `$0200-$0203` | Title cursor: Y from `tbl_C080_spr_Y` (80/90/A0), tile `$FA`, X from `tbl_C088_spr_X` (50/48/4C), attributes 0. |
| `sub_C3EF` and selector/design callers | `$0200` | Updates cursor Y; selector X choices are `tbl_C083_spr_X` (48/60/78/90/A8), design Y choices are `tbl_C08B_spr_Y` (30/48/78/90/A8/C0). |
| `ofs_001_C820_02_03` / `ofs_001_C820_05_03` | `$0200-$0203` | Hides sprite 0 during course/editor setup (Y=`$BF`, tile=`$FF`, X=`$F7`). |
| `ofs_001_C5C2_05_06` | `$0228-$022F` | Copies `tbl_C178` bytes `C7 DB 00 E0 C7 DD 00 E8`; also calls `sub_E1A3` to prepare an object sprite and increments the split-raster flag. |
| `sub_D1C7` via `sub_E1A3` | Variable OAM slots from `ram_0004_t01_data_oam_address` | Shared multi-record OAM writer; destination/stride/count depend on `ram_0003_t02` and `ram_0004`. Not yet integrated into list 5. |
| `sub_EC25` | `$0230-$0233` | Editor sprite: Y supplied in A, tile `$CA`, attributes 0, X from `tbl_E6F6[ram_00C2+2]`. |
| `loc_EC47` | `$0234-$0237` | Editor sprite: Y `$CF`, tile from `ram_05E0`, attributes 0, X `$E4`. |
| `$CCEF-$CD12` | `$02C0-$02CF` and `$02D0` | Shared transition animation: creates four sprites with Y=`$44`, tile from `tbl_C147`, attr 0, X from a calculated value; hides the next sprite at `$02D0`. |
| `$CD15-$CD1B` | `$02C0-$02C7` | Copies eight bytes from `tbl_C150_spr_data` (two four-byte sprite records) to OAM. |
| `sub_D14E` | `$0200-$02EC` | For non-state-9 modes hides sprites from `$02EC` down through `$0200`, then updates split-screen OAM/scroll; state 9 deliberately skips this blanket hide. |

The current renderer can composite 8x8 sprites, including palette selection, priority, flips, OAM ordering, and the 8-sprites-per-scanline limit. PPUCTRL is `$10` in the menu prelude, so the source selects 8x8 sprites and pattern table `$0000`; the 8x16 unsupported path is not required for these menu/editor states.

**B5 remains open:** the title/selector/design cursor path is represented, but the shared-transition OAM records and state-9 editor sprites are not yet emitted by the active C runtime. The renderer's sprite capability does not substitute for the missing source OAM writers.

### `sub_D1C7` OAM grid helper checkpoint (commit `73d87350db7c`)

Added `nesSubD1C7BuildOamGrid()`, a source-faithful isolated translation of the shared OAM tile/attribute and position writer:

- Calculates the 8-bit `rows * columns` count from the high/low nibbles of `ram_0003`.
- Writes tile and attribute bytes in the ASM order, including the even-index `ram_000A` marker-bit shift and special tile `$FC` behavior.
- Writes the Y/X position grid in column-major order, advancing coordinates by eight pixels.
- Preserves 8-bit OAM pointer wrap and the `ram_0002`, `ram_000A`, `ram_0000`, `ram_0008`, and `ram_0009` scratch effects.

Host regression covers a 2x2 grid, marker tile behavior, column-major placement, and OAM address wrap. Exact commit `73d87350db7ca19196e23d5178d27a1f6c0bf3d8`: host verification run 44360512660 passed; New 3DS build run 44360505911 passed.

**Not yet integrated:** `nesSubE1A3PrepareObjectSprite()` still does not call this helper through the state-9 list-5 runtime path. B5 remains open until the full editor OAM call chain is wired and compared.

### PPU command parser validation checkpoint (commit `ad608e0b0d18`)

- `testAllSourceStreams` replays every one of the 18 static PPU streams and all 7 palette streams through the source parser and checks that each has a nonzero write count.
- `testDynamicBuffer` exercises the variable `$0300` list, confirms its PPU write, and verifies that only `$0300/$0301` are cleared by the NMI epilogue.
- `testTapeLoad` exercises the zero-page `$00A1` list (`ram_0045=$1A`) while displaying an invalid tape header; it checks the expected nametable writes and PPU revision change.
- The parser now rejects a zero control/count record with `NES_PPU_COMMAND_INVALID_COUNT` rather than silently accepting it as a successful end; the true terminator remains a zero first byte (address high).
- Regression vectors cover truncated headers/literal/repeat payloads, invalid addresses, zero controls/counts, palette mirroring, and both vertical/horizontal nametable mapping.
- Exact commit `ad608e0b0d18b4331e5a8cb3143fbdbabbfb86ff`: host verification run 44357605524 succeeded; New 3DS build run 44357605310 succeeded.

This closes B2's parser/replay validation. It does not prove the screen artwork is correct; visual and OAM fidelity remain B5-B7 and D2-D5.

### Two-nametable renderer implementation checkpoint (commit `76abf59affc0734c`)

Implemented and integrated:

- Added `nesPpuRenderMirroredNametablesOpaque()` for the two physical 1 KiB nametables of the vertical-mirrored Excitebike cartridge.
- Both pages are rendered with their own tile bytes and attribute tables; the output compositor selects page A/B from PPUCTRL base bits and horizontal scroll, and wraps vertical position according to the vertical-mirroring layout.
- The live menu renderer now uses this compositor instead of always rendering page A.
- Sprite composition receives the final composited background-opacity map, so behind-background sprite priority uses the correct page/scroll result.
- Added host tests for base page A, base page B, horizontal scroll crossing from A to B, and the lower logical row under vertical mirroring.

Exact commit `76abf59affc0734c757e0a38c6c3f46e5b11b358`:
- `Verify NES routine translations`: success, run 44356838865.
- `Build Nintendo 3DS homebrew`: success, run 44356838616.

This completes the B3 source-backed two-page composition implementation and its automated validation. It does **not** complete B4: the separate `ram_0047` mid-frame split/raster path and its timed OAM updates are still absent. No physical New 3DS visual comparison was performed.

### Menu scroll/raster path audit

- `sub_C318` performs the menu setup prelude: hides sprites, clears nametable/attribute memory, resets scroll through `sub_D1A9`, clears `ram_scroll_X`, `ram_0050_scroll_X`, `ram_scroll_Y`, and `ram_0051_scroll_Y`, then sets PPUCTRL to `$10`.
- `vec_C23B_NMI` calls the split/raster path only when `ram_0047 != 0`: `sub_D0AB` updates per-object vertical band values, then `sub_D14E` waits for vblank, calls `sub_D1B3`, updates PPUCTRL/scroll, updates OAM for non-state-9, waits a source-timed delay, then writes the second nametable base and scroll values.
- `sub_D1B3` writes `ram_for_2000 | ram_004E_base_nametable` to PPUCTRL, then writes `ram_0050_scroll_X` and `ram_0051_scroll_Y` to PPU scroll.
- The second half of `sub_D14E` writes `ram_for_2000 | ram_004D_base_nametable`, then `ram_scroll_X` and `ram_scroll_Y`. This is a mid-frame split, not a single static scroll value.
- The menu prelude explicitly zeroes both scroll pairs. `ram_0047` is raised in race setup (`ofs_001_C875_02_06`), **in the state-9 design-editor setup** (`ofs_001_C5C2_05_06`, `INC ram_0047` at `$C617`), and in gameplay object logic (`$EBAE`). It is cleared on menu/editor-return paths (`$CA33`, `$CAA7`, and the state-9 B-button exit via `sub_EB17`). The raster path is therefore required by the Stage 2 design-editor path, not just the race; B4 is a blocking Stage 2 task.

B4 is **traced, not implemented**. The current menu state model does not yet reproduce the two-phase `ram_0047` raster/OAM update. Do not replace it with `menu_scroll_x/y` alone.

## Ordered completion plan — work one item at a time

The boxes below are the complete known Stage 2 work queue. A box is checked only when the corresponding source audit, implementation, and regression evidence exist. "Menu state flow" excludes race simulation itself, but must correctly hand off to the race state.

### A. State graph and source trace
- [x] A1. Inventory all 14 state handlers, setup-list selector values, and all nine setup-list command sequences.
- [x] A2. Identify title, selector, design, save/load, race-entry, and shared transition paths from the state handlers.
- [x] A3. Trace every branch and return from `ofs_CB3B_00`, `ofs_CB77_01`, `ofs_CBA4_02`, and `ofs_CC6B_03`; record exact conditions and state writes (see the transition-dispatcher trace below).
- [x] A4. Classify state-$09 as the interactive course-design/editor loop, distinct from race states `$02/$05/$0A`; trace its entry, per-frame path and B-button return to state `$08`. Its setup list 5/editor implementation remains a separate C3 handoff dependency.
- [ ] A5. Finish transitive traces for all reachable setup commands. List 5's six commands and C5C2 state writes are now inventoried; `sub_E1A3`, `sub_EC25`, `loc_EC47`, C820 dynamic PPU uploads and remaining list-3 dependencies still need full traces.
- [ ] A6. Audit reset/cold-start, return-to-title, cancel/back, and all menu-to-race handoffs against the actual ASM call order.

### B. PPU, OAM, and visual reconstruction
- [x] B1 (selection trace) — Map the static `$01-$19`, palette `$12-$18`, dynamic `$00`, and zero-page `$1A` sources and verify NROM/PPUCTRL pattern-table selection. The renderer fidelity portion remains open under B3-B7.
- [x] B2. Verified all 18 static PPU streams and all 7 palette streams, dynamic `$0300` upload, zero-page `$00A1` tape-load path, malformed records, bounds, terminators and palette mirroring with host tests. See validation checkpoint below.
- [x] B3. Implemented mapper-0 vertical address mirroring plus two-page composition using PPUCTRL base nametable bits and `$2005` scroll; host regressions cover A/B selection and horizontal scroll across the page boundary. Hardware/reference comparison remains D2/D3.
- [ ] B4. Source scroll/raster path traced (`sub_D0AB`, `sub_D14E`, `sub_D1B3`); implement the two-phase `$0047` NMI update and validate its scanline/OAM timing.
- [ ] B5. Trace every menu OAM writer and all cursor/decoration sprites, sprite priority, flips, size, scanline limits, and visibility; render all supported source attributes.
- [ ] B6. Reconstruct authentic title, selector, design, save/load, and transition artwork from actual CHR + PPU writes; remove diagnostic text/placeholder rendering from the normal menu path.
- [ ] B7. Verify menu palettes and palette transitions using the actual PPU palette RAM writes; account for palette mirroring and PPUMASK grayscale/emphasis if used.

### C. Menu input and transitions
- [x] C1. `sub_C7DE` input semantics are now executed from the statically recompiled 6502 code at runtime when a supported mapper-0, 16 KiB `rom.nes` is beside the 3DSX. `nesMenuCpuInit()` loads the original PRG; `nesMenuCpuInput()` seeds the relevant NES RAM and calls `$C7DE` through `nesCall()`. Title (count 3), course selector (count 5), and design menu (count 6) all use this path. The source-derived C helper remains a fallback for raw-CHR-only installations. The routine is already included in the differential `region_list.h` test set. This integration has not yet received a fresh CI run or physical New 3DS validation.
- [x] C2. Title entries 0/1 -> state 1, title entry 2 -> state 8, selector navigation and confirmation -> state 2 are implemented and covered by the menu frame-model tests. The destination race runtime remains Stage 3.
- [ ] C3. Complete all six design-menu choices: implement list 5/state-9 setup handoff and B-button return to state 8; preserve state-10 race handoff for Stage 3; finish save/load and return-to-title paths.
- [ ] C4. Complete the shared state-3/6/0B and state-4/7 transition sequencers with correct frame/timer behavior.
- [ ] C5. Complete save/load error, retry, header mismatch, missing file, corrupt file, and successful return behavior; clearly document differences from the original recorder hardware.

### D. Validation and acceptance
- [ ] D1. Add host regressions for every menu state, setup list, input edge case, transition, PPU/OAM write, and tape backend branch.
- [ ] D2. Create deterministic NES reference captures for each menu screen and transition (ROM identity/hash, emulator/version, frame, PPU/OAM state, and screenshot).
- [ ] D3. Compare native frame output and PPU/OAM state against those references; fix all visible/source-state mismatches.
- [ ] D4. Run full host test suite and exact-commit devkitARM/New 3DS build; retain CI links and commit SHA.
- [ ] D5. Verify cold boot, every menu route, save/load and return on a physical New Nintendo 3DS; record evidence.
- [ ] D6. Accept Stage 2 only after A–D pass. Keep Stage 3 blocked until then.

### Setup list 3: source-level command trace (`$C052-$C05A`)

This list is selected for states `$03/$06/$0B` and is currently **not executed by the native setup sequencer**. The source commands are:

| Index | Routine | Exact observed effects | Native status |
|---:|---|---|---|
| 0 | `ofs_001_C55E_03_01` | Writes `ram_0045=$06`; the NMI later uploads PPU stream 6 and clears the request. | Not wired into list 3 |
| 1 | `ofs_001_C56A_03_02` | Writes `ram_0033_timer=$04`, then `ram_0045=$04`. | Not wired into list 3 |
| 2 | `ofs_001_C455_03_03` | Starts with stream `$08`. If `ram_0041 < 8`, it only posts a stream when `ram_0046 != 0`, in which case it uses `$07`; if `ram_0041 >= 8`, it posts `$08`. | Not wired into list 3 |
| 3 | `ofs_001_C5A7_03_04` | Loads A=0 and jumps to `loc_C92C`; its tuple/index writes are handled by `sub_C92C` and downstream table lookups. | Helper translation exists, sequencer integration absent |
| 4 | `ofs_001_C485_03_05` | Calls `sub_C53A` to fetch the current 3-byte tuple from `$0580+`; compares it with the tuple at `$0068-$006A`; may copy the latter into `$0580+`; otherwise compares against the ROM tuple via `sub_C522`. It then computes `ram_0053` using 8-bit arithmetic based on `ram_03F8` and the tuple difference, with overflow path `$FF`, and stores the result. | Helper translation exists in `nesSubC485UpdateMenuSelection`; sequencer integration absent |

Transitive dependencies for command 4:

- `sub_C53A` loads `ram_0043`, calls `sub_CB06`, then reads the three-byte tuple at `$0580,Y`.
- `sub_CB06` calls `sub_CB13`; `sub_CB13` computes a 3-byte tuple offset from `ram_0043`, with special case 5 returning base `$20`, and adds `$10` when `ram_0046` is set. `sub_CB06` adds `$23` when `ram_03F2` is nonzero.
- `sub_C522` uses `sub_CB13` and reads the ROM tuple from `tbl_C091`.
- The comparison and distance calculation in `$C485-$C512` uses unsigned 8-bit carry/borrow and wrap behavior; it must not be simplified to ordinary signed arithmetic.
- Existing host helper tests cover `nesSubC485UpdateMenuSelection`, `nesSubC522SelectOptionTuple`, `nesSubC53AReadMenuTuple`, and `nesSubCB06/CB13` behavior, but they do not prove that the runtime setup sequencer invokes these routines in order.

**Next implementation dependency:** add the missing state bytes and a complete RAM view needed by the C5A7/C485 routines, then wire all five list-3 commands into the sequencer. Do not mark the list complete just because the individual arithmetic helper already has unit vectors.

### State $09: active-frame path, not yet classified as a menu

The source handler `ofs_000_C97C_09` is:

```asm
JSR sub_CA1B
JSR sub_EB17
JSR sub_DA6A
JSR sub_DBFE
JSR sub_E70B
JSR sub_F4FF
JSR sub_F68D
JSR sub_F755
JSR sub_C229
JMP loc_D19B
```

Source-backed observations:

- `sub_CA1B` copies the current held-controller byte `ram_btn_hold` to `ram_005C_obj`.
- `sub_DA6A` advances horizontal scroll from `ram_0060` and toggles `ram_004D_base_nametable` on carry.
- `sub_DBFE` updates `ram_03BF`, `ram_0050_scroll_X`, and toggles `ram_004E_base_nametable` on carry.
- `sub_E70B` iterates object slots 3..0 and adjusts object values/animation state.
- `sub_F4FF` selects a table through `ram_0043` and has an explicit state-$09 branch.
- `sub_F68D` and `sub_F755` inspect/update object slots and their associated data.
- `sub_C229` resets three bytes in each of `$05A0` and `$05C3`.
- `loc_D19B` waits for PPU vblank, updates PPUCTRL and scroll registers, and returns to the frame loop.

This is demonstrably a live per-frame object/scroll update path, not a static menu draw routine. The design-menu selection value 2 enters state $09 (via `$C3D2`). However, the available excerpts do not prove whether state $09 is a track editor, a design preview, or a gameplay mode. **Do not map it to a guessed screen or silently treat it as a completed menu.** A4 remains open until its entry setup, state-specific table selection, object meaning, and exit conditions are traced end-to-end.

### Title and course-selector handoff checkpoint (commit `630a6a4b77a0`)

The integrated menu frame-model tests now cover:
- Title menu cursor positions, Down/Up wrap and held-key repeat guard.
- Title entry 0 -> selector state 1, with the source mode/setup flags.
- Title entry 1 -> selector state 1, preserving `ram_03F2=1`.
- Title entry 2 -> design menu state 8 (via the `designMenu()` setup path).
- Selector Left/Right remapped to Up/Down, all five X positions and wraparound.
- Selector Start -> race handoff state 2, selected course in `ram_0043`, and reset setup sequencer flags.

Exact commit `630a6a4b77a0c4ec4ef3ec0d75935b95718d0689`: host verification run 44359376152 passed; New 3DS build run 44359376392 passed.

C2 is complete for the title/selector menu boundary. State 2's setup/race is still outside Stage 2; design-menu branches to state 9/10/12/13 remain under C3.

### Design-menu branch coverage checkpoint

The existing `testDesignConfirm` exercises all six selection values in `ofs_000_C3BD_08`:
- 0 and 1 -> state `$0A` (race handoff; Stage 3).
- 2 -> state `$09` (course-design editor; list 5 remains unsupported).
- 3 -> state `$0C` (save), with the file-backed recorder replacement covered by `testTapeSave`.
- 4 -> state `$0D` (load), including missing-file, header mismatch, retry/checksum failure and successful return in `testTapeLoad`.
- 5 -> clear `$0021-$00FF`, return to state `$00`, restore selection 2.

Thus the menu's six-way branch decision and the save/load/title-return paths are covered. The unresolved menu destination is state 9: it enters setup list 5, which is deliberately held by the current sequencer because its object/OAM/PPU dependencies are not integrated. The state-10 race path is a Stage 3 destination, not an unfinished menu cursor branch.

### `sub_C7DE` input equivalence checkpoint (commit `b2fa4fb21146`)

The native menu input helper now preserves the complete menu-visible side effects of the ASM routine:

- Reads NES press-edge bits `$3C`: Select `$20`, Start `$10`, Up `$08`, Down `$04`.
- Clears `ram_0049` only when none of those four buttons is pressed; otherwise sets `ram_0031_timer=$5D` before checking the repeat guard.
- Honors the existing `ram_0049` guard; a guarded input does not move the selection or change `ram_00FF`.
- Priority is Select, then Down, then Up, then Start. Down/Select wrap to index 0; Up from index 0 wraps to `count-1`.
- Start sets `ram_0030_timer=4` and returns action X=4 without changing selection.
- The source writes `ram_00FF=1` only through `bra_C817` on cursor movement (Up/Down/Select); Start jumps directly to `bra_C81B` and must not set it. The model now stores this byte in `GameState.menu_ram00FF`.
- State 1 remaps physical Left/Right to the NES Up/Down bits before calling the helper, matching `ofs_000_C395_01`.

Host vectors cover down/up wrap, six-entry wrap, Select priority, Start, repeat-guard hold, idle guard reset, and 1,000 bounded selection updates. Exact commit `b2fa4fb2114615ac84bc1b360fe40b0a8a4e471b`: host verification run 44359040421 passed; New 3DS build run 44359040418 passed.

C1 is complete. C2 (full title/selector transition flow) remains open; this helper equivalence alone does not complete the menu state machine.

### Setup list 5: state-$09 course-design editor initialization

List 5 (`off_C064_05`) is selected for state `$09`. It is not currently accepted by `menusRunSetupSequence`; the native sequencer stops at the unsupported-list guard. Its six commands are:

| Index | Routine | Source effects |
|---:|---|---|
| 0 | `ofs_001_C551_05_01` | For state `$09 >= 8`, posts `ram_0045=$12` (palette stream); unlike states below 8, it does not add `ram_0043`. |
| 1 | `ofs_001_C869_05_02` | Clears CPU RAM `$004D-$00EF`. |
| 2 | `ofs_001_C820_05_03` | Calls `sub_D13F` (three `sub_ECE4` calls at X=0,4,8); sets dynamic PPU pointer `$0000=$0300`, posts `ram_0045=$01`, clears `$0340-$03EF`, fills `$0400-$047F=$3B` and `$0480-$057F/$0500-$057F=$3D`, initializes sprite 0 as Y=`$BF`, tile=`$FF`, X=`$F7`, and sets `$00E8-$00EB` to `$3F,$1F,$25,$0A`. Dynamic PPU upload is source-faithful only after the complete `sub_D13F/sub_ECE4/sub_D3C1` path is integrated; see `STAGE3_TRACE.md`. |
| 3 | `ofs_001_C5A3_05_04` | Posts `ram_0045=$17`. |
| 4 | `ofs_001_C562_05_05` | Posts `ram_0045=$05`. |
| 5 | `ofs_001_C5C2_05_06` | Initializes the editor's object/course state and initial OAM. |

Command 5 (`$C5C2-$C61E`) details:

- Sets `ram_00B7=1`; calls `sub_C621`, which copies all 256 bytes `$06E0-$07DF` to `$05E0-$06DF` using the 8-bit X wrap.
- Clears `ram_00BC+1`, `ram_00BC+2`, `ram_00C4`, `ram_00C0`, `ram_0049`, and `ram_00C0+2`; then sets `ram_05E0=1`.
- Sets object fields `ram_00AC=6`, `ram_008C=$A0`, `ram_0080=$10`; sets OAM write pointer `$0004=4`, `$000B=0`, and calls `sub_E1A3`.
- Copies `tbl_C178[0..7]` to OAM `$0228-$022F`.
- Initializes `ram_00B0-$00B3` to `[$21,$05,$05,$03]`, sets `ram_0064=8`, `ram_00E0=9`, `ram_00B4=$7A`.
- Increments `ram_0040` and `ram_0047`, calls `sub_EC25` with A=`$D0`, then jumps to `loc_EC47`.

**Existing isolated translations:** `nesSubE1F9SelectObjectSprite()` and `nesSubE1A3PrepareObjectSprite()` already model the object-selection/table preparation path; `testNewE1F9` and `testSubE1A3Exact` cover selected branches. `nesSubC621CopyPage()` and `nesSubEC3BCopyPageDescending()` also exist. These helpers are not yet wired into the list-5 runtime sequence.

**Unresolved dependencies before list 5 can be accepted:** the final `sub_D1C7` OAM record emitter, direct `sub_EC25`/`loc_EC47` sprite writes, full state-9 object-array backing, C820's dynamic PPU uploads, and the `$0047` split-scroll timing. Do not replace these with a static placeholder bike or approximate object state.

### List-5 object/OAM dependency trace (source audit)

The final editor setup command is more than a few OAM constants:

- `sub_E1A3` reads object fields `ram_0080,X` and `ram_008C,X`, calls `sub_E1F9` to derive an object-animation/table index, looks up `tbl_D892` and `tbl_D8A9`, then calls `sub_D1C7` to write OAM data. Depending on the returned `ram_000E_temp` mode it writes a row of descending tile IDs through the indirect OAM pointer or writes the special `$AE` marker at offset `$15`.
- `sub_E1F9` begins by clearing `ram_000E_temp` and `ram_039F`, then branches on `ram_009C_obj,X`, `ram_00BC_obj,X`, `ram_0370_obj,X`, `ram_0036_timer,X`, `ram_0052_track_finished_flag`, and `ram_00AC_obj,X`. It has additional object-state and mode branches beyond the initial excerpt; it must be fully traced before porting `sub_E1A3`.
- `sub_D1C7` is the shared indirect OAM writer. It derives an OAM destination from `ram_0003_t02`, `ram_0004_t01_data_oam_address`, and loop counts; it can write multiple 4-byte OAM records and special marker values. A direct fixed sprite copy is not equivalent.
- `sub_EC25` writes OAM slot `$0230-$0233`: Y=A, tile=`$CA`, attributes=0, X=`tbl_E6F6[ram_00C2+2]`.
- `loc_EC47` writes OAM slot `$0234-$0237`: Y=`$CF`, tile=`ram_05E0`, attributes=0, X=`$E4`.
- `sub_EC3B` copies the 256-byte editor buffer `$05E0-$06DF` back to `$06E0-$07DF` using X wrap. This is the editor's commit/save-back path when leaving state 9.

This narrows the work but does not close A5: E1F9/E1A3 helpers already have host vectors, while D1C7's final OAM emission, EC25/EC47 integration, C820 dynamic PPU upload and the editor's PPU/scroll split are still missing.

### State $09 classification: course-design/editor loop

The design menu state `$08` has six choices. The state handler at `$C3BD-$C3E7` routes:
- selection 0 -> state `$09`, with `ram_03F2=0`;
- selection 1 -> state `$0A` (race handler; Stage 3);
- selection 2 -> state `$09`, preserving the existing `ram_03F2` mode value;
- selection 3 -> state `$0C` (save);
- selection 4 -> state `$0D` (load);
- selection 5 -> clear `$0021-$00FF` and return to title state `$00`.

State `$09` is not the title menu or race loop. It uses setup-list index 5 (`off_C064_05`) and then runs a dedicated active-frame course-design path. The per-frame handler:
1. copies controller hold to `ram_005C_obj` (`sub_CA1B`);
2. updates object/course state (`sub_EB17`);
3. updates horizontal scroll and nametable page (`sub_DA6A`, `sub_DBFE`);
4. updates object animation and track data (`sub_E70B`, `sub_F4FF`);
5. updates object/collision-related state (`sub_F68D`, `sub_F755`);
6. resets row buffers (`sub_C229`) and waits for vblank via `loc_D19B`.

**Return to design menu:** in `sub_EB17`, when the editor's mode/object gates permit input and `ram_0049=0`, the held B-button bit (`ram_005C_obj & $40`, after two ASLs) enters `bra_EBA8`. That branch calls `sub_C3B1` and `sub_EC3B`, then writes `ram_0047=0`, `ram_0042=0`, and `ram_0041=8`. This is the editor's explicit return to the six-choice design menu. Other branches process editor controls and course data; they are not a menu cursor.

**Scope boundary:** Stage 2 must implement the design-menu choices and the correct setup/return handoff. The full interactive course editor in state 9 is not itself a menu screen; its editing mechanics can be tracked separately from the Stage 3 race. However, the current native setup sequencer does not implement list 5, so selecting a state-9 option is not yet a working handoff. C3 remains open until the state-9 setup list is source-faithfully executed and the editor can return to state 8 without placeholders.

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


## Source-recompiler runtime integration — 2026-11-19

The new static recompiler is now used by the active Stage-2 menu input path rather than remaining only a host-side experiment:

- `source/nes_menu_cpu.c` loads the 16 KiB PRG from `rom.nes` beside the launched 3DSX, accepting only the verified iNES mapper-0 / one-PRG-bank layout (with optional trainer offset).
- It invokes the generated `nesRunCode()` through `nesCall()` at the original routine address `$C7DE`.
- The runtime seeds and copies back the exact RAM bytes used by `sub_C7DE`: `$0015`, `$0030`, `$0031`, `$0042`, `$0049`, and `$00FF`; the 6502 X return value is used as the action result.
- Title, course-selector, and design-menu input dispatch now prefer this recompiled routine. If a supported full ROM is not available (for example, a raw CHR-only setup), the existing tested native projection remains the fallback.
- `nes_gen_code.c` is already part of the source wildcard in the 3DS Makefile, so the generated CPU implementation and this adapter are linked by the ordinary build.

**Validation boundary:** `$C7DE` is present in `3ds-port/tests/oracle/region_list.h`, so the existing differential suite includes the source routine. However, no CI status for the new adapter commits has been confirmed yet, and the new runtime ROM-loading path has not been exercised on a physical New 3DS. This closes only the menu input helper integration; setup lists 3 and 5, transition rendering, PPU/OAM completion, and original-screen comparisons remain open.
