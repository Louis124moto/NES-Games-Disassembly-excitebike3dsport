#include <stdio.h>
#include "input.h"

static u32 s_held;
static u32 s_pressed;
static unsigned s_scans;

void hidScanInput(void) { ++s_scans; }
u32 hidKeysHeld(void) { return s_held; }
u32 hidKeysDown(void) { return s_pressed; }

static u32 keys_for_nes(uint8_t buttons)
{
    u32 keys = 0u;
    if ((buttons & 0x80u) != 0u) keys |= KEY_A;
    if ((buttons & 0x40u) != 0u) keys |= KEY_B;
    if ((buttons & 0x20u) != 0u) keys |= KEY_SELECT;
    if ((buttons & 0x10u) != 0u) keys |= KEY_START;
    if ((buttons & 0x08u) != 0u) keys |= KEY_UP;
    if ((buttons & 0x04u) != 0u) keys |= KEY_DOWN;
    if ((buttons & 0x02u) != 0u) keys |= KEY_LEFT;
    if ((buttons & 0x01u) != 0u) keys |= KEY_RIGHT;
    return keys;
}

int main(void)
{
    unsigned value;
    for (value = 0u; value < 256u; ++value) {
        InputState input;
        const uint8_t expected = (uint8_t)value;
        const u32 keys = keys_for_nes(expected);

        input.held = keys;
        input.pressed = keys;
        if (inputMapNesButtons(keys) != expected ||
            inputNesHeld(&input) != expected ||
            inputNesPressed(&input) != expected) {
            printf("FAIL: controller map input=%02X got=%02X\n",
                   expected, inputMapNesButtons(keys));
            return 1;
        }

        s_held = keys;
        s_pressed = keys;
        input.held = 0u;
        input.pressed = 0u;
        inputUpdate(&input);
        if (input.held != keys || input.pressed != keys ||
            inputNesHeld(&input) != expected ||
            inputNesPressed(&input) != expected) {
            printf("FAIL: inputUpdate mapping input=%02X held=%02X pressed=%02X\n",
                   expected, inputNesHeld(&input), inputNesPressed(&input));
            return 1;
        }
    }

    {
        InputState empty = { 0u, 0u };
        if (inputNesHeld(0) != 0u || inputNesPressed(0) != 0u ||
            inputNesHeld(&empty) != 0u || inputNesPressed(&empty) != 0u) {
            puts("FAIL: null/empty input mapping");
            return 1;
        }
    }

    if (s_scans != 256u) {
        printf("FAIL: hidScanInput called %u times, expected 256\n", s_scans);
        return 1;
    }

    puts("PASS: all 256 NES controller combinations map correctly from libctru keys.");
    return 0;
}
