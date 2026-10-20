#include <stdint.h>
#include <stdio.h>

#include "nes_routines.h"

/* Independent instruction-by-instruction 6502 reference for sub_DA15. */
static uint8_t referenceSubDA15(uint8_t ram0390, uint8_t ram0080, uint8_t ramY)
{
    uint8_t a = ram0390;
    unsigned int carry = 1u;
    int difference;
    unsigned int sum;

    /* SEC; SBC ram_0080_obj,X */
    difference = (int)a - (int)ram0080 - (int)(1u - carry);
    a = (uint8_t)difference;
    carry = difference >= 0 ? 1u : 0u;

    /* SBC #$08, consuming the carry from the previous SBC. */
    difference = (int)a - 0x08 - (int)(1u - carry);
    a = (uint8_t)difference;
    carry = difference >= 0 ? 1u : 0u;

    /* ASL; CLC; ADC ram_obj_pos_Y_lo,X */
    a = (uint8_t)(a << 1);
    carry = 0u;
    sum = (unsigned int)a + (unsigned int)ramY + carry;
    a = (uint8_t)sum;
    carry = sum > 0xFFu ? 1u : 0u;

    /* ADC #$04 consumes the carry produced by ADC ramY. */
    sum = (unsigned int)a + 0x04u + carry;
    a = (uint8_t)sum;

    return a;
}

int main(void)
{
    unsigned int x, y, z;
    unsigned long checked = 0;

    for (x = 0; x < 256u; ++x) {
        for (y = 0; y < 256u; ++y) {
            for (z = 0; z < 256u; ++z) {
                uint8_t expected = referenceSubDA15((uint8_t)x, (uint8_t)y, (uint8_t)z);
                uint8_t actual = nesSubDA15DerivedY((uint8_t)x, (uint8_t)y, (uint8_t)z);
                if (actual != expected) {
                    fprintf(stderr,
                            "Mismatch: ram0390=%02X ram0080=%02X ramY=%02X expected=%02X actual=%02X\n",
                            x, y, z, expected, actual);
                    return 1;
                }
                ++checked;
            }
        }
    }

    printf("PASS: %lu input combinations matched the 6502 reference.\n", checked);
    if (testSubD310() != 0) return 1;
    return 0;
}


/* Independent reference for sub_D310; model DEC/N flags and X loop bounds. */
static void referenceSubD310(uint8_t *timer0020, uint8_t timers[29])
{
    uint8_t decremented = (uint8_t)(*timer0020 - 1u);
    int x = 0x0E;

    *timer0020 = decremented;
    if ((decremented & 0x80u) != 0u) {
        *timer0020 = 0x0Au;
        x = 0x1C;
    }

    for (; x >= 0; --x) {
        if (timers[x] != 0u) {
            timers[x] = (uint8_t)(timers[x] - 1u);
        }
    }
}

static int testSubD310(void)
{
    unsigned int frame, timer_value;
    unsigned long checked = 0;

    for (frame = 0; frame < 256u; ++frame) {
        for (timer_value = 0; timer_value < 256u; ++timer_value) {
            uint8_t actual_frame = (uint8_t)frame;
            uint8_t expected_frame = (uint8_t)frame;
            uint8_t actual_timers[29];
            uint8_t expected_timers[29];
            unsigned int i;

            for (i = 0; i < 29u; ++i) {
                actual_timers[i] = (uint8_t)(timer_value + i * 37u + frame * 13u);
                expected_timers[i] = actual_timers[i];
            }
            if (((frame ^ timer_value) & 1u) != 0u) {
                for (i = 0; i < 29u; ++i) {
                    actual_timers[i] = (uint8_t)timer_value;
                    expected_timers[i] = (uint8_t)timer_value;
                }
            }

            referenceSubD310(&expected_frame, expected_timers);
            nesSubD310DecreaseAllTimers(&actual_frame, actual_timers);
            if (actual_frame != expected_frame) {
                fprintf(stderr, "sub_D310 frame mismatch: in=%02X expected=%02X actual=%02X\n",
                        frame, expected_frame, actual_frame);
                return 1;
            }
            for (i = 0; i < 29u; ++i) {
                if (actual_timers[i] != expected_timers[i]) {
                    fprintf(stderr,
                            "sub_D310 timer mismatch: frame=%02X value=%02X slot=%u expected=%02X actual=%02X\n",
                            frame, timer_value, i, expected_timers[i], actual_timers[i]);
                    return 1;
                }
            }
            ++checked;
        }
    }
    printf("PASS: sub_D310 matched reference for %lu frame/timer vectors.\n", checked);
    return 0;
}
