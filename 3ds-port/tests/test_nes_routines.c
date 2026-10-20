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
static int testSubDBFE(void);
static int testSubDC9A(void);
static int testSubDCDE(void);
static int testSubDD38(void);
static int testSubDD6F(void);
static int testSubDB95(void);
static int testSubDA6A(void);
static int testSubDB50(void);
static int testSubDCA0(void);


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
    if (testSubD326() != 0) return 1;
    if (testSubDA7A() != 0) return 1;
    if (testSubDBFE() != 0) return 1;
    if (testSubDC9A() != 0) return 1;
    if (testSubDCDE() != 0) return 1;
    if (testSubDD38() != 0) return 1;
    if (testSubDD6F() != 0) return 1;
    if (testSubDB95() != 0) return 1;
    if (testSubDA6A() != 0) return 1;
    if (testSubDB50() != 0) return 1;
    if (testSubDCA0() != 0) return 1;
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


static uint8_t refSBC(uint8_t a, uint8_t value, unsigned int *carry)
{
    int d = (int)a - (int)value - (int)(1u - *carry);
    *carry = d >= 0 ? 1u : 0u;
    return (uint8_t)d;
}

static int testSubDC9A(void)
{
    unsigned int a, b, c;
    unsigned long checked = 0u;
    for (a = 0; a < 256u; ++a) {
        for (b = 0; b < 256u; ++b) {
            for (c = 0; c < 256u; ++c) {
                unsigned int carry = 1u;
                uint8_t expected = refSBC((uint8_t)a, (uint8_t)b, &carry);
                expected = refSBC(expected, (uint8_t)c, &carry);
                if (nesSubDC9A((uint8_t)a, (uint8_t)b, (uint8_t)c) != expected) {
                    fprintf(stderr, "sub_DC9A mismatch: %02X %02X %02X\n", a, b, c);
                    return 1;
                }
                ++checked;
            }
        }
    }
    printf("PASS: sub_DC9A matched 6502 SBC chain for %lu triples.\n", checked);
    return 0;
}

static int testSubDBFE(void)
{
    unsigned int a, b;
    unsigned long checked = 0u;
    for (a = 0; a < 256u; ++a) {
        for (b = 0; b < 256u; ++b) {
            uint8_t actualBf = (uint8_t)b, expectedBf = (uint8_t)b;
            uint8_t actualScroll = (uint8_t)(a * 37u + b), expectedScroll = actualScroll;
            uint8_t actualNt = (uint8_t)((a ^ b) & 1u), expectedNt = actualNt;
            uint8_t expectedA = (uint8_t)a;
            unsigned int carry = expectedA & 1u;
            unsigned int i, sum;
            expectedA = (uint8_t)(expectedA >> 1);
            for (i = 0; i < 3u; ++i) {
                unsigned int next = expectedA & 1u;
                expectedA = (uint8_t)((expectedA >> 1) | (uint8_t)(carry << 7));
                carry = next;
            }
            sum = (unsigned int)expectedA + expectedBf;
            expectedBf = (uint8_t)sum;
            carry = sum > 0xFFu ? 1u : 0u;
            sum = (unsigned int)(uint8_t)a + expectedScroll + carry;
            expectedScroll = (uint8_t)sum;
            if (sum > 0xFFu) expectedNt = (uint8_t)(expectedNt ^ 1u);
            nesSubDBFEUpdateScroll((uint8_t)a, &actualBf, &actualScroll, &actualNt);
            if (actualBf != expectedBf || actualScroll != expectedScroll || actualNt != expectedNt) {
                fprintf(stderr, "sub_DBFE mismatch: %02X %02X\n", a, b);
                return 1;
            }
            ++checked;
        }
    }
    printf("PASS: sub_DBFE matched instruction reference for %lu pairs.\n", checked);
    return 0;
}

static int testSubDCDE(void)
{
    unsigned int timer, sample;
    unsigned long checked = 0u;
    for (timer = 0; timer < 256u; ++timer) {
        for (sample = 0; sample < 256u; ++sample) {
            uint8_t actual = (uint8_t)timer;
            uint8_t expected = actual;
            if (expected != 0u) {
                expected = (uint8_t)(expected - 1u);
                if ((sample & 0xC0u) != 0u) expected = 0u;
            }
            nesSubDCDE(&actual, (uint8_t)sample);
            if (actual != expected) {
                fprintf(stderr, "sub_DCDE mismatch: timer=%02X sample=%02X\n", timer, sample);
                return 1;
            }
            ++checked;
        }
    }
    printf("PASS: sub_DCDE matched timer/flag reference for %lu pairs.\n", checked);
    return 0;
}

static int testSubDD38(void)
{
    unsigned int x, lo, hi, mode;
    unsigned long checked = 0u;
    for (x = 0; x < 4u; ++x) {
        for (lo = 0; lo < 256u; ++lo) {
            for (hi = 0; hi < 256u; ++hi) {
                for (mode = 0; mode < 2u; ++mode) {
                    uint8_t r90[4] = {0x11u, 0x22u, 0x33u, 0x44u};
                    uint8_t r94[4] = {0u, 0u, 0u, 0u};
                    uint8_t r388[4] = {0u, 0u, 0u, 0u};
                    uint8_t r378[4] = {0u, 0u, 0u, 0u};
                    uint8_t r37c[4] = {0u, 0u, 0u, 0u};
                    uint8_t r380[4] = {0u, 0u, 0u, 0u};
                    uint8_t r00b0[4] = {0u, 0u, 0u, 0u};
                    uint8_t rff = 0xA5u;
                    unsigned int sum, carry;
                    uint8_t expectedLo, expectedHi;
                    r90[x] = (uint8_t)lo;
                    r94[x] = (uint8_t)hi;
                    r388[x] = mode ? 2u : 0u;
                    sum = lo + 0xAFu;
                    expectedLo = (uint8_t)sum;
                    carry = sum > 0xFFu ? 1u : 0u;
                    expectedHi = (uint8_t)(hi + carry);
                    if (mode) {
                        carry = expectedHi & 1u;
                        expectedHi = (uint8_t)(expectedHi >> 1);
                        expectedLo = (uint8_t)((expectedLo >> 1) | (uint8_t)(carry << 7));
                    }
                    nesSubDD38SetupObject((uint8_t)x, r90, r94, r388, r378, r37c, r380, r00b0, &rff);
                    if (r378[x] != expectedLo || r37c[x] != expectedHi ||
                        r380[x] != 0x0Fu || r00b0[x] != 2u) {
                        fprintf(stderr, "sub_DD38 mismatch: x=%u lo=%02X hi=%02X mode=%u\n", x, lo, hi, mode);
                        return 1;
                    }
                    if (x == 0u) {
                        uint8_t e = (uint8_t)(((uint8_t)hi << 4) ^ 0x30u);
                        if (rff != (e != 0u ? e : 0x08u)) {
                            fprintf(stderr, "sub_DD38 ram_00FF mismatch: hi=%02X\n", hi);
                            return 1;
                        }
                    } else if (rff != 0xA5u) {
                        fprintf(stderr, "sub_DD38 unexpectedly changed ram_00FF for X=%u\n", x);
                        return 1;
                    }
                    ++checked;
                }
            }
        }
    }
    printf("PASS: sub_DD38 matched reference for %lu slot/value/mode cases.\n", checked);
    return 0;
}

static int testSubDD6F(void)
{
    unsigned int pattern, x, carryIn;
    unsigned long checked = 0u;
    for (pattern = 0; pattern < 65536u; ++pattern) {
        for (x = 0; x < 4u; ++x) {
            for (carryIn = 0; carryIn < 2u; ++carryIn) {
                uint8_t r380[4], r38c[4], r384[4], r37c[4];
                uint8_t e380, e384, a, expected;
                unsigned int sum, carry = carryIn;
                int d;
                unsigned int i;
                for (i = 0; i < 4u; ++i) {
                    r380[i] = (uint8_t)(pattern + i * 43u);
                    r38c[i] = (uint8_t)((pattern >> 1) + i * 71u);
                    r384[i] = (uint8_t)((pattern >> 3) + i * 19u);
                    r37c[i] = (uint8_t)((pattern >> 5) + i * 97u);
                }
                e380 = (uint8_t)((unsigned int)r380[x] + r38c[x] + carry);
                sum = (unsigned int)r380[x] + r38c[x] + carry;
                carry = sum > 0xFFu ? 1u : 0u;
                sum = (unsigned int)r384[x] + carry;
                e384 = (uint8_t)sum;
                carry = sum > 0xFFu ? 1u : 0u;
                d = (int)(uint8_t)pattern - (int)r37c[x] - (int)(1u - carry);
                a = (uint8_t)d;
                carry = d >= 0 ? 1u : 0u;
                sum = (unsigned int)a + e384 + carry;
                expected = (uint8_t)sum;
                if (nesSubDD6F((uint8_t)x, (uint8_t)carryIn, r380, r38c, r384, r37c, (uint8_t)pattern) != expected ||
                    r380[x] != e380 || r384[x] != e384) {
                    fprintf(stderr, "sub_DD6F mismatch: pattern=%u x=%u carry=%u\n", pattern, x, carryIn);
                    return 1;
                }
                ++checked;
            }
        }
    }
    printf("PASS: sub_DD6F matched reference for %lu state combinations.\n", checked);
    return 0;
}

static int testSubDB95(void)
{
    unsigned int x, i;
    for (x = 1u; x <= 3u; ++x) {
        uint8_t arrays[20][4];
        uint8_t *p[20];
        for (i = 0; i < 20u; ++i) {
            unsigned int j;
            p[i] = arrays[i];
            for (j = 0; j < 4u; ++j) arrays[i][j] = (uint8_t)(0x20u + i * 7u + j);
        }
        nesSubDB95ClearOpponentBikeData((uint8_t)x,
            p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7],p[8],p[9],
            p[10],p[11],p[12],p[13],p[14],p[15],p[16],p[17],p[18],p[19]);
        for (i = 0; i < 20u; ++i) {
            unsigned int j;
            for (j = 0; j < 4u; ++j) {
                uint8_t expected = (j == x) ? 0u : (uint8_t)(0x20u + i * 7u + j);
                if (arrays[i][j] != expected) {
                    fprintf(stderr, "sub_DB95 mismatch: field=%u slot=%u\n", i, j);
                    return 1;
                }
            }
        }
    }
    printf("PASS: sub_DB95 clears exactly the 20 documented fields for slots 1..3.\n");
    return 0;
}


static int testSubDA6A(void)
{
    unsigned int scroll, delta, nt;
    unsigned long checked = 0u;
    for (scroll = 0; scroll < 256u; ++scroll) {
        for (delta = 0; delta < 256u; ++delta) {
            for (nt = 0; nt < 2u; ++nt) {
                uint8_t actualScroll = (uint8_t)scroll;
                uint8_t actualNt = (uint8_t)nt;
                unsigned int sum = scroll + delta;
                uint8_t expectedScroll = (uint8_t)sum;
                uint8_t expectedNt = (uint8_t)(nt ^ (sum > 0xFFu ? 1u : 0u));
                nesSubDA6AUpdateScroll(&actualScroll, (uint8_t)delta, &actualNt);
                if (actualScroll != expectedScroll || actualNt != expectedNt) {
                    fprintf(stderr, "sub_DA6A mismatch: scroll=%02X delta=%02X nt=%u\n", scroll, delta, nt);
                    return 1;
                }
                ++checked;
            }
        }
    }
    printf("PASS: sub_DA6A matched 131072 scroll/carry cases.\n");
    return 0;
}

static int testSubDCA0(void)
{
    unsigned int x, index, finished, flags;
    unsigned long checked = 0u;
    for (x = 0; x < 4u; ++x) {
        for (index = 0; index < 256u; ++index) {
            for (finished = 0; finished < 2u; ++finished) {
                for (flags = 0; flags < 8u; ++flags) {
                    uint8_t y[4] = {0u, 0u, 0u, 0u};
                    uint8_t a4[4] = {0u, 0u, 0u, 0u};
                    uint8_t h[4] = {0u, 0u, 0u, 0u};
                    uint8_t table[256];
                    uint8_t expected;
                    unsigned int i;
                    for (i = 0; i < 256u; ++i) table[i] = (uint8_t)(i ^ 0x5Au);
                    y[x] = (uint8_t)index;
                    a4[x] = (uint8_t)((flags & 1u) ? 1u : ((flags & 2u) ? 2u : 0u));
                    expected = (finished == 1u && x == 0u && flags == 0u) ? 0x0Au : table[index];
                    if (a4[x] == 1u) expected = 0x06u;
                    nesSubDCA0SetupSpriteHeight((uint8_t)x, y, (uint8_t)finished,
                        (uint8_t)((flags & 2u) ? 1u : 0u),
                        (uint8_t)((flags & 4u) ? 1u : 0u),
                        (uint8_t)((flags & 1u) ? 1u : 0u), a4, table, h);
                    if (h[x] != expected) {
                        fprintf(stderr, "sub_DCA0 mismatch: x=%u index=%u finished=%u flags=%u\n", x, index, finished, flags);
                        return 1;
                    }
                    ++checked;
                }
            }
        }
    }
    printf("PASS: sub_DCA0 matched %lu table/flag combinations.\n", checked);
    return 0;
}

static int testSubDB50(void)
{
    unsigned int x, scenario;
    uint8_t d8d5[256], d8cd[16];
    unsigned int i;
    unsigned long checked = 0u;
    for (i = 0; i < 256u; ++i) d8d5[i] = (uint8_t)(i ^ 0xA5u);
    for (i = 0; i < 16u; ++i) d8cd[i] = (uint8_t)(i * 7u);
    for (x = 0; x < 4u; ++x) {
        for (scenario = 0; scenario < 256u; ++scenario) {
            uint8_t r84[4], r1b[4], r70[4], r78[4], r74[4], r80[4];
            uint8_t r94 = (uint8_t)(scenario ^ 0x37u);
            uint8_t r90 = (uint8_t)(scenario ^ 0x92u);
            uint8_t finished = (uint8_t)((scenario >> 7) & 1u);
            uint8_t y = (uint8_t)(scenario & 0x0Fu);
            uint8_t expected78, expected74;
            for (i = 0; i < 4u; ++i) {
                r84[i] = (uint8_t)(scenario + i * 29u);
                r1b[i] = (uint8_t)(scenario + i * 17u);
                r70[i] = (uint8_t)(scenario + i * 31u);
                r78[i] = (uint8_t)(scenario ^ (i * 11u));
                r74[i] = (uint8_t)(scenario ^ (i * 23u));
                r80[i] = (uint8_t)(scenario + i * 47u);
            }
            expected78 = finished ? 3u : d8d5[y];
            expected74 = d8cd[r1b[x] & 0x0Fu];
            if (!finished && x == 3u && (r84[x] & 2u) == 0u && r94 != 0u &&
                (r84[x] == 0u || r80[x] < 0x40u)) {
                expected78 = r94;
                expected74 = r90;
                if (!(r84[x] != 0u && r70[0] == r70[3])) {
                    if (expected78 < 4u) expected78 = (uint8_t)(expected78 + 1u);
                } else {
                    expected78 = r94; /* assembly returns after initial stores */
                }
            }
            nesSubDB50SetupObject((uint8_t)x, y, finished, r84, r1b, r70, r78, r74, r80,
                                  r94, r90, d8d5, d8cd);
            if (r78[x] != expected78 || r74[x] != expected74) {
                fprintf(stderr, "sub_DB50 mismatch: x=%u scenario=%u\n", x, scenario);
                return 1;
            }
            ++checked;
        }
    }
    printf("PASS: sub_DB50 matched branch/table reference for %lu cases.\n", checked);
    return 0;
}
