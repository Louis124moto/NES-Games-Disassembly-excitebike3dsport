# Stage 3 — first course implementation TODO

Scope: **only the first original course** (the title-menu selector's first race choice). Do not start courses 2–5 or optional editor/tape-save features until this course is accepted.

## Source facts established (2026-11-19)

- Race state 2 enters `ofs_000_C99A_02`.
- For the title-menu route, `ram_0043` carries the selected course index and `ram_03F2=1`.
- Setup command 1 writes `ram_0045 = $12 + ram_0043` while `ram_0041 < 8`. Thus the first course selects setup/palette list **$12**.
- `tbl_C0F6/$C108` maps list $12 to `_off_000_D3D6_12`. That list contains palette writes to PPU $3F00 and $3F10. It is **palette data, not course geometry**; do not mistake it for the track layout.
- The race frame order in `ofs_000_C99A_02` is source-defined. Before the main object pass it calls `sub_D918`, `sub_DD8D`, `sub_E733`, `sub_E927`; after the optional `ram_03F2` branch it calls `sub_DDD1`, `sub_CD1F`, `sub_E96C`, `sub_DE31`, `sub_E836`, `sub_EA44`, `sub_F4FF`, `sub_E4C8`, `sub_DEE2`, `sub_E359`, `sub_E17F`, `sub_E456`, `sub_DA26`, `sub_E70B`, `sub_D19B`, `sub_CED0`, then `loc_E42B`.
- `sub_DA26` calls, for X=3 down to 0, `sub_DA58`, `sub_DC1A`, `sub_DCF2`, `sub_DCDE`; then `sub_DA6A`, `sub_DA7A`, `sub_DBFE`, followed by a timer-selected branch. The already-tested DA58/DA6A/DA7A helpers are only slices of this caller.

## Implementation order

### A. Complete the minimum native race simulation
- [ ] Build semantic per-bike state from the complete first-course setup path, including the C875 tail and player/opponent identity.
- [ ] Port the remaining state handlers called by `sub_D918` and the object-update chain; preserve the exact call order above.
- [ ] Complete the per-bike pass `DA58 → DC1A → DCF2 → DCDE`, then camera/opponent movement and `DBFE`.
- [ ] Complete steering, braking, posture, jump/landing, heat/overheat, collision/crash/recovery and opponent decisions.
- [ ] Differential-test each routine and deterministic whole-frame scenarios against the host-only original-ASM oracle.
- [ ] Integrate the semantic race update into the native application loop. No NES RAM image or CPU execution in the shipped runtime.

### B. Decode first-course data
- [ ] Trace the first-course geometry/data source and every table/pointer it consumes; distinguish palette list $12 from course data.
- [ ] Convert the course into a compact native structure (segments, surface, elevation, ramps, obstacles and collision bounds) using source-derived values.
- [ ] Add host tests for decoded segment count, boundaries, wrap/termination and source-to-native correspondence.
- [ ] Keep later courses out of the first implementation.

### C. Native presentation
- [ ] Identify the exact source CHR/tile/sprite assets and palette for course 1.
- [ ] Implement native 3DS course/background rendering and horizontal scrolling; no PPU/nametable emulator.
- [ ] Draw bikes, opponents, ramps/obstacles and HUD from semantic state.
- [ ] Compare fixed-frame captures with the original reference and record any known platform-specific differences.

### D. Acceptance gate
- [ ] First course can be entered from the native menu, raced from start to finish, and reaches the correct result/return flow.
- [ ] Physics, course events, opponent behavior and scoring pass deterministic differential scenarios.
- [ ] Host tests and exact-commit New 3DS build pass.
- [ ] Physical New 3DS run and visual comparison recorded.
- [ ] Only after acceptance, begin course 2.

## Guardrails

- A translated helper is not automatically an integrated game feature.
- Do not draw guessed course geometry or tune physics by feel and label it faithful.
- Host-side 6502 oracle use is test-only. The New 3DS runtime remains native C/C++ only.
- Do not claim playable status or hardware verification until the acceptance gate is met.
