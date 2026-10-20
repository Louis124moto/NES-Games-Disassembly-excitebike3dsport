#include <stdio.h>
#include <3ds.h>
#include "menus.h"

/*
 * Temporary native diagnostic overlay only.
 *
 * This is deliberately not presented as the original Excitebike title/menu
 * implementation. The original bank_FF.asm menu/state flow must be traced
 * and translated before this overlay can be replaced with a faithful menu.
 * It does not modify emulated NES state or invent game rules.
 */
void menusUpdate(GameState *state, unsigned int pressed)
{
    if (state == 0) return;
    if (pressed & KEY_X) state->show_help = !state->show_help;
}

void menusRender(const GameState *state)
{
    if (state == 0 || !state->show_help) return;

    printf("\n");
    printf("+----------------------------------+\n");
    printf("| EXCITEBIKE 3DS - PORT MENU       |\n");
    printf("+----------------------------------+\n");
    printf("| Runtime: native 3DS scaffold     |\n");
    printf("| NES gameplay: not integrated     |\n");
    printf("| A: throttle input path (stub)    |\n");
    printf("| B: boost input path (stub)       |\n");
    printf("| D-pad: position input path       |\n");
    printf("| X: close this diagnostic menu    |\n");
    printf("+----------------------------------+\n");
    printf("Faithful NES menu is a separate task.\n");
}
