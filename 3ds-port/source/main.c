#include <3ds.h>
#include "game.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    gameInit();
    while (aptMainLoop()) {
        gameUpdate();
        if (hidKeysDown() & KEY_START) break;
        gameRender();
    }
    gameExit();
    return 0;
}
