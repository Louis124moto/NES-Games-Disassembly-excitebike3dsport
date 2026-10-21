# Excitebike 3DS — faithful NES port

## Mandatory development order and acceptance gates

**The goal is a complete faithful Excitebike port, not merely a graphics viewer or a loosely similar game.** The original NES disassembly and its observable runtime behavior are the source of truth. The motorcycle's behavior and physics, as well as the rest of the game, must be faithful; visual resemblance alone is insufficient.

### Required order — do not skip or overlap gates

1. **Stage 1 — extractor:** finish, test, and validate the integrated native extractor before beginning the original menu. Extract and inspect every recoverable graphics/data class supported by the verified source path. Show raw CHR tiles in a tile atlas, provide an actual NES-rendered view when all required PPU data is known, and export useful formats to the SD card. Include diagnostics for banks, palettes, tile indices, nametables/attributes, sprites/OAM, and PPU state/address. Do not fabricate missing data.
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

> **Current status: early porting / source-translation stage. This is not yet a playable faithful port.** The native application scaffold and build infrastructure exist, and multiple routines have C translations and regression coverage. A newly added batch of 20 ASM entry translations is present in source, headers, and the host-test runner, but execution of that batch's tests and a corresponding 3DS build are still pending. No full-game equivalence result is claimed.

## Project goals

- Reproduce the original NES game, including observable 6502 behavior and hardware-dependent effects where relevant.
- Preserve the original disassembly and assets in the existing source tree; implement the 3DS port separately.
- Translate by routine or block when equivalence to the original instruction sequence can be demonstrated.
- Analyze each NES-specific hardware interaction individually and model its observable effects explicitly.
- Finish the original game's core features before optional enhancements or performance polish.
- Keep any 3DS-specific enhancements optional and disabled by default unless they are proven not to alter the faithful baseline.
- Target both the original Nintendo 3DS and New Nintendo 3DS.
- Distribute a homebrew `.3dsx` build; this project does not currently target a CIA release.

## Progress dashboard

Counts below deliberately distinguish inventory, implementation, test execution, gameplay integration, and fidelity validation. They are not interchangeable.

| Measure | Current evidence-based status |
|---|---|
| Original ASM inventory | 177 labels beginning with `sub_` are listed in `3ds-port/ASM_FUNCTION_INVENTORY.md`. This includes helpers, aliases, and labels that may not be standalone routines. |
| Newly documented translations | 20 ASM entries in the batch dated 2026-10-21 have C implementations and header declarations. This is a batch count, **not** a reconciled project-wide total. |
| Latest batch in test runner | A dedicated regression group is wired into `3ds-port/tests/test_nes_routines.c`. |
| Latest batch tests executed | **Not yet confirmed.** Do not count the batch as test-passing until the test executable actually runs successfully. |
| Latest batch 3DS build | **Pending confirmation** for the current code revision. |
| Latest batch integrated into gameplay | No integration has been demonstrated for this batch. |
| Full-game fidelity | Not achieved. There is no documented end-to-end scenario comparison establishing a complete faithful game. |

A function definition, matching name/address, passing unit test, successful build, gameplay integration, and fidelity verification are separate states. Never infer one from another.

## Runtime integration update — 2026-10-21

The diagnostic screen remains the cold-start view. Pressing START now resets the native menu model and routes subsequent frames through `menusUpdate()`; the CHR atlas is suppressed in menu mode. The menu-related C modules are included in the 3DSX target. This is an integration milestone only: the current `menusRender()` still emits a textual ASM-state trace and does **not** draw the original NES title screen. Menu state transitions, setup command lists, PPU data reconstruction, and fidelity validation remain incomplete; Menu input is mapped to the NES controller bit layout; no extra B-to-diagnostic or X-help behavior is added in menu mode.

## Current menu/graphics work

The native menu currently translates input/state behavior for ASM states 0, 1, and 8, including `sub_C7DE`, `sub_C3AF/sub_C3B1`, the state-8 choice branches, choice-5 RAM clearing, cursor-table coordinates, and timer projection. The setup sequencer is still only partially ported: states 0/1/8 have source-derived command lists, but state 2 and destination states 9/10/12 do not have complete setup lists. State 13 shares the verified state-8 setup and its entry wait is translated; the later `$FF` branch still depends on unported `sub_EC3B`/`loc_CC6B`. Therefore the title-to-selector input trace is **not a finished menu port**, and state 2 deliberately does not launch placeholder gameplay. Option names and authentic title-screen layout are not inferred from numeric selection indices.

Graphics extraction has a one-command Python pipeline (`3ds-port/tools/extract_nes_assets.py`) and a runtime CHR inspector built into the homebrew. On the 3DS, press Y to open the inspector; it first loads decoded tile pixels from `sdmc:/3ds/excitebike3ds/extracted/chr_pixels.bin`, or extracts them from a classic iNES ROM at `sdmc:/3ds/excitebike3ds/rom.nes` if no valid cache exists. X re-extracts the ROM and refreshes the cache; B/Y returns. The cache format is an 8-byte `EBCHR01` signature, a little-endian 16-bit tile count, and 64 decoded 2bpp pixel-index bytes per tile. It decodes up to 960 CHR-ROM tiles and displays a grayscale atlas without requiring a rebuild. The Python pipeline remains available for PNG/JSON/header outputs on a PC. It accepts iNES/NES 2.0 CHR-ROM or verified raw CHR ranges. This is not a menu-art reconstruction: the exact source graphics, palette/attribute data, nametable tile placement, transparency, scrolling and PPU timing still need tracing. No ROM dump is checked into the repository. On revision `2fb6e14470a604c32785e7e4f6c3bc68392ddfbd`, the Python extraction tests, host C routine suite, runtime iNES decoder tests, and state-13 entry regression test passed in [CI run 39525039178](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39525039178). The matching 3DS build also passed in [CI run 39525039042](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39525039042). These checks validate compilation and tests, not full menu fidelity.


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

## Stage 1 implementation status — extractor (NOT ACCEPTED)

The first native extraction/export pass is integrated into the 3DSX runtime. It is a partial Stage 1 implementation, not a completed or validated extractor.

### Current runtime behavior

- Put either a legally obtained `rom.nes` iNES/NES 2.0 ROM or the project’s raw `CHR_ROM.chr` dump beside the launched `.3dsx`. The iNES file takes precedence if both are present.
- On startup, the app checks the ROM fingerprint and cache, then extracts or reloads CHR-ROM tile pixels.
- Exports are written beside the app in `extracted/`: `chr_raw.bin` (original CHR bytes), `chr_pixels.bin` (decoded 2bpp indices), `chr_atlas.png` (256×240 grayscale-index atlas), and `manifest.txt`.
- The internal decoded cache is `extracted/chr_cache.bin`; it is distinct from the exported pixel data.
- Controls: D-pad left/right changes atlas pages; A opens the atlas and, while it is open, retries the export; B closes the atlas or retries extraction while the atlas is closed.
- START is deliberately locked out of the menu until Stage 1 is accepted.
- The PNG uses grayscale to expose 2bpp pixel indices. It does **not** claim to show Excitebike's actual palette or a complete original screen.

### Current hard limits and remaining work

- The runtime accepts ROM files up to 8 MiB and decodes at most 960 tiles for the current atlas/cache. The raw CHR export preserves the entire validated CHR-ROM byte range; if it contains more than 960 tiles, the decoded preview currently contains only the first 960.
- CHR-RAM is reported as requiring runtime PPU capture; it is not falsely treated as CHR-ROM. The repository’s standalone `Excitebike/CHR_ROM.chr` can be copied beside the `.3dsx` under the exact name `CHR_ROM.chr` to inspect the verified raw 8 KiB CHR dump without a full ROM file.
- Actual PPU palette RAM writes, nametable/attribute capture, CHR bank switching, compressed/generated PRG graphics, sprites/OAM, scrolling, timing, and the full advanced diagnostic UI are not yet integrated.
- This implementation has not been accepted on physical New Nintendo 3DS hardware. Host tests and a successful 3DS build are necessary but not sufficient for the Stage 1 gate.

`stb_image_write.h` is vendored from the public-domain stb project (Sean Barrett; v1.16) and is compiled into the native homebrew. No Python program is needed at runtime.

## Current next priorities

The mandatory staged acceptance gates above control all work. Do not use this list as permission to work on later stages in parallel.

1. Complete and validate the native `.3dsx` extractor (Stage 1), including SD exports, source-derived graphics/data diagnostics, regression tests, an exact-revision 3DS build, and hardware verification.
2. Only after Stage 1 is formally accepted, reconstruct and validate the original menu (Stage 2).
3. Only after Stage 2 is formally accepted, implement and validate Course 1, including the exact player motorcycle physics and all required race behavior (Stage 3).
4. Continue through subsequent original game states in source order, opening one stage at a time and recording acceptance evidence in `3ds-port/TODO.md`.

The runtime CHR cache now persists decoded tile pixels to the SD card and reloads them on subsequent launches. It remains a diagnostic tile cache, not a reconstructed title-screen texture pack. The menu remains blocked until Stage 1 is accepted. After that gate opens, integrate and validate menu setup commands and trace exact PPU tile/palette placement before proceeding to the first race. The detailed live checklist is in [`3ds-port/TODO.md`](3ds-port/TODO.md). The state-13 `$FF` completion branch has been translated and tested; the full menu remains incomplete while state 2 and destination states 9/10/12 are unported. See [`ASM_FUNCTION_INVENTORY.md`](3ds-port/ASM_FUNCTION_INVENTORY.md) for the label-by-label index and [`REFERENCE_MAPPING.md`](3ds-port/REFERENCE_MAPPING.md) for trace details and open questions.

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
