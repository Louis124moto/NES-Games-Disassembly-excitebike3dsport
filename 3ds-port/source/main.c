#include <3ds.h>
#include "game.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    gameInit(argc > 0 ? argv[0] : NULL);
    while (aptMainLoop()) {
        gameUpdate();
        gameRender();
    }
    gameExit();
    return 0;
}
