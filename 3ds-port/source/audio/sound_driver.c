/*
 * Native port of the source sound driver (sub_F844 and callees).
 * Labels in comments are the original CPU addresses. Branch structure is kept
 * 1:1 (goto labels) so differential tests against the ASM stay trivial.
 */
#include "sound_driver.h"

#include <string.h>

#define Z(a) (d->zp[(a)])
#define H(a) (d->high[(a) - 0x07E0u])

static uint8_t rd(const SoundDriver *d, unsigned addr)
{
    if (d->data == NULL || addr < SOUND_DATA_BASE || addr >= SOUND_DATA_END)
        return 0u;
    return d->data[addr - SOUND_DATA_BASE];
}

static void apu(SoundDriver *d, unsigned reg, uint8_t value)
{
    if (d->write_count < SOUND_MAX_WRITES) {
        d->writes[d->write_count].reg = (uint16_t)reg;
        d->writes[d->write_count].value = value;
        d->write_count++;
    }
}

/* sub_F871/F885/F889: returns Z=0 (non-zero) when a note was written. */
static int noteWrite(SoundDriver *d, uint8_t a, unsigned x)
{
    const uint8_t lo = rd(d, 0xFF00u + a + 1u);
    if (lo == 0u)
        return 0;
    apu(d, 0x4002u + x, lo);
    apu(d, 0x4003u + x, (uint8_t)(rd(d, 0xFF00u + a) | 0x08u));
    return 1;
}

/* sub_F86B: pulse-1 control then note. */
static void f86b(SoundDriver *d, uint8_t a, uint8_t x, uint8_t y)
{
    apu(d, 0x4000u, x);
    apu(d, 0x4001u, y);
    (void)noteWrite(d, a, 0u);
}

/* sub_FF53: note-length/pitch decode. Returns A; *xOut = original byte. */
static uint8_t f53(const SoundDriver *d, uint8_t v)
{
    const unsigned idx = (unsigned)(((v & 1u) << 2) | ((v >> 7) << 1) | ((v >> 6) & 1u));
    return rd(d, 0xFF3Eu + idx + H(0x07F0u));
}

static void sub_F8D4(SoundDriver *d, uint8_t entryX)
{
    uint8_t y, a, c;

    if (Z(0xF4) != 0u)
        return;
    y = Z(0xFF);
    a = Z(0xF0);
    c = (uint8_t)(Z(0xFF) & 1u); Z(0xFF) >>= 1; if (c) goto F88D;
    c = (uint8_t)(a & 1u); a >>= 1; if (c) goto F93A;
    c = (uint8_t)(Z(0xFF) & 1u); Z(0xFF) >>= 1; if (c) goto F89C;
    c = (uint8_t)(a & 1u); a >>= 1; if (c) goto F93A;      /* F8AB */
    c = (uint8_t)(a & 1u); a >>= 1; if (c) goto F8B9;
    c = (uint8_t)(Z(0xFF) & 1u); Z(0xFF) >>= 1; if (c) goto F8AE;
    c = (uint8_t)(Z(0xFF) & 1u); Z(0xFF) >>= 1; if (c) goto F90E;
    c = (uint8_t)(a & 1u); a >>= 1; if (c) goto F921;
    c = (uint8_t)(Z(0xFF) & 1u); Z(0xFF) >>= 1; if (c) goto F948;
    c = (uint8_t)(a & 1u); a >>= 1; if (c) goto F95B;
    c = (uint8_t)(Z(0xFF) & 1u); Z(0xFF) >>= 1; if (c) goto F96D;
    c = (uint8_t)(a & 1u); a >>= 1; if (c) goto F975;
    c = (uint8_t)(Z(0xFF) & 1u); Z(0xFF) >>= 1; if (c) goto F97D;
    c = (uint8_t)(a & 1u); a >>= 1; if (c) goto F98A;
    return;

F88D:
    Z(0xF0) = y; Z(0xF5) = 0x01u;
    f86b(d, 0x04u, 0x80u, 0x9Cu);
    return;
F89C:
    Z(0xF0) = y; Z(0xF5) = 0x10u;
    f86b(d, 0x30u, 0x85u, 0x85u);
    goto F93A;
F8AE:
    Z(0xF0) = y; Z(0xF5) = 0x09u;
    (void)noteWrite(d, 0x04u, 0u);
F8B9:
    a = Z(0xF5);
    if (a == 0x04u)
        goto F93E;
    y = 0x84u;
    if (a < 0x07u)
        y = 0x8Bu;
    apu(d, 0x4001u, y);
    apu(d, 0x4000u, (uint8_t)(a | 0x90u));
    goto F93A;
F90E:
    Z(0xF0) = y; Z(0xF5) = 0x06u;
    f86b(d, 0x3Cu, 0x9Cu, y);            /* Y still holds the request byte */
    Z(0xF6) = 0x03u;
    goto F92B;
F921:
    Z(0xF6)--;
    if (Z(0xF6) == 0u)
        goto F931;
    if (Z(0xF6) != 0x03u)
        return;
F92B:
    apu(d, 0x4001u, 0x9Au);
    return;
F931:
    apu(d, 0x4001u, 0x93u);
    Z(0xF6) = 0x06u;
    goto F93A;
F948:
    Z(0xF0) = y; Z(0xF5) = 0x03u;
F94E:
    f86b(d, 0x00u, 0x9Cu, y);
    Z(0xF6) = 0x04u;
    goto F965;
F95B:
    Z(0xF6)--;
    if (Z(0xF6) == 0u)
        goto F969;
F95F:
    if (Z(0xF6) != 0x04u)
        return;
F965:
    apu(d, 0x4001u, 0x9Au);
    return;
F969:
    apu(d, 0x4001u, 0x93u);
    Z(0xF6) = 0x06u;
    goto F93A;
F96D:
    Z(0xF0) = y; Z(0xF5) = 0x02u;
    goto F94E;
F975:
    Z(0xF6)--;
    if (Z(0xF6) != 0u)
        goto F95F;
    apu(d, 0x4001u, 0x94u);
    Z(0xF6) = 0x06u;
    goto F93A;
F97D:
    Z(0xF0) = y; Z(0xF5) = 0x21u;
    f86b(d, 0x30u, entryX, 0xBCu);       /* X is whatever the NMI left in it */
F98A:
    a = Z(0xF5);
    if (a == 0x15u)
        apu(d, 0x4001u, 0x7Fu);
    c = (uint8_t)(a & 1u);
    {
        const uint8_t idx = (uint8_t)(a >> 1);
        uint8_t v = rd(d, 0xF9A3u - 1u + idx);
        if (!c)
            v = 0x13u;
        apu(d, 0x4000u, v);
    }
F93A:
    Z(0xF5)--;
    if (Z(0xF5) != 0u)
        return;
F93E:
    Z(0xF0) = 0u;
    apu(d, 0x4000u, 0x90u);
}

/* Shared tail FAF7: pulse-2 control, then triangle pitch from a frequency step. */
static void engineTail(SoundDriver *d, uint8_t a, uint8_t x, uint8_t y)
{
    uint8_t c, t0, t1, ax, ay, n;

    apu(d, 0x4004u, x);
    apu(d, 0x4005u, y);
    H(0x07F9u) = a;
    t0 = 0x07u; t1 = 0xFFu;
    n = (uint8_t)(a >> 5);
    while (n != 0u) {
        c = (uint8_t)(t0 & 1u); t0 >>= 1;
        t1 = (uint8_t)((c << 7) | (t1 >> 1));
        n--;
    }
    ax = (uint8_t)(t0 >> 1);
    c = (uint8_t)(t0 & 1u);
    ay = (uint8_t)((c << 7) | (t1 >> 1));
    H(0x07F9u) = (uint8_t)(H(0x07F9u) << 3);
    for (;;) {
        c = (uint8_t)(ax & 1u); ax >>= 1;
        ay = (uint8_t)((c << 7) | (ay >> 1));
        c = (uint8_t)(H(0x07F9u) >> 7);
        H(0x07F9u) = (uint8_t)(H(0x07F9u) << 1);
        if (c) {
            unsigned sum = (unsigned)t1 + (unsigned)(uint8_t)~ay + 1u;
            t1 = (uint8_t)sum;
            sum = (unsigned)t0 + (unsigned)(uint8_t)~ax + (sum >> 8);
            t0 = (uint8_t)sum;
        }
        if (H(0x07F9u) == 0u)
            break;
    }
    Z(0x01) = t1; Z(0x00) = t0;
    apu(d, 0x4006u, t1);
    apu(d, 0x4007u, t0);
    if (Z(0xF1) == 0u && (Z(0xF7) | Z(0xFA)) != 0u)
        return;
    {
        unsigned sum;
        c = (uint8_t)(t0 & 1u); t0 >>= 1;
        t1 = (uint8_t)((c << 7) | (t1 >> 1));
        Z(0x00) = t0; Z(0x01) = t1;
        ay = (uint8_t)(t0 >> 1);
        c = (uint8_t)(t0 & 1u);
        a = (uint8_t)((c << 7) | (t1 >> 1));
        sum = (unsigned)a + (unsigned)t1;
        apu(d, 0x400Au, (uint8_t)sum);
        sum = (unsigned)ay + (unsigned)t0 + (sum >> 8);
        apu(d, 0x400Bu, (uint8_t)sum);
        apu(d, 0x4008u, 0x04u);
    }
}

static void sub_FA74(SoundDriver *d)
{
    uint8_t y, a, x, c;
    int borrowSet;

    y = Z(0xFE);
    if (y != 0u)
        goto FA4F;
    y = Z(0xF1);
    if (y != 0u)
        goto FA55;
    a = (uint8_t)(Z(0xFC) & 0x7Fu);
    if (a != 0u)
        goto FA95;
    a = Z(0xF3);
    if (a == 0u)
        return;
    Z(0xF3) = 0u; Z(0xF7) = 0u; H(0x07F8u) = 0u;
FA8F:
    apu(d, 0x4004u, 0x90u);
    return;
FA95:
    y = Z(0xF7);
    Z(0xF7)++;
    if (y != H(0x07F8u))
        goto FACF;
    if (a >= 0x30u)
        goto FABF;
    borrowSet = a >= Z(0xF3);
    c = (uint8_t)borrowSet;
    a = (uint8_t)(a - Z(0xF3));
    if (a == 0u)
        goto FAC3;
    if (c)
        goto FAB3;
    Z(0xF3)--;
    if (a >= 0xF8u)
        goto FABB;
    Z(0xF3)--;
    if (Z(0xF3) != 0u)
        goto FABB;
FAB3:
    Z(0xF3)++;
    if (a < 0x08u)
        goto FABB;
    Z(0xF3)++;
FABB:
    a = 0x01u;
    goto FAC8;
FABF:
    a = 0x30u;
    Z(0xF3) = a;
FAC3:
    a = rd(d, 0xF9F4u + Z(0xF3));
FAC8:
    H(0x07F8u) = a;
    Z(0xF7) = 0u;
FACF:
    {
        const unsigned sum = (unsigned)H(0x07FFu) + 4u;
        if ((uint8_t)sum < 0x40u)
            H(0x07FFu) = (uint8_t)sum;
    }
    a = (uint8_t)(rd(d, 0xF9C4u + Z(0xF3)) + rd(d, 0xF9B3u + Z(0xF7)));
    x = Z(0xFC);
    if ((x & 0x80u) == 0u) {
        x = rd(d, 0xF9B9u + Z(0xF7));
        y = 0x7Fu;
    } else {
        x = rd(d, 0xF9BFu + Z(0xF7));
        y = 0x89u;
    }
    engineTail(d, a, x, y);
    return;

FA4F:
    y = 0x15u;
    Z(0xF1) = y;
    goto FA5E;
FA55:
    H(0x07F3u)--;
    if (H(0x07F3u) != 0u)
        goto FA8F;
    Z(0xF1)--;
    if (Z(0xF1) == 0u)
        goto FA8F;
FA5E:
    x = rd(d, 0xFA24u + y);
    a = (x == 0x80u) ? 0x02u : 0x01u;
    H(0x07F3u) = a;
    a = rd(d, 0xFA39u + y);
    engineTail(d, a, x, 0x7Fu);
}

static void sub_FC3D(SoundDriver *d)
{
    uint8_t y, a, c;

FC3D:
    y = Z(0xFD);
    a = Z(0xF2);
    c = (uint8_t)(Z(0xFD) & 1u); Z(0xFD) >>= 1; if (c) goto FBFF;
    c = (uint8_t)(a & 1u); a >>= 1; if (c) goto FC06;
    c = (uint8_t)(Z(0xFD) & 1u); Z(0xFD) >>= 1; if (c) goto FC1F;
    c = (uint8_t)(a & 1u); a >>= 1; if (c) goto FC2E;
    c = (uint8_t)(a & 1u); a >>= 1; if (c) goto FC63;
    c = (uint8_t)(Z(0xFD) & 1u); Z(0xFD) >>= 1; if (c) goto FC57;
    return;
FBFF:
    Z(0xF2) = y; H(0x07F6u) = 0x2Eu;
FC06:
    if (y != 0u)
        H(0x07F7u) = (uint8_t)(y & 0x02u);
    {
        const unsigned idx = (unsigned)H(0x07F6u) - 1u;
        apu(d, 0x400Cu, rd(d, 0xFB77u + idx));
        a = rd(d, 0xFBA5u + idx);
        apu(d, 0x400Eu, a);
        if (a != 0u)
            goto FC75;
    }
FC1F:
    Z(0xF2) = y; H(0x07F6u) = 0u; H(0x07F7u) = 0u;
    apu(d, 0x400Eu, 0x0Eu);
FC2E:
    a = rd(d, 0xFBD3u + (H(0x07F6u) >> 3));
    apu(d, 0x400Cu, a);
    if (a != 0u)
        goto FC75;
    goto FC3D;                            /* falls into sub_FC3D */
FC57:
    Z(0xF2) = y; H(0x07F6u) = 0x0Cu;
    apu(d, 0x400Eu, 0x0Du);
FC63:
    y = H(0x07F6u);
    apu(d, 0x400Cu, rd(d, 0xFBF2u + y));
    if (y == 0x08u)
        apu(d, 0x400Eu, 0x0Eu);
FC75:
    apu(d, 0x400Fu, 0x08u);
    H(0x07F6u)--;
    if (H(0x07F6u) != 0u)
        return;
    y = H(0x07F7u);
    if (y != 0u)
        goto FC1F;
    Z(0xF2) = 0u;
}

static void sub_FC8D(SoundDriver *d)
{
    uint8_t a, y, x, v;

    a = Z(0xFB);
    if (a == 0u) {
        if (Z(0xF4) == 0u)
            return;
        goto FCD9;
    }
    Z(0xF4) = a;
    y = 0u;
    do {
        y++;
        v = (uint8_t)(a & 1u);
        a >>= 1;
    } while (!v);
    y = rd(d, 0xFDD3u - 1u + y);
    H(0x07F0u) = rd(d, 0xFDDBu - 8u + y);
    Z(0xF8) = rd(d, 0xFDDBu - 8u + 1u + y);
    Z(0xF9) = rd(d, 0xFDDBu - 8u + 2u + y);
    H(0x07E0u) = rd(d, 0xFDDBu - 8u + 3u + y);
    Z(0xFA) = rd(d, 0xFDDBu - 8u + 4u + y);
    H(0x07ECu) = rd(d, 0xFDDBu - 8u + 5u + y);
    H(0x07E5u) = 1u; H(0x07E1u) = 1u; H(0x07E9u) = 1u; H(0x07EDu) = 1u;
    H(0x07E4u) = 0u;
    H(0x07FBu)++;
FCD9:
#define SEQ(idx) rd(d, (unsigned)((Z(0xF9) << 8) | Z(0xF8)) + (uint8_t)(idx))
    y = H(0x07E0u);
    if (y == 0u)
        goto FD1A;
    H(0x07E1u)--;
    if (H(0x07E1u) != 0u)
        goto FD1A;
    H(0x07E0u)++;
    v = SEQ(y);
    a = f53(d, v);
    H(0x07E1u) = a;
    if (noteWrite(d, (uint8_t)(v & 0x3Eu), 0u)) {
        x = 0x4Fu;
        if ((Z(0xF4) & 0x60u) == 0u)
            x = 0x05u;
    } else {
        x = 0x10u;
    }
    apu(d, 0x4000u, x);
    y = 0x7Fu;
    if (Z(0xF4) & 1u)
        y = rd(d, 0xFC89u + (H(0x07FBu) & 3u));
    apu(d, 0x4001u, y);
FD1A:
    H(0x07E5u)--;
    if (H(0x07E5u) != 0u)
        goto FD63;
    y = H(0x07E4u);
    H(0x07E4u)++;
    v = SEQ(y);
    if (v == 0u) {
        Z(0xFA) = 0u; Z(0xF4) = 0u;
        apu(d, 0x4000u, 0x90u);           /* JMP loc_F942 */
        return;
    }
    a = f53(d, v);
    H(0x07E5u) = a;
    if ((Z(0xF4) & 0x60u) != 0u)
        goto FD63;
    if (noteWrite(d, (uint8_t)(v & 0x3Eu), 4u)) {
        x = 0x87u;
        if (a < 0x10u) {
            x = 0x84u;
            if (a < 0x08u)
                x = 0x82u;
        }
    } else {
        x = 0x10u;
    }
    apu(d, 0x4004u, x);
    apu(d, 0x4005u, 0x7Fu);
FD63:
    y = Z(0xFA);
    if (y == 0u)
        goto FD99;
    H(0x07E9u)--;
    if (H(0x07E9u) != 0u)
        goto FD99;
    Z(0xFA)++;
    v = SEQ(y);
    a = f53(d, v);
    H(0x07E9u) = a;
    a = (uint8_t)(a - 1u);
    if (a >= 0x0Cu)
        a = 0x0Cu;
    a = (uint8_t)(a << 2);
    if (Z(0xF4) == 0x20u)
        a = 0x81u;
    apu(d, 0x4008u, a);
    if (!noteWrite(d, (uint8_t)(v & 0x3Eu), 8u))
        apu(d, 0x4008u, 0u);
FD99:
    if ((Z(0xF4) & 0x1Bu) == 0u)
        return;
    H(0x07EDu)--;
    if (H(0x07EDu) != 0u)
        return;
    y = H(0x07ECu);
    H(0x07ECu)++;
    v = SEQ(y);
    a = f53(d, v);
    H(0x07EDu) = a;
    a = (uint8_t)(v & 0x3Eu);
    if (a == 0u)
        return;
    apu(d, 0x400Cu, 0u);
    apu(d, 0x400Eu, a == 0x20u ? 0x02u : 0x00u);
    apu(d, 0x400Fu, 0x08u);
#undef SEQ
}

void soundDriverReset(SoundDriver *d, const uint8_t *data)
{
    memset(d, 0, sizeof(*d));
    d->data = data;
}

void soundDriverSetRequests(SoundDriver *d, uint8_t fb, uint8_t fc,
                            uint8_t fd, uint8_t fe, uint8_t ff)
{
    Z(0xFB) = fb; Z(0xFC) = fc; Z(0xFD) = fd; Z(0xFE) = fe; Z(0xFF) = ff;
}

void soundDriverFrame(SoundDriver *d)
{
    d->write_count = 0u;
    apu(d, 0x4017u, 0xC0u);
    sub_F8D4(d, 2u);
    sub_FA74(d);
    sub_FC3D(d);
    sub_FC8D(d);
    Z(0xFF) = 0u; Z(0xFE) = 0u; Z(0xFD) = 0u; Z(0xFB) = 0u;
    if (H(0x07FFu) != 0u) {
        apu(d, 0x4011u, H(0x07FFu));
        H(0x07FFu)--;
    }
}
