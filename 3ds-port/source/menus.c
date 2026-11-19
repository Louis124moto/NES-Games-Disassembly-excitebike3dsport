#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "nes_active_stream.h"
#include "menus.h"
#include "nes_routines.h"
#include "nes_menu_cpu.h"

/*
 * Source-derived subset of sub_C2A9's command sequencer. The command-list
 * prelude (sub_C318) runs once after reset/state changes; command index 0 is
 * then selected from the original tbl_C01C_index/tbl_C02A/tbl_C033 sequence.
 * Menu setup lists are translated command-by-command where all transitive effects are known. Unsupported commands remain blocked rather than replaced with guessed behavior.
 */
/* tbl_C01C_index: ram_0041 -> index of the setup command list (off_C06x_xx tables). */
static uint8_t menusSetupListIndex(uint8_t asmState)
{
    static const uint8_t tblC01C[14] = {0u, 1u, 2u, 3u, 4u, 2u, 3u, 4u, 6u, 5u, 2u, 3u, 7u, 8u};
    return asmState < 14u ? tblC01C[asmState] : 0xFFu;
}

/* GameState is a menu-facing projection; the 2 KiB RAM image remains canonical
 * for source-recompiled routines. Copy only explicitly modelled regions. */
static void menusCopyStateToCpuRam(GameState *state)
{
    uint8_t *ram = state->menu_cpu_ram;
    ram[0x0020u] = state->menu_timer20;
    ram[0x0030u] = state->menu_timer30;
    ram[0x0031u] = state->menu_timer31;
    ram[0x0033u] = state->menu_timer33;
    ram[0x0034u] = state->menu_timer34;
    ram[0x0040u] = state->menu_ram0040;
    ram[0x0041u] = state->menu_asm_state;
    ram[0x0042u] = state->menu_selection;
    ram[0x0043u] = state->menu_ram0043;
    ram[0x0044u] = state->menu_ram0044;
    ram[0x0045u] = state->menu_ram0045;
    ram[0x0046u] = state->menu_ram0046;
    ram[0x0048u] = state->menu_ram0048;
    ram[0x0049u] = state->menu_repeat_guard;
    ram[0x004Au] = state->menu_ram004A;
    ram[0x004Bu] = state->menu_ram004B;
    ram[0x00FBu] = state->menu_ram00FB;
    ram[0x00FFu] = state->menu_ram00FF;
    ram[0x03F0u] = state->menu_ram03F0;
    ram[0x03F2u] = state->menu_ram03F2;
    ram[0x03F3u] = state->menu_ram03F3;
    ram[0x03F7u] = state->menu_ram03F7;
    ram[0x03F8u] = state->menu_ram03F8;
    memcpy(&ram[0x004Du], state->menu_ram004D_EF, sizeof(state->menu_ram004D_EF));
    memcpy(&ram[0x0060u], state->menu_ram0060, sizeof(state->menu_ram0060));
    memcpy(&ram[0x00A0u], state->menu_ramA0, sizeof(state->menu_ramA0));
    memcpy(&ram[0x0200u], state->menu_oam, sizeof(state->menu_oam));
    memcpy(&ram[0x0300u], state->menu_ram0300, sizeof(state->menu_ram0300));
    memcpy(&ram[0x05A0u], state->menu_ram05A0, sizeof(state->menu_ram05A0));
    memcpy(&ram[0x05C3u], state->menu_ram05C3, sizeof(state->menu_ram05C3));
    memcpy(&ram[0x05E0u], state->menu_ram05E0, sizeof(state->menu_ram05E0));
    memcpy(&ram[0x06E0u], state->menu_ram06E0, sizeof(state->menu_ram06E0));
}

static void menusCopyCpuRamToState(GameState *state)
{
    const uint8_t *ram = state->menu_cpu_ram;
    state->menu_timer20 = ram[0x0020u];
    state->menu_timer30 = ram[0x0030u];
    state->menu_timer31 = ram[0x0031u];
    state->menu_timer33 = ram[0x0033u];
    state->menu_timer34 = ram[0x0034u];
    state->menu_ram0040 = ram[0x0040u];
    state->menu_asm_state = ram[0x0041u];
    state->menu_selection = ram[0x0042u];
    state->menu_ram0043 = ram[0x0043u];
    state->menu_ram0044 = ram[0x0044u];
    state->menu_ram0045 = ram[0x0045u];
    state->menu_ram0046 = ram[0x0046u];
    state->menu_ram0048 = ram[0x0048u];
    state->menu_repeat_guard = ram[0x0049u];
    state->menu_ram004A = ram[0x004Au];
    state->menu_ram004B = ram[0x004Bu];
    state->menu_ram00FB = ram[0x00FBu];
    state->menu_ram00FF = ram[0x00FFu];
    state->menu_ram03F0 = ram[0x03F0u];
    state->menu_ram03F2 = ram[0x03F2u];
    state->menu_ram03F3 = ram[0x03F3u];
    state->menu_ram03F7 = ram[0x03F7u];
    state->menu_ram03F8 = ram[0x03F8u];
    memcpy(state->menu_ram004D_EF, &ram[0x004Du], sizeof(state->menu_ram004D_EF));
    memcpy(state->menu_ram0060, &ram[0x0060u], sizeof(state->menu_ram0060));
    memcpy(state->menu_ramA0, &ram[0x00A0u], sizeof(state->menu_ramA0));
    memcpy(state->menu_oam, &ram[0x0200u], sizeof(state->menu_oam));
    memcpy(state->menu_ram0300, &ram[0x0300u], sizeof(state->menu_ram0300));
    memcpy(state->menu_ram05A0, &ram[0x05A0u], sizeof(state->menu_ram05A0));
    memcpy(state->menu_ram05C3, &ram[0x05C3u], sizeof(state->menu_ram05C3));
    memcpy(state->menu_ram05E0, &ram[0x05E0u], sizeof(state->menu_ram05E0));
    memcpy(state->menu_ram06E0, &ram[0x06E0u], sizeof(state->menu_ram06E0));
}

static int menusRunSetupSequence(GameState *state)
{
    uint8_t commandIndex;
    uint8_t listIndex;

    /* bra_C2ED: sub_D24F_disable_rendering ($2001 = 0) runs first, every frame of the setup path. */
    state->menu_ppu_mask = 0u;

    /* ...then ram_0030_timer is checked before even the prelude runs. */
    if (state->menu_timer30 != 0u) return 1;

    if (state->menu_ram0044 == 0u) {
        /*
         * bra_C2ED: LDY ram_0044 / INC / DEY / BMI-style test: prelude iff the OLD ram_0044 is 0.
         * sub_C318 calls sub_D255 (fill all 256 OAM bytes with $F8),
         * sub_D23B/sub_D260 (clear two 1 KiB nametables to $FC with zero
         * attribute tables), then sub_D1A9 (reset both scroll registers).
         * $2001 is disabled by sub_D24F and $2000 becomes $10.
         */
        state->menu_oam[0] = 0xF8u;
        nesSubC318InitializePpu(state->menu_oam, state->menu_ppu_nametables,
                                &state->menu_scroll_x, &state->menu_scroll_y,
                                &state->menu_ppu_ctrl, &state->menu_ppu_mask);
        state->menu_ram0044 = 1u;
        return 1;
    }

    /*
     * sub_C2A9 bra_C2ED: the command list is NOT selected by ram_0041 directly but through
     * tbl_C01C_index (LDA tbl_C01C_index,X with X = ram_0041). Corrected 2026-11-17: this
     * code used ram_0041 itself, which ran the state-13 list for state 8 (design menu) and
     * the state-4 list for state 7, among others.
     */
    listIndex = menusSetupListIndex(state->menu_asm_state);

    /*
     * Never walk beyond a translated command table. In the 6502, this
     * index is consumed by a real pointer table; incrementing indefinitely
     * in the native stub would fabricate state and eventually wrap.
     */
    if (listIndex == 0u) {
        if (state->menu_ram0044 > 2u) return 1;
    } else if (listIndex == 1u) {
        if (state->menu_ram0044 > 6u) return 1;
    } else if (listIndex == 8u) {
        if (state->menu_ram0044 > 2u) return 1;
    } else if (listIndex == 3u) {
        if (state->menu_ram0044 > 5u) return 1;
    } else if (listIndex == 4u) {
        if (state->menu_ram0044 > 4u) return 1;
    } else if (listIndex == 6u) {
        if (state->menu_ram0044 > 3u) return 1;
    } else if (listIndex == 7u) {
        if (state->menu_ram0044 > 3u) return 1;
    } else if (listIndex == 5u) {
        /* Six original list-5 commands: $C551, $C869, $C820, $C5A3, $C562, $C5C2. */
        if (state->menu_ram0044 > 6u) return 1;
    } else {
        return 1; /* setup list is not translated; hold state for inspection */
    }

    commandIndex = (uint8_t)(state->menu_ram0044 - 1u);
    state->menu_ram0044 = (uint8_t)(state->menu_ram0044 + 1u);

    if (listIndex == 0u) {
        if (commandIndex == 0u) {
            /* ofs_001_C2BD_00_01 + sub_C409 */
            state->menu_ram03F8 = 0x04u;
            state->menu_ram0045 = 0x02u;
            state->menu_oam[1] = 0xFAu;
            state->menu_oam[3] = 0x50u; /* tbl_C088_spr_X[0] */
            state->menu_oam[2] = 0u;
            state->menu_ram0046 = 0u;
            if (state->menu_ram03F0 == 0u) state->menu_ram00FB = 1u;
            state->menu_ram03F0 = (state->menu_ram03F0 >= 2u)
                ? 0u : (uint8_t)(state->menu_ram03F0 + 1u);
            state->menu_timer31 = 0x38u;
            state->menu_oam[0] = 0x80u; /* tbl_C080_spr_Y[0] */
            return 1;
        }
        if (commandIndex == 1u) {
            /* ofs_001_C514_00_02 */
            state->menu_ram0045 = 0x18u;
            state->menu_repeat_guard = 1u; /* ram_0049 */
            state->menu_ram0040 = 1u;
            state->menu_ram0044 = 0u;
            return 1;
        }
    } else if (listIndex == 1u) {
        if (commandIndex == 0u) {
            /* ofs_001_C55E_01_01 */
            state->menu_ram0045 = 0x06u;
            return 1;
        }
        if (commandIndex == 1u) {
            /* ofs_001_C3F3_01_02 */
            state->menu_ram00FB = 0x04u;
            state->menu_ram0045 = 0x03u;
            state->menu_oam[1] = 0xFAu;
            state->menu_oam[3] = 0x48u; /* tbl_C088_spr_X[1] */
            state->menu_oam[2] = 0u;
            state->menu_ram0046 = 0u;
            state->menu_selection = 0u; /* returned A stored in ram_0042 */
            state->menu_timer34 = 0x2Du;
            state->menu_oam[0] = 0xB0u;
            return 1;
        }
        if (commandIndex == 2u) {
            /* ofs_001_C514_01_03 */
            state->menu_ram0045 = 0x18u;
            state->menu_repeat_guard = 1u;
            state->menu_ram0040 = 1u;
            state->menu_ram0044 = 0u;
            return 1;
        }
    } else if (listIndex == 3u) {
        static const uint16_t routines[5] = {
            0xC55Eu, /* ofs_001_C55E_03_01 */
            0xC56Au, /* ofs_001_C56A_03_02 */
            0xC455u, /* ofs_001_C455_03_03 */
            0xC5A7u, /* ofs_001_C5A7_03_04 */
            0xC485u  /* ofs_001_C485_03_05 */
        };
        menusCopyStateToCpuRam(state);
        if (!nesMenuCpuRunRamRoutine(routines[commandIndex], state->menu_cpu_ram)) {
            /* Without the original PRG, keep this command pending rather than guess. */
            state->menu_ram0044 = (uint8_t)(state->menu_ram0044 - 1u);
            return 1;
        }
        menusCopyCpuRamToState(state);
        return 1;
    } else if (listIndex == 4u) {
        if (commandIndex == 0u) { /* ofs_001_C55E_04_01 */
            state->menu_ram0045 = 0x06u;
            return 1;
        }
        if (commandIndex == 1u) { /* ofs_001_C571_04_02 */
            state->menu_ram0045 = 0x09u;
            return 1;
        }
        if (commandIndex == 2u) { /* ofs_001_C5AC_04_03 */
            static const uint8_t command[6] = {0x04u, 0x21u, 0x6Fu, 0x01u, 0x00u, 0x00u};
            unsigned int i;
            state->menu_timer34 = 0x12u;
            for (i = 0u; i < 6u; ++i) state->menu_ram0300[i] = command[i];
            state->menu_ram0300[4] = (uint8_t)(state->menu_ram0043 + 1u);
            return 1;
        }
        if (commandIndex == 3u) { /* ofs_001_C514_04_04: $18, then falls into C518 */
            state->menu_ram0045 = 0x18u;
            state->menu_repeat_guard = 1u;
            state->menu_ram0040 = 1u;
            state->menu_ram0044 = 0u;
            return 1;
        }
    } else if (listIndex == 6u) {
        if (commandIndex == 0u) { /* ofs_001_C55E_06_01 */
            state->menu_ram0045 = 0x06u;
            return 1;
        }
        if (commandIndex == 1u) { /* ofs_001_C575_06_02 */
            state->menu_ram0043 = 0x05u;
            state->menu_oam[3] = 0x4Cu; /* tbl_C088_spr_X[2]; sub_C409 does not write sprite Y */
            state->menu_oam[1] = 0xFAu;
            state->menu_oam[2] = 0u;
            state->menu_ram0046 = 0u;
            state->menu_ram0045 = 0x0Fu;
            return 1;
        }
        if (commandIndex == 2u) { /* ofs_001_C514_06_03: $18, then falls into C518 */
            state->menu_ram0045 = 0x18u;
            state->menu_repeat_guard = 1u;
            state->menu_ram0040 = 1u;
            state->menu_ram0044 = 0u;
            return 1;
        }
    } else if (listIndex == 5u) {
        if (commandIndex == 0u) { /* ofs_001_C551_05_01 */
            state->menu_cpu_ram[0x0041u] = state->menu_asm_state;
            state->menu_cpu_ram[0x0043u] = state->menu_ram0043;
            state->menu_cpu_ram[0x0045u] = state->menu_ram0045;
            memcpy(&state->menu_cpu_ram[0x004Du], state->menu_ram004D_EF,
                   sizeof(state->menu_ram004D_EF));
            if (!nesMenuCpuSetupCommand(0xC551u, state->menu_asm_state,
                                        state->menu_ram0043, &state->menu_ram0045,
                                        state->menu_ram004D_EF)) {
                /* Exact source-C fallback for environments without the PRG image. */
                state->menu_ram0045 = (state->menu_asm_state < 8u)
                    ? (uint8_t)(0x12u + state->menu_ram0043) : 0x12u;
            }
            state->menu_cpu_ram[0x0045u] = state->menu_ram0045;
            memcpy(&state->menu_cpu_ram[0x004Du], state->menu_ram004D_EF,
                   sizeof(state->menu_ram004D_EF));
            return 1;
        }
        if (commandIndex == 1u) { /* ofs_001_C869_05_02 */
            if (!nesMenuCpuSetupCommand(0xC869u, state->menu_asm_state,
                                        state->menu_ram0043, &state->menu_ram0045,
                                        state->menu_ram004D_EF)) {
                memset(state->menu_ram004D_EF, 0, sizeof(state->menu_ram004D_EF));
            }
            state->menu_cpu_ram[0x0045u] = state->menu_ram0045;
            memcpy(&state->menu_cpu_ram[0x004Du], state->menu_ram004D_EF,
                   sizeof(state->menu_ram004D_EF));
            return 1;
        }
        if (commandIndex == 2u) { /* ofs_001_C820_05_03 */
            uint8_t paletteWritten = 0u;
            state->menu_cpu_ram[0x0041u] = state->menu_asm_state;
            state->menu_cpu_ram[0x0043u] = state->menu_ram0043;
            state->menu_cpu_ram[0x0045u] = state->menu_ram0045;
            if (!nesMenuCpuEditorInit(state->menu_cpu_ram,
                                      state->menu_ppu_nametables,
                                      state->menu_ppu_palette,
                                      &state->menu_ppu_ctrl,
                                      &state->menu_ppu_mask,
                                      &state->menu_scroll_x,
                                      &state->menu_scroll_y,
                                      &paletteWritten))
                { state->menu_ram0044 = (uint8_t)(state->menu_ram0044 - 1u); return 1; }
            state->menu_ram0045 = state->menu_cpu_ram[0x0045u];
            state->menu_ram004B = state->menu_cpu_ram[0x004Bu];
            memcpy(state->menu_oam, &state->menu_cpu_ram[0x0200u],
                   sizeof(state->menu_oam));
            if (paletteWritten != 0u) state->menu_ppu_palette_loaded = 1u;
            state->menu_ppu_revision = (uint8_t)(state->menu_ppu_revision + 1u);
            return 1;
        }
        if (commandIndex == 3u) { /* ofs_001_C5A3_05_04 */
            if (!nesMenuCpuRunRamRoutine(0xC5A3u, state->menu_cpu_ram))
                state->menu_cpu_ram[0x0045u] = 0x17u;
            state->menu_ram0045 = state->menu_cpu_ram[0x0045u];
            return 1;
        }
        if (commandIndex == 4u) { /* ofs_001_C562_05_05 */
            if (!nesMenuCpuRunRamRoutine(0xC562u, state->menu_cpu_ram))
                state->menu_cpu_ram[0x0045u] = 0x05u;
            state->menu_ram0045 = state->menu_cpu_ram[0x0045u];
            return 1;
        }
        if (commandIndex == 5u) { /* ofs_001_C5C2_05_06: enters editor loop */
            if (!nesMenuCpuRunRamRoutine(0xC5C2u, state->menu_cpu_ram)) {
                state->menu_ram0044 = (uint8_t)(state->menu_ram0044 - 1u);
                return 1;
            }
            state->menu_ram0040 = state->menu_cpu_ram[0x0040u];
            state->menu_ram0045 = state->menu_cpu_ram[0x0045u];
            state->menu_repeat_guard = state->menu_cpu_ram[0x0049u];
            memcpy(state->menu_oam, &state->menu_cpu_ram[0x0200u],
                   sizeof(state->menu_oam));
            memcpy(state->menu_ram05A0, &state->menu_cpu_ram[0x05A0u],
                   sizeof(state->menu_ram05A0));
            memcpy(state->menu_ram05C3, &state->menu_cpu_ram[0x05C3u],
                   sizeof(state->menu_ram05C3));
            memcpy(state->menu_ram05E0, &state->menu_cpu_ram[0x05E0u],
                   sizeof(state->menu_ram05E0));
            memcpy(state->menu_ram06E0, &state->menu_cpu_ram[0x06E0u],
                   sizeof(state->menu_ram06E0));
            return 1;
        }
    } else if (listIndex == 7u) {
        if (commandIndex == 0u) { /* ofs_001_C869_07_01: clear $004D-$00EF */
            memset(state->menu_ram004D_EF, 0, sizeof(state->menu_ram004D_EF));
            return 1;
        }
        if (commandIndex == 1u) { /* ofs_001_C586_07_02 (+ sub_C621, tbl_C180) */
            static const uint8_t tblC180[4] = {0x17u, 0x01u, 0x00u, 0x01u};
            int x;
            /*
             * X = 3..0: $0061+X = tbl_C180[X]; for X < 3 first $0065+X = $05A0+X and
             * $0068+X = $05C3+X (the copy runs before the table store of the same X).
             */
            for (x = 3; x >= 0; --x) {
                if (x < 3) {
                    state->menu_ram0060[5 + x] = state->menu_ram05A0[x];
                    state->menu_ram0060[8 + x] = state->menu_ram05C3[x];
                }
                state->menu_ram0060[1 + x] = tblC180[x];
            }
            memcpy(state->menu_ram05E0, state->menu_ram06E0, sizeof(state->menu_ram05E0)); /* sub_C621 */
            state->menu_ram0045 = 0x10u; /* con_0045_10 */
            return 1;
        }
        if (commandIndex == 2u) { /* ofs_001_C518_07_03 */
            state->menu_repeat_guard = 1u;
            state->menu_ram0040 = 1u;
            state->menu_ram0044 = 0u;
            return 1;
        }
    } else if (listIndex == 8u) {
        if (commandIndex == 0u) {
            /* ofs_001_C582_08_01 */
            state->menu_ram0045 = 0x11u;
            return 1;
        }
        if (commandIndex == 1u) {
            /* ofs_001_C518_08_02 */
            state->menu_repeat_guard = 1u;
            state->menu_ram0040 = 1u;
            state->menu_ram0044 = 0u;
            return 1;
        }
    }

    /* Other command lists and transitive routines are still pending; do not execute unknown commands. */
    state->menu_action_x = 0u;
    state->menu_selection_changed = 0u;
    return 1;
}

/*
 * Source-derived menu dispatcher trace.
 *
 * State 8 is ofs_000_C3BD_08 ($C3BD); state 0 is ofs_000_C346_00
 * ($C346). Selection meanings and original title artwork are deliberately
 * not guessed. The menu receives per-frame press edges to match ram_btn_press; state/reset side effects follow bank_FF.asm.
 */

/* Prefer the original, statically recompiled 6502 input routine. The native helper is a
 * deliberately retained fallback for raw-CHR-only installations without a supported ROM. */
static uint8_t menusRunInput(uint8_t buttons, uint8_t count, GameState *state, uint8_t *changed)
{
    uint8_t before = state->menu_selection;
    uint8_t action = 0u;
    if (nesMenuCpuInput(buttons, count, &state->menu_selection,
                        &state->menu_repeat_guard, &state->menu_timer30,
                        &state->menu_timer31, &state->menu_ram00FF, &action)) {
        *changed = (uint8_t)(before != state->menu_selection);
        return action;
    }
    return nesSubC7DEMenuInput(buttons, count, &state->menu_selection,
                               &state->menu_repeat_guard, &state->menu_timer30,
                               &state->menu_timer31, changed, &state->menu_ram00FF);
}

void menusUpdate(GameState *state, unsigned int heldKeys, unsigned int pressedKeys)
{
    uint8_t nesButtons = 0u;
    uint8_t nesHeldButtons = 0u;
    uint8_t changed = 0u;

    if (state == 0) return;

    /*
     * vec_C23B_NMI increments ram_003F, then executes the original D310 timer
     * routine. The PRG-backed path preserves every timer byte in CPU RAM;
     * retain the small native projection only when no supported PRG is loaded.
     */
    if (!state->paused) {
        if (nesMenuCpuAvailable()) {
            state->menu_cpu_ram[0x0020u] = state->menu_timer20;
            state->menu_cpu_ram[0x0030u] = state->menu_timer30;
            state->menu_cpu_ram[0x0031u] = state->menu_timer31;
            state->menu_cpu_ram[0x0033u] = state->menu_timer33;
            state->menu_cpu_ram[0x0034u] = state->menu_timer34;
            state->menu_cpu_ram[0x003Fu] =
                (uint8_t)(state->menu_cpu_ram[0x003Fu] + 1u);
            if (nesMenuCpuRunRamRoutine(0xD310u, state->menu_cpu_ram)) {
                state->menu_timer20 = state->menu_cpu_ram[0x0020u];
                state->menu_timer30 = state->menu_cpu_ram[0x0030u];
                state->menu_timer31 = state->menu_cpu_ram[0x0031u];
                state->menu_timer33 = state->menu_cpu_ram[0x0033u];
                state->menu_timer34 = state->menu_cpu_ram[0x0034u];
            } else {
                nesSubD310DecreaseMenuTimers(&state->menu_timer20,
                                            &state->menu_timer30,
                                            &state->menu_timer31,
                                            &state->menu_timer34);
            }
        } else {
            nesSubD310DecreaseMenuTimers(&state->menu_timer20,
                                        &state->menu_timer30,
                                        &state->menu_timer31,
                                        &state->menu_timer34);
        }
    }

    /*
     * sub_C2A9 dispatches menu handlers only while ram_0040 is nonzero.
     * With it clear, run the translated command-list setup path first.
     */
    if (state->menu_ram0040 == 0u) {
        (void)menusRunSetupSequence(state);
        return;
    }

    /*
     * sub_C7DE reads ram_btn_press ($0015), not ram_btn_hold ($0018).
     * Map all eight physical 3DS face/d-pad buttons to the NES serial bit order.
     * Use the per-frame press edges supplied by inputUpdate. This prevents
     * a held 3DS key from being treated as a fresh NES press every frame.
     */
    if (heldKeys & KEY_A)      nesHeldButtons |= 0x80u;
    if (heldKeys & KEY_B)      nesHeldButtons |= 0x40u;
    if (heldKeys & KEY_SELECT) nesHeldButtons |= 0x20u;
    if (heldKeys & KEY_START)  nesHeldButtons |= 0x10u;
    if (heldKeys & KEY_UP)     nesHeldButtons |= 0x08u;
    if (heldKeys & KEY_DOWN)   nesHeldButtons |= 0x04u;
    if (heldKeys & KEY_LEFT)   nesHeldButtons |= 0x02u;
    if (heldKeys & KEY_RIGHT)  nesHeldButtons |= 0x01u;

    if (pressedKeys & KEY_A)      nesButtons |= 0x80u;
    if (pressedKeys & KEY_B)      nesButtons |= 0x40u;
    if (pressedKeys & KEY_SELECT) nesButtons |= 0x20u;
    if (pressedKeys & KEY_START)  nesButtons |= 0x10u;
    if (pressedKeys & KEY_UP)     nesButtons |= 0x08u;
    if (pressedKeys & KEY_DOWN)   nesButtons |= 0x04u;
    if (pressedKeys & KEY_LEFT)   nesButtons |= 0x02u;
    if (pressedKeys & KEY_RIGHT)  nesButtons |= 0x01u;

    state->menu_clear_ram_requested = 0u;

    /*
     * ofs_000_C346_00: when ram_0031_timer is zero, the ASM initializes
     * ram_0043/ram_004A, sets ram_03F7=$0F and enters state 2.
     */
    if (state->menu_asm_state == 0u) {
        if (state->menu_timer31 == 0u) {
            state->menu_ram0043 = 0u;
            state->menu_ram004A = 0u;
            state->menu_ram03F7 = 0x0Fu;
            nesSubC3B1ResetFrameFlags(&state->menu_ram0040, &state->menu_ram0044);
            state->menu_asm_state = 2u;
            state->menu_action_x = 0u;
            state->menu_selection_changed = 0u;
            return;
        }

        /* ofs_000_C346_00: the actual three-entry title menu. */
        state->menu_action_x = menusRunInput(nesButtons, 3u, state, &changed);
        state->menu_selection_changed = changed;
        if (state->menu_action_x == 0u) {
            static const uint8_t cursorY[3] = {0x80u, 0x90u, 0xA0u};
            if (state->menu_selection < 3u)
                state->menu_oam[0] = cursorY[state->menu_selection];
            state->menu_ram03F2 = 0u;
        } else {
            uint8_t selected = state->menu_selection;
            state->menu_ram03F7 = 0u;
            state->menu_ram0043 = 0u;
            state->menu_selection = 0u;
            state->menu_ram03F3 = 1u;
            state->menu_ram004A = 1u;
            state->menu_ram03F0 = 1u;
            state->menu_asm_state = (selected == 2u) ? 8u : 1u;
            state->menu_ram03F2 = selected;
            nesSubC3B1ResetFrameFlags(&state->menu_ram0040, &state->menu_ram0044);
        }
    } else if (state->menu_asm_state == 1u) {
        /* ofs_000_C395_01 remaps LEFT/RIGHT onto the vertical selector. */
        uint8_t selectorButtons = (uint8_t)((nesButtons & 0xF3u) |
                                           ((nesButtons & 0x03u) << 2));
        state->menu_action_x = menusRunInput(selectorButtons, 5u, state, &changed);
        state->menu_selection_changed = changed;
        if (state->menu_action_x == 0u) {
            static const uint8_t cursorX[5] = {0x48u, 0x60u, 0x78u, 0x90u, 0xA8u};
            if (state->menu_selection < 5u)
                state->menu_oam[3] = cursorX[state->menu_selection];
            state->menu_oam[2] = 0u; /* bra_C411 writes zero */
            state->menu_ram0046 = 0u;
        } else {
            state->menu_ram0043 = state->menu_selection;
            nesSubC3AFIncrementState(&state->menu_asm_state);
            nesSubC3B1ResetFrameFlags(&state->menu_ram0040, &state->menu_ram0044);
        }
    } else if (state->menu_asm_state == 8u) {
        state->menu_action_x = menusRunInput(nesButtons, 6u, state, &changed);
        state->menu_selection_changed = changed;
        if (state->menu_action_x == 0u) {
            /* tbl_C08B_spr_Y indexed by ram_0042, exactly as bra_C3EA. */
            static const uint8_t cursorY[6] = {
                0x30u, 0x48u, 0x78u, 0x90u, 0xA8u, 0xC0u
            };
            if (state->menu_selection < 6u)
                state->menu_oam[0] = cursorY[state->menu_selection];
        }
        if (state->menu_action_x != 0u) {
            nesSubC3BDConfirmSelection(
                state->menu_action_x, &state->menu_asm_state,
                &state->menu_selection, &state->menu_ram03F2,
                &state->menu_clear_ram_requested,
                &state->menu_ram0040, &state->menu_ram0044);
        }

        /*
         * bra_C3E4 -> loc_CA85_clear_0021_00FF:
         * RAM $0021-$00FF is cleared, ram_0049 is then incremented,
         * and ram_0042 is restored from the stack. The modeled fields below
         * are the menu-visible bytes in that range.
         */
        if (state->menu_clear_ram_requested != 0u) {
            state->menu_asm_state = 0u;       /* ram_0041 was cleared */
            state->menu_repeat_guard = 1u;   /* INC ram_0049 after clear */
            state->menu_timer30 = 0u;
            state->menu_timer31 = 0u;
            state->menu_ram0040 = 0u;
            state->menu_ram0043 = 0u;
            state->menu_ram0044 = 0u;
            state->menu_ram004A = 0u;
            /*
             * loc_CA85 clears every byte of $0021-$00FF, so the other modelled bytes in that
             * range go to zero too: ram_0045 ($45), the $0033/$0034 timers, ram_0046,
             * ram_0048, ram_004B, $004D-$00EF, ram_00FB. ram_0042 is saved and restored
             * (it stays 2), and ram_03F2/ram_03F0 live outside the cleared page.
             */
            state->menu_ram0045 = 0u;
            state->menu_timer33 = 0u;
            state->menu_timer34 = 0u;
            state->menu_ram0046 = 0u;
            state->menu_ram0048 = 0u;
            state->menu_ram004B = 0u;
            state->menu_ram00FB = 0u;
            state->menu_ram00FF = 0u;
            memset(state->menu_ram004D_EF, 0, sizeof(state->menu_ram004D_EF));
            /*
             * loc_CA85 clears CPU RAM $0021-$00FF inclusive, then increments
             * $0049 and restores the saved $0042 selection. Preserve $0020
             * and all bytes outside the source clear range.
             */
            memset(&state->menu_cpu_ram[0x0021u], 0, 0x00DFu);
            state->menu_cpu_ram[0x0042u] = state->menu_selection;
            state->menu_cpu_ram[0x0049u] = state->menu_repeat_guard;
            state->menu_action_x = 0u;
            state->menu_selection_changed = 0u;
        }
    } else if (state->menu_asm_state == 9u) {
        /*
         * ofs_000_C97C_09: original editor frame. Keep the full 2 KiB CPU RAM
         * persistent across calls; C5C2 initialized the object/course state.
         * PPUSTATUS bit 6 is acknowledged at frame granularity by the adapter.
         */
        uint8_t held = 0u;
        if (heldKeys & KEY_A) held |= 0x80u;
        if (heldKeys & KEY_B) held |= 0x40u;
        if (heldKeys & KEY_SELECT) held |= 0x20u;
        if (heldKeys & KEY_START) held |= 0x10u;
        if (heldKeys & KEY_UP) held |= 0x08u;
        if (heldKeys & KEY_DOWN) held |= 0x04u;
        if (heldKeys & KEY_LEFT) held |= 0x02u;
        if (heldKeys & KEY_RIGHT) held |= 0x01u;
        state->menu_cpu_ram[0x0014u] = held;
        state->menu_cpu_ram[0x0015u] = nesButtons;
        state->menu_cpu_ram[0x0041u] = state->menu_asm_state;
        state->menu_cpu_ram[0x0042u] = state->menu_selection;
        state->menu_cpu_ram[0x0040u] = state->menu_ram0040;
        state->menu_cpu_ram[0x0043u] = state->menu_ram0043;
        state->menu_cpu_ram[0x0044u] = state->menu_ram0044;
        state->menu_cpu_ram[0x0045u] = state->menu_ram0045;
        state->menu_cpu_ram[0x0046u] = state->menu_ram0046;
        state->menu_cpu_ram[0x0048u] = state->menu_ram0048;
        state->menu_cpu_ram[0x0049u] = state->menu_repeat_guard;
        state->menu_cpu_ram[0x004Bu] = state->menu_ram004B;
        state->menu_cpu_ram[0x00FBu] = state->menu_ram00FB;
        memcpy(&state->menu_cpu_ram[0x0200u], state->menu_oam, sizeof(state->menu_oam));
        memcpy(&state->menu_cpu_ram[0x0300u], state->menu_ram0300, sizeof(state->menu_ram0300));
        memcpy(&state->menu_cpu_ram[0x05A0u], state->menu_ram05A0, sizeof(state->menu_ram05A0));
        memcpy(&state->menu_cpu_ram[0x05C3u], state->menu_ram05C3, sizeof(state->menu_ram05C3));
        memcpy(&state->menu_cpu_ram[0x05E0u], state->menu_ram05E0, sizeof(state->menu_ram05E0));
        memcpy(&state->menu_cpu_ram[0x06E0u], state->menu_ram06E0, sizeof(state->menu_ram06E0));
        if (nesMenuCpuEditorFrame(state->menu_cpu_ram, held, nesButtons,
                                  state->menu_ppu_nametables, state->menu_ppu_palette,
                                  &state->menu_ppu_ctrl, &state->menu_ppu_mask,
                                  &state->menu_scroll_x, &state->menu_scroll_y)) {
            state->menu_asm_state = state->menu_cpu_ram[0x0041u];
            state->menu_selection = state->menu_cpu_ram[0x0042u];
            state->menu_ram0040 = state->menu_cpu_ram[0x0040u];
            state->menu_ram0043 = state->menu_cpu_ram[0x0043u];
            state->menu_ram0044 = state->menu_cpu_ram[0x0044u];
            state->menu_ram0045 = state->menu_cpu_ram[0x0045u];
            state->menu_ram0046 = state->menu_cpu_ram[0x0046u];
            state->menu_ram0048 = state->menu_cpu_ram[0x0048u];
            state->menu_repeat_guard = state->menu_cpu_ram[0x0049u];
            state->menu_ram004B = state->menu_cpu_ram[0x004Bu];
            state->menu_ram00FB = state->menu_cpu_ram[0x00FBu];
            memcpy(state->menu_oam, &state->menu_cpu_ram[0x0200u], sizeof(state->menu_oam));
            memcpy(state->menu_ram0300, &state->menu_cpu_ram[0x0300u], sizeof(state->menu_ram0300));
            memcpy(state->menu_ram05A0, &state->menu_cpu_ram[0x05A0u], sizeof(state->menu_ram05A0));
            memcpy(state->menu_ram05C3, &state->menu_cpu_ram[0x05C3u], sizeof(state->menu_ram05C3));
            memcpy(state->menu_ram05E0, &state->menu_cpu_ram[0x05E0u], sizeof(state->menu_ram05E0));
            memcpy(state->menu_ram06E0, &state->menu_cpu_ram[0x06E0u], sizeof(state->menu_ram06E0));
            state->menu_ppu_revision = (uint8_t)(state->menu_ppu_revision + 1u);
        }
        state->menu_action_x = 0u;
        state->menu_selection_changed = 0u;
    } else if (state->menu_asm_state == 3u || state->menu_asm_state == 6u ||
               state->menu_asm_state == 11u) {
        /*
         * The original state-3/6/0B handlers share the CB2C pointer-table
         * dispatcher. Execute the selected ROM routine against persistent
         * CPU RAM so its timer, PPU-buffer, OAM, and state side effects stay
         * source-derived rather than approximated in a second C state machine.
         */
        menusCopyStateToCpuRam(state);
        state->menu_cpu_ram[0x0014u] = nesHeldButtons;
        state->menu_cpu_ram[0x0015u] = nesButtons;
        if (nesMenuCpuRunRamRoutine(0xCB2Cu, state->menu_cpu_ram))
            menusCopyCpuRamToState(state);
        state->menu_action_x = 0u;
        state->menu_selection_changed = 0u;
    } else if (state->menu_asm_state == 4u || state->menu_asm_state == 7u) {
        /*
         * ofs_000_C465_04 / ofs_000_C465_07. This handler is not a race-frame
         * loop: it advances the result/transition menu timer and builds the
         * next dynamic PPU command through sub_C92C -> sub_C935 -> sub_CAFA.
         * Run the original instructions so the table arithmetic and buffer
         * bytes remain ROM-derived. The persistent CPU RAM carries the
         * transitive scratch/table state between frames.
         */
        state->menu_cpu_ram[0x0034u] = state->menu_timer34;
        state->menu_cpu_ram[0x0040u] = state->menu_ram0040;
        state->menu_cpu_ram[0x0041u] = state->menu_asm_state;
        state->menu_cpu_ram[0x0043u] = state->menu_ram0043;
        state->menu_cpu_ram[0x0044u] = state->menu_ram0044;
        state->menu_cpu_ram[0x0045u] = state->menu_ram0045;
        state->menu_cpu_ram[0x0046u] = state->menu_ram0046;
        state->menu_cpu_ram[0x03F2u] = state->menu_ram03F2;
        state->menu_cpu_ram[0x03F3u] = state->menu_ram03F3;
        state->menu_cpu_ram[0x03F8u] = state->menu_ram03F8;
        if (nesMenuCpuRunRamRoutine(0xC465u, state->menu_cpu_ram)) {
            state->menu_timer34 = state->menu_cpu_ram[0x0034u];
            state->menu_ram0040 = state->menu_cpu_ram[0x0040u];
            state->menu_asm_state = state->menu_cpu_ram[0x0041u];
            state->menu_ram0043 = state->menu_cpu_ram[0x0043u];
            state->menu_ram0044 = state->menu_cpu_ram[0x0044u];
            state->menu_ram0045 = state->menu_cpu_ram[0x0045u];
            state->menu_ram0046 = state->menu_cpu_ram[0x0046u];
            state->menu_ram03F2 = state->menu_cpu_ram[0x03F2u];
            state->menu_ram03F3 = state->menu_cpu_ram[0x03F3u];
            state->menu_ram03F8 = state->menu_cpu_ram[0x03F8u];
            memcpy(state->menu_ram0300, &state->menu_cpu_ram[0x0300u],
                   sizeof(state->menu_ram0300));
            state->menu_ram00FB = state->menu_cpu_ram[0x00FBu];
        }
        state->menu_action_x = 0u;
        state->menu_selection_changed = 0u;
    } else if (state->menu_asm_state == 12u || state->menu_asm_state == 13u) {
        /*
         * ofs_000_C43D_0C (tape SAVE, ram_0048 = 1) and ofs_000_C41C_0D (tape LOAD, ram_0048 = 2).
         * With ram_0044 == 0 the handler only posts the request (bra_C44E / bra_C450). The main
         * thread (menusMainThreadStep) then runs the data-recorder routine and stores $FF in
         * ram_0048 when it is done; only then does the handler finish.
         */
        uint8_t done;
        if (state->menu_asm_state == 13u) {
            done = nesSubC41CState13Entry(&state->menu_ram0044, &state->menu_ram0048);
        } else if (state->menu_ram0044 == 0u) {
            state->menu_ram0048 = 1u;
            state->menu_ram0044 = 1u;
            done = 0u;
        } else {
            done = (uint8_t)(state->menu_ram0048 == 0xFFu);
        }
        if (done != 0u) {
            if (state->menu_asm_state == 13u) {
                /* $C428-$C437: $0065+X -> $05A0+X and $0068+X -> $05C3+X (X = 2..0), then sub_EC3B. */
                int x;
                for (x = 2; x >= 0; --x) {
                    state->menu_ram05A0[x] = state->menu_ram0060[5 + x];
                    state->menu_ram05C3[x] = state->menu_ram0060[8 + x];
                }
                nesSubEC3BCopy256(state->menu_ram05E0, state->menu_ram06E0);
            }
            /* loc_C447 */
            state->menu_ram00FB = 0x40u;
            nesSubC3B1ResetFrameFlags(&state->menu_ram0040, &state->menu_ram0044); /* sub_C3AF's reset */
            nesLocCC6BUpdateMenuState(&state->menu_asm_state, state->menu_ram004B);
        }
        state->menu_action_x = 0u;
        state->menu_selection_changed = 0u;
    } else {
        /* Other dispatch states require their own verified ASM blocks. */
        state->menu_action_x = 0u;
        state->menu_selection_changed = 0u;
    }

}

void menusRender(const GameState *state)
{
    unsigned int i;
    if (state == 0) return;

    printf("\nEXCITEBIKE - ASM MENU TRACE\n");
    printf("---------------------------\n");
    printf("ram_0041 state: %u\n", (unsigned int)state->menu_asm_state);
    printf("ram_0042 selection: %u\n", (unsigned int)state->menu_selection);
    printf("ram_03F2 mode: %u | clear path: %u\n",
           (unsigned int)state->menu_ram03F2,
           (unsigned int)state->menu_clear_ram_requested);

    printf("cursor X/Y: %02X/%02X\n", (unsigned int)state->menu_oam[3], (unsigned int)state->menu_oam[0]);
    if (state->menu_ram0040 == 0u) {
        printf("ASM setup sequencer: active (ram_0044=%u)\n",
               (unsigned int)state->menu_ram0044);
        {
            const uint8_t list = menusSetupListIndex(state->menu_asm_state);
            if (list == 2u || list == 5u || list == 0xFFu) {
                printf("Setup command list %u (state %u) belongs to the race flow: not ported.\n",
                       (unsigned int)list, (unsigned int)state->menu_asm_state);
            }
        }
        if (state->show_help)
            printf("Only source-verified command effects are applied; incomplete transitive routines remain blocked.\n");
        return;
    }
    if (state->menu_asm_state == 0u) {
        for (i = 0u; i < 3u; ++i) {
            printf("%c Title option %u [ASM label unresolved]\n",
                   i == (unsigned int)state->menu_selection ? '>' : ' ', i);
        }
        printf("UP/DOWN or SELECT: move; START: confirm\n");
    } else if (state->menu_asm_state == 1u) {
        for (i = 0u; i < 5u; ++i) {
            printf("%c Selector option %u [ASM label unresolved]\n",
                   i == (unsigned int)state->menu_selection ? '>' : ' ', i);
        }
        printf("LEFT/RIGHT: move; START: confirm\n");
    } else if (state->menu_asm_state == 8u) {
        for (i = 0u; i <= 5u; ++i) {
            printf("%c Option %u [ASM label unresolved]\n",
                   i == (unsigned int)state->menu_selection ? '>' : ' ', i);
        }
        printf("UP/DOWN or SELECT: move; START: source confirm branch\n");
    } else if (state->menu_asm_state == 2u) {
        printf("State 2: original ASM dispatches to the gameplay loop.\n");
        printf("Native gameplay is still a separate incomplete subsystem.\n");
    } else if (state->menu_asm_state == 13u) {
        printf("State 13 entry: ram_0048=%02X ram_0044=%02X\n",
               (unsigned int)state->menu_ram0048, (unsigned int)state->menu_ram0044);
        printf("State-13 entry wait and $FF completion branch are translated.\n");
    } else {
        printf("ASM destination state %u reached; its native path is not ported.\n",
               (unsigned int)state->menu_asm_state);
    }

    if (state->show_help) {
        printf("Title graphics, palette/nametable mapping, and option names are not yet reconstructed.\n");
        printf("This is an input/state trace, not the original NES title screen.\n");
    }
}

static void menusRefreshDrawable(GameState *state)
{
    /* Background is drawable only when $2001 enables it AND real palette data exists. */
    state->menu_ppu_background_ready =
        (uint8_t)(((state->menu_ppu_mask & 0x08u) != 0u && state->menu_ppu_palette_loaded) ? 1u : 0u);
}

void menusNmiUpload(GameState *state)
{
    NesActiveStreamInfo info;
    int wroteMask = 0;
    int isPalette = 0;
    NesNmiUploadResult result;

    if (state == 0) return;
    /* vec_C23B_NMI: $2001 is only (re)written when ram_0045 is zero. */
    state->menu_ram_for_2001 = nesExcitebikeNmiRamFor2001(state->menu_ram0045,
                                                          state->menu_ram_for_2001, &wroteMask);
    if (wroteMask) state->menu_ppu_mask = state->menu_ram_for_2001;

    /* sub_C27F: ram_0045 selects the list; index 0 is the dynamic buffer at $0301. */
    if (state->menu_ram0045 != 0u && nesExcitebikeActiveStream(state->menu_ram0045, &info)) {
        isPalette = info.isPalette;
    }
    if (state->menu_ram0045 == 0u) {
        result = nesExcitebikeNmiUploadDynamic(state->menu_ram0300, state->menu_ppu_nametables,
                                               state->menu_ppu_palette);
    } else if (state->menu_ram0045 == 0x1Au) {
        result = nesExcitebikeNmiUploadZeroPage(&state->menu_ram0045, state->menu_ramA0,
                                                state->menu_ppu_nametables, state->menu_ppu_palette);
    } else {
        result = nesExcitebikeNmiUploadStatic(&state->menu_ram0045, state->menu_ppu_nametables,
                                              state->menu_ppu_palette);
    }
    if (result == NES_NMI_UPLOAD_APPLIED) {
        if (isPalette) state->menu_ppu_palette_loaded = 1u;
        state->menu_ppu_revision = (uint8_t)(state->menu_ppu_revision + 1u);
    }
    /* vec_C23B_NMI: after OAM DMA and sub_C27F, ram_0047 enables D14E. */
    if (state->menu_cpu_ram[0x0047u] != 0u &&
        nesMenuCpuRasterSplit(state->menu_cpu_ram, state->menu_ppu_nametables,
                              state->menu_ppu_palette, &state->menu_ppu_ctrl,
                              &state->menu_ppu_mask, &state->menu_scroll_x,
                              &state->menu_scroll_y)) {
        memcpy(state->menu_ram0300, &state->menu_cpu_ram[0x0300u],
               sizeof(state->menu_ram0300));
        memcpy(state->menu_oam, &state->menu_cpu_ram[0x0200u], sizeof(state->menu_oam));
    }
    menusRefreshDrawable(state);
}

static MenusTapeIo s_tapeIo;
static int s_tapeIoSet;

void menusSetTapeIo(const MenusTapeIo *io)
{
    if (io != 0) { s_tapeIo = *io; s_tapeIoSet = 1; }
    else { memset(&s_tapeIo, 0, sizeof(s_tapeIo)); s_tapeIoSet = 0; }
}

enum {
    TAPE_IDLE = 0,       /* main loop bra_C1D8: waiting for ram_0048 to become positive */
    TAPE_READ_BLOCK0,    /* sub_C739 after C32F/C716: sub_C792 reads block 0 */
    TAPE_WAIT_ERROR_19,  /* con_0045_19 posted, NMI back on, waiting for the acknowledgement */
    TAPE_WAIT_HEADER,    /* con_0045_1A posted (header bytes shown), waiting for the acknowledgement */
    TAPE_WAIT_HEADER_BAD,/* same list, but the header mismatched: restart afterwards */
    TAPE_READ_BLOCK1     /* sub_C726 + sub_C792: block 1 */
};

int menusTapeWaiting(const GameState *state)
{
    return state != 0 && state->menu_nmi_enabled == 0u &&
           (state->menu_tape_phase == TAPE_READ_BLOCK0 || state->menu_tape_phase == TAPE_READ_BLOCK1);
}

/* Error path bra_C744_loop: con_0045_19, sub_C339 (NMI on), wait for the NMI to consume it. */
static void tapePostError19(GameState *state)
{
    state->menu_ram0045 = 0x19u;
    state->menu_nmi_enabled = 1u;
    state->menu_tape_phase = TAPE_WAIT_ERROR_19;
}

/*
 * Main-thread half of the tape handoff, run once between two NMIs. Mirrors the loop at
 * bra_C1D8 ($C1D8-$C1F4) and sub_C6C8 / sub_C739 ($C6C8-$C7DC) with the recorder replaced by
 * the MenusTapeIo back end.
 */
static void menusMainThreadStep(GameState *state)
{
    static const uint8_t tblC180[4] = {0x17u, 0x01u, 0x00u, 0x01u};
    uint8_t buffer[64];
    int r;
    unsigned int y, mismatches;

    switch (state->menu_tape_phase) {
    case TAPE_IDLE:
        /* LDA ram_0048; BMI loop; BEQ loop -> only values $01-$7F start a transfer. */
        if (state->menu_ram0048 == 0u || (state->menu_ram0048 & 0x80u) != 0u) return;
        state->menu_nmi_enabled = 0u; /* sub_C32F */
        if (state->menu_ram0048 == 1u) {
            /* sub_C6C8: write block 0 ($0060, $40 bytes) then block 1 ($05E0, $100 bytes). */
            if (s_tapeIoSet && s_tapeIo.save != 0) {
                (void)s_tapeIo.save(s_tapeIo.ctx, state->menu_ram0060, state->menu_ram05E0);
            }
            state->menu_ram0048 = 0xFFu;      /* loc_C1ED */
            state->menu_nmi_enabled = 1u;     /* sub_C339 */
        } else {
            state->menu_tape_phase = TAPE_READ_BLOCK0; /* sub_C739 */
        }
        return;
    case TAPE_READ_BLOCK0:
        if (!s_tapeIoSet || s_tapeIo.readBlock == 0) return; /* waits for tape signal forever */
        r = s_tapeIo.readBlock(s_tapeIo.ctx, 0u, buffer, sizeof(buffer));
        if (r == 0) return;
        if (r < 0) { tapePostError19(state); return; }
        memcpy(state->menu_ram0060, buffer, sizeof(buffer)); /* STA (ram_000E),Y */
        /* $C751-$C77D: bytes 1..4 -> $00A4..$00A7, counted against tbl_C180; list shown at $224F. */
        mismatches = 0u;
        for (y = 1u; y <= 4u; ++y) {
            state->menu_ramA0[3u + y] = buffer[y];
            if (buffer[y] != tblC180[y - 1u]) ++mismatches;
        }
        state->menu_ramA0[8] = 0u;
        state->menu_ramA0[0] = 0x07u;
        state->menu_ramA0[1] = 0x22u;
        state->menu_ramA0[2] = 0x4Fu;
        state->menu_ramA0[3] = 0x04u;
        state->menu_ram0045 = 0x1Au;
        state->menu_nmi_enabled = 1u; /* sub_C339 */
        state->menu_tape_phase = mismatches != 0u ? TAPE_WAIT_HEADER_BAD : TAPE_WAIT_HEADER;
        return;
    case TAPE_WAIT_ERROR_19:
    case TAPE_WAIT_HEADER_BAD:
        if (state->menu_ram0045 != 0u) return; /* spin until the NMI consumed the list */
        state->menu_nmi_enabled = 0u;          /* bra_C739_loop: sub_C32F, read block 0 again */
        state->menu_tape_phase = TAPE_READ_BLOCK0;
        return;
    case TAPE_WAIT_HEADER:
        if (state->menu_ram0045 != 0u) return;
        state->menu_nmi_enabled = 0u;          /* sub_C32F, sub_C726 */
        state->menu_tape_phase = TAPE_READ_BLOCK1;
        return;
    case TAPE_READ_BLOCK1:
        if (!s_tapeIoSet || s_tapeIo.readBlock == 0) return;
        r = s_tapeIo.readBlock(s_tapeIo.ctx, 1u, state->menu_ram05E0, sizeof(state->menu_ram05E0));
        if (r == 0) return;
        if (r < 0) { tapePostError19(state); return; } /* BCS bra_C744_loop (restarts at block 0) */
        state->menu_ram0048 = 0xFFu;           /* loc_C1ED */
        state->menu_nmi_enabled = 1u;          /* sub_C339 */
        state->menu_tape_phase = TAPE_IDLE;
        return;
    default:
        state->menu_tape_phase = TAPE_IDLE;
        return;
    }
}

void menusNmiFrame(GameState *state, unsigned int heldKeys, unsigned int pressedKeys)
{
    if (state == 0) return;
    /* While the main thread owns the CPU with NMI disabled (sub_C32F) no vec_C23B_NMI runs. */
    if (state->menu_nmi_enabled != 0u) {
        menusNmiUpload(state);                         /* C32F, $2001, OAM DMA, sub_C27F */
        menusUpdate(state, heldKeys, pressedKeys);     /* D310 timers, joypad, sub_C2A9 */
        menusRefreshDrawable(state);                   /* $2001 as left at the end of the NMI */
    }
    menusMainThreadStep(state);                        /* bra_C1D8 loop, between two NMIs */
}
