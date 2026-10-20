#include <3ds.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    printf("Excitebike 3DS\n");
    printf("----------------\n");
    printf("Native 3DS port scaffold\n\n");
    printf("Status: initialization test\n");
    printf("Gameplay is not implemented yet.\n\n");
    printf("Press START to exit.\n");

    while (aptMainLoop()) {
        hidScanInput();
        const u32 keysDown = hidKeysDown();

        if (keysDown & KEY_START) {
            break;
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}
