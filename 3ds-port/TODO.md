# Excitebike 3DS — roadmap and task tracker

This is the ordered source of truth for the remaining port work. Tasks are checked only after they are actually completed and, where applicable, verified. The GitHub Actions workflow compiles the project on every push and on pull requests targeting `main`.

## Phase 1 — Build foundation

- [x] Create the `3ds-port/` project structure.
- [x] Add the initial C entry point and devkitARM Makefile.
- [x] Add a GitHub Actions workflow that attempts a native 3DS build on each push and uploads the `.3dsx`/ `.elf` artifacts.
- [ ] Verify the first GitHub Actions run; fix any container, dependency, Makefile, or linker errors.
- [ ] Confirm that the generated `.3dsx` launches on a real 3DS or a compatible emulator.
- [x] Add a `.gitignore` for generated build output and temporary files.

## Phase 2 — Application structure

- [ ] Define separate modules and interfaces for game state, input, rendering, track data, bike physics, collision, audio, and menus.
- [ ] Add a stable frame/update loop with clear timing and frame-rate handling.
- [ ] Add a simple top-screen status/debug view and bottom-screen control/help view.
- [ ] Document build steps and hardware/emulator testing.

## Phase 3 — Playable prototype

- [ ] Draw a basic test track and ground.
- [ ] Add a controllable bike placeholder.
- [ ] Implement acceleration, braking, steering, and speed display.
- [ ] Implement jumping and landing.
- [ ] Add basic collision and crash/recovery behavior.
- [ ] Add a restartable test course and finish condition.

## Phase 4 — Excitebike gameplay

- [ ] Inspect the original NES source and document its game logic, data formats, and asset/license constraints.
- [ ] Port or reimplement the track representation and track selection.
- [ ] Implement ramps, terrain effects, overheating, and other core mechanics as supported by the source.
- [ ] Add opponents and race progression.
- [ ] Implement menus, scoring, and results.
- [ ] Add sound and music using assets that can legally be redistributed, or newly created replacements.

## Phase 5 — 3DS polish and release

- [ ] Optimize rendering, memory use, and frame pacing for original 3DS hardware.
- [ ] Test controls and layout on original 3DS and New 3DS hardware where available.
- [ ] Test clean builds and document known limitations.
- [ ] Produce a versioned `.3dsx` release and release notes.

## Progress rules

- Keep tasks in dependency order; do not mark a task complete merely because a file was created.
- Update this checklist whenever a task is completed or its scope changes.
- Record build/test evidence in the relevant commit or workflow run when possible.
- The project is **not playable yet** until the prototype and gameplay tasks are implemented and tested.
