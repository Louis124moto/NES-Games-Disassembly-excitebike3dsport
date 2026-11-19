> **Owner-approved architecture (2026-11-19): faithful native port only.** The final game on `main` is to be reimplemented in native C/C++ for Nintendo 3DS. It must not execute a 6502 CPU, emulate NES hardware, or require/load `rom.nes`. The original ASM remains the behavioral reference; native code must reproduce its menus, physics, courses, opponents, scoring, graphics and sound. Differential comparison against the original is mandatory, but any emulator/oracle is host-test-only. The `enhanced` branch will be created only after the faithful `main` port is accepted. The earlier static-recompiler/software-NES-system experiment is not the final architecture and is being retired from the game runtime. See [the controlling native-port specification](3ds-port/TODO.md) and [native porting rules](3ds-port/PORTING_RULES.md).

# Excitebike 3DS — faithful NES port

## Mandatory development order and acceptance gates

**The goal is a complete faithful Excitebike port, not merely a graphics viewer or a loosely similar game.** The original NES disassembly and its observable runtime behavior are the source of truth. The motorcycle's behavior and physics, as well as the rest of the game, must be faithful; visual resemblance alone is insufficient.

### Required order — do not skip or overlap gates

1. **Stage 1 — extractor:** finish, test, and validate the integrated native extractor before beginning the original menu. Extract and inspect every recoverable graphics/data class supported by the verified source path. Show raw CHR tiles in a tile atlas with source-derived palette/subpalette variants, provide an actual NES-rendered view when all required PPU data is known, and export useful formats to the SD card. Include diagnostics for banks, palettes, tile indices, nametables/attributes, sprites/OAM, and PPU state/address. Do not fabricate missing data.
2. **Stage 2 — original menu:** only after Stage 1 is accepted, reconstruct the original menu using source-confirmed CHR, palette RAM writes, nametable/attribute data, sprite/OAM state, bank changes, and relevant timing. Preserve original layout, colors, transitions, and controller behavior.
3. **Stage 3 — first race:** only after Stage 2 is accepted, implement the first course in the original startup/gameplay order. Trace and reproduce track rendering/data, player and opponent state, controller input, motorcycle acceleration/braking/overheating/cooling/jumping/posture/landing/crashes, collisions, timing, scoring, and race transitions from the ASM. Do not substitute generic or plausible physics for the original.
4. **Stage 4 onward — remaining game:** proceed through the next original gameplay/menu/result flow in source order. Define the next stage from the disassembly only after the current stage passes its acceptance gate. Do not skip ahead to later courses or features to hide unfinished earlier work.

### What “100% validated” means for a stage

A stage remains **in progress** until all relevant criteria below are satisfied and evidence is recorded:
- assembly/data path and dependencies traced; confirmed facts separated from unknowns;
- implementation integrated into the actual 3DSX runtime, not just isolated test helpers;
- regression tests cover normal paths, branch paths, boundaries, and relevant hardware/state interactions;
- host tests pass and a 3DS build succeeds for the exact same commit;
- reproducible comparisons against the original NES behavior pass for every behavior within the stage's stated scope;
- runtime behavior is tested on New Nintendo 3DS hardware where hardware validation is relevant;
- output, limitations, test commands/results, commit, and CI links are recorded in the TODO.

A successful compile, unit test, screenshot, or launch by itself is **not** 100% fidelity validation. If a criterion is blocked or cannot be proven, the stage stays unaccepted; document the missing evidence and keep working on that stage. Never begin the next stage while the current one is unaccepted.

### Extractor and UI requirements

- The extractor is native code compiled into the 3DSX; no Python or PC-side preprocessor is required for normal 3DS operation.
- Run the extraction workflow at startup before the original menu; allow explicit re-extraction and SD-card export. Keep raw-data viewing and composed-screen viewing distinct.
- Use the D-pad and A/B controls; use a minimal image-centric interface with a switch between simple viewing and advanced diagnostics. The top screen displays the selected image/view; the bottom screen holds controls and status/technical details.
- Analyze all supported source data, including compressed or generated graphics where the ASM confirms how they are produced. If a format or graphics path is not understood, continue analysis and label it as unresolved instead of guessing.
- Use actual PPU RAM writes as the source of palette truth. Build the rendered screen progressively from proven source behavior; show missing/unknown parts explicitly rather than inventing them.
- Export useful image and binary representations, including PNG and source data such as CHR/palette/nametable where available. Store exports on the SD card; the 3DSX must also work without a PC tool.
- Prioritize complete, accurate extraction even when it is slow. Optimize blocking/incremental behavior only after correctness is established.
- Keep features disabled until their full required inputs and source behavior are validated.

A native Nintendo 3DS homebrew port aiming to reproduce the original NES **Excitebike** as faithfully as possible, including its gameplay rules, course data, opponents, menus, scoring, sound, and relevant timing and hardware-visible behavior.

The original NES disassembly is the behavioral reference. Its source files must remain unchanged. The port translates verified logic and data into native C for the 3DS ARM11/libctru environment; NES 6502 assembly and NES hardware registers cannot simply be linked into a native 3DS executable.

> **Current status (2026-11-19): the faithful native port is not complete.** Stage 1's extractor was owner-accepted, with verification debt. The previous menu/PPU/recompiler work is an experimental emulator-oriented implementation and is not accepted as the native game. Stage 2 must now be completed as native C/C++ menus and front-end code; Stage 3 remains blocked until then. Existing ASM traces, asset extraction, and host-side differential tests may be reused, but the generated 6502 runtime and NES hardware emulation are being removed from the final executable. No playable native race or full-game fidelity is claimed.

> **Runtime cutover (2026-11-19):** the live 3DS loop no longer starts the NES 6502/system/APU runtime, and those runtime objects are excluded from the 3DS link. The old emulator/recompiler sources remain in the repository for offline reference/test cleanup; they are not the game. At this intermediate commit the app is an asset inspector, not a playable native Excitebike port. Native menus and gameplay have not yet been implemented. See the runtime cutover checklist in `3ds-port/TODO.md`.

## Project goals

- Reproduce the original game's observable behavior in native game logic, rendering, and audio; NES hardware emulation is not part of the final runtime.
- Preserve the original disassembly and assets in the existing source tree; implement the 3DS port separately.
- Translate by routine or block when equivalence to the original instruction sequence can be demonstrated.
- Analyze the original ASM and model only the resulting game behavior in native subsystems; do not reproduce NES hardware registers as runtime devices.
- Finish the original game's core features before optional enhancements or performance polish.
- Keep any 3DS-specific enhancements optional and disabled by default unless they are proven not to alter the faithful baseline.
- Target both the original Nintendo 3DS and New Nintendo 3DS.
- Distribute a homebrew `.3dsx` build; this project does not currently target a CIA release.

## Progress dashboard

| Measure | Current evidence-based status |
|---|---|
| Original ASM inventory | 177 labels beginning with `sub_` are listed in `3ds-port/ASM_FUNCTION_INVENTORY.md`; this includes aliases/helpers and is not a count of standalone routines. |
| Stage 1 — extractor | Accepted by owner decision on 2026-11-17; source/hardware verification debt remains in `3ds-port/TODO.md`. |
| Stage 2 — native menus/front-end | NOT YET IMPLEMENTED as the required native architecture. Existing menu/PPU/recompiler work is historical reference material only. Stage 3 blocked. |
| Stage 3 — first race | BLOCKED until Stage 2 is fully accepted. Existing state-2 trace/helpers are parked; no playable race or player physics implementation is claimed. |
| Latest code batch | Historical emulator/recompiler experiment only; it is not the final runtime architecture. Native C/C++ menu and gameplay reimplementation is the active direction. |
| Latest host test / 3DS build | Historical emulator-oriented runtime validation only; it does not validate the newly approved native-port architecture. A fresh native-runtime baseline is required. |
| Full-game fidelity | Not achieved; no end-to-end race comparison is documented. |

A function definition, passing unit test, successful build, gameplay integration, and fidelity verification are separate states. Never infer one from another.

A function definition, matching name/address, passing unit test, successful build, gameplay integration, and fidelity verification are separate states. Never infer one from another.

## Current implementation status — 2026-11-19

### Extractor and original menu

- Stage 1 was accepted by owner decision on 2026-11-17. Its remaining source-audit, edge-case, export, and reproducibility items remain follow-up debt; this is not a claim that every extractor criterion has been independently verified.
- Stage 2 remains reopened at the owner's request. The runtime now connects the source PRG menu handlers, editor/save-load paths, full timing-only NMI prefix, both D14E scroll phases, split-aware PPU rendering and pixel-derived sprite-0 hit timing. Original-NES frame-by-frame comparison, remaining editor object interactions, pre-render/odd-frame calibration, sound and physical New 3DS verification remain incomplete. See `3ds-port/STAGE2_TRACE.md`; Stage 3 remains blocked.
- Commit `2bc8d15` added a centered 4:3 viewport and full OAM/scroll redraw tracking. Commit `8707ef5` added explicit PPU render-failure diagnostics. The viewport/diagnostic changes still need a fresh hardware retest and a title-menu capture comparison.
- Do not replace missing source-confirmed menu art with invented rectangles, labels, or placeholder graphics.

### Stage 3 — first course

The current source trace is `3ds-port/STAGE3_TRACE.md`. It records the state-2 entry, six setup commands, and the original race-frame call order. The setup module currently contains isolated translations for C551, C869, the post-D13F portion of C820, C5A3, C566, and the head of C875. The newly added `nesRaceSetupECE4` translates the `$ECE4` dynamic PPU command-buffer builder for the three records in `tbl_E6A1`.

Important limits:

- `sub_ECE4` is only the buffer builder. The jump to `loc_C289` and execution by `sub_D3C1` are not yet represented by this helper.
- `sub_D13F` calls the builder three times in source order (X = 0, 4, 8); the complete PPU side effects still need a native model.
- State-2 list index 2 is not yet wired into the active setup sequencer.
- Player-slot identity, player input/update path, track decoding, opponents, race timing, scoring, collision, crash/recovery, and authentic physics remain unverified.
- Placeholder modules for physics/collision/track/audio remain excluded from the runtime build. No generic physics may be presented as the NES game's behavior.

### Tracking files

- `3ds-port/TODO.md` is the controlling roadmap and acceptance record.
- `3ds-port/STAGE3_TRACE.md` contains the active first-race source trace.
- A file named `progress.md` is not present in this repository; do not treat an old summary as a current progress record.

## Repository layout

- `Excitebike/` — original NES disassembly, source assets, and assembly scripts. Preserve these sources unchanged.
- `_scripts/` — shared build helpers.
- `3ds-port/source/` — native C application and translated NES routines.
- `3ds-port/include/` — native port interfaces and routine declarations.
- `3ds-port/tests/` — host-side routine regression tests.
- `3ds-port/TODO.md` — phase roadmap, task gates, batch log, and current priorities.
- `3ds-port/ASM_FUNCTION_INVENTORY.md` — per-label inventory and translation status.
- `3ds-port/REFERENCE_MAPPING.md` — source-to-native notes, RAM mapping, trace evidence, and unresolved interpretations.
- `.github/workflows/` — automated test and Nintendo 3DS build workflows.

## Build the original NES disassembly

Run from the repository root.

**Linux / macOS**
```sh
sh Excitebike/assemble.sh
```

**Windows**
```bat
Excitebike\assemble.bat
```

These commands assemble the original NES project; they do not build the 3DS homebrew.

## Build the Nintendo 3DS homebrew

Install devkitPro with devkitARM and libctru using the official [devkitPro getting-started guide](https://devkitpro.org/wiki/Getting_Started). Run from the repository root:

```sh
make -C 3ds-port
```

A successful build should produce:

- `3ds-port/build/excitebike3ds.3dsx`
- `3ds-port/build/excitebike3ds.elf`

The intended distributable is the `.3dsx` homebrew. Build success alone does not prove the app is playable or faithful.

## Architecture and current limitations

The native project has modules/interfaces for game state, input, rendering, bike physics, collision, track, audio, and menus. The application scaffold has previously been reported to launch on real 3DS hardware, but its display and module structure are not a faithful game implementation.

Placeholder physics must not be presented as original game behavior. In particular, `sub_E733/sub_E73B` is an object-position and track-event path, not the player's acceleration model. The player update path and player-slot identity must be established from the disassembly before implementing player physics.

## Fidelity requirements

Translations must preserve observable behavior, including where relevant:

- 8-bit wraparound, carry/borrow propagation, and 16-bit address behavior;
- status flags and their effects on later branches;
- registers, stack behavior, RAM writes, scratch variables, and aliasing;
- entry points, call order, loop bounds, lookup-table access, and branch conditions;
- timing and effects associated with PPU/APU, controller input, vblank, and other NES-specific hardware;
- edge cases, initialization, reset paths, and interactions with the surrounding game state.

For a block-level translation, document the exact ASM address range and the boundaries/dependencies not yet translated. Hardware interactions should be modeled through explicit interfaces or observable outputs rather than silently omitted.

## Verification policy

Use these states consistently:

1. **Analyzed** — the relevant assembly and dependencies have been inspected; confirmed facts and hypotheses are distinguished.
2. **Translated** — native C implementation exists and is declared where appropriate.
3. **Tested** — relevant host regression tests have actually executed and passed.
4. **Build-verified** — the same code revision builds for the 3DS and produces the expected artifacts.
5. **Integrated** — the implementation is connected through a verified call/update path.
6. **Fidelity-verified** — tests or reproducible scenarios compare the relevant observable behavior against the original NES reference.

Only claim the states supported by evidence. Host tests do not replace hardware/build validation, and isolated routine tests do not establish complete-game fidelity. Record CI run links and code revisions in the task tracker when available.

## Stage 1 implementation status — extractor (owner-accepted by decision 2026-11-17; verification debt remains)

The first native extraction/export pass is integrated into the 3DSX runtime. It is a partial Stage 1 implementation, not a completed or validated extractor.

### Current runtime behavior

- Put either a legally obtained Excitebike `rom.nes` iNES/NES 2.0 ROM or the project’s raw `CHR_ROM.chr` dump beside the launched `.3dsx`. The iNES file takes precedence if both are present. The decoder can inspect other CHR-ROM files, but the embedded PPU command lists and palette presets are Excitebike-specific.
- On startup, the app checks the ROM fingerprint and cache, then extracts or reloads CHR-ROM tile pixels.
- Exports are written beside the app in `extracted/`: raw CHR, backwards-compatible raw decoded pixels, `chr_pixels.ebp` (self-describing EBPX v1 tile pack), grayscale diagnostic atlas, seven source palette snapshots/masks/streams, 18 PPU command streams, 18 nametable/palette snapshots with write masks, 18 color `ppu_preview_XX.png` images, and `manifest.txt`.
- The internal decoded cache is `extracted/chr_cache.bin`; it is distinct from the exported pixel data.
- Controls: D-pad left/right changes atlas pages, palette presets, or PPU streams; up/down cycles atlas, palette, and PPU snapshot views; A opens the viewer or exports while open; B closes the viewer or retries extraction while closed.
- START is deliberately locked out of the menu until Stage 1 is accepted.
- The PNG uses grayscale to expose 2bpp pixel indices. It does **not** claim to show Excitebike's actual palette or a complete original screen.

### Current hard limits and remaining work

- The runtime accepts ROM files up to 8 MiB. The 960-tile export cap has been removed (2026-10-30): the complete raw CHR is retained on the heap, `chr_pixels.bin` and `chr_pixels.ebp` are streamed in 64-tile chunks and contain ALL source tiles, and the on-device atlas decodes 375-tile pages on demand. `chr_atlas.png` and `ppu_preview_*.png` still compose only the first 960 tiles; this limit is documented in the export manifest.
- The loaded ROM is validated at the header level against canonical Excitebike (mapper 0, 16 KiB PRG-ROM, 8 KiB CHR-ROM) and the identity summary is shown on the bottom screen and in the manifest. This is a header signature check only; PRG content identity is not claimed. Hardware validation remains pending.
- CHR-RAM is reported as requiring runtime PPU capture; it is not falsely treated as CHR-ROM. The repository’s standalone `Excitebike/CHR_ROM.chr` can be copied beside the `.3dsx` under the exact name `CHR_ROM.chr` to inspect the verified raw 8 KiB CHR dump without a full ROM file.
- Seven source-defined palette command streams from `bank_FF.asm` addresses D3D6–D450 are replayed through a native interpreter matching `sub_D3C1` write-list semantics. Their 32-byte snapshots, write masks, and raw command streams are exported. Each of the 18 source PPU command snapshots now also exports a 256x240 RGB PNG by composing decoded CHR tiles with its source-derived nametable, attributes and palette values; missing/unverified tiles are hatched. The versioned `chr_pixels.ebp` uses a 16-byte little-endian EBPX v1 header followed by 64 one-byte pixel indices per tile, while `chr_pixels.bin` remains unchanged for compatibility. These are diagnostic/static command-list replays, not full live-game screens. The active palette choice for each live game state, selecting the correct PPU stream for each game state, runtime identity validation for arbitrary ROMs, compressed/generated PRG graphics analysis, sprites/OAM, scrolling, and timing remain unintegrated. The canonical mapper-0 Excitebike source has no CHR banking. Static nametable/attribute snapshots from 18 source command streams are exported with write masks.
- The owner accepted Stage 1 by decision on 2026-11-17. This is not a requirement-by-requirement hardware fidelity report; the unchecked items above remain follow-up debt.

`stb_image_write.h` is vendored from the public-domain stb project (Sean Barrett; v1.16) and is compiled into the native homebrew. No Python program is needed at runtime.

## Current next priorities

Stage 2 is the active and only implementation priority. Follow `3ds-port/TODO.md` and `3ds-port/STAGE2_TRACE.md`: finish reset/cancel and save/load edge cases, integrate the remaining NMI/OAM work, reconstruct authentic menu artwork, and validate every route against the original NES and physical New 3DS. Stage 3 work in `3ds-port/STAGE3_TRACE.md` is parked until Stage 2 is accepted.

Stage 2's authentic menu artwork and hardware/reference comparison remain explicit follow-up debt; no complete menu fidelity claim is made.
 
## Upstream source and credits

The original disassembly originates from [NES Games Disassembly](https://github.com/cyneprepou4uk/NES-Games-Disassembly). Preserve relevant upstream credits and follow the original project's applicable terms when reusing or redistributing its work.

## Use of AI assistance

AI tools have been used as development assistants for repository analysis, documentation, assembly-to-C translation work, and test planning. AI-generated suggestions are not treated as authoritative: translated behavior must be checked against the original NES disassembly, and test/build/integration/fidelity status must only be claimed when supported by reproducible evidence. The use of AI does not imply that the port is complete, correct, or verified. Human review remains necessary for changes and project decisions.

## Contribution rules

- Keep the original NES source unchanged.
- Use consistent technical English for C comments, identifiers, and documentation.
- Record the source label, CPU address/ROM offset where known, dependencies, register/flag inputs and outputs, memory effects, and unresolved assumptions.
- Add tests for branch paths and boundary cases; use independent instruction-level references where practical.
- Do not mark a task complete until its required tests, build, integration, and fidelity evidence have been recorded.
- Keep optional 3DS enhancements separate from the faithful baseline.


## Native PPU background renderer

The background compositor is implemented in C in `3ds-port/source/nes_ppu_render.c`
and linked into the native `.3dsx`. It combines decoded 2bpp CHR pixels with a
960-byte nametable, 64 attribute bytes, and 16 source-derived background palette
values. The renderer is connected to the top-screen path and centered at native
NES resolution when `GameState.menu_ppu_background_ready` is set by a future
verified PPU setup path.

That readiness flag intentionally remains off until the ASM-derived menu code
has populated real nametable, attribute, and palette state. Until then the 3DS
continues to show the state trace instead of displaying fabricated graphics.
Sprite/OAM composition, scroll and mapper bank changes are not implemented by
this background-only renderer. The C regression test is
`3ds-port/tests/test_nes_ppu_render.c`; CI compiles and runs it.



## Stage 1 source-audit update — 2026-11-16

Two source-led changes were integrated on `main`: the top screen no longer receives `consoleInit(GFX_TOP)` (which selects RGB565 while the renderer writes BGR8), and the live `ram_0045` reader is traced through `vec_C23B_NMI → sub_C27F → sub_D3C1`. The earlier `$D55A/$CC2D` consumer attribution was corrected. The 27-entry table now distinguishes static ROM lists, the dynamic buffer at `$0301`, and the zero-page list at `$00A1`.

The 2026-11-16 host regression workflow passed on the trace commit; the matching 3DS build is tracked in [CI run 43955143207](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/43955143207). These source and CI milestones do **not** accept Stage 1: physical New Nintendo 3DS confirmation, dynamic-buffer replay, complete active-state selection, and the remaining extractor requirements are still open. See `3ds-port/TODO.md`.
