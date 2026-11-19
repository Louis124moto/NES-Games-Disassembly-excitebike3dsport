# Native port architecture

## Goal

Build Excitebike as a native Nintendo 3DS game. The final executable contains native ARM code for game rules, state, rendering and audio. It does not execute the original NES program and does not emulate NES hardware.

## Runtime dependency boundary

```text
                Native Excitebike game
                         |
       +-----------------+------------------+
       |                 |                  |
  Game flow         Semantic game state   Native assets
       |                 |                  |
  Menus / race      Bike / track /       Sprites / tiles /
  editor / results  opponents / results  palettes / sound data
       |                 |                  |
       +-----------------+------------------+
                         |
               Platform-independent C
                         |
       +-----------------+------------------+
       |                 |                  |
   libctru input     3DS graphics       3DS audio
   HID actions       native renderer    native mixer
       |                 |                  |
       +-----------------+------------------+
                         |
                 Nintendo 3DS hardware
```

The diagram is conceptual. Keep game rules deterministic and independent of libctru; platform backends consume semantic state and render/audio events.

## Explicitly forbidden in the final runtime

- 6502 CPU core, interpreter, JIT or generated 6502-to-C execution engine.
- NES ROM loader/boot sequence or runtime dependency on `rom.nes`.
- NES bus, CPU register file or a full NES RAM image as the live game state.
- PPU/APU/mapper emulation, NMI/IRQ emulation, scanline timing or sprite-0-hit simulation.
- Rendering by replaying NES PPU writes at runtime.
- Audio by emulating APU registers at runtime.

These are not performance optimizations to be disabled later; they are outside the product architecture.

## Permitted development-only reference tools

The original disassembly and ROM may be used to understand and verify behavior. A 6502 reference CPU or an external emulator may be used by host-side tools/tests to generate independent expected results. Such code must:

- live under host-only test/tool directories (for example `tests/oracle/`);
- be excluded from the devkitARM/3DS source glob and link;
- never be initialized or called by the 3DS application;
- never be required to build, launch or play the released game.

The source ASM remains unchanged and is the authority for original behavior.

## Native modules

Use these as responsibility boundaries, adapting names to the actual codebase:

| Module | Native responsibility |
|---|---|
| `game_state.c` | Semantic game/session state and reset |
| `game_flow.c` | Title-to-race-to-results state machine |
| `input.c` | libctru HID mapped to semantic game actions |
| `menus.c` | Title, selector, design menu, editor and transitions |
| `bike.c` | Rider/bike state and frame update |
| `bike_physics.c` | Source-derived speed, acceleration, braking, heat, jumps, posture, landing and recovery |
| `track.c` | Native course geometry, terrain, ramps and scrolling |
| `opponents.c` | Opponent state, AI and movement |
| `collision.c` | Source-derived collision rules |
| `renderer.c` | Native 3DS rendering of semantic scenes/sprites/HUD |
| `audio.c` | Native playback/mixing from verified source sound data |
| `save_data.c` | Native SD save/load format and validation |
| platform backends | 3DS graphics, audio, input, storage and timing |

Do not create a module full of guessed placeholders just to match this table. Trace the original behavior first and implement only understood slices.

## Semantic state, not NES memory

The original zero-page and RAM symbols are valuable trace references, not the final C data model. Translate the meaning of state into explicit structures, for example:

- `BikeState`: speed/fraction, heat, throttle/brake state, posture, airborne/landing/crash state, position.
- `TrackState`: course identity, geometry, current segment, terrain/ramp properties and scroll.
- `OpponentState`: active state, position, speed, behavior mode and collision state.
- `MenuState`: screen, selection, timers and transition phase.
- `RaceState`: frame/time, progress, finish state, score and results.

These are conceptual names, not claims that every field is already mapped. Each field must be justified by the ASM trace and documented source dependencies.

Preserve the original arithmetic where observable: byte/word truncation, carry/borrow, signed comparisons, lookup-table indexing, update order and timer cadence. Use explicit-width integer types where they help make those rules clear.

## Rendering and assets

- Extract original CHR, palette, sprite, course and menu data into documented native formats.
- Convert source tile/palette data into native textures/atlases or sprite data before runtime.
- Reconstruct screens from verified game-level scene data, not a live PPU model.
- Use native 3DS rendering APIs and a 3DS-appropriate screen layout.
- Compare fixed-frame screenshots against original NES reference captures. Account for scaling/letterboxing separately from logical game composition.
- Do not invent missing graphics; trace them or mark them unresolved.

The asset inspector is a development utility. It may display raw CHR or static extracted previews, but it must be separable from the playable game and must not impose a ROM requirement on the game.

## Audio

- Identify sound/music data and event triggers from the original ASM.
- Convert verified data into a native representation suitable for 3DS playback/mixing.
- Reproduce event order, pitch, duration and timing as closely as possible.
- Do not recreate the APU as a runtime hardware device or emulate its registers.

## Differential verification

For each native subsystem:

1. Derive expected behavior from the original ASM.
2. Use a host-only oracle or independently parsed vectors to produce expected outputs.
3. Feed identical initial semantic conditions and input sequences to the native C implementation.
4. Compare state and observable events after each original game frame where frame behavior matters.
5. Cover normal cases, branch conditions, boundaries, resets, transitions and combined inputs.
6. Keep vector generation reproducible and document source/ROM identity and test preconditions.
7. Record test counts and CI run links without implying that a large count alone proves complete behavior.

For physics, compare at minimum speed/fraction, acceleration/deceleration mode, heat, position, posture, airborne/landing/crash/recovery state, opponent state, timers and events as each path becomes understood.

## Build and acceptance

The 3DS executable must build with devkitARM/libctru and run without any NES ROM. For each milestone, require host tests, differential tests, exact-commit 3DS build, integration into the real game loop and physical New 3DS testing where relevant.

A native function can be translated and tested while the game remains unplayable. Report those statuses separately. The product is not a faithful port until the full game flow has been implemented and accepted.

## Branch policy

- `main`: faithful native baseline only.
- `enhanced`: create only after full acceptance of `main`, from the exact accepted commit.
- No enhancements or speculative substitutes in the faithful branch.
