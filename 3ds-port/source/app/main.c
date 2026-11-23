#include <3ds.h>
#include "game.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (!gameInit(argc > 0 ? argv[0] : NULL)) return 1;
    while (aptMainLoop()) {
        gameUpdate();
        gameRender();
    }
    gameExit();
    return 0;
}
