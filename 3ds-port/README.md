> **Current native-port status — 2026-11-19:** The owner accepted the title/selector/design menu baseline as sufficient to proceed with gameplay and explicitly deferred the optional editor and tape save/load submenus. The native race engine is now in progress: the active-bike speed dispatcher (`sub_CD1F`) and the speed/terrain prefix of `sub_CD59` are translated for all four bike slots and covered by 132,000 ASM-oracle vectors. This is not yet a playable race: the rest of the per-frame object update, full physics, opponents, course data/rendering, collisions, race flow, audio and results remain incomplete. Normal game startup does not load or execute `rom.nes`; the 6502 oracle is host-test-only.

# Excitebike 3DS — native homebrew port

This directory contains the native Nintendo 3DS application. The original NES disassembly remains in `../Excitebike/` and is preserved separately.

## Requirements and build

Install devkitPro with devkitARM and libctru using the official instructions: https://devkitpro.org/wiki/Getting_Started

From this directory:

```sh
make
make clean
```

The CI build should produce `build/excitebike3ds.3dsx` and `build/excitebike3ds.elf`. Generated objects, dependency files, and final binaries belong under `build/`. The `.3dsx` can be copied to an SD card for a homebrew launcher, once the build and launch have been verified.

## Runtime target — faithful native port

The final application is a **native Excitebike implementation in C/C++ for Nintendo 3DS**. It must not load or execute `rom.nes`, run generated 6502 code, or use a software NES CPU/PPU/APU. The original ASM is studied and translated into native game logic; verified original graphics, course data and sound assets may be imported into native formats.

The previous SELECT-to-run-ROM / software-NES-system path is retired as the product direction. The old generated 6502 recompiler, NES CPU/system/APU game runtime, PRG menu bridge, and emulator menu have been deleted from the active source tree. Only a host-side 6502 reference CPU remains for differential tests. See the controlling specification in [TODO.md](TODO.md) and the rules in [PORTING_RULES.md](PORTING_RULES.md).

The released game must launch and play without `rom.nes` or `dspfirm.cdc`. Native menus, gameplay, rendering, audio, input and save/load are implemented as independent modules and validated against reproducible original-NES reference scenarios.


## Architecture status

The current executable has a **native front-end prototype**. Normal game startup reads only the exported EBPX CHR pack, source PPU snapshots and palette presets; it does not open or execute `rom.nes`. The optional asset inspector can read a ROM/CHR file as source material when explicitly opened with X. The old 6502/system/APU gameplay path and PRG-backed menu recompiler have been deleted from the active source tree. The host-only 6502 reference CPU is under tests/oracle/ and is not part of the 3DS build. The inspector's static PPU preview helpers remain utility-only.

The native title/selector/design menu state, initial setup lists, cursor behavior and direct-framebuffer scene renderer are integrated in the app loop. Static native scene backgrounds are now available for the editor and tape save/load states using exported source snapshots; they do not yet implement editor controls or the SD-card save/load transfer. Remaining transition branches, race loop, opponents, course rendering, scoring and audio are incomplete. Native bike code now includes source-derived acceleration/deceleration, object-index-aware terrain gating, four-bike speed-control dispatch, temperature/overheat and movement helpers. The speed-control pass is covered by 120,000 `sub_CD59` vectors and 12,000 `sub_CD1F` frame vectors. It is not yet integrated into the complete playable race loop. Do not treat the old PPU/menu prototype as the new implementation. The first playable native slice will be accepted only after its deterministic host tests match the original ASM reference and the same commit builds for 3DS. See [TODO.md](TODO.md) for the exact gates.

## Fidelity approach

The NES source is 6502 assembly and relies on NES-specific hardware. It cannot be linked directly into a native ARM11/libctru executable. The port will translate verified gameplay logic and data into native modules, then compare behaviour against the original. Any subsystem remains marked incomplete until validated.

## Menu port status (updated 2026-10-21)

**The menu is not finished yet.** The active runtime remains diagnostic-only: it does not pretend that partial ASM translations are a working menu. Current source-derived work includes the `sub_C2A9` dispatcher trace, the state-2 command-list trace, isolated state-selection helpers (`C455`, `C55E`, `C56A`), table lookup helpers (`C522`, `C53A`, `C5A7`), and a new native translation of `ofs_001_C485_03_05` with branch/arithmetic regression vectors. The C485 vectors are included in the passing host suite on revision `601e7e3` ([CI run 39577668191](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39577668191)); the helper remains isolated and is not integrated into the active runtime.

The owner has authorized race-engine work before optional editor/save-load completion. Remaining menu transition work required to enter and leave a race stays in scope. The next race-engine gates are to complete the source-defined per-frame object update order, player and opponent physics, course data/rendering, collision, and finish/results flow; then verify the full flow against the original NES version. State 3 and several other transition states remain incomplete. `sub_C621` now has an isolated source-faithful 256-byte copy translation and regression vector, but its callers are not integrated; the state-2 race-start path still has unported data-buffer and track-preparation dependencies. The four-entry `sub_CB2C` indirect handler lookup is now translated and regression-tested, but is not yet wired into a running native menu state machine. Isolated helpers do not count as a complete menu.

**After the menu passes its end-to-end gate, work on Course 1 only.** Finish the first course's track data, player controls/physics, opponents, collisions, overheating, race start/finish and scoring as one verifiable slice. Do not start courses 2+ until Course 1 is validated against the original. See [TODO.md](TODO.md) for the detailed acceptance gates and decisions still needed.

### Extract graphics and prepare the native diagnostic atlas

Run from the repository root with a legally obtained ROM or raw CHR dump:

```sh
python3 3ds-port/tools/extract_nes_assets.py game.nes
```

Outputs go to `3ds-port/build/extracted-assets/`: `chr-atlas.png`, `chr-tiles.json`, and `nes_chr_assets.h`. Add `--tiles` for individual PNGs. For a verified raw 2bpp range inside a CHR-RAM image, add both `--chr-offset 0x... --chr-size 0x...`; do not guess the range. Add `--install-header` to copy the generated header into `3ds-port/include/nes_chr_assets.h` so the native renderer can display the diagnostic atlas on the next build. The atlas is not the original title-screen layout.

### Test and build

```sh
python3 -m unittest discover -s 3ds-port/tools -p 'test_*.py'
make -C 3ds-port
```

Historical verification on `2fb6e14470a604c32785e7e4f6c3bc68392ddfbd` (before disabling the placeholder runtime): Python extractor tests, host C routine regression tests, runtime iNES decoder tests, and the state-13 entry test passed ([CI](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39525039178)). The matching 3DS build and artifact checks also passed ([CI](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39525039042)). These checks do not establish that the menu is visually or behaviorally complete.

### Built-in runtime CHR extractor (on the 3DS)

The homebrew now includes a runtime inspector; the Python scripts are optional PC-side tools, not a runtime dependency. Copy a legally obtained iNES or supported NES 2.0 ROM with CHR-ROM named `rom.nes` into the **same SD-card folder as `excitebike3ds.3dsx`**. Launch the homebrew and press **Y**. The extractor derives this path from the launched `.3dsx` path, so it does not require a fixed `sdmc:/3ds/excitebike3ds/` folder. The decoded cache is saved beside it at `extracted/chr_pixels.bin`.

- **Y**: open/close the extractor
- **X**: reload the ROM from the SD card
- **B**: close the extractor and return to the menu

The inspector validates the iNES magic and header lengths, accounts for the optional 512-byte trainer, checks the PRG/CHR ranges against the file size, and decodes up to 960 raw 2bpp CHR tiles for a grayscale atlas. It reports errors instead of reading beyond the ROM. The runtime decoder now supports standard NES 2.0 ROM-size fields and exponent/multiplier size fields within the 8 MiB runtime input limit. CHR-RAM snapshots, compressed graphics in PRG, PPU palette/nametable mapping, and original menu reconstruction are still unsupported. Do not rename a random file to `rom.nes` and assume it is compatible.

### Latest menu routine batch

The setup prelude now models the side effects visible in the source: all 256 OAM bytes become `$F8`, both `$2000` and `$2400` nametable blocks are initialized to `$FC` for 960 bytes and zero for the 64-byte attribute region, scroll resets to zero, PPU mask is disabled, and PPU control becomes `$10`. The host regression suite has a test for these exact memory effects. State 2's six-command list is still incomplete; commands C820 and C875 have deep data-buffer/race-start dependencies and must not be treated as ordinary menu rendering. States 3, 5, 7 (blocked at sub_C621), 9, 10, 11, and 12 remain incomplete. State 13's setup, entry wait, and `$FF` completion branch (`sub_EC3B`/`loc_CC6B`) are translated and host-tested. The main menu is not yet fidelity-complete.

## Runtime extraction log on the bottom screen

At startup, the homebrew now checks `extracted/chr_pixels.bin` first and falls back to extracting CHR-ROM from `rom.nes` when the cache is missing or invalid. The bottom-screen console remains visible even when the atlas viewer is closed and reports the status, decoded tile count, expected file locations, and viewer state.

- **Y** — open/close the top-screen grayscale CHR atlas.
- **X** — retry ROM extraction and refresh the cache while the atlas is open.
- **B** — close the atlas.
- **ROM location** — `rom.nes` in the same directory as the launched `.3dsx`.
- **Cache location** — `extracted/chr_pixels.bin` beside the `.3dsx`.

A message saying “Loaded … CHR tiles from extracted cache” means the cache passed its format and pixel-index checks. “Extracted … CHR tiles; cache saved” means the current ROM was decoded and the cache write succeeded. Any file/decoder error is shown on the bottom screen. The log is designed to make failures visible, but the `.3dsx` argument path and SD-card I/O still require verification on the target 3DS/homebrew launcher.

This remains a CHR-pattern diagnostic tool, not a reconstructed title screen: authentic menu layout, palettes, nametable/attribute data, sprite ordering, and PPU timing remain menu-completion tasks. See [TODO.md](TODO.md) for the acceptance checklist.


### Fixes for the 3DS hardware report (2026-10-21)

The bottom-screen log is now intentionally short, with one action per line, to avoid wrapping text beyond the visible console area. The top-screen atlas renderer now uses the 3DS 8×8 tiled framebuffer layout rather than treating the framebuffer as linear memory; it also clears the screen when the viewer is closed. NES 2.0 size decoding and a regression case have been added. The latest CI results must be checked before these fixes are considered validated, and a real-console run is still required.


### Authentic NES palette preview (2026-10-21)

The CHR atlas renderer now maps decoded 2bpp pixels through a 64-colour NES master-palette RGB table. It also includes palette bytes read directly from the original `Excitebike/bank_FF.asm` streams `_off_000_D3D6_12`, `_off_000_D3E9_13`, `_off_000_D3FC_14`, `_off_000_D40F_15` and `_off_000_D422_16`. **Atlas palette (updated 2026-11-17):** the atlas opens in grayscale. X/Y cycle grayscale, background sub-palettes 0-3 and sprite sub-palettes 0-3 (the same choices as the CHR viewer of Mesen); L/R cycle the seven source palette presets (D3D6-D450); pixel 0 always uses the universal background entry, as the PPU does. A pixel whose palette entry the preset never writes stays gray (no more magenta hatching). The previous behaviour, one fixed background sub-palette for every tile, produced red/green/light-blue tiles on hardware because most tiles are never drawn with that palette in the game. Because the viewer shows raw CHR tiles without the original nametable/attribute map, a selected palette is a diagnostic preview and does not claim each tile's true in-game palette assignment. Menu rendering still requires source-faithful PPU upload, nametable, attribute and OAM integration.


### Persistent build outputs on `main`

On successful pushes to `main`, GitHub Actions compiles the project and commits `3ds-port/build/excitebike3ds.3dsx` and `3ds-port/build/excitebike3ds.elf` back to `main`. The workflow still uploads downloadable artifacts for each run. The committed binaries are only refreshed after a successful build; pull requests and non-main refs do not publish binaries to `main`.



### ROM identity, uncapped exports, and active-stream/OAM view — 2026-10-30

- `nes_rom_identify` validates the loaded ROM header against canonical Excitebike (mapper 0, 16 KiB PRG-ROM, 8 KiB CHR-ROM, vertical mirroring) and reports an honest summary on the bottom screen and in the export manifest. Header-level check only; PRG content identity is not claimed. Host test: `3ds-port/tests/test_nes_rom_identify.c`.
- The 960-tile export cap is removed. The complete raw CHR is retained on the heap (bounded by the 8 MiB ROM limit); `chr_pixels.bin` and `chr_pixels.ebp` are streamed in 64-tile chunks and contain ALL source tiles; the atlas view decodes 375-tile pages on demand so every tile is reachable. `chr_atlas.png` and the PPU preview PNGs still compose only the first 960 tiles, and the manifest says so.
- `nes_chr_cache` extracts the versioned `EBCHR03` decoded-pixel cache (magic, u16 tile count, u32 FNV-1a ROM fingerprint, pixel payload) into a host-testable module. `test_nes_chr_cache.c` covers round-trip, stale fingerprint, bad magic, truncated payload, trailing data, invalid pixel indices, and the 960-tile cache cap.
- `nes_active_stream` maps ram_0045 (CPU `$0045`, table `$C0F6` at ROM offset 0x0106): indices `$01`-`$11` and `$19` resolve to the 18 static PPU update lists, `$12`-`$18` to the seven palette command lists (D3D6-D450), including the source-named values "blue background" (`$02`) and "title screen" (`$18`) from `bank_val.inc`. It also tabulates the 18 setup-command `$0045` writes per menu state. `test_nes_active_stream.c` verifies the mapping and table on the host.
- View 5 (D-pad up/down) reports the ROM identity, whether the header matches Excitebike, the live `$0041`/`$0045` values from the menu GameState, the setup-command `$0045` writes for the current state, and OAM hidden/visible counts (Y=`$F8` hides sprites via sub_D255). The live consumer of `$0045` is the NMI (`vec_C23B_NMI` at `$C23B` calling `sub_C27F`, traced from the ASM on 2026-11-16; the earlier `$D55A`/`$CC2D` attribution was wrong), and the view shows indices `$00` (dynamic PPU buffer at `$0301`) and `$1A` (zero-page list at `$00A1`) instead of "uncaptured". What the dynamic buffer holds at a given frame is not replayed yet.
- CI compiles and runs the three new host tests plus the extended export test (which verifies 1000-tile exports beyond the old cap). CI status was pending at push time. Hardware verification on New 3DS — atlas paging beyond tile 960, cache behavior, and the active-stream view — remains required, and Stage 1 stays NOT ACCEPTED.


### Source-traced NMI consumer and dynamic-buffer audit — 2026-11-16

The live `ram_0045` reader is the NMI path `vec_C23B_NMI → sub_C27F → sub_D3C1`. The table has 27 entries (`$00-$1A`): `$01-$19` are static lists, `$00` points at the dynamic buffer beginning at `$0301` with cursor `$0300`, and `$1A` points at the zero-page list at `$00A1`. The previous `$D55A/$CC2D` reader attribution was incorrect.

A first-pass source census now records the buffer initializers and append paths around `C5B5`, `C8ED`, `C935-C978`, `CB77`, `CC32-CC67`, `D11F-D2F7`, and the per-object command setup at `F5F8/F5FD/F602` and `F6E0/F6E5/F6EA`. This is not yet a frame-accurate replay: mapping each path to its caller/game state and proving which structures reach the `$00` table entry remain open. The source disassembly is unchanged.

## Historical implementation note — emulator-oriented menu prototype

The following previous implementation was built around a static 6502 recompiler, persistent NES CPU RAM, a software PPU timing model, and PPU/OAM composition. It is **not the accepted native menu implementation** and must not be used as the final game's runtime architecture. Its source traces, menu branch vectors, asset mappings, and screenshots may be mined as offline references while rewriting the menu in native C/C++.

The native Stage 2 menu must draw through the 3DS renderer, consume verified extracted assets, implement the original menu state machine directly, and compare its visible behavior and input transitions against the original NES. It must not require `rom.nes`, a 6502 runtime, or PPU register emulation.


### Historical PPU-renderer prototype notes (not the native-port target)

The retired PPU-renderer prototype kept the NES picture in a 4:3 viewport on the New 3DS top screen (400x240): the 256x240 PPU image is scaled horizontally to 320x240 with nearest-neighbour sampling and centered with 40-pixel black side bars. It is intentionally not stretched to the full 5:3 LCD. That prototype also watched the complete 256-byte OAM page and scroll registers when deciding whether to repaint. If the selected background pattern table is unavailable in the loaded CHR data, or the PPU nametable renderer rejects the source data, the bottom-screen menu diagnostics report the reason instead of drawing guessed menu art. These details are historical reference only; the final native renderer must not consume emulated PPU state. See the controlling native-port plan in `TODO.md`.
