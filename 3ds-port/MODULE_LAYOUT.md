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

The former root-level `source/*.c` compatibility symlinks have been removed:
workflows, tests, scripts and docs now use the real module paths, and
`source/` contains only the module directories. Public headers live in
`include/` (there are no headers inside `source/`).

## Enforcement

`tests/test_native_runtime_boundary.py` (run by CI) fails when:
- `source/` contains anything but the module directories, or a header sits in a module directory;
- `frontend`, `engine`, `assets` or the pure parts of `graphics`/`audio` (scene builder, sound driver/synth/data) include libctru/citro3d;
- `engine`, `platform` or `audio` depend on another module, `frontend` on anything but `engine`, or `graphics` on anything but `frontend`/`engine`/`assets`;
- PPU/software-framebuffer leftovers or emulator sources reappear in the runtime.
Physical key mapping stays in `platform` (the extractor takes semantic confirm/cancel).
