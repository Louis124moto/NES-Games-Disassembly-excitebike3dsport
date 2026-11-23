#include <stdio.h>
#include <string.h>
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

        memset(&input, 0, sizeof(input));
        if (inputMapNesButtons(keys) != expected) {
            printf("FAIL: controller map input=%02X got=%02X\n", expected, inputMapNesButtons(keys));
            return 1;
        }
        s_held = keys;
        s_pressed = keys;
        inputUpdate(&input);
        /* First frame: A was not held before, so press == hold (sub_D347). */
        if (input.held != keys || input.pressed != keys ||
            inputNesHeld(&input) != expected || inputNesPressed(&input) != expected) {
            printf("FAIL: inputUpdate mapping input=%02X held=%02X pressed=%02X\n",
                   expected, inputNesHeld(&input), inputNesPressed(&input));
            return 1;
        }
        /* Second frame, same keys held: only A turns into a no-press (D37A-D384);
         * every other press bit stays level, as in the source. */
        s_pressed = 0u;
        inputUpdate(&input);
        if (inputNesHeld(&input) != expected ||
            inputNesPressed(&input) != (uint8_t)(expected & 0x7Fu)) {
            printf("FAIL: A edge handling input=%02X hold=%02X press=%02X\n",
                   expected, inputNesHeld(&input), inputNesPressed(&input));
            return 1;
        }
    }

    {
        InputState empty;
        memset(&empty, 0, sizeof(empty));
        if (inputNesHeld(0) != 0u || inputNesPressed(0) != 0u ||
            inputNesHeld(&empty) != 0u || inputNesPressed(&empty) != 0u) {
            puts("FAIL: null/empty input mapping");
            return 1;
        }
    }

    if (s_scans != 512u) {
        printf("FAIL: hidScanInput called %u times, expected 512\n", s_scans);
        return 1;
    }

    puts("PASS: all 256 NES controller combinations map correctly with source hold/press semantics.");
    return 0;
}
