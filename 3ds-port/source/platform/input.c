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


/* Original NES serial controller bit order: A, B, Select, Start,
 * Up, Down, Left, Right from bit 7 to bit 0. */
uint8_t inputMapNesButtons(u32 keys)
{
    uint8_t buttons = 0u;

    if ((keys & KEY_A) != 0u) buttons |= 0x80u;
    if ((keys & KEY_B) != 0u) buttons |= 0x40u;
    if ((keys & KEY_SELECT) != 0u) buttons |= 0x20u;
    if ((keys & KEY_START) != 0u) buttons |= 0x10u;
    if ((keys & KEY_UP) != 0u) buttons |= 0x08u;
    if ((keys & KEY_DOWN) != 0u) buttons |= 0x04u;
    if ((keys & KEY_LEFT) != 0u) buttons |= 0x02u;
    if ((keys & KEY_RIGHT) != 0u) buttons |= 0x01u;

    return buttons;
}

uint8_t inputNesHeld(const InputState *input)
{
    return input == 0 ? 0u : inputMapNesButtons(input->held);
}

uint8_t inputNesPressed(const InputState *input)
{
    return input == 0 ? 0u : inputMapNesButtons(input->pressed);
}
