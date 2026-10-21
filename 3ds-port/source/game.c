#include "game.h"
#include "audio.h"
#include "bike_physics.h"
#include "collision.h"
#include "game_state.h"
#include "input.h"
#include "menus.h"
#include "renderer.h"
#include "track.h"

static GameState s_game;
static InputState s_input;

void gameInit(void)
{
    gameStateReset(&s_game);
    s_input.held = 0;
    s_input.pressed = 0;
    rendererInit();
    trackInit();
    audioInit();
}

void gameUpdate(void)
{
    inputUpdate(&s_input);
    menusUpdate(&s_game, s_input.held, s_input.pressed);
    if (!s_game.paused && s_game.menu_asm_state == 2u) {
        bikePhysicsUpdate(&s_game.bike, &s_input);
        collisionResolve(&s_game.bike);
        trackUpdate();
    }
    audioUpdate();
}

void gameRender(void)
{
    rendererBeginFrame();
    rendererDraw(&s_game);
    menusRender(&s_game);
    rendererEndFrame();
}

void gameExit(void)
{
    audioExit();
    trackExit();
    rendererExit();
}
