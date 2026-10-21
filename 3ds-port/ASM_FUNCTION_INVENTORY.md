# Excitebike ASM routine inventory

Source: `Excitebike/bank_FF.asm` on branch `nes-game`. This is a tracking index, not a claim that routines are already ported. There are **177 labels beginning with `sub_`**. The label count includes small helpers and some labels that may be aliases or non-returning loops; each must be classified before it is considered a standalone function.

## Status legend
- `Translated + tested`: exact native implementation has a matching regression test and passing CI.
- `Analyzed`: behavior traced/documented, but not yet fully translated.
- `Pending`: not yet fully analyzed or translated.
- Hardware-specific routines (PPU/APU/interrupt handling) need a native equivalent, not a literal register write.


## $C000–$CFFF · startup, menus and flow

| CPU address | ASM label | Status | Calls / notes |
|---|---|---|---|
| `$C1FD` | `sub_C1FD` | Pending | — |
| `$C229` | `sub_C229` | Pending | — |
| `$C27F` | `sub_C27F` | Pending | ; see con_0045 |
| `$C2A9` | `sub_C2A9` | Analyzed (dispatcher trace; native integration pending) | Selects `$0040` animation dispatch or timer-gated setup-list dispatch via `$0041`, `$0044`, `tbl_C01C`, `tbl_C02A/$C033`; see `REFERENCE_MAPPING.md`. |
| `$C318` | `sub_C318` | Analyzed (cleanup trace; native PPU integration pending) | Calls `sub_D255`, `sub_D23B`, `sub_D1A9`; zeroes scroll bytes and `$00FC`, then enters `sub_C333`. |
| `$C32F` | `sub_C32F` | Translated in C (RAM-level helper; hardware integration pending) | `nesSubC32FMaskPpuCtrl`; masks bit 7 of `$10` shadow, not a native PPU implementation. |
| `$C333` | `sub_C333` | Translated in C (RAM-level helper; hardware integration pending) | `nesSubC333WritePpuCtrl`; preserves the `$2000` shadow write semantics, not a native PPU implementation. |
| `$C339` | `sub_C339` | Pending | — |
| `$C3AF` | `sub_C3AF` | Pending | — |
| `$C3B1` | `sub_C3B1` | Pending | — |
| `$C409` | `sub_C409` | Pending | ; in |
| `$C522` | `sub_C522` | Pending | calls: sub_CB13 |
| `$C53A` | `sub_C53A` | Pending | — |
| `$C53E` | `sub_C53E` | Pending | calls: sub_CB06 |
| `$C621` | `sub_C621` | Pending | — |
| `$C62D` | `sub_C62D` | Pending | — |
| `$C639` | `sub_C639` | Pending | — |
| `$C64E` | `sub_C64E` | Pending | — |
| `$C653` | `sub_C653` | Pending | — |
| `$C66E` | `sub_C66E` | Pending | calls: sub_C653 |
| `$C68A` | `sub_C68A` | Pending | ; in |
| `$C691` | `sub_C691_unk___infinite_wait` | Pending | calls: sub_C639 |
| `$C6A5` | `sub_C6A5` | Pending | ; out |
| `$C6C8` | `sub_C6C8` | Pending | calls: sub_C716, sub_C6D1, sub_C726 |
| `$C6D1` | `sub_C6D1` | Pending | — |
| `$C716` | `sub_C716` | Pending | — |
| `$C726` | `sub_C726` | Pending | — |
| `$C739` | `sub_C739` | Pending | — |
| `$C792` | `sub_C792` | Pending | ; in |
| `$C7DE` | `sub_C7DE` | Translated in C; host regression wired, execution pending | input: ram_btn_press, ram_0042; callers pass bounds 3/5/6 |
| `$C92C` | `sub_C92C` | Pending | — |
| `$C935` | `sub_C935` | Pending | calls: sub_DF7F |
| `$C94B` | `sub_C94B` | Pending | calls: sub_C971 |
| `$C971` | `sub_C971` | Pending | — |
| `$CA08` | `sub_CA08` | Pending | — |
| `$CA1B` | `sub_CA1B` | Pending | — |
| `$CA38` | `sub_CA38_pause_and_unpause` | Pending | — |
| `$CA9B` | `sub_CA9B` | Pending | calls: sub_C3AF |
| `$CB06` | `sub_CB06` | Pending | calls: sub_CB13 |
| `$CB13` | `sub_CB13` | Pending | — |
| `$CC15` | `sub_CC15` | Pending | — |
| `$CC6B` | `sub_CC6B` | Pending | — |
| `$CCCA` | `sub_CCCA` | Pending | — |
| `$CD1F` | `sub_CD1F` | Pending | — |
| `$CD59` | `sub_CD59` | Pending | — |
| `$CDEE` | `sub_CDEE` | Pending | — |
| `$CE29` | `sub_CE29` | Pending | — |
| `$CE58` | `sub_CE58` | Pending | — |
| `$CE5C` | `sub_CE5C` | Pending | — |
| `$CED0` | `sub_CED0` | Pending | — |
| `$CF0C` | `sub_CF0C` | Pending | calls: sub_CF96, sub_CFDB |
| `$CF47` | `sub_CF47` | Pending | calls: sub_CF88, sub_CF96 |
| `$CF6A` | `sub_CF6A` | Pending | calls: sub_CF88, sub_CF96 |
| `$CF88` | `sub_CF88` | Pending | calls: sub_E7FF; ; out |
| `$CF8B` | `sub_CF8B` | Pending | ; out |
| `$CF96` | `sub_CF96` | Pending | — |
| `$CFCB` | `sub_CFCB` | Pending | — |
| `$CFDB` | `sub_CFDB` | Pending | — |
| `$CFE3` | `sub_CFE3` | Translated in C; exhaustive regression wired, execution pending | called by sub_CFEA |
| `$CFEA` | `sub_CFEA` | Translated in C; exhaustive regression wired, execution pending | calls: sub_CFE3 |

## $D000–$DFFF · input, timing and object state

| CPU address | ASM label | Status | Calls / notes |
|---|---|---|---|
| `$D000` | `sub_D000` | Translated in C; audit/test coverage recorded separately | — |
| `$D018` | `sub_D018` | Pending | ; out |
| `$D0AB` | `sub_D0AB` | Translated in C; audit/test coverage recorded separately | — |
| `$D0C6` | `sub_D0C6` | Pending | — |
| `$D0F4` | `sub_D0F4` | Pending | — |
| `$D11F` | `sub_D11F` | Translated in C; audit/test coverage recorded separately | — |
| `$D13F` | `sub_D13F` | Pending | calls: sub_ECE4 |
| `$D14E` | `sub_D14E` | Pending | — |
| `$D19B` | `sub_D19B` | Pending | — |
| `$D1A9` | `sub_D1A9` | Pending | — |
| `$D1B3` | `sub_D1B3` | Pending | — |
| `$D1C7` | `sub_D1C7` | Pending | ; in |
| `$D23B` | `sub_D23B` | Pending | — |
| `$D24F` | `sub_D24F_disable_rendering` | Pending | — |
| `$D255` | `sub_D255_hide_all_sprites` | Pending | — |
| `$D260` | `sub_D260` | Pending | calls: sub_C333 |
| `$D2FF` | `sub_D2FF` | Translated in C; audit/test coverage recorded separately | — |
| `$D310` | `sub_D310_decrease_all_timers` | Translated + tested (host exhaustive test passed; 3DS build pending) | — |
| `$D326` | `sub_D326` | Translated + host regression passed; 3DS build pending | — |
| `$D347` | `sub_D347_read_joy_regs` | Pending | calls: sub_D35B |
| `$D35B` | `sub_D35B` | Translated + host regression passed; 3DS build pending | — |
| `$D3C1` | `sub_D3C1` | Pending | — |
| `$D918` | `sub_D918` | Pending | — |
| `$D924` | `sub_D924` | Pending | — |
| `$DA15` | `sub_DA15` | Translated + tested (host regression passed; 3DS build pending) | — |
| `$DA26` | `sub_DA26` | Pending | — |
| `$DA58` | `sub_DA58` | Translated in C; audit/test coverage recorded separately | — |
| `$DA6A` | `sub_DA6A` | Translated + tested; host tests and 3DS build passed on `a7537aff` | Helper and regression test added |
| `$DA7A` | `sub_DA7A` | Translated + tested; host regression passed, 3DS build status being checked | Exact helper and instruction-level regression test added |
| `$DA9F` | `sub_DA9F` | Pending | — |
| `$DAE3` | `sub_DAE3` | Pending | calls: sub_DB95_clear_opponent_bike_data, sub_E7FF; ; in |
| `$DB50` | `sub_DB50` | Translated + tested; host tests and 3DS build passed on `a7537aff` | Helper and regression test added |
| `$DB95` | `sub_DB95_clear_opponent_bike_data` | Translated + tested; host tests and 3DS build passed on code revision `730a1048` | Exact instruction-level/RAM-field regression test added |
| `$DBC4` | `sub_DBC4` | Pending | — |
| `$DBFE` | `sub_DBFE` | Translated + tested; host tests and 3DS build passed on code revision `730a1048` | Exact instruction-level/RAM-field regression test added |
| `$DC1A` | `sub_DC1A` | Pending | calls: sub_DC97 |
| `$DC97` | `sub_DC97` | Translated in C; audit/test coverage recorded separately | — |
| `$DC9A` | `sub_DC9A` | Translated + tested; host tests and 3DS build passed on code revision `730a1048` | Exact instruction-level/RAM-field regression test added |
| `$DCA0` | `sub_DCA0` | Translated + tested; host tests and 3DS build passed on `a7537aff` | Helper and regression test added |
| `$DCC7` | `loc_DCC7` | Translated + tested; host tests and 3DS build passed on `a7537aff` | Shared helper and regression test added |
| `$DCDE` | `sub_DCDE` | Translated + tested; host tests and 3DS build passed on code revision `730a1048` | Exact instruction-level/RAM-field regression test added |
| `$DCF2` | `sub_DCF2` | Pending | — |
| `$DD06` | `sub_DD06` | Analyzed (partial trace; translation pending) | `$DD06-$DD37` traced; depends on `sub_DD38` and `sub_DD6F`; full caller/callee equivalence not verified |
| `$DD38` | `sub_DD38` | Translated + tested; host tests and 3DS build passed on code revision `730a1048` | Exact instruction-level/RAM-field regression test added |
| `$DD6F` | `sub_DD6F` | Translated + tested; host tests and 3DS build passed on code revision `730a1048` | Exact instruction-level/RAM-field regression test added |
| `$DD8D` | `sub_DD8D` | Pending | — |
| `$DDD1` | `sub_DDD1` | Pending | — |
| `$DE31` | `sub_DE31` | Pending | — |
| `$DEBB` | `sub_DEBB` | Pending | — |
| `$DEE2` | `sub_DEE2` | Pending | — |
| `$DF30` | `sub_DF30` | Pending | — |
| `$DF7F` | `sub_DF7F` | Pending | calls: sub_DFA2; ; in |
| `$DFA2` | `sub_DFA2` | Translated in C; audit/test coverage recorded separately | — |
| `$DFA4` | `sub_DFA4` | Translated in C; audit/test coverage recorded separately | — |
| `$DFB2` | `sub_DFB2` | Translated in C; audit/test coverage recorded separately | — |
| `$DFD5` | `sub_DFD5` | Pending | — |

## $E000–$EFFF · race/object updates

| CPU address | ASM label | Status | Calls / notes |
|---|---|---|---|
| `$E091` | `sub_E091` | Translated in C; audit/test coverage recorded separately | — |
| `$E098` | `sub_E098` | Translated in C; audit/test coverage recorded separately | — |
| `$E09F` | `sub_E09F` | Pending | calls: sub_DC97 |
| `$E16C` | `sub_E16C` | Translated + regression-tested; 3DS build passed | OAM writes at Y=6,5,2,1; instruction-level edge cases still need expansion |
| `$E17F` | `sub_E17F` | Pending | calls: sub_E3F4 |
| `$E1A3` | `sub_E1A3` | Command setup + overlay regression passed; 3DS build passed | `$D1C7` renderer remains separate |
| `$E1F9` | `sub_E1F9` | Full branch-level translation; host regression and 3DS build passed | Includes `$000E` and `$039F` side effects and all RTS paths |
| `$E26A` | `sub_E26A` | Command-preparation model; host regression and 3DS build passed | Gate and renderer parameters translated; `loc_D1C7` rendering remains separate |
| `$E2F6` | `sub_E2F6` | Full OAM path translated; host regression and 3DS build passed | Two wheel sprites and all early-return gates |
| `$E359` | `sub_E359_display_temperature_meter_with_sprites` | Host regression and 3DS build passed | Heat update and four OAM sprite records |
| `$E3F4` | `sub_E3F4` | Translated + regression-tested; 3DS build passed | Four-slot unsigned Y sort and slot-3 override |
| `$E456` | `sub_E456` | Translated + regression-tested; host suite and 3DS build passed | Culling, opponent reset, and sprite hiding |
| `$E49B` | `sub_E49B_hide_12_sprites` | Translated + regression-tested; 3DS build passed | Actual helper invokes `sub_E4AD` for each clear low bit |
| `$E4AD` | `sub_E4AD_hide_4_sprites` | Translated + regression-tested; 3DS build passed | Literal loop writes three Y bytes at offsets 0,4,8 |
| `$E4C8` | `sub_E4C8` | Host regression and 3DS build passed | Object overlap scan; preserves scratch `$000B` input/output |
| `$E52C` | `sub_E52C` | Translated in C; audit/test coverage recorded separately | — |
| `$E70B` | `sub_E70B` | Translated in C; audit/test coverage recorded separately | — |
| `$E733` | `sub_E733` | Pending | — |
| `$E73B` | `sub_E73B` | Pending | — |
| `$E794` | `sub_E794` | Pending | — |
| `$E7FC` | `sub_E7FC` | Translated in C; audit/test coverage recorded separately | — |
| `$E7FF` | `sub_E7FF` | Translated in C; audit/test coverage recorded separately | — |
| `$E803` | `sub_E803` | Translated in C; audit/test coverage recorded separately | calls: sub_E80D_prepare_pointers |
| `$E80D` | `sub_E80D_prepare_pointers` | Pending | ; out |
| `$E836` | `sub_E836` | Translated in C; audit/test coverage recorded separately | — |
| `$E84F` | `sub_E84F` | Pending | — |
| `$E86F` | `sub_E86F` | Pending | — |
| `$E893` | `sub_E893` | Pending | ; in |
| `$E927` | `sub_E927` | Pending | — |
| `$E96C` | `sub_E96C` | Analyzed (partial block only; translation pending) | `nesLocE96EApplyVerticalDelta` covers `$E96E-$E97D` only; remaining state logic and player-slot identity unresolved |
| `$EA44` | `sub_EA44` | Host regression and 3DS build passed | Special wheel tile and fallback wheel rendering |
| `$EAC5` | `sub_EAC5` | Pending | — |
| `$EB17` | `sub_EB17` | Pending | — |
| `$EC25` | `sub_EC25` | Pending | — |
| `$EC3B` | `sub_EC3B` | Pending | — |
| `$EC5D` | `sub_EC5D` | Pending | calls: sub_EC25 |
| `$EC75` | `sub_EC75` | Pending | — |
| `$EC99` | `sub_EC99` | Pending | calls: sub_F6FB, sub_ECBF |
| `$ECBF` | `sub_ECBF` | Pending | — |
| `$ECC9` | `sub_ECC9` | Pending | — |
| `$ECE4` | `sub_ECE4` | Pending | — |

## $F000–$FFFF · graphics, data and audio

| CPU address | ASM label | Status | Calls / notes |
|---|---|---|---|
| `$F4FF` | `sub_F4FF` | Pending | — |
| `$F64E` | `sub_F64E` | Pending | — |
| `$F676` | `sub_F676` | Pending | — |
| `$F681` | `sub_F681` | Pending | — |
| `$F68D` | `sub_F68D` | Pending | calls: sub_F839 |
| `$F6FB` | `sub_F6FB` | Pending | — |
| `$F755` | `sub_F755` | Pending | — |
| `$F821` | `sub_F821` | Pending | — |
| `$F826` | `sub_F826` | Pending | — |
| `$F82E` | `sub_F82E_prepare_pointers` | Pending | ; out |
| `$F839` | `sub_F839` | Pending | ; out |
| `$F844` | `sub_F844` | Pending | calls: sub_F8D4, sub_FA74, sub_FC3D, sub_FC8D |
| `$F86B` | `sub_F86B` | Pending | — |
| `$F871` | `sub_F871` | Pending | — |
| `$F885` | `sub_F885` | Pending | — |
| `$F889` | `sub_F889` | Pending | — |
| `$F8D4` | `sub_F8D4` | Pending | — |
| `$FA74` | `sub_FA74` | Pending | — |
| `$FC3D` | `sub_FC3D` | Pending | — |
| `$FC8D` | `sub_FC8D` | Pending | — |
| `$FF53` | `sub_FF53` | Pending | — |

| `sub_DA58` | Implemented | `nesSubDA58UpdateObjectFraction` |
| `sub_E091` | Implemented | `nesSubE091AdjustValue` |
| `sub_E098` | Implemented | `nesSubE098AdjustValue` |

## Translation policy

Translate routines in dependency order rather than blindly copying address order: pure arithmetic/timer/input helpers first; then object state and coordinate updates; then bike/race mechanics; then track/course and scoring; finally menus, PPU rendering and APU audio. Preserve 6502 carry/negative/zero flags, byte wraparound, loop bounds, call order, and RAM aliases. A routine is only marked complete after reference tests and the native build pass.

| `sub_DFA2` | Implemented | `nesSubDFA2` (shared arithmetic entry with `sub_DFA4`) |
| `sub_DFA4` | Implemented | `nesSubDFA4` |

| `sub_D0AB` | Implemented | `nesSubD0ABComputeVerticalBands` |
| `sub_DFB2` | Implemented | `nesSubDFB2SelectBand` |


| `sub_D000` | Implemented | `nesSubD000ShouldSetupObject` |
| `sub_D11F` | Implemented | `nesSubD11FInitializeUpdateBuffer` |
| `sub_D255` | Implemented | `nesSubD255HideAllSprites` |
| `sub_D2FF` | Implemented | `nesSubD2FFAdvanceBufferIndex` |
| `sub_E70B` | Implemented | `nesSubE70BUpdateObjectOffsets` |
| `sub_E80D` | Implemented | `nesSubE80DPreparePointer` |
| `sub_E803` | Implemented | `nesSubE803ReadData` |
| `sub_E7FF` | Implemented | `nesSubE7FFReadObjectData` |
| `sub_E7FC` | Implemented | `nesSubE7FCReadObjectBandData` |
| `sub_E836` | Implemented | `nesSubE836UpdateObjectDifferences` |

| `sub_E16C` | Implemented | `nesSubE16CWriteSpriteStrip` |
| `sub_E3F4` | Implemented | `nesSubE3F4SortObjectsByY` |
| `sub_E4AD` | Implemented | `nesSubE4ADHideFourSprites` |
| `sub_E49B` | Implemented | `nesSubE49BHideSelectedSpriteGroups` |
| `sub_E2AF` | Implemented | `nesSubE2AFDecrementCountdown` |
| `loc_E2BD` | Implemented | `nesLocE2BDUpdateFinishBlink` |
| `loc_E2D0` | Implemented | `nesLocE2D0CountdownReachedOne` |
| `sub_E2F6` | State model | `nesSubE2F6ShouldDrawWheel` |
| `sub_E1F9` | State model | `nesSubE1F9SelectObjectSprite` |
| `loc_E3AA` | State model | `nesLocE3AAUpdateHeatFlags` |


## Additional translated entries (2026-10-21)

| ASM entry | C translation | Coverage/status |
|---|---|---|
| `sub_CFCB` | `nesSubCFCBCompareObjectPosition` | Lexicographic compare and both flag-mask branches |
| `sub_CFDB` | `nesSubCFDBClearUpperFlags` | Zero early return and low-nibble mask |
| `sub_CFEA` | `nesSubCFEAUpdateComparisonFlags` | Zero, equal, less-than, greater-than |
| `sub_CE58` | `nesSubCE58ObjectIndexMatches` | Equality/Z result |
| `loc_CAFA` | `nesLocCAFAFillCommand` | Seven writes and 8-bit X wrap |
| `loc_CD13` | `nesLocCD13CopySpriteTemplate` | Eight OAM bytes |
| `loc_D0A6` | `nesLocD0A6StoreAndReturn` | Indexed store and A return |
| `loc_D197` | `nesLocD197WriteScrollRegister` | PPU scroll write effect |
| `loc_EC47` | `nesLocEC47WriteFixedSprite` | Four fixed sprite bytes |
| `loc_CA85` | `nesLocCA85ClearRam` | RAM range clear and preserved $42 |
| `sub_C339` | `nesSubC339WaitForVblank` | Finite trace of original bit-7 polling |
| `sub_D19B` | `nesSubD19BWaitForStatus` | Finite trace of original bit-6 polling |
| `sub_D347` | `nesSubD347ReadControllers` | Strobe and two controller decodes |
| `loc_C4FC` | `nesLocC4FCComputeScaledValue` | Both arithmetic branches |
| `sub_C92C` | `nesSubC92CSelectTableEntry` | Full table-index path |
| `sub_C935` | `nesSubC935BuildCommand` | DF7F/DFA2 conversion and copy |
| `sub_ECC9` | `nesSubECC9CheckSpriteBytes` | Four required byte comparisons |
| `bra_C411` | `nesLocC411WriteSpriteX` | Sprite X, attribute, flag |
| `loc_C3EF` | `nesLocC3EFWriteSpriteY` | Sprite Y store |
| `sub_C409` | `nesSubC409SetupStaticSprite` | Tile, indexed X, attribute, flag |

Native test source includes a dedicated regression group; execution and 3DS compilation are still pending.


### Menu dependency batch — 2026-10-21

| ASM entry | C translation / integration | Coverage |
|---|---|---|
| `ofs_000_C346_00` | `menusUpdate` state 0 | Bound-3 input, state 1/8 branches, timer-zero state 2 path |
| `ofs_000_C395_01` | `menusUpdate` state 1 | Input remap, bound-5 selector, cursor X, state advance |
| `sub_C3AF` | `nesSubC3AFIncrementState` | 8-bit increment/wrap |
| `sub_C3B1` | `nesSubC3B1ResetFrameFlags` | Clear only `$0040/$0044` |
| `ofs_000_C3BD_08` | `nesSubC3BDConfirmSelection` + `menusUpdate` | Selection 0–5 branches and clear path |
| `tbl_C080_spr_Y` | `menusUpdate` state 0 cursor table | Source bytes copied from ASM |
| `tbl_C083_spr_X` | `menusUpdate` state 1 cursor table | Source bytes copied from ASM |
| `tbl_C08B_spr_Y` | `menusUpdate` state 8 cursor table | Source bytes copied from ASM |
| `sub_D310` | Existing `nesSubD310DecreaseAllTimers` | Integrated for the tracked menu timer bytes |


| `sub_D310` menu projection | `nesSubD310DecreaseMenuTimers` | Reuses tested 10-frame divider for menu timer bytes `$30/$31`; menu-specific regression vectors added, execution pending |
