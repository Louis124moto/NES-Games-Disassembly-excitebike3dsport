/*
 * Results-screen phase handlers (bank_FF.asm $CB2C-$CCDA). Labels in comments
 * are the source labels; carry-flag hand-offs that the source relies on
 * (CC15 -> CC7D -> sub_DD6F's ADC) are tracked explicitly.
 */
#include "race_results.h"
#include "nes_routines.h"

#define B(a) (*raceWorldByte(w, (a)))

static const uint8_t kC0DA[4] = { 0x78u, 0x70u, 0x80u, 0xB0u };
static const uint8_t kC0DE[4] = { 0x37u, 0x3Fu, 0x3Fu, 0x47u };
static const uint8_t kC0E2[3] = { 0xB7u, 0xB9u, 0xB9u };
static const uint8_t kC0E5[3] = { 0x01u, 0x01u, 0x41u };
static const uint8_t kC0E8[4] = { 0x04u, 0x0Cu, 0x14u, 0x1Cu };
static const uint8_t kC0EC[10] = { 0x21u, 0xF2u, 0x23u, 0x43u, 0x22u, 0x32u, 0x22u, 0x8Cu, 0x24u, 0xCFu };
static const uint8_t kC134[8] = { 0x00u, 0xFCu, 0xFCu, 0xFCu, 0x79u, 0x04u, 0x72u, 0x22u };

#define CON_0045_0A 0x0Au
#define CON_0045_0B 0x0Bu
#define CON_0045_0D 0x0Du
#define CMD_LIMIT 0x30u /* keep every command byte inside $0300-$033F */

/* sub_C3AF falls into sub_C3B1: INC $0041, then $0040 = $0044 = 0. */
static void subC3AF(RaceWorld *w)
{
    B(0x41u) = (uint8_t)(B(0x41u) + 1u);
    B(0x40u) = 0u;
    B(0x44u) = 0u;
}

/* sub_CC6B / ofs_CC6B_03 */
static void subCC6B(RaceWorld *w)
{
    uint8_t a = 0u;
    subC3AF(w);
    if (B(0x4Bu) != 0u)
        return;
    if (B(0x41u) >= 0x09u)
        a = 0x08u;
    B(0x41u) = a;
}

/* sub_DFA4 with the carry the source leaves behind (SBC loop, then ADC). */
static void divmod10(uint8_t a, uint8_t *quot, uint8_t *rem, uint8_t *carry)
{
    uint8_t x = 0u, c;
    do {
        c = (uint8_t)(a >= 10u);
        a = (uint8_t)(a - 10u);
        x = (uint8_t)(x + 1u);
    } while ((a & 0x80u) == 0u);
    x = (uint8_t)(x - 1u);
    {
        const unsigned sum = (unsigned)a + 10u + c;
        *rem = (uint8_t)sum;
        *carry = (uint8_t)(sum > 0xFFu);
    }
    *quot = x;
}

/* sub_CCCA -> loc_D1C7: one 2x1 podium/marker sprite pair in OAM. */
static void subCCCA(RaceWorld *w, uint8_t x, uint8_t t01, uint8_t t02, uint8_t t0B)
{
    NesSubD1C7OamGridState g;
    g.ram0000_x = kC0DA[x];
    g.ram0001_y = t01;
    g.ram0002_tile = t02;
    g.ram0003_grid = 0x21u;
    g.ram0004_oamBase = kC0E8[x];
    g.ram0008_maxIndex = 0u;
    g.ram0009_y = 0u;
    g.ram000A_markerBits = 0u;
    g.ram000B_attribute = t0B;
    (void)nesSubD1C7BuildOamGrid(raceWorldByte(w, 0x200u), &g);
}

/* sub_CC15: returns the carry flag it leaves for loc_CC7D. */
static uint8_t subCC15(RaceWorld *w)
{
    const uint8_t a = (uint8_t)(B(0x3ADu) - 0x10u);
    uint8_t carry;

    if (a == B(0x33u) && B(0x20u) == 0u) {
        uint8_t y;
        carry = 1u;                                /* CMP equal sets carry */
        y = (uint8_t)(B(0x52u) - 1u);
        if (y != 0u) {
            B(0x45u) = CON_0045_0D;
        } else {
            const unsigned sum = (unsigned)B(0x4Bu) + CON_0045_0B;
            B(0x45u) = (uint8_t)sum;
            carry = (uint8_t)(sum > 0xFFu);
        }
        return carry;
    }

    /* bra_CC32: append the 4-tile rank text command and the rank digits. */
    {
        const unsigned x = B(0x300u);
        uint8_t y, q, r, q2, r2, c;
        unsigned k;
        carry = (uint8_t)((7u + x) > 0xFFu);       /* sub_C971 */
        B(0x300u) = (uint8_t)(x + 7u);
        for (k = 0u; k < 8u; ++k)
            B(0x301u + x + k) = kC134[7u - k];
        if ((B(0x3Fu) & 0x10u) != 0u)
            return carry;
        y = (uint8_t)x;
        divmod10(B(0x53u), &q, &r, &c);
        B(0x307u + y) = r;
        divmod10(q, &q2, &r2, &carry);
        if (q2 != 0u) {
            B(0x305u + y) = q2;
            B(0x306u + y) = r2;
        } else if (r2 != 0u) {
            B(0x306u + y) = r2;
        }
        return carry;
    }
}

/* loc_CC7D */
static void locCC7D(RaceWorld *w, uint8_t carry)
{
    uint8_t t01 = B(0x55u), t02, x;

    if (B(0x56u) == 0u) {
        if (B(0x2Fu) != 0u) {
            B(0x380u) = 0x1Fu;
            B(0x38Cu) = 0x1Fu;
            B(0x378u) = 0xAFu;
            B(0x37Cu) = 0x01u;
            B(0x384u) = 0x00u;
            t02 = 0xBDu;
            goto CCBB;
        }
        /* CCA3 */
        {
            const uint8_t a = nesSubDD6F(0u, carry, raceWorldByte(w, 0x380u),
                                         raceWorldByte(w, 0x38Cu), raceWorldByte(w, 0x384u),
                                         raceWorldByte(w, 0x37Cu), t01);
            B(0x55u) = a;
            t01 = a;
            if (B(0x3F1u) < a) {
                B(0x55u) = B(0x3F1u);
                B(0x2Fu) = 0x0Au;
            }
        }
    }
    t02 = 0xBBu;                                   /* bra_CCB9 */
CCBB:
    x = (uint8_t)(B(0x53u) - 1u);
    if (x >= 3u)
        x = 3u;
    subCCCA(w, x, t01, t02, 0x02u);
}

/* ofs_CB3B_00 */
static void phase0(RaceWorld *w)
{
    if (B(0x41u) < 0x08u) {
        B(0x42u) = B(0x3F2u);
        if (B(0x53u) < 0x04u) {
            if (B(0x46u) != 0u) {
                const uint8_t course = B(0x43u);
                uint8_t carry;
                if (course == 0x04u) {
                    B(0x3F3u) = (uint8_t)(B(0x3F3u) + 1u);
                    carry = 1u;
                    if ((B(0x3F3u) & 1u) == 0u) {
                        const uint8_t half = (uint8_t)(B(0x3F8u) >> 1);
                        if (half != 0u)
                            B(0x3F8u) = half;      /* then SEC */
                    }
                } else {
                    carry = (uint8_t)(course >= 0x04u);
                }
                if (carry == 0u)
                    B(0x43u) = (uint8_t)(B(0x43u) + 1u);
            }
            B(0x46u) = 1u;
            B(0x4Bu) = 1u;
        }
    }
    B(0x33u) = 0x04u;                              /* bra_CB9F */
    B(0x44u) = (uint8_t)(B(0x44u) + 1u);
}

/* ofs_CB77_01 */
static unsigned phase1(RaceWorld *w)
{
    unsigned x, y;
    uint8_t a;
    if (B(0x33u) != 0u)
        return 0u;
    x = B(0x300u);
    if (x > CMD_LIMIT)
        return 1u;
    for (y = 0u; y != 7u; ++y)
        B(0x304u + x + y) = B(0x3D9u + y);
    /* sub_C94B with table index 4 (PPU $2232) */
    B(0x300u) = (uint8_t)(x + 0x0Au);
    B(0x301u + x) = kC0EC[4];
    B(0x302u + x) = kC0EC[5];
    B(0x305u + x) = 0xFBu;
    B(0x308u + x) = 0xFBu;
    B(0x303u + x) = 0x07u;
    B(0x30Bu + x) = 0x00u;
    a = B(0x4Bu) != 0u ? 0x20u : 0x31u;
    B(0x3ADu) = a;
    B(0x33u) = (uint8_t)(a + 3u);
    B(0x44u) = (uint8_t)(B(0x44u) + 1u);
    return 0u;
}

/* ofs_CBA4_02 */
static unsigned phase2(RaceWorld *w)
{
    const uint8_t timer = B(0x33u);
    int x;

    if (B(0x300u) > CMD_LIMIT)
        return 1u;
    if (timer == 0u) {                             /* bra_CB9F with A = 0 */
        B(0x33u) = 0u;
        B(0x44u) = (uint8_t)(B(0x44u) + 1u);
        return 0u;
    }
    if (timer == B(0x3ADu)) {                      /* bra_CBC9 */
        uint8_t a, t00;
        if (B(0x20u) != 0u)
            return 0u;
        B(0x2Fu) = 0x03u;
        for (x = 2; x >= 0; --x)
            subCCCA(w, (uint8_t)x, kC0DE[x], kC0E2[x], kC0E5[x]);
        a = 0x01u;
        {
            uint8_t xr = (uint8_t)(B(0x53u) - 1u);
            uint8_t t0D = 0x03u;
            t00 = xr;
            if (xr < 0x03u) {
                t0D = xr;
                a = 0x02u;
            }
            B(0xFBu) = a;
            B(0x56u) = (uint8_t)(a ^ 0x02u);
            B(0x54u) = kC0DA[t0D];
            B(0x55u) = kC0DE[t0D];
            B(0x3F1u) = kC0DE[t0D];
        }
        if (t00 == 0u)
            B(0x45u) = CON_0045_0A;
        return 0u;
    }
    if (timer > B(0x3ADu))                         /* BCS: timer above the wait value */
        return 0u;
    if (B(0x4Bu) == 0u && (B(0x14u) & 0xF0u) != 0u) {   /* bra_CBBF */
        subCC6B(w);
        if (B(0x41u) == 0u)
            subC3AF(w);
        return 0u;
    }
    {
        const uint8_t carry = subCC15(w);
        locCC7D(w, carry);
    }
    return 0u;
}

unsigned raceResultsPhaseStep(RaceWorld *w)
{
    if (w == 0)
        return 1u;
    switch (B(0x44u)) {
    case 0u: phase0(w); return 0u;
    case 1u: return phase1(w);
    case 2u: return phase2(w);
    case 3u: subCC6B(w); return 0u;
    default: return 1u;
    }
}
