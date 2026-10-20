#include <3ds.h>
#include "menus.h"

void menusUpdate(GameState *state, unsigned int pressed)
{
    if (state == 0) return;
    if (pressed & KEY_X) state->show_help = !state->show_help;
}

void menusRender(const GameState *state)
{
    (void)state;
    /* Menu rendering will follow after the first playable prototype. */
}
