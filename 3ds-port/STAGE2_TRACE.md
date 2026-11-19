<!-- NATIVE-PORT STATUS NOTICE (2026-11-19): This file is retained as source-trace/history evidence. Any old menu implementation described as PRG-backed, recompiled, PPU/NMI-driven, RAM-backed or emulator-dependent is retired and MUST NOT be restored to the 3DS runtime. Reimplement the documented observable menu behavior in semantic native C state, native rendering and native input. The current controlling plan is TODO.md; architecture is NATIVE_PORT_ARCHITECTURE.md. Historical tests/vectors may be reused only after confirming they test game behavior rather than NES hardware emulation. -->

## Native C translation progress — 2026-11-19

The former 6502 recompiler / NES-system runtime is not the target and has been deleted from the active game source tree. The functions below are native C reimplementations; the original PRG/ASM is executed only by the host-side differential oracle.

- Menu selection: `nativeMenuUpdateSelection` translates `sub_C7DE`; 393,216 vectors pass.
- Title/selector/design state handlers: `nativeMenuUpdate` covers source states 0, 1 and 8.
- Shared menu transition phase 0: `nativeMenuTransitionPhase0` translates `CB3B` for states 3/6/11; 51,840 vectors pass.
- Shared menu transition phase 3: `nativeMenuTransitionPhase3` translates `CC6B`, including the original A-register-zero write when `$004B=0`; differential tests cover all state values and relevant flag/setup/phase combinations.
- Platform input adapter: `inputMapNesButtons` maps libctru A/B/Select/Start/D-pad bits into the original NES controller byte; all 256 combinations pass.
- Timer-expired timed transition: `nativeMenuCompleteTimedTransition` translates the `C477-C484` branch used by `C465`.
- Timer bank: `nativeMenuTickTimers` translates `sub_D310` and is compared to ASM.
- Physics and movement slices are listed in `STAGE3_TRACE.md` and `TODO.md`; they do not yet constitute a complete playable physics loop.

The latest exact code commit, `f214e5bbc96473896ca3abbbbf9e8cb6c8cc05f8`, passed host differential CI [44585477913](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44585477913) and the New 3DS build [44585478080](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44585478080).

**Important:** these are isolated source-derived native functions, not a complete menu, renderer, editor, or game runtime. The remaining sections below are historical source-trace evidence. Any mention of PRG-backed recompiler execution describes the retired experiment and must not be reintroduced into the product runtime.

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
| `$03` | `ofs_000_CB2C_03` | 3 | Shared transition dispatcher; PRG-backed setup and all four phases have ROM vectors |
| `$04` | `ofs_000_C465_04` | 4 | Partial transition/timer and setup-list support |
| `$05` | `ofs_000_C99A_05` race | 2 | Stage 3, blocked |
| `$06` | `ofs_000_CB2C_06` | 3 | Same shared transition dispatcher; state-6 phase-0/phase-3 vectors added |
| `$07` | `ofs_000_C465_07` | 4 | Partial transition/timer and setup-list support |
| `$08` | `ofs_000_C3BD_08` design menu | 6 | Input/cursor/confirm branches and list 6 partially translated |
| `$09` | `ofs_000_C97C_09` | 5 | Course-design/editor frame; list-5 setup, C97C frame and guarded B-return are PRG-backed |
| `$0A` | `ofs_000_C99A_0A` race | 2 | Stage 3, blocked |
| `$0B` | `ofs_000_CB2C_0B` | 3 | Same shared transition dispatcher; state-11 phase-0/phase-3 vectors added |
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
- Design state `$08`: selections 0/1 set the mode and advance twice to state `$0A` (race handoff, Stage 3); selection 2 advances to state `$09` (course editor); selection 3 enters save state `$0C`; selection 4 enters load state `$0D`; selection 5 clears `$0021-$00FF` and returns to title state `$00`. The reset/return branches now have host vectors; natural in-game route and hardware validation remain.
- States `$03`, `$06`, `$0B` share the indirect dispatcher at `$CB2C`, selecting `ofs_CB3B_00`, `ofs_CB77_01`, `ofs_CBA4_02`, or `ofs_CC6B_03` by `ram_0044`. They are a shared transition state machine, not three independent screens.
- States `$04` and `$07` share `$C465): wait on `ram_0034_timer`, update a table tuple through `sub_C92C`, then increment state and normalize to `$02` when the old state is below `$08`; otherwise return to `$08`.
- State `$09` is the interactive course-design/editor loop. Its six-command setup list, per-frame `$C97C` handler and guarded `$EB17` B-return now execute through the PRG recompiler with persistent CPU RAM. The `$D19B` PPUSTATUS wait is still frame-boundary approximated, and every editor object interaction has not yet been reference-verified.
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

The native PPU path now keeps both physical 1 KiB pages and the mapper-0 mirroring mode. `rendererDrawPpuBackground()` passes the full two-page nametable image to `nesPpuRenderMirroredNametablesOpaque()`, which applies PPUCTRL base selection, scroll, vertical/horizontal mirroring and `$3000` aliases. Host regressions cover page A/B, horizontal scroll across the page boundary and the lower logical row. B3 implementation is complete; original-NES frame comparison remains under D2/D3.

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
- This verifies stream and pattern-table selection. The native renderer now composes both physical nametables, palette RAM and supported OAM sprites; B4 raster timing and complete sprite coverage remain under validation, and artwork acceptance still requires original-NES reference captures.

### Mirroring implementation checkpoint (commit `79a3f28136bc`)

Implemented in the native PPU command path:

- Added `nesPpuApplyCommandStreamMirrored()` with explicit horizontal/vertical mirroring modes; retained the original flat helper as a compatibility wrapper.
- Added canonical PPU address handling for `$3000-$3EFF` mirrors and palette aliases.
- Routed Excitebike static stream uploads and source PPU snapshots through **vertical** mirroring, matching iNES header flag 6 = `$01`.
- Added host vectors that distinguish vertical from horizontal mirroring and verify the `$3000` alias.

Validation on exact commit `79a3f28136bc1507c42527178addb9ec8dab1eea`:
- Host workflow `Verify NES routine translations`: success (run 44356209040).
- New 3DS/devkitARM build workflow: success (run 44356209037).

B3 implementation is complete: the renderer composes both physical nametables with PPUCTRL base selection and scroll, and host regressions cover vertical/horizontal mirroring plus `$3000` aliases. No physical console screenshot/reference comparison was run; that remains D2/D3.

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

B4 is **integrated in the host runtime, with exact-hardware timing still open**. The timing-only PRG adapter now reproduces the Stage-2 NMI sequence in source order: `sub_C32F`, the conditional `$2001` write, OAM DMA, `sub_C27F`, `sub_D0AB`, then `sub_D14E`. It captures both PPUCTRL/scroll pairs and timestamps the second `$2005` write; the native PPU parser applies the same command stream to nametable/palette memory. The renderer composes top and bottom nametable/pattern-table/scroll phases at the captured scanline and dot. Sprite-0 hit is derived from decoded CHR, OAM position/flip/size, PPUMASK clipping and the split-composited background-opacity map; C97C receives the computed CPU-cycle threshold, and the next NMI preserves the previous hit until the modeled pre-render clear. ROM-backed tests cover both previous-hit and no-hit timing branches, both scroll pairs, split-dot composition and the editor wait. The exact-commit host workflow and New 3DS build passed on `19304fbc5293ebcd79e81a1afcf4bf760e9b0c9b`: [host tests](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44520711483) · [3DS build](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44520711481). Remaining timing debt: calibrate the 2273-cycle pre-render clear against exact PPU/NMI edge phase and odd-frame behavior; then compare raster/OAM output on original NES and physical New 3DS. Do not replace this path with `menu_scroll_x/y` alone.

## Ordered completion plan — work one item at a time

The boxes below are the complete known Stage 2 work queue. A box is checked only when the corresponding source audit, implementation, and regression evidence exist. "Menu state flow" excludes race simulation itself, but must correctly hand off to the race state.

### A. State graph and source trace
- [x] A1. Inventory all 14 state handlers, setup-list selector values, and all nine setup-list command sequences.
- [x] A2. Identify title, selector, design, save/load, race-entry, and shared transition paths from the state handlers.
- [x] A3. Trace every branch and return from `ofs_CB3B_00`, `ofs_CB77_01`, `ofs_CBA4_02`, and `ofs_CC6B_03`; record exact conditions and state writes (see the transition-dispatcher trace below).
- [x] A4. Classify state-$09 as the interactive course-design/editor loop, distinct from race states `$02/$05/$0A`; trace its entry, per-frame path and B-button return to state `$08`. Its setup list 5/editor implementation remains a separate C3 handoff dependency.
- [ ] A5. Finish transitive traces for all reachable setup commands. List 5's six commands and list 3's five commands now have PRG-backed recompiler paths; the full state-9 frame loop and other reachable-list dependencies remain.
- [ ] A6. Audit reset/cold-start, return-to-title, cancel/back, and all menu-to-race handoffs against the actual ASM call order.

### B. PPU, OAM, and visual reconstruction
- [x] B1 (selection trace) — Map the static `$01-$19`, palette `$12-$18`, dynamic `$00`, and zero-page `$1A` sources and verify NROM/PPUCTRL pattern-table selection. The renderer fidelity portion remains open under B3-B7.
- [x] B2. Verified all 18 static PPU streams and all 7 palette streams, dynamic `$0300` upload, zero-page `$00A1` tape-load path, malformed records, bounds, terminators and palette mirroring with host tests. See validation checkpoint below.
- [x] B3. Implemented mapper-0 vertical address mirroring plus two-page composition using PPUCTRL base nametable bits and `$2005` scroll; host regressions cover A/B selection and horizontal scroll across the page boundary. Hardware/reference comparison remains D2/D3.
- [ ] B4. `sub_D14E` now runs from the original PRG when `$0047` is set, including the two scroll phases; integrate the preceding `sub_D0AB`, replace the frame-boundary sprite-hit approximation with PPU-derived collision, and validate scanline/OAM timing.
- [ ] B5. Trace every menu OAM writer and all cursor/decoration sprites, sprite priority, flips, size, scanline limits, and visibility; render all supported source attributes.
- [ ] B6. Reconstruct authentic title, selector, design, save/load, and transition artwork from actual CHR + PPU writes; remove diagnostic text/placeholder rendering from the normal menu path.
- [ ] B7. Verify menu palettes and palette transitions using the actual PPU palette RAM writes; account for palette mirroring and PPUMASK grayscale/emphasis if used.

### C. Menu input and transitions
- [x] C1. `sub_C7DE` input semantics are now executed from the statically recompiled 6502 code at runtime when a supported mapper-0, 16 KiB `rom.nes` is beside the 3DSX. `nesMenuCpuInit()` loads the original PRG; `nesMenuCpuInput()` seeds the relevant NES RAM and calls `$C7DE` through `nesCall()`. Title (count 3), course selector (count 5), and design menu (count 6) all use this path. The source-derived C helper remains a fallback for raw-CHR-only installations. The routine is already included in the differential `region_list.h` test set. This integration has not yet received a fresh CI run or physical New 3DS validation.
- [x] C2. Title entries 0/1 -> state 1, title entry 2 -> state 8, selector navigation and confirmation -> state 2 are implemented and covered by the menu frame-model tests. The destination race runtime remains Stage 3.
- [ ] C3. List 5/state-9 setup and the original guarded EB17 B-return branch now execute through the PRG recompiler; a ROM-backed test exercises the return branch with explicit gate-state preconditions. Still verify the natural in-game route, all editor object interactions, state-10 handoff (Stage 3), save/load, and return-to-title paths.
- [ ] C4. List 3 and the shared CB2C dispatcher are now connected to source PRG execution for states 3/6/0B; C465 already runs from the PRG for states 4/7. Add vectors for every dispatcher phase, state variant, and timer branch before treating the sequencers as complete.
- [ ] C5. Complete save/load error, retry, header mismatch, missing file, corrupt file, and successful return behavior; clearly document differences from the original recorder hardware.

### D. Validation and acceptance
- [ ] D1. ROM-backed regressions now cover list-3 setup/first dispatch phase, C465 transitions 4→5 and 7→2, all six list-5 setup commands, one C97C editor frame, and the guarded EB17 B-return branch. Add coverage for remaining states, input edges, PPU/OAM writes, and every tape backend branch.
- [ ] D2. Create deterministic NES reference captures for each menu screen and transition (ROM identity/hash, emulator/version, frame, PPU/OAM state, and screenshot).
- [ ] D3. Compare native frame output and PPU/OAM state against those references; fix all visible/source-state mismatches.
- [x] D4. Host workflow and exact-commit New 3DS build passed on `a547c0f2a341fc2bb17d23570aaca798aa452f39` (2026-11-19): [host tests](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44510600117) · [3DS build](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/44510600084). A later RAM-reset cleanup is being revalidated.
- [ ] D5. Verify cold boot, every menu route, save/load and return on a physical New Nintendo 3DS; record evidence. No physical-console verification is claimed.
- [ ] D6. Accept Stage 2 only after A–D pass. Keep Stage 3 blocked until then.

### Setup list 3: source-level command trace (`$C052-$C05A`)

This list is selected for states `$03/$06/$0B` and is now executed through the PRG-backed native setup sequencer when a supported `rom.nes` is loaded. The source commands are:

| Index | Routine | Exact observed effects | Native status |
|---:|---|---|---|
| 0 | `ofs_001_C55E_03_01` | Writes `ram_0045=$06`; the NMI later uploads PPU stream 6 and clears the request. | PRG-backed list-3 command 0 |
| 1 | `ofs_001_C56A_03_02` | Writes `ram_0033_timer=$04`, then `ram_0045=$04`. | PRG-backed list-3 command 1 |
| 2 | `ofs_001_C455_03_03` | Starts with stream `$08`. If `ram_0041 < 8`, it only posts a stream when `ram_0046 != 0`, in which case it uses `$07`; if `ram_0041 >= 8`, it posts `$08`. | PRG-backed list-3 command 2 |
| 3 | `ofs_001_C5A7_03_04` | Loads A=0 and jumps to `loc_C92C`; its tuple/index writes are handled by `sub_C92C` and downstream table lookups. | PRG-backed list-3 command 3 |
| 4 | `ofs_001_C485_03_05` | Calls `sub_C53A` to fetch the current 3-byte tuple from `$0580+`; compares it with the tuple at `$0068-$006A`; may copy the latter into `$0580+`; otherwise compares against the ROM tuple via `sub_C522`. It then computes `ram_0053` using 8-bit arithmetic based on `ram_03F8` and the tuple difference, with overflow path `$FF`, and stores the result. | PRG-backed list-3 command 4 |

Transitive dependencies for command 4:

- `sub_C53A` loads `ram_0043`, calls `sub_CB06`, then reads the three-byte tuple at `$0580,Y`.
- `sub_CB06` calls `sub_CB13`; `sub_CB13` computes a 3-byte tuple offset from `ram_0043`, with special case 5 returning base `$20`, and adds `$10` when `ram_0046` is set. `sub_CB06` adds `$23` when `ram_03F2` is nonzero.
- `sub_C522` uses `sub_CB13` and reads the ROM tuple from `tbl_C091`.
- The comparison and distance calculation in `$C485-$C512` uses unsigned 8-bit carry/borrow and wrap behavior; it must not be simplified to ordinary signed arithmetic.
- Existing host helper tests cover `nesSubC485UpdateMenuSelection`, `nesSubC522SelectOptionTuple`, `nesSubC53AReadMenuTuple`, and `nesSubCB06/CB13` behavior, but they do not prove that the runtime setup sequencer invokes these routines in order.

**Remaining validation:** the ROM-backed test currently proves list-3 setup and the first state-3 dispatcher phase. Add vectors for state 6 and 11, all four CB2C phases, timer branches, and the downstream OAM/PPU writes before calling the transition animation fidelity-verified.

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
- 2 -> state `$09` (course-design editor; all six setup commands now have a recompiled path, but the editor frame loop is incomplete).
- 3 -> state `$0C` (save), with the file-backed recorder replacement covered by `testTapeSave`.
- 4 -> state `$0D` (load), including missing-file, header mismatch, retry/checksum failure and successful return in `testTapeLoad`.
- 5 -> clear `$0021-$00FF`, return to state `$00`, restore selection 2.

Thus the menu's six-way branch decision and the save/load/title-return paths are covered. State 9 is the course editor: its six setup commands and `$C97C` per-frame handler now execute through the static recompiler, with the original `$EB17` B-button branch. The state-10 race path is a Stage 3 destination, not an unfinished menu cursor branch.

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

List 5 (`off_C064_05`) is selected for state `$09`. The native sequencer now dispatches its six commands through the PRG-backed recompiler when a supported `rom.nes` is available. Its six commands are:

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

**Runtime integration:** `$C5C2` now executes through the generated 6502 code with persistent 2 KiB CPU RAM, so the original `$C621`, `$E1A3`, `$E1F9`, `$D1C7`, `$EC25` and `$EC47` call chain runs together. The isolated C helpers remain useful for differential tests, but are not substituted for this original instruction path.

**Unresolved dependencies after setup:** the state-$09 frame handler `$C97C` and its object/update/render callees, the `$D19B` vblank wait, the `$0047` split-scroll/raster timing, B-button return through `$EB17`, and deterministic original-NES frame comparison. Do not replace these with a static placeholder bike or approximate object state.

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

**Scope boundary:** Stage 2 must implement the design-menu choices and a working setup/return handoff. The six list-5 setup commands now have a recompiled path, but state 9 is not yet a working handoff because its per-frame handler and B-button return are not integrated. C3 remains open until the editor can run and return to state 8 without placeholders.

### Shared transition dispatcher: completed branch trace (`$CB2C-$CC7C`)

All three state handlers `$03/$06/$0B` enter the same code. `ram_0044` is an index into the four-entry pointer tables `tbl_C12C_lo`/`tbl_C130_hi`:

| `ram_0044` | Target | Source behavior |
|---:|---|---|
| 0 | `ofs_CB3B_00` | Copies `ram_03F2` to selection `ram_0042` when `ram_0041 < 8`. If `ram_0053 >= 4`, or state `>=8`, it skips the selection update. Otherwise, when `ram_0046` is set, it advances `ram_0043` with a special blink/half-rate path for selection 4 and `ram_03F8`; then sets `ram_0046=1`, `ram_004B=1`, and starts timer `$0033 = $04`. Phase 1 later sets `$03AD` to `$20` or `$31` and `$0033` to `$23` or `$34`. |
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
- The native renderer has a source-derived static/dynamic PPU command interpreter and CHR/nametable/palette/sprite compositor. The editor NMI timing adapter executes C32F, conditional mask write, OAM DMA, C27F, D0AB and D14E; it captures both scroll phases and the second $2005 write cycle. The renderer composites the raster split and supplies pixel-derived sprite-0 hit timing to C97C. Exact NMI-edge/pre-render calibration, odd-frame behavior and original-NES reference comparison remain open.
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


## Source-recompiled editor setup prelude — 2026-11-19

The state-$09 setup list 5 now has a recompiled runtime path for all six original commands when a supported `rom.nes` is loaded:

- Command 0 executes `$C551` and posts `$12` for editor state 9.
- Command 1 executes `$C869` and clears `$004D-$00EF`.
- Command 2 executes `$C820`, including all three `$ECE4` builders, their dynamic `$D3C1` PPU uploads, the `$0340-$03EF` clear, the `$0400-$057F` setup and sprite-0 initialization. The adapter captures `$2006/$2007` writes into the existing vertical-mirroring nametable and palette model; it does not fake the stream contents.
- Commands 3 and 4 execute `$C5A3` and `$C562` (`ram_0045=$17/$05`).
- Command 5 executes `$C5C2`, including `$C621`, `$E1A3/$E1F9/$D1C7`, `$EC25` and `$EC47`, using persistent 2 KiB NES CPU RAM. This connects the existing D1C7 OAM implementation through the original instruction stream rather than manually approximating its output.
- The adapter only whitelists these audited entry points. `$C820` and `$C5C2` require the supported PRG image; if it is unavailable or execution stops, the sequencer rolls back to that command instead of silently skipping it. `$C551/$C869/$C5A3/$C562` retain their source-equivalent C fallbacks.

**Important boundary:** the six setup commands, per-frame state-$09 handler `$C97C`, and guarded B-button return via `$EB17` are now integrated through the PRG recompiler. The live PPUSTATUS/raster timing remains approximate; every object interaction, shared-transition OAM branch, artwork and reference-frame comparison still need verification.

**Regression:** `tests/test_menu_cpu_rom.c` loads the original 16 KiB PRG and verifies all six list-5 setup commands, one C97C editor frame, and the guarded EB17 B-return branch. `testEditorSetupPrelude` separately verifies that the no-ROM fallback remains blocked at `$C820`.

**Validation boundary:** the host ROM integration workflow and exact-commit New 3DS build passed on the later integration commits recorded below. Physical-hardware and original-NES reference capture comparison remain outstanding.


### 2026-11-19 — state 4/7 transition handler and host-link repair

- [x] ofs_000_C465_04 / ofs_000_C465_07 are now dispatched by menusUpdate through nesMenuCpuRunRamRoutine($C465) when the original PRG is loaded.
- [x] The adapter carries persistent 2 KiB CPU RAM and synchronizes the menu-visible timer/state bytes plus $0300-$031F dynamic command buffer. The original routine's sub_C92C -> sub_C935 -> sub_CAFA chain remains in generated 6502 code; no guessed C replacement is used.
- [x] Added $C465 to the adapter's audited entry-point allowlist.
- [x] Fixed the host menu workflow to link nes_menu_cpu.c, nes_call.c, and nes_gen_code.c; previously the menu test compile omitted the implementation of the newly called adapter functions. The workflow now triggers when the adapter source/header changes.
- [ ] A dedicated ROM-backed frame-vector test for both timer branches is still needed. Until that exists, this is an integration step, not a claim of full state-4/7 correctness.
- [ ] The full state-09 editor loop, natural B-button return route, pixel-accurate PPU raster/vblank timing, menu artwork, sound, and hardware comparison remain open.


### 2026-11-19 — original state-09 editor frame dispatch

- [x] Added `nesMenuCpuEditorFrame`, executing the original recompiled `ofs_000_C97C_09` entry once per host frame with persistent 2 KiB CPU RAM.
- [x] Passed NES-order held and pressed controller bytes to `$0014/$0015`; retained the original `sub_EB17` object/input logic, including its guarded B-button return to state `$08`.
- [x] Synchronized editor OAM, `$0300-$031F` dynamic PPU buffer, course/object working pages `$05A0/$05C3/$05E0/$06E0`, scroll, PPUCTRL/PPUMASK, nametable and palette writes back into GameState.
- [x] Added a regression test proving that, without a supported `rom.nes`, state 9 holds instead of inventing a return or modifying the PPU model.
- [ ] `sub_D19B` waits for PPUSTATUS bit `$40` (sprite-0 hit). vec_C23B_NMI now runs the original D14E routine before the C97C editor handler whenever ram_0047 is set. D14E executes its 6431-cycle delay and scroll/PPU writes; the following C97C call sees sprite-hit bit $40. Pixel-level sprite collision remains approximated, not a full PPU pipeline; exact editor visuals and timing are not yet claimed.
- [ ] Add ROM-backed vectors for entry/setup/frame/B-return, validate PPU writes against a NES emulator trace, and test on physical New 3DS.


### 2026-11-19 — NMI raster split ordering

- [x] Added `nesMenuCpuRasterSplit` for the original `sub_D14E` routine.
- [x] `menusNmiUpload` now runs D14E after the NMI PPU stream upload when persistent `ram_0047` is nonzero, matching the source order before the state handler.
- [x] D14E's source delay loop, dynamic `$0300` command construction, PPUCTRL and scroll writes are executed by the static recompiler and synchronized back to GameState. The next state-09 C97C call sees sprite-hit bit `$40` set.
- [ ] The host bus models the sprite-hit flag at frame boundaries; it does not render pixels to determine the real sprite-0 collision. Emulator trace and physical New 3DS timing comparison remain mandatory.


### 2026-11-19 — source-PRG-backed integration regression

- [x] CI now wraps the exact 16 KiB PRG emitted from `Excitebike/bank_FF.asm` in a temporary iNES header, loads it through `nesMenuCpuInit`, and exercises the actual recompiled menu code.
- [x] The integration test verifies C465 timer-zero transitions `4 -> 5` and `7 -> 2`, all six state-9 list-5 setup commands, C5C2's object/course-page side effects, and one C97C frame receiving LEFT as NES controller bit `$02`.
- [x] The complete host workflow and New 3DS build both pass on commit `bc035d9` (2026-11-19).
- [ ] The guarded B-button exit now has a source-PRG-backed branch vector with seeded preconditions; natural route coverage, every editor object interaction, and pixel-accurate PPU timing remain open.


### 2026-11-19 — source-PRG-backed list-3 and shared-transition integration

- [x] Setup list 3 now executes its five original entries in order: $C55E, $C56A, $C455, $C5A7, and $C485.
- [x] States 3, 6, and 11 now dispatch through the original $CB2C pointer table using persistent 2 KiB CPU RAM; OAM and dynamic PPU command-buffer effects are synchronized back to GameState.
- [x] The runtime uses the original $D310 timer routine when a supported PRG is loaded, preserving the complete $0021-$003D timer bank instead of decrementing only the projected menu timers.
- [x] The ROM-backed integration workflow passes list-3 setup and the first state-3 dispatcher phase, alongside the C465, list-5, C97C, and guarded B-return vectors.
- [x] The exact-commit New 3DS build and host workflow pass on $a547c0f2a341fc2bb17d23570aaca798aa452f39.
- [ ] Add ROM-backed vectors for state 6 and 11, all remaining CB2C phases, and timer/input edge cases. Pixel-accurate PPU behavior and physical New 3DS validation remain acceptance blockers.


### 2026-11-19 — ROM-backed D0AB/D14E integration regression

- [x] The raster adapter now executes the original `sub_D0AB` vertical-band preparation before `sub_D14E`, matching `vec_C23B_NMI` call order.
- [x] Added a PRG-backed vector for object Y inputs `$10/$18/$20/$30`; it verifies `$0360-$0363 = 4/3/2/0`, `$004C = $003F & 3`, the two scroll/nametable phases, the `sub_D11F` dynamic-buffer header, and that state 9 does not hide editor OAM.
- [x] Host tests and New 3DS build passed on `7a400486fba8a39f1ce108e47754400bbc49aaac`.
- [x] Pixel-derived sprite-0 overlap, previous-hit persistence, full source NMI prelude cycle accounting, both PPUCTRL/scroll phases and scanline/dot split composition are integrated and host-tested. Remaining: calibrate the pre-render clear and odd-frame phase against hardware, then compare exact raster output against an original-NES capture.

## Native rewrite trace — shared transition phases 1 and 2 (2026-11-19)

The dispatcher at `$CB2C` selects one of four handlers from `ram_0044`: `$CB3B`, `$CB77`, `$CBA4`, or `$CC6B`. Phases 0 and 3 have isolated native C translations. This section records the source dependencies for phases 1 and 2 before translating their visible behavior into native scene state.

### Phase 1 — `ofs_CB77_01` (`$CB77-$CBA3`)

Confirmed from the unmodified `Excitebike/bank_FF.asm`:

1. Read `ram_0033_timer` into Y. If it is nonzero, return without changing the transition phase.
2. When it reaches zero, set the temporary table index to 4 and copy seven bytes from `ram_03D9,Y` through `ram_03DF` into the dynamic update-buffer area beginning at `ram_0304 + ram_0300`.
3. Call `sub_C94B`. It advances the legacy buffer length through `sub_C971`, selects `tbl_C0EC_ppu_address[4] = $24CF`, and appends the source command metadata and terminator. This buffer is a NES PPU upload description, not a data structure that may be retained by the native game.
4. Set `ram_03AD` to `$20` when `ram_004B` is nonzero, otherwise `$31`. Set `ram_0033_timer` to that value plus 3, then increment `ram_0044`.

The seven-byte source payload is read at timer-zero entry; the bytes are not yet classified as a native visual effect. The value at `$24CF` is a source PPU address only. With the cartridge's vertical mirroring, it resolves to physical nametable B, tile row 6, column 15; the native renderer must use a semantic screen/tile coordinate, not emulate a PPU address or command stream.

**Required before native implementation:** trace every producer of `ram_03D9-$03DF` (including the copy paths around `sub_D0F4` and `sub_DF48`), establish what the seven bytes represent in each caller state, and compare the resulting visible transition against the original. Then represent the effect as native menu-scene data and preserve only the source-derived timer/phase behavior. Do not copy `sub_C94B/sub_C971` as a runtime PPU-buffer builder.

### Phase 2 — `ofs_CBA4_02` (`$CBA4-$CC14`)

Confirmed first-pass control flow:

- A zero `ram_0033_timer` falls through the shared exit at `$CB9F`, increments `ram_0044`, and returns.
- A nonzero timer greater than `ram_03AD` returns without advancing the phase.
- Equality with `ram_03AD` enters the ten-frame gate at `ram_0020`; after the gate expires, the routine initializes timer/index state and iterates three table-selected transition records through `sub_CCCA`.
- Before equality, the branch depends on `ram_004B` and the held A/B controller bits. It calls `sub_CC15` or `sub_CC6B`, with the state-zero return path able to call `sub_C3AF`.
- The remaining paths select source tables `tbl_C0DA`, `tbl_C0DE`, `tbl_C0E2`, `tbl_C0E5`, and `tbl_C0E8`, then route into `loc_D1C7` to prepare OAM data. `sub_CC15` also constructs dynamic PPU upload data from `tbl_C134` and related state.

This is not yet a complete semantic trace. The native port must identify the player-visible transition phases, timing, overlay contents and input response, then express those as native state and renderer events. The dynamic PPU buffer, OAM page, and `loc_D1C7` algorithm are source evidence only; none belongs in the final game runtime.

### Next implementation gate

- [ ] Trace the seven-byte transition payload's producers and meaning.
- [ ] Trace the full phase-2 branch graph, including `sub_CC15`, `sub_CC6B`, `loc_CC7D`, `sub_CCCA`, and their source tables.
- [ ] Define semantic native transition fields and renderer events from the visible result.
- [ ] Add frame-by-frame differential tests for phase timing, input branches, state/flag changes and visible transition events.
- [ ] Integrate phases 1/2 into the native menu state machine only after the semantic effect is understood.

### Follow-up source audit — seven-byte phase-1 payload

A repository-wide read of `bank_FF.asm` found the following confirmed references to `ram_03D9-$03DF`:

- `sub_D0F4` copies all seven bytes from `$03D9-$03DF` to `$03D1-$03D7`, then prepares a separate transfer descriptor at `$03D0` for source address `$2356`; its caller `sub_D14E` invokes it during the PPU/OAM setup path when source state is not 9.
- `bra_DF48_loop` restores the seven bytes from `$03D1-$03D7` to `$03D9-$03DF`.
- `ofs_002_EA8F_08` copies the seven values from `$03D9-$03DF` into seven sprite Y slots in OAM page `$02C0-$02DF` when its track-finished/timer conditions are met.
- The phase-1 handler `$CB77` copies the seven values to the dynamic update stream, whose table entry 4 targets source PPU address `$24CF`.

This proves that the bytes are reused as visual/layout data in more than one source path; it does **not** yet prove that they are always seven ordinary tile IDs. Their meaning is contextual and must be resolved from the state-9/menu caller and the source data setup before the native renderer treats them as a tile row. Therefore the initial native phase-1 patch implementation is a provisional translation of the known destination/timing only; it is **not integrated** and must not be used to render the menu until the payload classification gate below passes.

### Native phase-1 code checkpoint — 2026-11-19

- Added `transition_duration`, a seven-byte semantic payload/patch, and native destination metadata to `NativeMenuState`.
- Added `nativeMenuTransitionPhase1`: it waits for timer slot `$0033` to reach zero, records the seven-byte payload as a native scene patch at layer 1 / tile coordinate (15,6), selects duration `$20` or `$31` from `ram_004B`, loads duration + 3, and advances phase 1 to phase 2.
- Added unit coverage for the timer wait, payload copy, destination metadata, both durations, and ignoring non-phase-1 calls in `test_native_menu_state.c`.

**Important limitation:** the phase-1 destination is derived from source address `$24CF` under the documented vertical mirroring map, but the seven-byte payload's visual interpretation remains open. The test suite now includes both native unit cases and a 64-vector differential check against the host-only ASM oracle (two transition-flag values × 32 deterministic payloads). The differential check compares phase advancement, `$03AD` duration, `$0033` timer and all seven payload bytes. The function is not called by the app loop and does not make the menu integrated or visually complete.

### Updated next gate

- [ ] Trace which state/callers populate `$03D1-$03D7` and `$03D9-$03DF`, including the state-9 branch around `sub_D14E`, and classify the seven bytes for each use.
- [ ] Validate whether `$24CF` receives a tile-row update, sprite/layout data, or a mixed command payload in every phase-1 caller.
- [ ] Extend the host ASM oracle test to compare all phase-1 state/timer side effects, then test the semantic scene patch separately against captured original frames.
- [ ] Do not integrate phase 1 into the visible native renderer until the payload classification and frame comparison are complete.


### Phase-2 source trace refinement — 2026-11-19

The first-pass branch outline above is now expanded from the original instructions at $CBA4-$CCDA and the shared sprite builder at $D1C7-$D23A:

- $CBA4: $0033 == 0 advances through the shared $CB9F exit; $0033 > $03AD returns; equality enters the $0020 ten-frame gate. When the gate expires, $002F and $000D are set to 3 and three records are processed with X = 2, 1, 0.
- Before equality, $004B != 0 selects sub_CC15. Otherwise held controller bits masked by con_btns_ABSS select sub_CC6B when A/B/Select/Start are held; with none held, sub_CC15 is selected. The sub_CC6B route calls sub_C3AF; if $0041 is zero after that call, control jumps back to loc_C3AF.
- sub_CC15: when $0033 == $03AD - 16 and $0020 == 0, it selects state $0045 = $0D, or $0045 = $004B + $0B when $0052 == 1. Otherwise it appends the fixed $C134 source descriptor to the legacy PPU upload buffer. If $003F & $10 is clear, sub_DFA2 derives two source parameters from $0053 and writes them into descriptor offsets $0305-$0307; if both are zero it returns without the second write.
- After sub_CC15, loc_CC7D uses $0056. If nonzero, it bypasses the $002F branch and uses Y = $BB. If zero and $002F != 0, it sets source object fields $0380=$038C=$1F, $0378=$AF, $037C=1, $0384=0, and uses Y = $BD. If $002F == 0, it calls sub_DD6F, compares the result with $03F1, and may clamp $0055 and restart $002F at 10. It then clamps X to 0..3 from $0053-1, sets $000B=2, and calls sub_CCCA.
- sub_CCCA selects $0000 from $C0DA[X], sets $0003=$21, selects OAM offset $0004 from $C0E8[X], and calls $D1C7. For the three-record loop, X = 2,1,0 yields source parameter tuples (Y=$80, tile-base=$B9, attribute=$41, OAM offset=$14), (Y=$70, tile-base=$B9, attribute=$01, OAM offset=$0C), and (Y=$78, tile-base=$B7, attribute=$01, OAM offset=$04) respectively. The values come from $C0DA/$C0DE/$C0E2/$C0E5/$C0E8; they describe source sprite-layout inputs, not a native PPU or OAM format.
- $D1C7 expands those parameters into source OAM entries. This algorithm is not suitable for the native runtime. The native implementation should eventually create equivalent semantic sprite instances using extracted graphics and native sprite/quad rendering.

What this trace does not yet prove: the user-visible identity/meaning of every sprite group, the precise $DD6F dynamic value semantics, all caller-state combinations, and the full visual result across frames. Therefore phase 2 is more precisely traced, but it is not translated, tested, integrated, or accepted. No phase-2 native code is added in this batch.

### Updated phase-2 gate

- [ ] Trace sub_DD6F and its inputs/outputs, plus the callers that set $0053, $03F1, $004B, $0052, and $003F for this transition.
- [ ] Identify the source graphics represented by tile bases $B7/$B9 and confirm sprite group positions/attributes against the original menu capture.
- [ ] Define semantic transition events/sprite instances; do not port $D1C7 as an OAM builder.
- [ ] Implement the phase-2 timing and branch decisions in native C, with host ASM-oracle differential coverage for every branch.
- [ ] Add native renderer integration and fixed-frame visual comparisons before marking phase 2 complete.


### Dynamic transition value routine — sub_DD6F

The routine called at $CCA5 is sub_DD6F ($DD6F-$DD8C), not a pure lookup:

- With X = 0, it adds ram_038C_obj[0] to the low byte ram_0380_obj[0], then propagates carry into ram_0384_obj[0]. This advances the source object's fixed-point position.
- It then subtracts ram_037C_obj[0] from the current ram_0001_t04 value and adds ram_0384_obj[0], returning the resulting byte in A. The routine uses the processor carry state across its arithmetic; a native port must reproduce the source preconditions/carry semantics or, preferably, derive the equivalent fixed-point operation from the complete caller path.
- loc_CC7D stores this return value in ram_0055, compares it with ram_03F1, and clamps/restarts ram_002F when the result crosses the configured limit.

This confirms that phase 2 includes a moving native-object parameter, not just a static sprite-table selection. The current trace still lacks the complete initialization/meaning of the object fields and their visual identity. No translation is justified until those source preconditions are established.


## Native setup-command and cursor slice — 2026-11-19

The native C menu now translates the first complete setup lists for the three front-end states:

- List 0 / title: C2BD sets source visual key $02, title animation fields, cursor sprite tile/position, and idle timer; C514 sets palette key $18, debounce and setup-complete state.
- List 1 / selector: C55E sets visual key $06; C3F3 sets visual key $03, cursor sprite and $0034 timer; C514 completes setup with palette key $18.
- List 6 / design: C55E sets visual key $06; C575 sets selected-course field 5, cursor X/tile and visual key $0F; C514 completes setup.
- The live input handlers now also update the cursor position using the source tables at $C080, $C083 and $C08B. The contiguous adjacent table bytes are preserved for the same out-of-range test values as the source reads.
- The host test compares the semantic fields and cursor sprite bytes against direct ASM calls. The menu state/timer/transition differential suite reports 76,416 matching vectors; the setup command test passes. The exact code commit cb10fee3a7397021f7bb1a5901c6db69acd7f515 has a successful host verification workflow and 3DS build.

These setup functions are still isolated: the application loop does not yet run the native menu, and there is no native scene renderer consuming the source asset keys. The next rendering work must load the existing exported EBPX/snapshot files and draw native tiles/sprites directly; the old PPU preview/compositor is not the final game renderer.


## Native editor and tape-screen scene assets — 2026-11-19

The exported snapshot set includes source command streams 05, 16 and 17, which correspond to the final visible background setup streams used by the editor (state 9), tape-save (state 12) and tape-load (state 13) paths. The exporter names these files by one-based stream slot, so the source keys $05/$10/$11 are read as snapshot files 05/16/17 respectively.

- `native_menu_assets.c` now loads snapshots 05, 16 and 17 into separate native scene buffers, in addition to title/selector/design scenes.
- The loader now imports palette presets 12 through 18 and their write masks. The editor setup's C551 branch sees state 9 ($09 >= $08), so it selects source palette key $12 (palette preset 12); other current front-end/tape scenes retain preset 18.
- `native_menu_renderer.c` draws these three additional backgrounds directly to the top framebuffer for states 9, 12 and 13. No PPU interpreter, OAM builder, CPU or ROM is used.
- `test_native_menu_assets.c` creates synthetic exported snapshots and all seven palette files, then verifies the six scene buffers and palette indexing.

**Scope limitation:** these are static command-stream snapshots, not complete original frames. The editor's C5C2 object setup, editor cursor/object interaction and guarded B-return are not translated here; save/load state handshakes and the native SD-card file backend are also not implemented. Dynamic transition sprites, missing OAM overlays, visual comparisons and physical New 3DS validation remain open. This milestone must not be described as a finished editor or functional save/load menu.

## Native front-end runtime and scene rendering — 2026-11-19

The initial title/selector/design path is no longer just an isolated state helper:

- `game.c` initializes `NativeMenuState`, ticks its timers, dispatches input, executes one source setup command per frame, and renders the native menu scene. The optional extractor is opened with X and is not initialized on normal startup.
- `native_menu_assets.c` reads only the exported `chr_pixels.ebp`, four source snapshots, and palette-18 files. It validates the EBPX header, exact file lengths, tile count, snapshot ordering and palette files. It does not open a ROM.
- `native_menu_renderer.c` merges the source setup-list snapshots into three native tilemap assets, selects palette groups from the source attribute bytes, draws decoded CHR pixels directly to the 3DS top framebuffer, and draws the menu cursor as a native sprite instance. It does not call the PPU command interpreter, PPU nametable compositor or OAM builder.
- Host test `test_native_menu_assets.c` creates synthetic exported files and verifies EBPX loading, ordered scene merge and palette loading without a ROM. The host verification workflow passes on commit `9504c4b5afd940247d1459e736c3ea235a301876`; the matching 3DS build also passes.

This is a real native renderer path for the three source setup scenes, but it is not yet visually accepted. The exported static snapshots may not include every dynamic update. Remaining work: compare captures against the original, translate dynamic transition updates, finish the editor and tape/save/load flows, and verify on physical New 3DS. Unsupported states clear the native scene instead of displaying invented artwork.
