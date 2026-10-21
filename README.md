# Excitebike 3DS — faithful NES port

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

## Current menu/graphics work

The native menu currently translates input/state behavior for ASM states 0, 1, and 8, including `sub_C7DE`, `sub_C3AF/sub_C3B1`, the state-8 choice branches, choice-5 RAM clearing, cursor-table coordinates, and timer projection. The setup sequencer is still only partially ported: states 0/1/8 have source-derived command lists, but state 2 and destination states 9/10/12/13 do not have complete setup lists. Therefore the title-to-selector input trace is **not a finished menu port**, and state 2 deliberately does not launch placeholder gameplay. Option names and authentic title-screen layout are not inferred from numeric selection indices.

Graphics extraction has a one-command Python pipeline (`3ds-port/tools/extract_nes_assets.py`) and a runtime CHR inspector built into the homebrew. On the 3DS, press Y to open the inspector; it reads a classic iNES ROM from `sdmc:/3ds/excitebike3ds/rom.nes`, X reloads it, and B/Y returns. It decodes up to 960 CHR-ROM tiles and displays a grayscale atlas without requiring a rebuild. The Python pipeline remains available for PNG/JSON/header outputs on a PC. It accepts iNES/NES 2.0 CHR-ROM or verified raw CHR ranges. This is not a menu-art reconstruction: the exact source graphics, palette/attribute data, nametable tile placement, transparency, scrolling and PPU timing still need tracing. No ROM dump is checked into the repository. On revision `e3459d1e518603b73375aaa9ff06215358c7e8a6`, the Python extraction tests, host C regression suite, and host tests for the runtime iNES CHR decoder passed in [CI run 39524481254](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39524481254). The matching 3DS build also passed in [CI run 39524481329](https://github.com/Louis124moto/NES-Games-Disassembly-excitebike3dsport/actions/runs/39524481329). These checks validate compilation and tests, not full menu fidelity.


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

## Current next priorities

1. Establish player-slot identity and initialization/update paths by tracing the relevant RAM writers, callers, and indirect writes.
2. Trace the actual player input and movement path, then prioritize acceleration, braking, overheating/cooling, jumping, posture, landing, and crashes.
3. Finish and validate each translation batch: run host tests, correct mismatches, build the 3DS target, and compare boundary cases against the assembly.
4. Integrate validated routines in batches only after their dependencies and call order are understood.
5. Reconstruct course/terrain data and rendering, opponents, race progression, scoring/results, menus, and audio.
6. Compare complete gameplay scenarios against the original NES version; document any known differences.
7. Test on both original 3DS and New 3DS hardware, then optimize and prepare a versioned `.3dsx` release.

The detailed live checklist is in [`3ds-port/TODO.md`](3ds-port/TODO.md). See [`ASM_FUNCTION_INVENTORY.md`](3ds-port/ASM_FUNCTION_INVENTORY.md) for the label-by-label index and [`REFERENCE_MAPPING.md`](3ds-port/REFERENCE_MAPPING.md) for trace details and open questions.

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
