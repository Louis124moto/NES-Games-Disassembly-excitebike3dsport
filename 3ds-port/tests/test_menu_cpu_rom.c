/*
 * ROM-backed integration smoke test for the state-09 editor path.
 * The workflow wraps the exact 16 KiB PRG emitted by asm_to_prg.py in a
 * temporary iNES header and writes ./rom.nes beside this test executable.
 */
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include "game_state.h"
#include "menus.h"
#include "nes_menu_cpu.h"

static int fail(const char *what, unsigned int frame, const GameState *s)
{
    fprintf(stderr, "ROM menu test: %s at frame %u (state=%u setup=%u ram40=%u ram45=%02X)\n",
            what, frame, (unsigned int)s->menu_asm_state,
            (unsigned int)s->menu_ram0044, (unsigned int)s->menu_ram0040,
            (unsigned int)s->menu_ram0045);
    return 1;
}

static int testC465StateAdvance(uint8_t initialState, uint8_t expectedState)
{
    GameState s;
    gameStateReset(&s);
    s.menu_asm_state = initialState;
    s.menu_ram0040 = 1u;
    s.menu_timer34 = 0u;
    menusUpdate(&s, 0u, 0u);
    if (s.menu_asm_state != expectedState || s.menu_ram0040 != 0u ||
        s.menu_ram0044 != 0u) {
        fprintf(stderr, "ROM menu test: C465 state %u -> %u (expected %u)\n",
                (unsigned int)initialState, (unsigned int)s.menu_asm_state,
                (unsigned int)expectedState);
        return 0;
    }
    return 1;
}

static int testRasterSplitD0AB(void)
{
    uint8_t cpuRam[0x800];
    uint8_t nametables[2048];
    uint8_t palette[32];
    uint8_t ctrl = 0u, mask = 0u, scrollX = 0u, scrollY = 0u;
    unsigned int i;

    memset(cpuRam, 0, sizeof(cpuRam));
    memset(nametables, 0, sizeof(nametables));
    memset(palette, 0, sizeof(palette));
    cpuRam[0x0041u] = 9u;       /* editor: D14E must not blanket-hide OAM */
    cpuRam[0x0010u] = 0x10u;    /* source PPUCTRL baseline */
    cpuRam[0x004Eu] = 0x01u;    /* first raster half: nametable B */
    cpuRam[0x004Du] = 0x02u;    /* second raster half: nametable A + vertical bit */
    cpuRam[0x0050u] = 0x20u;
    cpuRam[0x0051u] = 0x30u;
    cpuRam[0x0012u] = 0x40u;
    cpuRam[0x0013u] = 0x50u;
    cpuRam[0x003Fu] = 0x2Bu;
    cpuRam[0x00B8u] = 0x10u;
    cpuRam[0x00B9u] = 0x18u;
    cpuRam[0x00BAu] = 0x20u;
    cpuRam[0x00BBu] = 0x30u;
    for (i = 0u; i < 256u; ++i) cpuRam[0x0200u + i] = (uint8_t)(i ^ 0x5Au);

    if (!nesMenuCpuRasterSplit(cpuRam, nametables, palette, &ctrl, &mask,
                               &scrollX, &scrollY)) {
        fputs("ROM menu test: D0AB/D14E raster adapter failed\n", stderr);
        return 0;
    }
    if (cpuRam[0x0360u] != 4u || cpuRam[0x0361u] != 3u ||
        cpuRam[0x0362u] != 2u || cpuRam[0x0363u] != 0u) {
        fprintf(stderr, "ROM menu test: D0AB vertical bands mismatch %u,%u,%u,%u\n",
                cpuRam[0x0360u], cpuRam[0x0361u],
                cpuRam[0x0362u], cpuRam[0x0363u]);
        return 0;
    }
    if (cpuRam[0x004Cu] != 3u || ctrl != 0x12u ||
        scrollX != 0x40u || scrollY != 0x50u) {
        fprintf(stderr, "ROM menu test: D14E split mismatch ctrl=%02X scroll=%02X/%02X phase=%u\n",
                ctrl, scrollX, scrollY, cpuRam[0x004Cu]);
        return 0;
    }
    if (cpuRam[0x0300u] != 0x11u || cpuRam[0x0301u] != 0x23u ||
        cpuRam[0x0302u] != 0xA0u || cpuRam[0x0312u] != 0u) {
        fputs("ROM menu test: D14E dynamic OAM/PPU preparation mismatch\n", stderr);
        return 0;
    }
    for (i = 0u; i < 256u; ++i) {
        if (cpuRam[0x0200u + i] != (uint8_t)(i ^ 0x5Au)) {
            fputs("ROM menu test: editor D14E unexpectedly hid OAM\n", stderr);
            return 0;
        }
    }
    return 1;
}

static int testSharedTransitionList3(void)
{
    GameState s;
    unsigned int frame;

    gameStateReset(&s);
    memset(s.menu_cpu_ram, 0, sizeof(s.menu_cpu_ram));
    s.menu_asm_state = 3u;
    s.menu_ram03F8 = 4u; /* source title/setup precondition */
    for (frame = 0u; frame < 10u && s.menu_ram0040 == 0u; ++frame)
        menusNmiFrame(&s, 0u, 0u);

    if (s.menu_ram0040 == 0u || s.menu_ram0045 != 0x18u ||
        s.menu_repeat_guard != 1u || s.menu_ram0044 != 0u) {
        fprintf(stderr, "ROM menu test: list-3 PRG setup stopped at frame %u (setup=%u ram40=%u ram45=%02X)\n",
                frame, (unsigned int)s.menu_ram0044,
                (unsigned int)s.menu_ram0040, (unsigned int)s.menu_ram0045);
        return 0;
    }

    /* The next NMI dispatches state 3 through the original CB2C pointer table. */
    menusNmiFrame(&s, 0u, 0u);
    if (s.menu_ram0044 != 1u || s.menu_ram0046 != 1u ||
        s.menu_ram004B != 1u || s.menu_timer33 != 4u) {
        fprintf(stderr, "ROM menu test: CB2C state-3 phase 0 mismatch (phase=%u timer33=%u flag46=%u flag4B=%u)\n",
                (unsigned int)s.menu_ram0044, (unsigned int)s.menu_timer33,
                (unsigned int)s.menu_ram0046, (unsigned int)s.menu_ram004B);
        return 0;
    }
    return 1;
}

int main(void)
{
    GameState s;
    unsigned int frame;

    gameStateReset(&s);
    if (!nesMenuCpuInit("./test_menu_cpu_rom")) {
        fputs("ROM menu test: failed to load supported rom.nes\n", stderr);
        return 1;
    }
    if (!testC465StateAdvance(4u, 5u) || !testC465StateAdvance(7u, 2u))
        return 1;
    if (!testRasterSplitD0AB())
        return 1;
    if (!testSharedTransitionList3())
        return 1;

    /* Enter state 9 directly, then execute its real six-command setup list. */
    s.menu_asm_state = 9u;
    for (frame = 0u; frame < 8u && s.menu_ram0040 == 0u; ++frame)
        menusNmiFrame(&s, 0u, 0u);

    if (s.menu_ram0040 == 0u)
        return fail("list-5 setup did not reach the editor", frame, &s);
    if (s.menu_cpu_ram[0x0049u] != 0u)
        return fail("C5C2 did not clear the editor input guard", frame, &s);
    if (s.menu_cpu_ram[0x00B7u] != 1u || s.menu_cpu_ram[0x00BEu] != 0u)
        return fail("C5C2 object state differs from the source setup", frame, &s);
    if (s.menu_ram05E0[0] != 1u)
        return fail("C5C2 course page was not copied into GameState", frame, &s);

    /* One original C97C frame must return through the emulated sprite-hit wait. */
    menusNmiFrame(&s, KEY_LEFT, KEY_LEFT);
    if (s.menu_asm_state != 9u)
        return fail("editor frame unexpectedly changed state", frame + 1u, &s);
    if (s.menu_cpu_ram[0x005Cu] != 0x02u)
        return fail("C97C did not copy held LEFT into ram_005C", frame + 1u, &s);

    /*
     * Exercise the source-guarded EB9E -> EBA8 return branch. EB9E is selected
     * only for ram_00BC_obj+3 > 1; seed that traced mode explicitly so this
     * vector tests the B-button branch without claiming the preceding editor
     * interaction naturally produces that mode.
     */
    s.menu_cpu_ram[0x00BEu] = 0u; /* ram_00BC_obj+2: EB17 must not early-return */
    s.menu_cpu_ram[0x00B4u] = 0u; /* skip the initial countdown */
    s.menu_cpu_ram[0x00BDu] = 0u;
    s.menu_cpu_ram[0x00B9u] = 0u; /* EB23 gate */
    s.menu_cpu_ram[0x00BFu] = 2u; /* ram_00BC_obj+3: enter bra_EB9E */
    s.menu_cpu_ram[0x00C1u] = 0u; /* select bra_EB49 before the mode branch */
    s.menu_cpu_ram[0x0049u] = 0u; /* return input is not locked */
    menusNmiFrame(&s, KEY_B, KEY_B);
    if (s.menu_asm_state != 8u || s.menu_ram0040 != 0u ||
        s.menu_selection != 0u || s.menu_cpu_ram[0x0047u] != 0u)
        return fail("held B did not execute the original EB9E/EBA8 return to state 8",
                    frame + 2u, &s);
    if (memcmp(s.menu_ram06E0, s.menu_ram05E0, sizeof(s.menu_ram06E0)) != 0)
        return fail("EBA8 return did not commit the 256-byte editor page", frame + 2u, &s);

    puts("PASS: original ROM D0AB/D14E raster split, list-3/CB2C, list-5/editor setup, C97C frame, and guarded EB17 B-return.");
    return 0;
}
