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
- The menu prelude explicitly zeroes both scroll pairs. `ram_0047` is raised in race setup (`ofs_001_C875_02_06`) and in gameplay object logic (`$EBAE)); it is cleared on menu-return paths (`$CA33`, `$CAA7`). The raster path is therefore primarily the race/return transition path, but it must be preserved for the Stage 2 shared transitions when the flag is still set.

B4 is **traced, not implemented**. The current menu state model does not yet reproduce the two-phase `ram_0047` raster/OAM update. Do not replace it with `menu_scroll_x/y` alone.

## Ordered completion plan — work one item at a time

The boxes below are the complete known Stage 2 work queue. A box is checked only when the corresponding source audit, implementation, and regression evidence exist. "Menu state flow" excludes race simulation itself, but must correctly hand off to the race state.

### A. State graph and source trace
- [x] A1. Inventory all 14 state handlers, setup-list selector values, and all nine setup-list command sequences.
- [x] A2. Identify title, selector, design, save/load, race-entry, and shared transition paths from the state handlers.
- [x] A3. Trace every branch and return from `ofs_CB3B_00`, `ofs_CB77_01`, `ofs_CBA4_02`, and `ofs_CC6B_03`; record exact conditions and state writes (see the transition-dispatcher trace below).
- [x] A4. Classify state-$09 as the interactive course-design/editor loop, distinct from race states `$02/$05/$0A`; trace its entry, per-frame path and B-button return to state `$08`. Its setup list 5/editor implementation remains a separate C3 handoff dependency.
- [ ] A5. Trace each reachable setup command's full transitive calls and RAM/PPU/APU side effects; include `$30-$3D` timers and one-command-per-NMI sequencing.
- [ ] A6. Audit reset/cold-start, return-to-title, cancel/back, and all menu-to-race handoffs against the actual ASM call order.

### B. PPU, OAM, and visual reconstruction
- [x] B1 (selection trace) — Map the static `$01-$19`, palette `$12-$18`, dynamic `$00`, and zero-page `$1A` sources and verify NROM/PPUCTRL pattern-table selection. The renderer fidelity portion remains open under B3-B7.
- [ ] B2. Verify the native PPU command parser against every stream reachable from menu setup lists; test terminators, lengths, PPU address bounds, palette mirroring, and unsupported writes.
- [x] B3. Implemented mapper-0 vertical address mirroring plus two-page composition using PPUCTRL base nametable bits and `$2005` scroll; host regressions cover A/B selection and horizontal scroll across the page boundary. Hardware/reference comparison remains D2/D3.
- [ ] B4. Source scroll/raster path traced (`sub_D0AB`, `sub_D14E`, `sub_D1B3`); implement the two-phase `$0047` NMI update and validate its scanline/OAM timing.
- [ ] B5. Trace every menu OAM writer and all cursor/decoration sprites, sprite priority, flips, size, scanline limits, and visibility; render all supported source attributes.
- [ ] B6. Reconstruct authentic title, selector, design, save/load, and transition artwork from actual CHR + PPU writes; remove diagnostic text/placeholder rendering from the normal menu path.
- [ ] B7. Verify menu palettes and palette transitions using the actual PPU palette RAM writes; account for palette mirroring and PPUMASK grayscale/emphasis if used.

### C. Menu input and transitions
- [ ] C1. Compare `sub_C7DE` byte-for-byte with native input: repeat guard, timer, Select/Start priority, Up/Down, and exact wrap rules.
- [ ] C2. Complete title and selector navigation/confirmation and the transitions into all destination states.
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
