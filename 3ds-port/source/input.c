#include "input.h"

void inputUpdate(InputState *input)
{
    if (input == 0) {
        return;
    }

    hidScanInput();
    input->held = hidKeysHeld();
    input->pressed = hidKeysDown();
}
