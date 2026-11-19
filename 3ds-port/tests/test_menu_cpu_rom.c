/*
 * ROM-backed integration smoke test for the state-09 editor path.
 * The workflow wraps the exact 16 KiB PRG emitted by asm_to_prg.py in a
 * temporary iNES header and writes ./rom.nes beside this test executable.
 */
#include <stdio.h>
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

int main(void)
{
    GameState s;
    unsigned int frame;
    uint8_t revision;

    gameStateReset(&s);
    if (!nesMenuCpuInit("./test_menu_cpu_rom")) {
        fputs("ROM menu test: failed to load supported rom.nes\n", stderr);
        return 1;
    }

    /* Enter state 9 directly, then execute its real six-command setup list. */
    s.menu_asm_state = 9u;
    for (frame = 0u; frame < 8u && s.menu_ram0040 == 0u; ++frame)
        menusNmiFrame(&s, 0u, 0u);

    if (s.menu_ram0040 == 0u)
        return fail("list-5 setup did not reach the editor", frame, &s);
    if (s.menu_cpu_ram[0x0049u] != 1u)
        return fail("C5C2 did not initialize the editor input guard", frame, &s);
    if (s.menu_cpu_ram[0x00B7u] != 1u || s.menu_cpu_ram[0x00BEu] != 0u)
        return fail("C5C2 object state differs from the source setup", frame, &s);
    if (s.menu_ram05E0[0] != 2u)
        return fail("C5C2 course page was not copied into GameState", frame, &s);

    /* One original C97C frame must return through the emulated sprite-hit wait. */
    revision = s.menu_ppu_revision;
    menusNmiFrame(&s, 0u, 0u);
    if (s.menu_asm_state != 9u)
        return fail("idle editor frame unexpectedly changed state", frame + 1u, &s);
    if (s.menu_ppu_revision == revision)
        return fail("C97C frame did not complete or publish its PPU update", frame + 1u, &s);

    puts("PASS: original ROM list-5 setup and one recompiled state-09 editor frame.");
    return 0;
}
