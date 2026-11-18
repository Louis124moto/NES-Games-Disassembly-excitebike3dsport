# Stage 3 trace — first course entry (started 2026-11-18)

Status: **source trace only. No gameplay code is ported yet.** Everything below is read from `Excitebike/bank_FF.asm`; nothing is inferred from slot numbers or generic physics.

## Entry
- Track selector (state 1) confirm → `sub_C3AF` INC state → state 2 (`tbl_C000` → `ofs_000_C99A_02`), with `ram_0043` = selected track, `ram_03F2` = 1 for the selection from the title menu (design-menu path uses states 8→0A).
- Setup list for state 2 (`tbl_C01C_index` = 2 → `tbl_C02A_lo/tbl_C033_hi`), six commands run by `sub_C2A9` one per frame (`ram_0044`):
  1. `ofs_001_C551_02_01`: `ram_0045 = $12 + ram_0043` when `ram_0041 < 8` (PPU list $12–$16, palette per track), else $12.
  2. `ofs_001_C869_02_02`: clear `$004D–$00EF` (X from $EF down to $4D).
  3. `ofs_001_C820_02_03`: `sub_D13F` (`sub_ECE4` for X=0,4,8), `ram_0045=1`, `ram_004B=0`, clear `$0340–$03EF`, fill `$0400–$047F=$3B`, `$0480–$057F=$3D`, sprite 0 `Y=$BF,T=$FF,X=$F7`, `ram_00E8=$3F, 00E9=$1F, 00EB=$0A, 00EA=$25`.
  4. `ofs_001_C5A3_02_04`: `ram_0045 = $17`.
  5. `ofs_001_C566_02_05`: `ram_0045 = $0E`.
  6. `ofs_001_C875_02_06`: `INC ram_0047, INC ram_0040`, `ram_0034_timer` default 5, `E8=$34, E9=$14`, `03F1=$A0`, `03B4=03B0=$80`, `03BD=$FF`, `0370=1`, `03BE=2`, then per-object init loop from `tbl_C0B4/C0B0/C0B8` (X=3, or 0 when `ram_03F2==0`) — remainder not yet read.
- Race frame handler `ofs_000_C99A` (states 2/5/0A): `sub_CA08` (4× `sub_DCA0`+`sub_DFB2`, pause logic, `ram_005C = ram_btn_hold`), then unless paused: `sub_D918, sub_DD8D, sub_E733, sub_E927`, finish test `ram_0052`, then `sub_DA9F/DBC4/DEBB` (when `ram_03F2≠0`), `sub_DF30, sub_EAC5, sub_DFD5, sub_E09F`, `sub_DDD1, sub_CD1F, sub_E96C, sub_DE31, sub_E836, sub_EA44, sub_F4FF, sub_E4C8, sub_DEE2, sub_E359, sub_E17F, sub_E456, sub_DA26, sub_E70B, sub_D19B, sub_CED0, loc_E42B`.

## Not yet traced (required before any gameplay is accepted)
Player-slot establishment (commands 6/`sub_E1A3`/`sub_EC25`/`loc_EC47`, RAM writers of the player object, input via `ram_005C`), every subroutine in the frame handler above, PPU lists $01/$12–$17 contents, `sub_D14E` raster-split wait (sprite-0 hit), track data decoding, opponents, timers, scoring, crashes/collisions.

## Next slice
Port commands 1, 2, 4, 5 (pure data writes) with host tests, then 3 and 6 once `sub_ECE4`, `sub_E1A3` are traced.
