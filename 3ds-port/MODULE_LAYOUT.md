# Module layout

The native application is physically grouped by responsibility. Original
`source/*.c` paths are retained as relative symlinks for compatibility with
existing host tests, scripts and downstream build commands. The devkitARM
Makefile compiles only real files in the module directories, so sources are
never compiled twice.

| Directory | Responsibility |
|---|---|
| `source/app/` | Application entry point and top-level lifecycle |
| `source/frontend/` | Native title, course selector, menu state and menu assets |
| `source/engine/` | Deterministic game rules, bike physics, race scheduler, AI, course stream and track state |
| `source/graphics/` | Scene construction, citro3d/PICA200 renderer and vertex shader |
| `source/audio/` | Native audio backend |
| `source/assets/` | Asset extraction, CHR decoding, palettes and course export |
| `source/platform/` | libctru input adapter |
| `include/` | Public module interfaces and semantic data structures |
| `tests/`, `tools/` | Host-only verification, oracle and development tools |

## Dependency direction

- `app` composes frontend, engine, graphics, audio and platform services.
- `frontend` owns menu state and requests gameplay actions; it does not own race physics.
- `engine` remains deterministic C and must not depend on libctru/citro3d.
- `graphics` consumes semantic game state and exported native assets.
- `platform` translates hardware input into semantic controls.
- `assets` is an import/development boundary; the playable runtime never executes the NES ROM.
- `tests/oracle` remains host-only and is never included in the 3DS target.

## Compatibility policy

The root-level source symlinks are migration support for existing host tests and
scripts. New implementation files belong in their owning module directory.
New tests should prefer the real module path. Remove compatibility symlinks only
after all workflows and downstream scripts have migrated.
