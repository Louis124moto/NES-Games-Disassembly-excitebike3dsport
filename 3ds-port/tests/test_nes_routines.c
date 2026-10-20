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

static int testSubD310(void);
static int testSubD35B(void);
static int testSubD326(void);
static int testSubDA7A(void);

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
    if (testSubD35B() != 0) return 1;
    if (testSubD326() != 0) return 1;\n    if (testSubDA7A() != 0) return 1;
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


/* Independent reference for sub_D326, preserving the initial SEC decision
 * and the carry chain across each memory ROR. */
static void referenceSubD326(uint8_t bytes[8])
{
    unsigned int carry = 0u;
    unsigned int i;
    uint8_t tmp0 = (uint8_t)(bytes[0] & 0x02u);
    uint8_t tmp1 = (uint8_t)(bytes[1] & 0x02u);
    uint8_t different = (uint8_t)(tmp0 ^ tmp1);

    if (different != 0u) carry = 1u;
    for (i = 0u; i < 8u; ++i) {
        uint8_t value = bytes[i];
        uint8_t outgoing = (uint8_t)(value & 1u);
        bytes[i] = (uint8_t)((value >> 1) | (uint8_t)(carry << 7));
        carry = outgoing;
    }
}

static int testSubD326(void)
{
    unsigned int first, second, pattern;
    unsigned long checked = 0;

    for (first = 0; first < 256u; ++first) {
        for (second = 0; second < 256u; ++second) {
            uint8_t actual[8];
            uint8_t expected[8];
            unsigned int i;
            for (i = 0; i < 8u; ++i) {
                actual[i] = (uint8_t)(first + second * (i + 1u) + i * 29u);
                expected[i] = actual[i];
            }
            actual[0] = expected[0] = (uint8_t)first;
            actual[1] = expected[1] = (uint8_t)second;
            pattern = (first ^ second) & 3u;
            if (pattern == 1u) {
                for (i = 2; i < 8u; ++i) actual[i] = expected[i] = (uint8_t)first;
            } else if (pattern == 2u) {
                for (i = 2; i < 8u; ++i) actual[i] = expected[i] = (uint8_t)(255u - second);
            } else if (pattern == 3u) {
                for (i = 2; i < 8u; ++i) actual[i] = expected[i] = (uint8_t)(i & 1u ? 0xFFu : 0x00u);
            }
            referenceSubD326(expected);
            nesSubD326RotateInputBits(actual);
            for (i = 0; i < 8u; ++i) {
                if (actual[i] != expected[i]) {
                    fprintf(stderr,
                            "sub_D326 mismatch: first=%02X second=%02X slot=%u expected=%02X actual=%02X\n",
                            first, second, i, expected[i], actual[i]);
                    return 1;
                }
            }
            ++checked;
        }
    }
    printf("PASS: sub_D326 matched reference for %lu input vectors.\n", checked);
    return 0;
}


/* Reference model for the RAM-level decode and A-button repeat suppression. */
static void referenceSubD35B(const uint8_t serial_samples[8], uint8_t previous_hold,
                             uint8_t *hold, uint8_t *press)
{
    uint8_t accumulator = 0u;
    unsigned int i;

    for (i = 0; i < 8u; ++i) {
        uint8_t saved = serial_samples[i];
        uint8_t shifted = (uint8_t)(saved >> 1);
        uint8_t combined = (uint8_t)(shifted | saved);
        uint8_t carry = (uint8_t)(combined & 1u);
        accumulator = (uint8_t)((accumulator << 1) | carry);
    }

    *hold = accumulator;
    *press = accumulator;
    if ((*hold & 0x80u) != 0u && (previous_hold & 0x80u) != 0u) {
        *press = (uint8_t)(accumulator & 0x7Fu);
    }
}

static int testSubD35B(void)
{
    unsigned int mask, previous, encoding;
    unsigned long checked = 0;

    for (mask = 0; mask < 256u; ++mask) {
        for (previous = 0; previous < 256u; ++previous) {
            for (encoding = 0; encoding < 3u; ++encoding) {
                uint8_t samples[8];
                uint8_t actual_hold, actual_press, expected_hold, expected_press;
                unsigned int i;
                for (i = 0; i < 8u; ++i) {
                    uint8_t active = (uint8_t)((mask >> (7u - i)) & 1u);
                    samples[i] = active ? (uint8_t)(encoding + 1u) : 0u;
                }
                referenceSubD35B(samples, (uint8_t)previous, &expected_hold, &expected_press);
                nesSubD35BDecodeController(samples, (uint8_t)previous, &actual_hold, &actual_press);
                if (actual_hold != expected_hold || actual_press != expected_press) {
                    fprintf(stderr,
                            "sub_D35B mismatch: mask=%02X previous=%02X encoding=%u hold=%02X/%02X press=%02X/%02X\n",
                            mask, previous, encoding, expected_hold, actual_hold, expected_press, actual_press);
                    return 1;
                }
                ++checked;
            }
        }
    }
    printf("PASS: sub_D35B matched reference for %lu controller vectors.\n", checked);
    return 0;
}


/* Independent reference: model each 6502 instruction and its carry flag. */
static void referenceSubDA7A(uint8_t active[4], const uint8_t delta[4],
                             uint8_t posXLo[4], uint8_t posXHi[4],
                             uint8_t ram0390[4])
{
    int x;
    for (x = 3; x >= 1; --x) {
        uint8_t a;
        unsigned int carry;
        int difference;
        if (active[x] == 0u) continue;

        a = delta[x];
        carry = 0u; /* CLC */
        {
            unsigned int sum = (unsigned int)a + posXLo[x] + carry;
            a = (uint8_t)sum;
            carry = sum > 0xFFu ? 1u : 0u;
        }
        if (carry != 0u) posXHi[x] = (uint8_t)(posXHi[x] + 1u);

        carry = 1u; /* SEC */
        difference = (int)a - (int)delta[0] - (int)(1u - carry);
        a = (uint8_t)difference;
        carry = difference >= 0 ? 1u : 0u;
        posXLo[x] = a;
        if (carry == 0u) posXHi[x] = (uint8_t)(posXHi[x] - 1u);

        carry = 1u; /* SEC before SBC ram_0060 */
        difference = (int)ram0390[x] - (int)delta[0] - (int)(1u - carry);
        ram0390[x] = (uint8_t)difference;
    }
}

static int testSubDA7A(void)
{
    unsigned int pattern;
    unsigned long checked = 0u;

    /* Deterministic exhaustive byte pairs for delta[0], delta[x], posXLo[x],
       plus edge patterns in the high and 0390 bytes and active flags. */
    for (pattern = 0; pattern < 65536u; ++pattern) {
        uint8_t activeA[4], activeB[4], deltaA[4], deltaB[4];
        uint8_t loA[4], loB[4], hiA[4], hiB[4], rA[4], rB[4];
        unsigned int i;
        deltaA[0] = deltaB[0] = (uint8_t)(pattern >> 8);
        for (i = 1; i < 4u; ++i) {
            deltaA[i] = deltaB[i] = (uint8_t)(pattern + i * 53u);
            loA[i] = loB[i] = (uint8_t)((pattern >> 1) + i * 71u);
            hiA[i] = hiB[i] = (uint8_t)((pattern >> 3) + i * 29u);
            rA[i] = rB[i] = (uint8_t)((pattern >> 5) + i * 97u);
            activeA[i] = activeB[i] = (uint8_t)(((pattern >> (i - 1u)) & 1u) ? 1u : 0u);
        }
        activeA[0] = activeB[0] = 0u;
        loA[0] = loB[0] = 0u;
        hiA[0] = hiB[0] = 0u;
        rA[0] = rB[0] = 0u;

        referenceSubDA7A(activeA, deltaA, loA, hiA, rA);
        nesSubDA7AUpdateScrollSlots(activeB, deltaB, loB, hiB, rB);
        for (i = 0; i < 4u; ++i) {
            if (loA[i] != loB[i] || hiA[i] != hiB[i] || rA[i] != rB[i]) {
                fprintf(stderr, "sub_DA7A mismatch: pattern=%u slot=%u\n", pattern, i);
                return 1;
            }
        }
        ++checked;
    }
    printf("PASS: sub_DA7A matched instruction reference for %lu patterns.\n", checked);
    return 0;
}
