#include "nes_routines.h"
#include <stddef.h>

/*
 * Mirrors the 6502 instruction sequence:
 *
 *   LDA ram_0390_obj,X
 *   SEC
 *   SBC ram_0080_obj,X
 *   SBC #$08
 *   ASL
 *   CLC
 *   ADC ram_obj_pos_Y_lo,X
 *   ADC #$04
 *   STA ram_006C_obj,X
 *
 * Keep each accumulator result at 8 bits. In particular, the second SBC
 * consumes the carry/no-borrow flag produced by the first SBC, and the final
 * ADC #$04 consumes the carry produced by ADC ramY.
 */
uint8_t nesSubDA15DerivedY(uint8_t ram0390, uint8_t ram0080, uint8_t ramY)
{
    uint8_t accumulator;
    uint8_t carry;

    /* SEC; SBC ram_0080_obj,X */
    accumulator = (uint8_t)(ram0390 - ram0080);
    carry = (ram0390 >= ram0080) ? 1u : 0u;

    /* SBC #$08 consumes the previous SBC carry. */
    accumulator = (uint8_t)(accumulator - 0x08u - (carry ? 0u : 1u));

    /* ASL A is an 8-bit shift; CLC then precedes ADC ramY. */
    accumulator = (uint8_t)(accumulator << 1);
    {
        unsigned int sum = (unsigned int)accumulator + (unsigned int)ramY;
        accumulator = (uint8_t)sum;
        carry = (sum > 0xFFu) ? 1u : 0u;
    }

    /* ADC #$04 consumes the carry from ADC ramY. */
    accumulator = (uint8_t)(accumulator + 0x04u + carry);

    return accumulator;
}


/* Exact byte-level translation of sub_D310 ($D310).
 * timer array index 0 maps to RAM $0021; index 28 maps to RAM $003D.
 * BPL tests bit 7 of DEC's 8-bit result. */
void nesSubD310DecreaseAllTimers(uint8_t *timer0020, uint8_t timers[29])
{
    uint8_t frame_timer = (uint8_t)(*timer0020 - 1u);
    unsigned int count;
    unsigned int i;

    *timer0020 = frame_timer;
    if ((frame_timer & 0x80u) != 0u) {
        *timer0020 = 0x0Au;
        count = 29u;
    } else {
        count = 15u;
    }

    for (i = 0u; i < count; ++i) {
        if (timers[i] != 0u) {
            timers[i] = (uint8_t)(timers[i] - 1u);
        }
    }
}


/*
 * Exact byte-level translation of sub_D326 ($D326).
 * The initial carry is set when bit 1 differs between RAM $0018 and $0019.
 * ROR then propagates carry through the eight consecutive bytes $0018-$001F.
 */
void nesSubD326RotateInputBits(uint8_t bytes[8])
{
    unsigned int i;
    unsigned int carry = (((bytes[0] & 0x02u) ^ (bytes[1] & 0x02u)) != 0u) ? 1u : 0u;

    for (i = 0u; i < 8u; ++i) {
        unsigned int next_carry = (unsigned int)(bytes[i] & 0x01u);
        bytes[i] = (uint8_t)((bytes[i] >> 1) | (uint8_t)(carry << 7));
        carry = next_carry;
    }
}


/*
 * RAM-level translation of sub_D35B ($D35B), excluding the physical $4016/$4017
 * reads. Each serial read contributes a 1 if either bit 0 or bit 1 is set,
 * matching LSR / ORA saved sample / LSR carry behavior. Eight ROL operations
 * assemble the standard NES bit order: A, B, Select, Start, Up, Down, Left, Right.
 */
void nesSubD35BDecodeController(const uint8_t serial_samples[8], uint8_t previous_hold,
                                uint8_t *hold, uint8_t *press)
{
    uint8_t buttons = 0u;
    unsigned int i;

    for (i = 0u; i < 8u; ++i) {
        uint8_t sample = serial_samples[i];
        uint8_t bit = (uint8_t)(((sample | (sample >> 1)) & 1u) != 0u);
        buttons = (uint8_t)((buttons << 1) | bit);
    }

    *hold = buttons;
    *press = buttons;
    if ((buttons & 0x80u) != 0u && (previous_hold & 0x80u) != 0u) {
        *press = (uint8_t)(buttons & 0x7Fu);
    }
}


/*
 * Exact byte-level translation of sub_DA7A ($DA7A).
 *
 * The original starts X=$03 and processes X=$03,$02,$01; slot zero is
 * deliberately excluded. For each active slot, the first ADC updates the
 * high byte on carry, then SEC/SBC subtracts the global RAM[$0060] value
 * from the low byte and decrements the high byte on borrow. The final SBC
 * independently updates RAM[$0390+X]. All stored values wrap to 8 bits.
 */
void nesSubDA7AUpdateScrollSlots(uint8_t active[4], const uint8_t delta[4],
                                uint8_t posXLo[4], uint8_t posXHi[4],
                                uint8_t ram0390[4])
{
    int x;

    for (x = 3; x >= 1; --x) {
        uint8_t accumulator;
        unsigned int sum;
        int difference;

        if (active[x] == 0u) {
            continue;
        }

        /* LDA delta,X; CLC; ADC posXLo,X */
        sum = (unsigned int)delta[x] + (unsigned int)posXLo[x];
        accumulator = (uint8_t)sum;
        if (sum > 0xFFu) {
            posXHi[x] = (uint8_t)(posXHi[x] + 1u);
        }

        /* SEC; SBC delta[0] -- carry set means no extra subtraction. */
        difference = (int)accumulator - (int)delta[0];
        accumulator = (uint8_t)difference;
        posXLo[x] = accumulator;
        if (difference < 0) {
            posXHi[x] = (uint8_t)(posXHi[x] - 1u);
        }

        /* LDA ram_0390_obj,X; SEC; SBC ram_0060; STA ram_0390_obj,X */
        difference = (int)ram0390[x] - (int)delta[0];
        ram0390[x] = (uint8_t)difference;
    }
}

/* Exact instruction-level translation of sub_DBFE ($DBFE). */
void nesSubDBFEUpdateScroll(uint8_t ram0060, uint8_t *ram03BF,
                            uint8_t *scrollX, uint8_t *baseNametable)
{
    uint8_t a = ram0060;
    unsigned int carry = (unsigned int)(a & 1u);
    unsigned int i, sum;
    a = (uint8_t)(a >> 1); /* LSR */
    for (i = 0u; i < 3u; ++i) {
        unsigned int next = (unsigned int)(a & 1u);
        a = (uint8_t)((a >> 1) | (uint8_t)(carry << 7));
        carry = next;
    }
    sum = (unsigned int)a + (unsigned int)*ram03BF; /* CLC; ADC */
    *ram03BF = (uint8_t)sum;
    carry = sum > 0xFFu ? 1u : 0u;
    sum = (unsigned int)ram0060 + (unsigned int)*scrollX + carry; /* LDA preserves C */
    *scrollX = (uint8_t)sum;
    if (sum > 0xFFu) *baseNametable = (uint8_t)(*baseNametable ^ 1u);
}

/* sub_DC97/sub_DC9A: LDA ram_03F1; SEC; SBC ram_00BC_obj,X; SBC ram_obj_pos_Y_lo,X. */
uint8_t nesSubDC9A(uint8_t ram03F1, uint8_t ram00BC, uint8_t ramY)
{
    int d = (int)ram03F1 - (int)ram00BC;
    uint8_t a = (uint8_t)d;
    unsigned int carry = d >= 0 ? 1u : 0u;
    d = (int)a - (int)ramY - (int)(1u - carry);
    return (uint8_t)d;
}

/* sub_DCDE: DEC RAM[$0374+X] before checking RAM[$005C+X] & $C0. */
void nesSubDCDE(uint8_t *timer0374, uint8_t ram005C)
{
    if (*timer0374 == 0u) return;
    *timer0374 = (uint8_t)(*timer0374 - 1u);
    if ((ram005C & 0xC0u) != 0u) *timer0374 = 0u;
}

/* Exact RAM-level translation of sub_DD38 ($DD38), including ROR carry. */
void nesSubDD38SetupObject(uint8_t x, uint8_t ram0090[4], uint8_t ram0094[4],
                           uint8_t ram0388[4], uint8_t ram0378[4],
                           uint8_t ram037C[4], uint8_t ram0380[4],
                           uint8_t ram00B0[4], uint8_t *ram00FF)
{
    unsigned int sum, carry;
    uint8_t a;
    if (x > 3u) return;
    ram00B0[x] = 0x02u;
    ram0380[x] = 0x0Fu;
    sum = (unsigned int)ram0090[x] + 0xAFu;
    ram0378[x] = (uint8_t)sum;
    carry = sum > 0xFFu ? 1u : 0u;
    sum = (unsigned int)ram0094[x] + carry;
    ram037C[x] = (uint8_t)sum;
    if (ram0388[x] == 0x02u) {
        carry = (unsigned int)(ram037C[x] & 1u);
        ram037C[x] = (uint8_t)(ram037C[x] >> 1);
        ram0378[x] = (uint8_t)((ram0378[x] >> 1) | (uint8_t)(carry << 7));
    }
    if (x == 0u) {
        a = (uint8_t)(ram0094[x] << 4);
        a = (uint8_t)(a ^ 0x30u);
        *ram00FF = a != 0u ? a : 0x08u;
    }
}

/* sub_DD6F: incoming carry is an explicit input because ADC uses it unchanged. */
uint8_t nesSubDD6F(uint8_t x, uint8_t carryIn, uint8_t ram0380[4],
                   uint8_t ram038C[4], uint8_t ram0384[4],
                   uint8_t ram037C[4], uint8_t temp0001)
{
    unsigned int sum, carry;
    int d;
    uint8_t a;
    if (x > 3u) return 0u;
    sum = (unsigned int)ram0380[x] + ram038C[x] + (unsigned int)(carryIn != 0u);
    ram0380[x] = (uint8_t)sum;
    carry = sum > 0xFFu ? 1u : 0u;
    sum = (unsigned int)ram0384[x] + carry;
    ram0384[x] = (uint8_t)sum;
    carry = sum > 0xFFu ? 1u : 0u;
    d = (int)temp0001 - (int)ram037C[x] - (int)(1u - carry);
    a = (uint8_t)d;
    carry = d >= 0 ? 1u : 0u;
    sum = (unsigned int)a + ram0384[x] + carry;
    return (uint8_t)sum;
}

/* sub_DB95 clears only the 20 fields listed in the original assembly. */
void nesSubDB95ClearOpponentBikeData(uint8_t x, uint8_t ram00A8[4],
    uint8_t ram00B8[4], uint8_t ram00BC[4], uint8_t ram00A0[4],
    uint8_t ram00A4[4], uint8_t ram0058[4], uint8_t ram00CC[4],
    uint8_t ram036C[4], uint8_t ram0090[4], uint8_t ram0094[4],
    uint8_t ram0098[4], uint8_t ram009C[4], uint8_t ram00B0[4],
    uint8_t ram00D4[4], uint8_t ram0388[4], uint8_t ram00C0[4],
    uint8_t ram039C[4], uint8_t ram00DC[4], uint8_t ram0070[4],
    uint8_t ram0370[4])
{
    if (x == 0u || x > 3u) return;
    ram00A8[x] = ram00B8[x] = ram00BC[x] = ram00A0[x] = ram00A4[x] = 0u;
    ram0058[x] = ram00CC[x] = ram036C[x] = ram0090[x] = ram0094[x] = 0u;
    ram0098[x] = ram009C[x] = ram00B0[x] = ram00D4[x] = ram0388[x] = 0u;
    ram00C0[x] = ram039C[x] = ram00DC[x] = ram0070[x] = ram0370[x] = 0u;
}


/* Exact translation of sub_DA6A ($DA6A). */
void nesSubDA6AUpdateScroll(uint8_t *scrollX, uint8_t delta,
                            uint8_t *baseNametable)
{
    unsigned int sum = (unsigned int)*scrollX + (unsigned int)delta;
    *scrollX = (uint8_t)sum;
    if (sum > 0xFFu) {
        *baseNametable = (uint8_t)(*baseNametable ^ 0x01u);
    }
}

/* sub_DB50 ($DB50): table bytes are supplied from the original ROM tables. */
void nesSubDB50SetupObject(uint8_t x, uint8_t y, uint8_t trackFinished,
                           uint8_t ram0084[4], uint8_t ram001B[4],
                           uint8_t ram0070[4], uint8_t ram0078[4],
                           uint8_t ram0074[4], uint8_t ram0080[4],
                           uint8_t ram0094Player, uint8_t ram0090Player,
                           const uint8_t tblD8D5[256], const uint8_t tblD8CD[16])
{
    uint8_t value;
    if (x > 3u) return;

    if (trackFinished != 0u) {
        value = 0x03u;
    } else if (x == 3u && (ram0084[x] & 0x02u) == 0u && ram0094Player != 0u &&
               (ram0084[x] == 0u || ram0080[x] < 0x40u)) {
        ram0078[x] = ram0094Player;
        ram0074[x] = ram0090Player;
        if (ram0084[x] != 0u && ram0070[0] == ram0070[3]) return;
        if (ram0078[x] < 0x04u) ram0078[x] = (uint8_t)(ram0078[x] + 1u);
        return;
    } else {
        value = tblD8D5[y];
    }

    ram0078[x] = value;
    value = (uint8_t)(ram001B[x] & 0x0Fu);
    ram0074[x] = tblD8CD[value];
}

/* Exact RAM-level translation of sub_DCA0 ($DCA0). */
void nesSubDCA0SetupSpriteHeight(uint8_t x, uint8_t ram00D4[4],
    uint8_t trackFinished, uint8_t ram0098Player, uint8_t ram009CPlayer,
    uint8_t ram0058Player, uint8_t ram00A4[4], uint8_t tblD88B[256],
    uint8_t ram0368[4])
{
    uint8_t value;
    if (x > 3u) return;
    if (trackFinished == 1u && (x | ram0098Player | ram009CPlayer | ram0058Player) == 0u) {
        value = 0x0Au;
    } else {
        value = tblD88B[ram00D4[x]];
    }
    ram0368[x] = value;
    if (ram00A4[x] == 1u) ram0368[x] = 0x06u;
}


/* Exact translation of the shared entry loc_DCC7 ($DCC7).
 * A supplies target; Y supplies the timer value to write. */
void nesLocDCC7ApproachValue(uint8_t x, uint8_t target, uint8_t duration,
                             uint8_t timers[4], uint8_t values[4])
{
    uint8_t current;
    if (x > 3u || timers[x] != 0u) return;
    timers[x] = duration;
    current = values[x];
    if (current == target) return;
    if (current > target) {
        current = (uint8_t)(current - 1u);
        current = (uint8_t)(current - 1u);
    }
    values[x] = (uint8_t)(current + 1u);
}


/* Exact byte/carry translation of sub_DA58 ($DA58). */
void nesSubDA58UpdateObjectFraction(uint8_t x, uint8_t ram0094[4],
                                    uint8_t ram0060[4], uint8_t ram0090[4],
                                    uint8_t ram0394[4])
{
    unsigned int sum;
    if (x > 3u) return;
    ram0060[x] = ram0094[x];
    sum = (unsigned int)ram0090[x] + (unsigned int)ram0394[x];
    ram0394[x] = (uint8_t)sum;
    if (sum > 0xFFu) ram0060[x] = (uint8_t)(ram0060[x] + 1u);
}

/* CMP #$E7; if below, ADC #$0C with CMP's carry clear. */
uint8_t nesSubE091AdjustValue(uint8_t value)
{
    if (value >= 0xE7u) return value;
    return (uint8_t)((unsigned int)value + 0x0Cu);
}

/* CMP #$E7; if below, ADC #$02 with CMP's carry clear. */
uint8_t nesSubE098AdjustValue(uint8_t value)
{
    if (value >= 0xE7u) return value;
    return (uint8_t)((unsigned int)value + 0x02u);
}


/*
 * Exact instruction-level model of sub_DFA4 ($DFA4).
 * Preconditions: divisor != 0 and both output pointers are valid.
 * The loop terminates on the 6502 BPL condition (N == 0), not on borrow.
 */
void nesSubDFA4(uint8_t inputA, uint8_t divisor,
                uint8_t *outX, uint8_t *outA)
{
    uint8_t a = inputA;
    uint8_t x = 0u;
    uint8_t carry = 0u;

    while (1) {
        uint16_t difference;
        x = (uint8_t)(x + 1u);
        difference = (uint16_t)a - (uint16_t)divisor;
        a = (uint8_t)difference;
        carry = (uint8_t)(difference <= 0xFFu);
        if ((a & 0x80u) != 0u) break;
    }

    x = (uint8_t)(x - 1u);
    {
        uint16_t sum = (uint16_t)a + (uint16_t)divisor + (uint16_t)carry;
        a = (uint8_t)sum;
    }
    *outX = x;
    *outA = a;
}

/* sub_DFA2 ($DFA2): LDX #$0A, then fall through into sub_DFA4. */
void nesSubDFA2(uint8_t inputA, uint8_t *outX, uint8_t *outA)
{
    nesSubDFA4(inputA, 0x0Au, outX, outA);
}


/* sub_D0AB: emulate SBC sign tests and the Y countdown for all four objects. */
void nesSubD0ABComputeVerticalBands(const uint8_t ram00B8[4], uint8_t ram0360[4])
{
    int x;
    for (x = 3; x >= 0; --x) {
        uint8_t a = (uint8_t)(ram00B8[x] - 0x10u);
        uint8_t y = 5u;
        if ((a & 0x80u) == 0u) {
            for (;;) {
                y = (uint8_t)(y - 1u);
                if (y == 0u) break;
                a = (uint8_t)(a - 0x08u);
                if ((a & 0x80u) != 0u) break;
            }
        }
        ram0360[x] = y;
    }
}

/*
 * sub_DFB2: choose the Y band using CMP's N flag (signed result bit),
 * not a conventional unsigned comparison.
 */
void nesSubDFB2SelectBand(uint8_t x, const uint8_t ram009C[4],
                          const uint8_t ram00B8[4], const uint8_t ram00A4[4],
                          uint8_t ram0070[4], const uint8_t tblD913[5])
{
    uint8_t y = 0u;
    uint8_t a;
    if (x > 3u) return;
    if (ram009C[x] == 0u) {
        a = ram00B8[x];
        for (;;) {
            uint8_t cmp = (uint8_t)(a - tblD913[y]);
            if ((cmp & 0x80u) != 0u) break;
            y = (uint8_t)(y + 1u);
            if ((uint8_t)(y - 5u) & 0x80u) continue;
            y = 0u;
            break;
        }
        if (y != 0u && ram00A4[x] == 1u) {
            y = (uint8_t)(y + 2u);
        }
    }
    ram0070[x] = y;
}


/* sub_D000: reproduce the gate and Y value before the jump to sub_DB50.
 * x must be a valid object slot (0..3); caller invokes sub_DB50 when true. */
uint8_t nesSubD000ShouldSetupObject(uint8_t x, const uint8_t ram0080[4],
                                    const uint8_t ram001A[4], uint8_t *outY)
{
    if (x > 3u || outY == NULL) return 0u;
    *outY = 0u; /* LDY #$00 */
    if (x == 3u) return 1u;
    if ((ram0080[x] & 0x10u) == 0u) return 0u;
    if (ram001A[x] < 0xC0u) return 0u;
    if (ram001A[x] == 0xC0u) *outY = 1u; /* INY only for exact equality */
    return 1u;
}

/* sub_D11F: exact writes to RAM $0300..$0312. */
void nesSubD11FInitializeUpdateBuffer(uint8_t ram0300_to_0312[19])
{
    uint8_t x;
    ram0300_to_0312[0] = 0x11u;
    ram0300_to_0312[1] = 0x23u;
    ram0300_to_0312[2] = 0xA0u;
    ram0300_to_0312[3] = 0x0Eu;
    for (x = 0x0Eu; x != 0u; --x) {
        ram0300_to_0312[3u + x] = 0xFEu;
    }
    ram0300_to_0312[0x12u] = 0u; /* X is zero after the loop */
}

/* sub_D255: Y starts at zero, wraps through all 256 byte values, and writes
 * $F8 to every OAM byte. */
void nesSubD255HideAllSprites(uint8_t oam[256])
{
    unsigned int y;
    for (y = 0u; y < 256u; ++y) oam[y] = 0xF8u;
}

/* sub_D2FF: INX/CMP #$3F; on the overflow path the routine resets X,
 * clears RAM[$0301+X], and discards two return addresses from the stack. */
void nesSubD2FFAdvanceBufferIndex(uint8_t *x, uint8_t ram0300,
                                 uint8_t ram0301_to_0400[256],
                                 uint8_t *stackBytesDiscarded)
{
    uint8_t next;
    if (x == NULL || stackBytesDiscarded == NULL) return;
    next = (uint8_t)(*x + 1u);
    *x = next;
    if (next < 0x3Fu) return;
    *x = ram0300;
    ram0301_to_0400[ram0300] = 0u;
    *stackBytesDiscarded = (uint8_t)(*stackBytesDiscarded + 2u);
}

/* sub_E70B: branch on the SBC result's N/Z flags, not C or V. */
void nesSubE70BUpdateObjectOffsets(uint8_t ram0060[4], uint8_t ram0064[4],
                                   uint8_t ram00E0[4], uint8_t ram00D8[4])
{
    int x;
    for (x = 3; x >= 0; --x) {
        uint8_t a;
        if (ram0060[x] == 0u) {
            ram00D8[x] = 0u;
            continue;
        }
        a = (uint8_t)(ram0064[x] - ram0060[x]); /* SEC; SBC */
        if (a == 0u || (a & 0x80u) != 0u) {
            ram0064[x] = (uint8_t)(a + 0x08u); /* CLC; ADC #$08 */
            ram00E0[x] = (uint8_t)((ram00E0[x] + 1u) & 0x3Fu);
        } else {
            ram0064[x] = a;
        }
        ram00D8[x] = 0u;
    }
}

/* sub_E80D: select one of the six original data pages using Y. */
uint8_t nesSubE80DPreparePointer(uint8_t y, const uint8_t *const dataPages[6],
                                 const uint8_t **outData)
{
    if (y >= 6u || dataPages == NULL || outData == NULL) return 0u;
    *outData = dataPages[y];
    return 1u;
}

/* sub_E803: prepare pointer using Y, save that Y, then read page[dataIndex]. */
uint8_t nesSubE803ReadData(uint8_t y, uint8_t dataIndex,
                           const uint8_t *const dataPages[6],
                           uint8_t *savedY, uint8_t *outA)
{
    const uint8_t *data;
    if (savedY == NULL || outA == NULL ||
        !nesSubE80DPreparePointer(y, dataPages, &data) || data == NULL) return 0u;
    *savedY = y; /* STY ram_0007_t08 */
    *outA = data[dataIndex]; /* LDA (ram_0003_t01_data),Y */
    return 1u;
}

/* sub_E7FF: load the per-object data index, then fall through to sub_E803. */
uint8_t nesSubE7FFReadObjectData(uint8_t x, uint8_t y,
                                 const uint8_t ram00E0[4],
                                 const uint8_t *const dataPages[6],
                                 uint8_t *savedY, uint8_t *outA)
{
    if (x > 3u || ram00E0 == NULL) return 0u;
    return nesSubE803ReadData(y, ram00E0[x], dataPages, savedY, outA);
}

/* sub_E7FC: load Y from RAM[$0360+X], then execute sub_E7FF. */
uint8_t nesSubE7FCReadObjectBandData(uint8_t x, const uint8_t ram0360[4],
                                     const uint8_t ram00E0[4],
                                     const uint8_t *const dataPages[6],
                                     uint8_t *savedY, uint8_t *outA)
{
    if (x > 3u || ram0360 == NULL) return 0u;
    return nesSubE7FFReadObjectData(x, ram0360[x], ram00E0,
                                    dataPages, savedY, outA);
}

/* sub_E836: skip objects with nonzero RAM[$00B0+X]; others use sub_DC97. */
void nesSubE836UpdateObjectDifferences(uint8_t ram00B0[4], uint8_t ram00BC[4],
                                       uint8_t ram00B8[4], uint8_t ram008C[4],
                                       uint8_t ram03F1)
{
    int x;
    for (x = 3; x >= 0; --x) {
        if (ram00B0[x] == 0u) {
            ram008C[x] = nesSubDC9A(ram03F1, ram00BC[x], ram00B8[x]);
        }
    }
}

/* sub_E16C: exact Y offsets reached by its DEC Y / BPL loop. */
void nesSubE16CWriteSpriteStrip(uint8_t oam[256],uint8_t address,uint8_t firstTile){oam[(uint8_t)(address+6u)]=0x60u;oam[(uint8_t)(address+5u)]=firstTile;oam[(uint8_t)(address+2u)]=0x60u;oam[(uint8_t)(address+1u)]=(uint8_t)(firstTile-1u);}
/* sub_E3F4: initialize object order and sort four indices by unsigned Y. */
void nesSubE3F4SortObjectsByY(uint8_t order[4],const uint8_t y[4],uint8_t active3,uint8_t y3){uint8_t yy[4],i,p;for(i=0;i<4u;i++){order[i]=i;yy[i]=y[i];}if(active3)yy[3]=y3;for(p=3u;p>0u;p--)for(i=0;i<p;i++)if(yy[order[i]]>=yy[order[i+1u]]){uint8_t t=order[i];order[i]=order[i+1u];order[i+1u]=t;}}
/* sub_E4AD: writes $F8 to three OAM Y bytes, each four bytes apart. */
void nesSubE4ADHideFourSprites(uint8_t oam[256],uint8_t index,uint8_t offset){uint8_t b=(uint8_t)(index+offset);oam[b]=0xF8u;oam[(uint8_t)(b+4u)]=0xF8u;oam[(uint8_t)(b+8u)]=0xF8u;}
/* sub_E49B: X=0..2; call the group hider only if bit 0 is clear. */
void nesSubE49BHideSelectedSpriteGroups(uint8_t oam[256],uint8_t index,const uint8_t flags[3],const uint8_t offsets[3]){uint8_t x;for(x=0;x<3u;x++)if((flags[x]&1u)==0u)nesSubE4ADHideFourSprites(oam,index,offsets[x]);}
/* sub_E2AF: SEC/SBC low byte; borrow decrements the high byte. */
void nesSubE456CullObjectSprites(uint8_t oam[256],NesSpriteCullState *s,
                                  const uint8_t oamIndex[4],const uint8_t spriteOffset[3]){
    int row;
    if(s==NULL)return;
    for(row=3;row>=0;row--){
        uint8_t slot=s->ram0088[row],x,flags[3];
        if(slot==0u)continue;
        x=slot;
        flags[0]=flags[1]=flags[2]=s->ram0084[x];
        if((s->ram0084[x]&1u)==0u && s->ram0080[x]>=0x70u && s->ram0080[x]<0x90u){
            nesSubDB95ClearOpponentBikeData(x,s->ram00A8,s->ram00B8,s->ram00BC,s->ram00A0,
                s->ram00A4,s->ram0058,s->ram00CC,s->ram036C,s->ram0090,s->ram0094,
                s->ram0098,s->ram009C,s->ram00B0,s->ram00D4,s->ram0388,s->ram00C0,
                s->ram039C,s->ram00DC,s->ram0070,s->ram0370);
            s->ram03A6_flag=1u;
        }else{
            if(s->ram0080[x]>=0xF8u)flags[1]=(uint8_t)(flags[1]+1u);
            if(s->ram0080[x]>=0xF0u)flags[2]=(uint8_t)(flags[2]+1u);
        }
        nesSubE49BHideSelectedSpriteGroups(oam,oamIndex[row],flags,spriteOffset);
    }
}
uint8_t nesSubE26APrepareSpecialSprite(uint8_t x,uint8_t ram039C3,
    const uint8_t ram0084[4],const uint8_t ram0390[4],const uint8_t ram0080[4],
    const uint8_t ram006C[4],const uint8_t ram00A4[4],const uint8_t ram0398[4],
    const uint8_t tblD90B[3],uint8_t ram000B,NesPreparedSpriteCommand *out){
    uint8_t y;
    if(x>3u||out==NULL||ram039C3==0u||ram0084[x]==2u||ram0390[x]>=0xF8u||ram0390[x]<ram0080[x])return 0u;
    y=ram0398[x];
    if(y>=3u)return 0u;
    out->scratch0=ram0390[x];
    out->scratch1=(uint8_t)(0xA8u-ram006C[x]);
    out->scratch2=tblD90B[y];
    out->scratch3=0x22u;
    out->scratch4=ram039C3;
    out->scratchB=(uint8_t)(ram000B|(ram00A4[x]!=0u?0x20u:0u));
    out->transformY=(uint8_t)(y==0u?0u:(y^0x03u));
    return 1u;
}
void nesSubE359UpdateTemperatureMeter(uint8_t finished,uint8_t ram005C,uint8_t raceStarted,
    uint8_t *timer003C,uint8_t *fraction003B5,uint8_t *coarse003B6,
    uint8_t *overheatLatch,uint8_t *ram00FC,uint8_t oam[256]){
    static const uint8_t tblD8F7[4]={0x3Fu,0x0Fu,0x07u,0x07u};
    static const uint8_t tblD8FB[4]={0x08u,0x20u,0x11u,0x11u};
    uint8_t tile=0xFEu;
    if(timer003C==NULL||fraction003B5==NULL||coarse003B6==NULL||overheatLatch==NULL||ram00FC==NULL||oam==NULL)return;
    if(finished==0u){
        if(*overheatLatch!=0u){if((*timer003C&1u)==0u)tile=0xFCu;}
        else{
            uint8_t a=(uint8_t)(ram005C&0xC0u),y,carry;
            if(a!=0u){
                if(raceStarted==0u)a=0x80u;
                carry=(uint8_t)(a>>7);a=(uint8_t)(a<<1);
                {uint8_t oldCarry=carry;carry=(uint8_t)(a>>7);a=(uint8_t)((uint8_t)(a<<1)|oldCarry);}
                {uint8_t oldCarry=carry;carry=(uint8_t)(a>>7);a=(uint8_t)((uint8_t)(a<<1)|oldCarry);}
            }
            y=a;
            if(*coarse003B6<tblD8FB[y]){
                unsigned int sum=(unsigned int)*fraction003B5+(unsigned int)tblD8F7[y];
                *fraction003B5=(uint8_t)sum;if(sum>0xFFu)*coarse003B6=(uint8_t)(*coarse003B6+1u);
            }else if(*coarse003B6>tblD8FB[y]){
                uint8_t old=*fraction003B5;*fraction003B5=(uint8_t)(old-0x0Bu);
                if(old<0x0Bu&&*coarse003B6!=0u)*coarse003B6=(uint8_t)(*coarse003B6-1u);
            }
            if(*coarse003B6>=0x20u){*timer003C=*coarse003B6;*overheatLatch=*coarse003B6;}
            if(*coarse003B6>=0x1Au){y=(uint8_t)(y-1u);if(y==0u)*ram00FC=(uint8_t)(*ram00FC|0xB0u);}
        }
    }else{tile=0xFEu;}
    {uint8_t spriteX=(uint8_t)(*coarse003B6+0x50u);int index;
        for(index=0x0C;index>=0;index-=4){uint8_t base=(uint8_t)(0xF0u+(uint8_t)index);
            oam[base]=0xCFu;oam[(uint8_t)(base+1u)]=tile;oam[(uint8_t)(base+2u)]=0x20u;oam[(uint8_t)(base+3u)]=spriteX;
            spriteX=(uint8_t)(spriteX+8u);
        }
    }
}
uint8_t nesSubE2AFUpdateFinishCountdown(uint8_t delta,uint8_t finished,uint8_t frameTimer,
    uint8_t *low03BD,uint8_t *high03BE,uint8_t *blink03BB,
    const uint8_t tblD8E5[2],NesPreparedSpriteCommand *out){
    uint8_t oldLow;
    if(low03BD==NULL||high03BE==NULL||blink03BB==NULL||tblD8E5==NULL||out==NULL)return 0u;
    oldLow=*low03BD;*low03BD=(uint8_t)(oldLow-delta);
    if(oldLow<delta)*high03BE=(uint8_t)(*high03BE-1u);
    if(finished!=0u){if(finished!=1u)return 0u;if(frameTimer==0u)*blink03BB=(uint8_t)(*blink03BB^1u);}
    if(*high03BE!=1u)return 2u;
    out->scratch0=*low03BD;out->scratch1=0x50u;out->scratch2=tblD8E5[*blink03BB&1u];
    out->scratch3=0x32u;out->scratch4=0xC0u;out->scratchB=0u;out->transformY=0u;
    return 1u;
}
void nesSubE2AFDecrementCountdown(uint8_t *lo,uint8_t *hi,uint8_t delta){uint8_t old=*lo;*lo=(uint8_t)(old-delta);if(old<delta)*hi=(uint8_t)(*hi-1u);}
/* loc_E2BD: toggle only when finish flag is exactly 1 and timer is zero. */
void nesLocE2BDUpdateFinishBlink(uint8_t finished,uint8_t timer,uint8_t *blink){if(finished==1u&&timer==0u)*blink=(uint8_t)(*blink^1u);}
/* loc_E2D0: LDY RAM03BE / DEY / BNE. */
uint8_t nesLocE2D0CountdownReachedOne(uint8_t countdown){return (uint8_t)(countdown==1u);}
/* sub_E2F6: exact preconditions before wheel-sprite writes. */
uint8_t nesSubE2F6ShouldDrawWheel(uint8_t x,const uint8_t r36c[4],const uint8_t r94[4],const uint8_t r90[4],const uint8_t rbc[4],const uint8_t r84[4],const uint8_t r80[4]){if(x>3u||r36c[x]==0u||(uint8_t)(r94[x]|r90[x])==0u||rbc[x]!=0u)return 0u;if((r84[x]&1u)==0u)return 0u;return (uint8_t)(r80[x]>=0x10u);}
void nesSubE2F6DrawWheelSprites(uint8_t x,uint8_t yOffset,const uint8_t ram036C[4],
    const uint8_t ram0094[4],const uint8_t ram0090[4],const uint8_t ram00BC[4],
    const uint8_t ram0084[4],const uint8_t ram0080[4],const uint8_t ram008C[4],
    uint8_t ram003F,uint8_t oam[256]){
    uint8_t xx,yy,t0=0xF6u,t1=0xF7u,attr;
    if(x>3u||ram036C[x]==0u||(uint8_t)(ram0094[x]|ram0090[x])==0u||ram00BC[x]!=0u||
       (ram0084[x]&1u)==0u||ram0080[x]<0x10u)return;
    xx=(uint8_t)(ram0080[x]+0xF9u);oam[(uint8_t)(0x97u+yOffset)]=xx;
    xx=(uint8_t)(xx+0xF8u);oam[(uint8_t)(0x9Bu+yOffset)]=xx;
    yy=(uint8_t)(ram008C[x]+0x08u);oam[(uint8_t)(0x98u+yOffset)]=yy;
    yy=(uint8_t)(yy+0x08u);oam[(uint8_t)(0x94u+yOffset)]=yy;
    if((ram003F&0x04u)!=0u){t0=0xF7u;t1=0xF6u;}
    oam[(uint8_t)(0x95u+yOffset)]=t0;oam[(uint8_t)(0x99u+yOffset)]=t1;
    attr=(uint8_t)(ram036C[x]&0x02u);
    oam[(uint8_t)(0x96u+yOffset)]=attr;oam[(uint8_t)(0x9Au+yOffset)]=attr;
    if(ram0094[x]==0u)oam[(uint8_t)(0x98u+yOffset)]=0xF8u;
}
void nesSubEA44UpdateWheelSprites(uint8_t oam[256],const uint8_t ram0084[4],
    const uint8_t ram0080[4],const uint8_t ram00B0[4],const uint8_t ram00B4[4],
    const uint8_t ram00BC[4],const uint8_t ramY[4],const uint8_t tblE69A[7],
    const uint8_t ram036C[4],const uint8_t ram0094[4],const uint8_t ram0090[4],
    const uint8_t ram008C[4],uint8_t ram003F){
    int xi;
    for(xi=3;xi>=0;xi--){
        uint8_t x=(uint8_t)xi,yOffset=(uint8_t)(x<<3),a=ram0084[x];
        uint8_t special=0u;
        if((a&1u)!=0u){if(ram0080[x]<0xF4u)special=1u;}
        else if((a&2u)==0u&&ram0080[x]<0xF6u)special=1u;
        if(special!=0u&&ram00B0[x]!=0u){
            uint8_t y=nesSubDC9A(0xB6u,ram00BC[x],ramY[x]);
            oam[(uint8_t)(0x94u+yOffset)]=y;
            oam[(uint8_t)(0x97u+yOffset)]=(uint8_t)(ram0080[x]+0x0Au);
            if(ram00B4[x]<7u)oam[(uint8_t)(0x95u+yOffset)]=tblE69A[ram00B4[x]];
            oam[(uint8_t)(0x96u+yOffset)]=0u;
        }else{
            nesSubE2F6DrawWheelSprites(x,yOffset,ram036C,ram0094,ram0090,ram00BC,ram0084,ram0080,ram008C,ram003F,oam);
        }
    }
}
/* sub_E1F9 state-only return selection, including the $000E side effect. */
uint8_t nesSubE1F9SelectObjectSprite(uint8_t x,const uint8_t r9c[4],const uint8_t rbc[4],const uint8_t r370[4],const uint8_t r36[4],const uint8_t rac[4],const uint8_t r98[4],const uint8_t rb0[4],const uint8_t r58[4],const uint8_t rb8[4],const uint8_t r90[4],const uint8_t r94[4],const uint8_t rdc[4],uint8_t finished,uint8_t frameFlags,uint8_t *ram039C3,uint8_t *temp){
    uint8_t a,y;
    if(x>3u||temp==NULL||ram039C3==NULL)return 0u;
    *temp=0u;*ram039C3=0u;
    a=r9c[x];
    if(a!=0u&&a!=5u){
        y=(rbc[x]!=0u||r370[x]!=0u)?2u:6u;
        if(r36[x]==0u)return (uint8_t)(y|0x10u);
        return (uint8_t)((uint8_t)(r36[x]-1u)|0x10u);
    }
    if(finished==1u&&rac[x]==0x0Au){*temp=2u;return 2u;}
    if((uint8_t)(r98[x]|rb0[x])!=0u)return rac[x];
    if(r58[x]!=0u&&(r58[x]!=0x0Du||rb8[x]<0x20u))return rac[x];
    if((uint8_t)(r90[x]|r94[x])==0u){*temp=1u;return rac[x];}
    if(rac[x]!=6u)return rac[x];
    if(rdc[x]==0u)return (frameFlags&2u)!=0u?0x13u:rac[x];
    return (rdc[x]&0x80u)!=0u?0x15u:0x14u;
}
uint8_t nesSubE1A3PrepareObjectSprite(uint8_t x,uint8_t oamAddress,
    const uint8_t ram0080[4],const uint8_t ram008C[4],const uint8_t ram009C[4],
    const uint8_t ram00BC[4],const uint8_t ram0370[4],const uint8_t ram0036[4],
    const uint8_t ram00AC[4],const uint8_t ram0098[4],const uint8_t ram00B0[4],
    const uint8_t ram0058[4],const uint8_t ram00B8[4],const uint8_t ram0090[4],
    const uint8_t ram0094[4],const uint8_t ram00DC[4],uint8_t finished,uint8_t frameFlags,
    const uint8_t tblD892[23],const uint8_t tblD8A9[23],uint8_t oam[256],
    uint8_t *ram039C3,uint8_t *temp,uint8_t *renderType,NesPreparedSpriteCommand *out){
    uint8_t selected,y;
    int tileOffset;
    if(x>3u||ram039C3==NULL||temp==NULL||renderType==NULL||out==NULL||oam==NULL)return 0u;
    out->scratch0=ram0080[x];out->scratch1=ram008C[x];
    selected=nesSubE1F9SelectObjectSprite(x,ram009C,ram00BC,ram0370,ram0036,ram00AC,
        ram0098,ram00B0,ram0058,ram00B8,ram0090,ram0094,ram00DC,finished,frameFlags,ram039C3,temp);
    if(selected>=23u)return 0u;
    y=selected;out->scratch2=tblD892[y];out->scratch3=0x33u;out->scratch4=oamAddress;
    if(y==0x16u){*ram039C3=oamAddress;out->scratch4=(uint8_t)(out->scratch4+0x0Cu);
        out->scratch1=(uint8_t)(out->scratch1+0x08u);out->scratch3=0x23u;}
    *renderType=tblD8A9[y];
    if(*temp==0u)return 1u;
    if((*temp&1u)!=0u){oam[(uint8_t)(out->scratch4+0x15u)]=0xAEu;return 1u;}
    {uint8_t tile=0xB2u;for(tileOffset=0x11;tileOffset>=0;tileOffset-=4){
        if(tileOffset!=0x09){oam[(uint8_t)(out->scratch4+(uint8_t)tileOffset)]=tile;tile=(uint8_t)(tile-1u);}
    }}
    return 1u;
}
/* loc_E3AA: Y is inherited from the prior CMP; model it as an explicit input. */
void nesLocE3AAUpdateHeatFlags(uint8_t heat,uint8_t priorY,uint8_t *timer,uint8_t *latch,uint8_t *flags){uint8_t x=0u;if(heat>=0x20u){*timer=heat;*latch=heat;}if(heat>=0x1Au&&(uint8_t)(priorY-1u)==0u)x=0xB0u;*flags=(uint8_t)(*flags|x);}


/* sub_E52C: SEC/SBC indexed Y coordinates; nonzero only for wrapped distance < $12. */
uint8_t nesSubE52CCheckVerticalOverlap(uint8_t x,uint8_t y,const uint8_t ram008c[4]){
    uint8_t a=(uint8_t)(ram008c[x]-ram008c[y]);
    return (a>=0xECu||a<0x12u)?a:0u;
}
/* sub_E4C8: preserve the original scratch-byte dependency at $000B.
 * $000B is deliberately an input/output: the ASM comments identify it as
 * uninitialized by this routine, so resetting it here would not be faithful.
 */
void nesSubE4C8CheckOverlappingObjects(const uint8_t ram0084[4],const uint8_t ram0080[4],
                                       const uint8_t ram008c[4],uint8_t *ram000b,uint8_t *ram03a3){
    int x;
    if(ram000b==NULL||ram03a3==NULL)return;
    for(x=1;x>=0;x--){
        uint8_t a=ram0084[x];
        uint8_t carry=(uint8_t)(a&1u);
        if(carry==0u){
            if(a!=0u)continue;
            if(ram0080[x]<0xF8u)continue;
        }
        {
            uint8_t y=(uint8_t)(x+1);
            uint8_t count=0u;
            while(y<4u){
                a=ram0084[y];
                carry=(uint8_t)(a&1u);
                if(carry==0u){
                    if(a!=0u || ram0080[x]<0xF8u){y++;continue;}
                }
                a=nesSubE52CCheckVerticalOverlap((uint8_t)x,y,ram008c);
                if(a!=0u){
                    count=(uint8_t)(count+1u);
                    *ram000b=(uint8_t)((uint8_t)(*ram000b<<2)|y);
                }
                y++;
            }
            if(count>=2u){
                if(count!=2u){*ram03a3=1u;return;}
                {
                    uint8_t packed=*ram000b;
                    uint8_t y2=(uint8_t)(packed&3u);
                    uint8_t x2=(uint8_t)((packed>>2)&3u);
                    if(nesSubE52CCheckVerticalOverlap(x2,y2,ram008c)!=0u){
                        *ram03a3=1u;return;
                    }
                }
            }
        }
    }
    *ram03a3=0u;
}


/* Exact translated block at loc_E96E in sub_E96C ($E96C).
 * X runs 3..0. CMP #$15 / CMP #$14 bypass the Y update for those states;
 * otherwise CLC; ADC ram_00DC_obj,X; STA ram_obj_pos_Y_lo,X, with 8-bit wrap. */
void nesLocE96EApplyVerticalDelta(const uint8_t state[4], uint8_t posY[4], const uint8_t delta[4])
{
    int x;
    for (x = 3; x >= 0; --x) {
        if (state[x] != 0x15u && state[x] != 0x14u) {
            posY[x] = (uint8_t)(posY[x] + delta[x]);
        }
    }
}

/* Exact RAM-level translation of bank_FF.asm sub_E84F ($E84F):
 * STA ram_00BC_obj,X; STA ram_00E4_obj,X. */
void nesSubE84FStorePair(uint8_t x, uint8_t value, uint8_t ram00BC[4], uint8_t ram00E4[4])
{
    if (x < 4u) {
        ram00BC[x] = value;
        ram00E4[x] = value;
    }
}

/* Exact RAM-level translation of bank_FF.asm sub_E86F ($E86F):
 * A is saved, then ram_00E4_obj,X - A is stored in ram_00BC_obj,X.
 * SEC/SBC is modulo 256; no borrow is propagated elsewhere. */
uint8_t nesSubE86FSubtractFromE4(uint8_t x, uint8_t a, const uint8_t ram00E4[4], uint8_t ram00BC[4])
{
    uint8_t result;
    if (x >= 4u) return 0u;
    result = (uint8_t)(ram00E4[x] - a);
    ram00BC[x] = result;
    return result;
}

/* Exact RAM-level translation of bank_FF.asm sub_E893 ($E893):
 * store A in $00D4,X and $00B4,X; return $00C8,X - $00D0,X. */
uint8_t nesSubE893SetOffsetsAndSubtract(uint8_t x, uint8_t a,
                                       const uint8_t ram00C8[4], const uint8_t ram00D0[4],
                                       uint8_t ram00D4[4], uint8_t ram00B4[4])
{
    if (x >= 4u) return 0u;
    ram00D4[x] = a;
    ram00B4[x] = a;
    return (uint8_t)(ram00C8[x] - ram00D0[x]);
}

/* Exact sub_E73B ($E73B): add $0060,X to X-low, with 8-bit wrap.
 * This routine does not update the high byte. */
void nesSubE73BAddXLow(uint8_t x, const uint8_t ram0060[4], uint8_t posXLo[4])
{
    if (x < 4u) posXLo[x] = (uint8_t)(posXLo[x] + ram0060[x]);
}

/* Exact sub_E733 ($E733): call sub_E73B for X=3,2,1. The original BNE
 * after DEX means slot 0 is intentionally omitted. */
void nesSubE733UpdateThreeXLowSlots(const uint8_t ram0060[4], uint8_t posXLo[4])
{
    int x;
    for (x = 3; x > 0; --x) {
        posXLo[x] = (uint8_t)(posXLo[x] + ram0060[x]);
    }
}

/* sub_E7FC ($E7FC): LDY $0360,X, then fall through to sub_E7FF. */
uint8_t nesSubE7FCLoadDataIndex(uint8_t x, const uint8_t ram0360[4])
{
    return x < 4u ? ram0360[x] : 0u;
}

/* sub_E7FF ($E7FF): LDA $00E0,X; STA zero-page data index. */
uint8_t nesSubE7FFLoadDataIndex(uint8_t x, const uint8_t ram00E0[4])
{
    return x < 4u ? ram00E0[x] : 0u;
}

/* sub_E80D ($E80D): exact pointer tables tbl_E54E_lo / tbl_E554_hi. */
void nesSubE80DPointerBytes(uint8_t tableIndex, uint8_t *low, uint8_t *high)
{
    static const uint8_t lo[6] = {0x00u,0x40u,0x80u,0xC0u,0x00u,0x40u};
    static const uint8_t hi[6] = {0x04u,0x04u,0x04u,0x04u,0x05u,0x05u};
    if (tableIndex < 6u && low != NULL && high != NULL) {
        *low = lo[tableIndex];
        *high = hi[tableIndex];
    }
}

/* sub_E803 ($E803): pointer setup followed by LDA (pointer),Y.
 * ram0400To07FF represents CPU RAM $0400-$07FF, allowing the same
 * 8-bit Y addition and page crossing as the 6502 indirect-indexed read. */
uint8_t nesSubE803ReadIndexedRam(uint8_t tableIndex, uint8_t dataIndex,
                                const uint8_t ram0400To07FF[1024])
{
    static const uint16_t base[6] = {0x000u,0x040u,0x080u,0x0C0u,0x100u,0x140u};
    if (tableIndex >= 6u || ram0400To07FF == NULL) return 0u;
    return ram0400To07FF[(uint16_t)(base[tableIndex] + dataIndex)];
}

/* Exact decision/write effects of dispatch entry ofs_002_E818_07.
 * CMP #$07 followed by BPL is a signed-N test; preserve that detail. */
void nesOfsE81807Apply(uint8_t x, uint8_t ram00B0[4], const uint8_t ram00AC[4],
                       const uint8_t ram0094[4], const uint8_t ram0090[4], uint8_t ram0098[4])
{
    uint8_t state;
    if (x >= 4u || ram00B0[x] != 0u) return;
    state = ram00AC[x];
    if ((int8_t)(uint8_t)(state - 0x07u) >= 0) return;
    state = ram0094[x];
    if (state < 0x03u) {
        if (state != 0x02u || (ram0090[x] & 0x80u) == 0u) return;
    }
    ram0098[x] = 0xFFu;
}

/* Exact dispatch entry ofs_002_E845_04: D8=80, E893(A=05), LSR, E84F. */
void nesOfsE84504Apply(uint8_t x, const uint8_t ram00C8[4], const uint8_t ram00D0[4],
                       uint8_t ram00D8[4], uint8_t ram00D4[4], uint8_t ram00B4[4],
                       uint8_t ram00BC[4], uint8_t ram00E4[4])
{
    uint8_t a;
    if (x >= 4u) return;
    ram00D8[x] = 0x80u; ram00D4[x] = 0x05u; ram00B4[x] = 0x05u;
    a = (uint8_t)(ram00C8[x] - ram00D0[x]);
    a = (uint8_t)(a >> 1);
    ram00BC[x] = a; ram00E4[x] = a;
}

/* Exact dispatch entry ofs_002_E854_05: E893(A=02), LSR, then E86F. */
void nesOfsE85405Apply(uint8_t x, const uint8_t ram00C8[4], const uint8_t ram00D0[4],
                       const uint8_t ram00E4[4], uint8_t ram00D4[4], uint8_t ram00B4[4],
                       uint8_t ram00BC[4])
{
    uint8_t a;
    if (x >= 4u) return;
    ram00D4[x] = 0x02u; ram00B4[x] = 0x02u;
    a = (uint8_t)((uint8_t)(ram00C8[x] - ram00D0[x]) >> 1);
    ram00BC[x] = (uint8_t)(ram00E4[x] - a);
}

/* Exact dispatch entry ofs_002_E85D_02: E893(A=06), E84F, D8=60. */
void nesOfsE85D02Apply(uint8_t x, const uint8_t ram00C8[4], const uint8_t ram00D0[4],
                       uint8_t ram00D8[4], uint8_t ram00D4[4], uint8_t ram00B4[4],
                       uint8_t ram00BC[4], uint8_t ram00E4[4])
{
    uint8_t a;
    if (x >= 4u) return;
    ram00D4[x] = 0x06u; ram00B4[x] = 0x06u;
    a = (uint8_t)(ram00C8[x] - ram00D0[x]);
    ram00BC[x] = a; ram00E4[x] = a; ram00D8[x] = 0x60u;
}

/* Exact dispatch entry ofs_002_E86A_03: E893(A=01), then E86F. */
void nesOfsE86A03Apply(uint8_t x, const uint8_t ram00C8[4], const uint8_t ram00D0[4],
                       const uint8_t ram00E4[4], uint8_t ram00D4[4], uint8_t ram00B4[4],
                       uint8_t ram00BC[4])
{
    uint8_t a;
    if (x >= 4u) return;
    ram00D4[x] = 0x01u; ram00B4[x] = 0x01u;
    a = (uint8_t)(ram00C8[x] - ram00D0[x]);
    ram00BC[x] = (uint8_t)(ram00E4[x] - a);
}

/* Exact dispatch entry ofs_002_E879_09, including the conditional +$10. */
void nesOfsE87909Apply(uint8_t x, const uint8_t ram00C8[4], const uint8_t ram00D0[4],
                       const uint8_t ram00A0[4], uint8_t ram00D8[4],
                       uint8_t ram00D4[4], uint8_t ram00B4[4],
                       uint8_t ram00BC[4], uint8_t ram00E4[4])
{
    uint8_t a;
    if (x >= 4u) return;
    ram00D4[x] = 0x04u; ram00B4[x] = 0x04u;
    a = (uint8_t)((uint8_t)(ram00C8[x] - ram00D0[x]) << 1);
    ram00BC[x] = a; ram00E4[x] = a;
    if (ram00A0[x] != 0u) {
        a = (uint8_t)(ram00BC[x] + 0x10u);
        ram00BC[x] = a; ram00E4[x] = a;
    }
    ram00D8[x] = 0x40u;
}

/* Exact effects of dispatch entry ofs_002_E8BF_0E: A4=0 and A0=0. */
void nesOfsE8BF0EReset(uint8_t x, uint8_t ram00A4[4], uint8_t ram00A0[4])
{
    if (x < 4u) { ram00A4[x] = 0u; ram00A0[x] = 0u; }
}

/* Exact effects of ofs_002_E8C6_0F: A4=1, then increment if Y-low < $20. */
void nesOfsE8C60FSetStateFromY(uint8_t x, uint8_t yLow, uint8_t ram00A4[4])
{
    if (x < 4u) ram00A4[x] = (uint8_t)(1u + (yLow < 0x20u ? 1u : 0u));
}

/* Exact effects of ofs_002_E8E3_01: A=1 then branch to store A in A0,X. */
void nesOfsE8E301SetA0(uint8_t x, uint8_t ram00A0[4])
{
    if (x < 4u) ram00A0[x] = 1u;
}

/* Exact effects of ofs_002_E8F5_14: jump to E84F with A=$30. */
void nesOfsE8F514SetPair30(uint8_t x, uint8_t ram00BC[4], uint8_t ram00E4[4])
{
    if (x < 4u) { ram00BC[x] = 0x30u; ram00E4[x] = 0x30u; }
}

/* Exact effects of ofs_002_E8FA_10: jump to E84F with A=$10. */
void nesOfsE8FA10SetPair10(uint8_t x, uint8_t ram00BC[4], uint8_t ram00E4[4])
{
    if (x < 4u) { ram00BC[x] = 0x10u; ram00E4[x] = 0x10u; }
}

/* Exact effects of ofs_002_E963_00: clear CC, D4 and B4 for slot X. */
void nesOfsE96300ClearThree(uint8_t x, uint8_t ram00CC[4], uint8_t ram00D4[4], uint8_t ram00B4[4])
{
    if (x < 4u) { ram00CC[x] = 0u; ram00D4[x] = 0u; ram00B4[x] = 0u; }
}


/*
 * Portable RAM/register-effect models of three source fragments from bank_FF.asm.
 * These do not perform PPU I/O: callers consume the explicit register writes.
 */

/* sub_D0F4 ($D0F4): exact setup before JMP loc_D2A3. */
void nesSubD0F4PrepareCommandStream(const uint8_t ram03D9To03DF[7],
                                    uint8_t ram03D0To03D7[8],
                                    uint8_t *sourceLo, uint8_t *sourceHi,
                                    uint8_t *dataLo, uint8_t *dataHi)
{
    unsigned int i;
    if (ram03D9To03DF == NULL || ram03D0To03D7 == NULL ||
        sourceLo == NULL || sourceHi == NULL || dataLo == NULL || dataHi == NULL) {
        return;
    }
    for (i = 0u; i < 7u; ++i) {
        ram03D0To03D7[i + 1u] = ram03D9To03DF[i];
    }
    ram03D0To03D7[0] = 0x17u;
    ram03D0To03D7[2] = 0xFBu;
    ram03D0To03D7[5] = 0xFBu;
    *sourceLo = 0x56u;
    *sourceHi = 0x23u;
    *dataLo = 0xD0u;
    *dataHi = 0x03u;
}

/* sub_D1A9: status-read/latch reset followed by two literal $2005 writes of zero. */
void nesSubD1A9PrepareScrollWrites(uint8_t scrollWrites[2], uint8_t *statusReads)
{
    if (scrollWrites == NULL || statusReads == NULL) return;
    scrollWrites[0] = 0u;
    scrollWrites[1] = 0u;
    *statusReads = (uint8_t)(*statusReads + 1u);
}

/* sub_D1B3: represent PPU writes in source order without touching NES hardware. */
void nesSubD1B3PrepareScrollWrites(uint8_t ramFor2000, uint8_t baseNametable,
                                  uint8_t scrollX, uint8_t scrollY,
                                  uint8_t *ppuCtrl, uint8_t scrollWrites[2],
                                  uint8_t *statusReads)
{
    if (ppuCtrl == NULL || scrollWrites == NULL || statusReads == NULL) return;
    *ppuCtrl = (uint8_t)(ramFor2000 | baseNametable);
    scrollWrites[0] = scrollX;
    scrollWrites[1] = scrollY;
    *statusReads = (uint8_t)(*statusReads + 1u);
}


/* Exact register/RAM-effect translations from bank_FF.asm, batch 2026-10-21. */
void nesSubC333WritePpuCtrl(uint8_t a, uint8_t *ramFor2000, uint8_t *ppuCtrl)
{
    if (ramFor2000 == NULL || ppuCtrl == NULL) return;
    *ppuCtrl = a;
    *ramFor2000 = a;
}

void nesSubC3AFIncrementRaceCounter(uint8_t *ram0041)
{
    if (ram0041 != NULL) *ram0041 = (uint8_t)(*ram0041 + 1u);
}

void nesSubC3B1ClearCounters(uint8_t *ram0040, uint8_t *ram0044)
{
    if (ram0040 != NULL) *ram0040 = 0u;
    if (ram0044 != NULL) *ram0044 = 0u;
}

/* X runs 00, FF, FE, ... 01: every byte of both 256-byte pages is copied. */
void nesSubC621CopyPage(const uint8_t ram06E0[256], uint8_t ram05E0[256])
{
    unsigned int i;
    if (ram06E0 == NULL || ram05E0 == NULL) return;
    for (i = 0u; i < 256u; ++i) ram05E0[i] = ram06E0[i];
}

void nesSubC68AIncrementPointer(uint8_t *lo, uint8_t *hi)
{
    if (lo == NULL || hi == NULL) return;
    *lo = (uint8_t)(*lo + 1u);
    if (*lo == 0u) *hi = (uint8_t)(*hi + 1u);
}

/* LDX reads the old command length; ADC is explicitly CLC + RAM[$0300]. */
uint8_t nesSubC971AddCommandLength(uint8_t a, uint8_t ram0300,
                                  uint8_t *newRam0300, uint8_t *xOut)
{
    if (newRam0300 == NULL || xOut == NULL) return a;
    *xOut = ram0300;
    *newRam0300 = (uint8_t)(a + ram0300);
    return *newRam0300;
}

void nesSubCA1BCopyButtonHold(uint8_t ramButtonHold, uint8_t *ram005C)
{
    if (ram005C != NULL) *ram005C = ramButtonHold;
}

/* Exact loop/branch behavior of sub_CB13, including Y=$05 special case. */
uint8_t nesSubCB13ComputeSpriteCode(uint8_t yIn, uint8_t ram0046Flag)
{
    uint8_t y = yIn;
    uint8_t a;
    if (y == 0x05u) return 0x20u;
    a = 0u;
    for (;;) {
        y = (uint8_t)(y - 1u); /* DEY; BMI tests bit 7. */
        if ((y & 0x80u) != 0u) break;
        a = (uint8_t)(a + 3u); /* CLC; ADC #$03. */
        if (a == 0u) break;    /* BNE loop; zero exits early. */
    }
    if (ram0046Flag != 0u) a = (uint8_t)(a | 0x10u);
    return a;
}

void nesSubCFE3ClearObjectFlags(uint8_t x, uint8_t ram005C[256])
{
    if (ram005C != NULL) nesSubCFE3MaskFlags(&ram005C[x]);
}

/* sub_D24F: STA $2001 with A=0. The caller applies this value to its PPU. */
void nesSubD24FDisableRendering(uint8_t *ppuMask)
{
    if (ppuMask != NULL) *ppuMask = 0u;
}

/* sub_CF8B returns carry clear only for A in [$3B,$3D], set otherwise. */
uint8_t nesSubCF8BCarryOut(uint8_t a)
{
    return (a >= 0x3Bu && a < 0x3Eu) ? 0u : 1u;
}

/* CLC / ADC #1 / CMP #$40 / LDA #0 only when the increment equals $40. */
uint8_t nesSubECBFIncrementAndWrap(uint8_t a)
{
    uint8_t result = (uint8_t)(a + 1u);
    return result == 0x40u ? 0u : result;
}

/* Fills RAM $0304..$030D with $FC; other bytes remain untouched. */
void nesSubF676FillCommandPayload(uint8_t ram0300To0315[22])
{
    unsigned int address;
    if (ram0300To0315 == NULL) return;
    for (address = 4u; address <= 13u; ++address) ram0300To0315[address] = 0xFCu;
}

/* INX then stores X at $00B7 and clears $00C5. */
uint8_t nesSubF826AdvanceObject(uint8_t x, uint8_t *ram00B7, uint8_t *ram00C5)
{
    x = (uint8_t)(x + 1u);
    if (ram00B7 != NULL) *ram00B7 = x;
    if (ram00C5 != NULL) *ram00C5 = 0u;
    return x;
}

void nesSubF82EPreparePointer(uint8_t x, const uint8_t tblF063Lo[256],
                              const uint8_t tblF087Hi[256],
                              uint8_t *ptrLo, uint8_t *ptrHi)
{
    if (tblF063Lo == NULL || tblF087Hi == NULL || ptrLo == NULL || ptrHi == NULL) return;
    *ptrLo = tblF063Lo[x];
    *ptrHi = tblF087Hi[x];
}

void nesSubF839PreparePointer(uint8_t x, const uint8_t tblF4F5Lo[256],
                              const uint8_t tblF4FAHi[256],
                              uint8_t *ptrLo, uint8_t *ptrHi)
{
    if (tblF4F5Lo == NULL || tblF4FAHi == NULL || ptrLo == NULL || ptrHi == NULL) return;
    *ptrLo = tblF4F5Lo[x];
    *ptrHi = tblF4FAHi[x];
}

/* sub_C62D writes literal $05 to the controller strobe register $4016. */
void nesSubC62DControllerStrobe(uint8_t *controller4016)
{
    if (controller4016 != NULL) *controller4016 = 0x05u;
}

/* INC $00B1; only when it reaches $20, clear it and toggle bit 2 of $00B0. */
void nesSubF6FBAdvanceAudioPhase(uint8_t *ram00B0, uint8_t *ram00B1)
{
    if (ram00B0 == NULL || ram00B1 == NULL) return;
    *ram00B1 = (uint8_t)(*ram00B1 + 1u);
    if (*ram00B1 == 0x20u) {
        *ram00B1 = 0u;
        *ram00B0 = (uint8_t)(*ram00B0 ^ 0x04u);
    }
}

/* sub_F871: A becomes Y; absolute indexed reads use 16-bit address wrap.
 * For Y=$FF, tbl_FF00+1,Y reads CPU address $0000. */
void nesSubF871WriteAudioTimer(uint8_t a, const uint8_t tblFF00[256],
                               uint8_t byteAt0000, uint8_t audio4000To400F[16])
{
    uint8_t y = a;
    uint8_t highPlusOne;
    if (tblFF00 == NULL || audio4000To400F == NULL) return;
    highPlusOne = (y == 0xFFu) ? byteAt0000 : tblFF00[(uint8_t)(y + 1u)];
    if (highPlusOne == 0u) return;
    audio4000To400F[2u] = highPlusOne;
    audio4000To400F[3u] = (uint8_t)(tblFF00[y] | 0x08u);
}

/* sub_F885 loads X=$04, then shares the exact F873 table-read sequence. */
void nesSubF885WriteAudioTimer(uint8_t a, const uint8_t tblFF00[256],
                               uint8_t byteAt0000, uint8_t audio4000To400F[16])
{
    uint8_t y = a;
    uint8_t highPlusOne;
    if (tblFF00 == NULL || audio4000To400F == NULL) return;
    highPlusOne = (y == 0xFFu) ? byteAt0000 : tblFF00[(uint8_t)(y + 1u)];
    if (highPlusOne == 0u) return;
    audio4000To400F[6u] = highPlusOne;
    audio4000To400F[7u] = (uint8_t)(tblFF00[y] | 0x08u);
}


/* Additional byte-accurate translations checked against bank_FF.asm on 2026-10-21. */
void nesSubC1FDInitializeRam(const uint8_t tblC091[256], uint8_t cpuRam[2048])
{
    int x;
    if (tblC091 == NULL || cpuRam == NULL) return;
    for (x = 0x1E; x >= 0; --x) {
        cpuRam[0x0580u + (unsigned)x] = tblC091[x];
        cpuRam[0x05A3u + (unsigned)x] = tblC091[x];
    }
    cpuRam[0x06E0] = 0x01u;
    for (x = 0x01; x < 0xCF; ++x) cpuRam[0x06E0u + (unsigned)x] = 0x40u;
    cpuRam[0x06E0u + 0xCFu] = 0x09u;
    cpuRam[0x05CE] = 0xA5u;
    cpuRam[0x05CF] = 0x5Au;
}

void nesSubC229InitializeRows(uint8_t cpuRam[2048])
{
    unsigned int x;
    uint8_t a = 0x0Au;
    if (cpuRam == NULL) return;
    for (x = 0u; x < 3u; ++x) {
        cpuRam[0x05A0u + x] = a;
        cpuRam[0x05C3u + x] = a;
        a = 0u;
    }
}

/* sub_C32F: LDA ram_for_2000 / AND #$7F, then falls through sub_C333. */
void nesSubC32FMaskPpuCtrl(uint8_t *ramFor2000, uint8_t *ppuCtrl)
{
    uint8_t a;
    if (ramFor2000 == NULL || ppuCtrl == NULL) return;
    a = (uint8_t)(*ramFor2000 & 0x7Fu);
    *ppuCtrl = a;
    *ramFor2000 = a;
}

static uint8_t nesSubCB06Index(uint8_t ram0002, uint8_t ram0046Flag, uint8_t ram03F2)
{
    uint8_t a = nesSubCB13ComputeSpriteCode(ram0002, ram0046Flag);
    if (ram03F2 != 0u) a = (uint8_t)(a + 0x23u); /* CLC; ADC #$23 */
    return a; /* TAX is not performed; the caller uses this value as Y. */
}

uint8_t nesSubCB06ComputeTableIndex(uint8_t ram0002, uint8_t ram0046Flag, uint8_t ram03F2)
{
    return nesSubCB06Index(ram0002, ram0046Flag, ram03F2);
}

void nesSubC53ESelectTableEntry(uint8_t ram0002, uint8_t ram0046Flag, uint8_t ram03F2,
                                const uint8_t cpuRam[2048],
                                uint8_t *ram0005, uint8_t *ram0006, uint8_t *ram0007)
{
    uint8_t y;
    if (cpuRam == NULL || ram0005 == NULL || ram0006 == NULL || ram0007 == NULL) return;
    y = nesSubCB06Index(ram0002, ram0046Flag, ram03F2);
    *ram0005 = cpuRam[(uint16_t)(0x0580u + y)];
    *ram0006 = cpuRam[(uint16_t)(0x0581u + y)];
    *ram0007 = cpuRam[(uint16_t)(0x0582u + y)];
}

void nesSubC53ASelectTableEntry(uint8_t ram0043, uint8_t ram0046Flag, uint8_t ram03F2,
                                uint8_t *ram0002, const uint8_t cpuRam[2048],
                                uint8_t *ram0005, uint8_t *ram0006, uint8_t *ram0007)
{
    if (ram0002 == NULL) return;
    *ram0002 = ram0043; /* STA ram_0002_t07 before falling through to sub_C53E. */
    nesSubC53ESelectTableEntry(*ram0002, ram0046Flag, ram03F2, cpuRam,
                               ram0005, ram0006, ram0007);
}

void nesSubC716ConfigureLoop(uint8_t *ram000D, uint8_t *ram000E, uint8_t *ram000F,
                             uint8_t *ram0005, uint8_t *ram0006)
{
    if (ram000D == NULL || ram000E == NULL || ram000F == NULL ||
        ram0005 == NULL || ram0006 == NULL) return;
    *ram000D = 0x40u; *ram000E = 0x60u; *ram000F = 0x00u;
    *ram0005 = 0x28u; *ram0006 = 0x28u;
}

void nesSubC726ConfigureLoop(uint8_t *ram000D, uint8_t *ram000E, uint8_t *ram000F,
                             uint8_t *ram0005, uint8_t *ram0006)
{
    if (ram000D == NULL || ram000E == NULL || ram000F == NULL ||
        ram0005 == NULL || ram0006 == NULL) return;
    *ram000D = 0x00u; *ram000E = 0xE0u; *ram000F = 0x05u;
    *ram0005 = 0x14u; *ram0006 = 0x14u;
}

static uint8_t nesSbcByte(uint8_t a, uint8_t value, uint8_t *carry)
{
    unsigned int borrow = (*carry != 0u) ? 0u : 1u;
    unsigned int subtrahend = (unsigned int)value + borrow;
    uint16_t diff = (uint16_t)a - (uint16_t)subtrahend;
    *carry = ((unsigned int)a >= subtrahend) ? 1u : 0u;
    return (uint8_t)diff;
}

/* sub_DC97: LDA $03F1 then falls through SEC / SBC / SBC from sub_DC9A. */
uint8_t nesSubDC97SubtractObjectY(uint8_t ram03F1, uint8_t x, const uint8_t ram00BC[4],
                                  const uint8_t ramPosYLo[4], uint8_t *carryOut)
{
    uint8_t carry = 1u;
    uint8_t a;
    if (ram00BC == NULL || ramPosYLo == NULL) return ram03F1;
    a = nesSbcByte(ram03F1, ram00BC[x & 3u], &carry);
    a = nesSbcByte(a, ramPosYLo[x & 3u], &carry);
    if (carryOut != NULL) *carryOut = carry;
    return a;
}

void nesSubEC25WriteSprite(uint8_t a, uint8_t ram00C2, const uint8_t tblE6F6[256],
                           uint8_t oam[256])
{
    if (tblE6F6 == NULL || oam == NULL) return;
    oam[0x30] = a;
    oam[0x31] = 0xCAu;
    oam[0x32] = 0x00u;
    oam[0x33] = tblE6F6[ram00C2];
}

void nesSubEC5DWriteSprite(uint8_t ram003F, uint8_t ram00C2,
                           const uint8_t tblE6F6[256], uint8_t oam[256])
{
    if (oam == NULL) return;
    nesSubEC25WriteSprite(0xF8u, ram00C2, tblE6F6, oam);
    oam[0x28] = ((ram003F & 0x10u) != 0u) ? 0xF8u : 0xC7u;
    oam[0x2C] = oam[0x28];
}

/* sub_EC3B copies in this exact order: X=0, then $FF down through $01. */
void nesSubEC3BCopyPageDescending(uint8_t cpuRam[2048])
{
    unsigned int x;
    if (cpuRam == NULL) return;
    cpuRam[0x06E0] = cpuRam[0x05E0];
    for (x = 0xFFu; x >= 1u; --x)
        cpuRam[0x06E0u + x] = cpuRam[0x05E0u + x];
}

/* sub_EC75: exact zeroing, indexed table read, compare and conditional low-nibble write. */
void nesSubEC75InitializeObjectPositions(uint8_t ram00B0[4], uint8_t ram00C0[4],
                                         const uint8_t tblE6E1[256],
                                         uint8_t ramPosXLo[4], uint8_t ramPosYLo[4],
                                         uint8_t ram00BC[4])
{
    uint8_t a;
    if (ram00B0 == NULL || ram00C0 == NULL || tblE6E1 == NULL ||
        ramPosXLo == NULL || ramPosYLo == NULL || ram00BC == NULL) return;
    ramPosXLo[2] = 0u; ramPosXLo[0] = 0u; ramPosXLo[1] = 0u;
    ramPosYLo[2] = ram00B0[2];
    ramPosYLo[3] = ram00B0[0];
    ram00BC[0] = ram00B0[1];
    a = tblE6E1[ram00C0[2]];
    ramPosYLo[0] = a;
    if (a >= 0x11u) ramPosXLo[0] = (uint8_t)(a & 0x0Fu);
}

void nesSubF681CopyCommandBytes(uint16_t sourcePointer, uint8_t xIn, uint8_t yIn,
                                const uint8_t cpuMemory[65536], uint8_t cpuRam[2048],
                                uint8_t *xOut, uint8_t *yOut)
{
    uint8_t x = xIn, y = yIn;
    unsigned int iterations = (0x0Fu - (unsigned int)xIn) & 0xFFu;
    unsigned int i;
    if (cpuMemory == NULL || cpuRam == NULL) return;
    if (iterations == 0u) iterations = 256u;
    for (i = 0u; i < iterations; ++i) {
        uint16_t address;
        y = (uint8_t)(y + 1u); /* INY occurs before the indirect read. */
        address = (uint16_t)(sourcePointer + y);
        cpuRam[(uint16_t)(0x0303u + x)] = cpuMemory[address];
        x = (uint8_t)(x + 1u);
    }
    if (xOut != NULL) *xOut = x;
    if (yOut != NULL) *yOut = y;
}

uint8_t nesSubF821InitializeBufferAndAdvance(uint8_t x, uint8_t cpuRam[2048])
{
    if (cpuRam == NULL) return (uint8_t)(x + 1u);
    cpuRam[(uint16_t)(0x05E0u + x)] = 0x40u;
    x = (uint8_t)(x + 1u); /* Falls through to sub_F826. */
    cpuRam[0x00B7] = x;
    cpuRam[0x00C5] = 0x00u;
    return x;
}

void nesSubF86BWriteAudioRegisters(uint8_t x, uint8_t y, uint8_t a,
                                   const uint8_t tblFF00[256], uint8_t byteAt0000,
                                   uint8_t audio4000To400F[16])
{
    uint8_t highPlusOne;
    if (tblFF00 == NULL || audio4000To400F == NULL) return;
    audio4000To400F[0] = x; audio4000To400F[1] = y;
    highPlusOne = (a == 0xFFu) ? byteAt0000 : tblFF00[(uint8_t)(a + 1u)];
    if (highPlusOne == 0u) return;
    audio4000To400F[2] = highPlusOne;
    audio4000To400F[3] = (uint8_t)(tblFF00[a] | 0x08u);
}

void nesSubF889WriteThirdAudioTimer(uint8_t a, const uint8_t tblFF00[256],
                                    uint8_t byteAt0000, uint8_t audio4000To400F[16])
{
    uint8_t highPlusOne;
    if (tblFF00 == NULL || audio4000To400F == NULL) return;
    highPlusOne = (a == 0xFFu) ? byteAt0000 : tblFF00[(uint8_t)(a + 1u)];
    if (highPlusOne == 0u) return;
    audio4000To400F[10] = highPlusOne;
    audio4000To400F[11] = (uint8_t)(tblFF00[a] | 0x08u);
}

void nesSubC94BBuildCommand(uint8_t *ram0300, uint8_t tableIndex,
                            const uint8_t tblC0EC[257], uint8_t ram0300To040A[267],
                            uint8_t *xOut)
{
    uint8_t x;
    if (ram0300 == NULL || tblC0EC == NULL || ram0300To040A == NULL) return;
    x = *ram0300;
    *ram0300 = (uint8_t)(x + 0x0Au); /* sub_C971: LDX old length; CLC; ADC #$0A. */
    if (xOut != NULL) *xOut = x;
    ram0300To040A[(unsigned int)x + 1u] = tblC0EC[tableIndex];
    ram0300To040A[(unsigned int)x + 2u] = tblC0EC[(unsigned int)tableIndex + 1u];
    ram0300To040A[(unsigned int)x + 5u] = 0xFBu;
    ram0300To040A[(unsigned int)x + 8u] = 0xFBu;
    ram0300To040A[(unsigned int)x + 3u] = 0x07u;
    ram0300To040A[(unsigned int)x + 11u] = 0x00u;
}

static void nesControllerDelayEffect(uint8_t originalA, uint8_t count,
                                     uint8_t *aOut, uint8_t *ram0007, uint8_t *ram000C,
                                     uint8_t controllerWrites[2], uint8_t delayIterations[2])
{
    if (aOut != NULL) *aOut = originalA; /* PHA ... PLA preserves the input A. */
    if (ram0007 != NULL) *ram0007 = 0u;   /* DEC loop reaches zero. */
    if (ram000C != NULL) *ram000C = 0u;
    if (controllerWrites != NULL) {
        controllerWrites[0] = 0x04u;
        controllerWrites[1] = 0xFFu;
    }
    if (delayIterations != NULL) {
        delayIterations[0] = count;
        delayIterations[1] = count;
    } /* Each DEC/BNE loop executes count times; instruction-cycle timing is not simulated. */
}

void nesSubC64EControllerDelay(uint8_t a, uint8_t *aOut, uint8_t *ram0007,
                               uint8_t *ram000C, uint8_t controllerWrites[2], uint8_t delayIterations[2])
{
    nesControllerDelayEffect(a, 0x34u, aOut, ram0007, ram000C, controllerWrites, delayIterations);
}

void nesSubC653ControllerDelay(uint8_t a, uint8_t *aOut, uint8_t *ram0007,
                               uint8_t *ram000C, uint8_t controllerWrites[2], uint8_t delayIterations[2])
{
    nesControllerDelayEffect(a, 0x6Au, aOut, ram0007, ram000C, controllerWrites, delayIterations);
}

/* Exact byte/flag translation of sub_FF53 ($FF53).
 * Models the incoming carry consumed by ROR, the following three ROLs,
 * and the absolute-indexed table read at $FF3E,Y (including page wrap).
 */
uint8_t nesSubFF53Lookup(uint8_t inputA, uint8_t carryIn, uint8_t ram07F0,
                         const uint8_t cpuMemory[65536], uint8_t *yOut)
{
    uint8_t a = inputA;
    uint8_t x = inputA;
    uint8_t carry = (uint8_t)(carryIn & 1u);
    uint16_t address;

    /* ROR A */
    {
        uint8_t nextCarry = (uint8_t)(a & 1u);
        a = (uint8_t)((a >> 1) | (uint8_t)(carry << 7));
        carry = nextCarry;
    }
    a = x; /* TXA leaves carry unchanged. */
    /* ROL A; ROL A; ROL A */
    for (unsigned int i = 0u; i < 3u; ++i) {
        uint8_t nextCarry = (uint8_t)(a >> 7);
        a = (uint8_t)((uint8_t)(a << 1) | carry);
        carry = nextCarry;
    }
    a = (uint8_t)(a & 0x07u); /* AND #$07 */
    a = (uint8_t)(a + ram07F0); /* CLC; ADC ram_07F0 */
    if (yOut != NULL) *yOut = a; /* TAY */
    if (cpuMemory == NULL) return 0u;
    address = (uint16_t)(0xFF3Eu + (uint16_t)a);
    return cpuMemory[address]; /* LDA tbl_FF3E,Y */
}

/* Exact RAM-level translation of sub_EC99 ($EC99).
 * The F6FB call is represented by the already translated phase helper.
 * ramB0Object[2] and [3] are the two bytes updated through sub_ECBF.
 */
void nesSubEC99AdvanceObjectPhase(uint8_t ram0060, uint8_t *ram00B9Object1,
                                  uint8_t *ram00C0Object0, uint8_t ram00B0Object2,
                                  uint8_t ram00B0Object3, uint8_t *ram00B0Global,
                                  uint8_t *ram00B1Global, uint8_t *ram00B0Object2Out,
                                  uint8_t *ram00B0Object3Out)
{
    uint8_t updatedY;
    if (ram0060 == 0u || ram00B9Object1 == NULL ||
        ram00C0Object0 == NULL || ram00B0Global == NULL ||
        ram00B1Global == NULL || ram00B0Object2Out == NULL ||
        ram00B0Object3Out == NULL) return;

    updatedY = (uint8_t)(ram0060 + *ram00B9Object1);
    *ram00B9Object1 = updatedY;
    if (updatedY != 0x08u) return;

    *ram00C0Object0 = 0x01u;
    *ram00B9Object1 = 0x00u;
    nesSubF6FBAdvanceAudioPhase(ram00B0Global, ram00B1Global);
    *ram00B0Object2Out = nesSubECBFIncrementAndWrap(ram00B0Object2);
    *ram00B0Object3Out = nesSubECBFIncrementAndWrap(ram00B0Object3);
}


/* bra_C411: stores the supplied sprite X, clears its attribute and ram0046. */
void nesLocC411WriteSpriteX(uint8_t a, uint8_t spriteOam[4], uint8_t *ram0046)
{
    if (spriteOam != NULL) {
        spriteOam[3] = a;
        spriteOam[2] = 0u;
    }
    if (ram0046 != NULL) *ram0046 = 0u;
}

/* loc_C3EF: the entry consists solely of STA ram_spr_Y; RTS. */
void nesLocC3EFWriteSpriteY(uint8_t a, uint8_t *spriteY)
{
    if (spriteY != NULL) *spriteY = a;
}


/* sub_C409: Y indexes tbl_C088_spr_X; writes sprite tile $FA, X from the
 * table, attribute $00 and ram0046=$00. A is $00 on return. */
uint8_t nesSubC409SetupStaticSprite(uint8_t y, const uint8_t tblC088[256],
                                    uint8_t spriteOam[4], uint8_t *ram0046)
{
    if (tblC088 == NULL || spriteOam == NULL) return 0u;
    spriteOam[1] = 0xFAu;
    spriteOam[3] = tblC088[y];
    spriteOam[2] = 0u;
    if (ram0046 != NULL) *ram0046 = 0u;
    return 0u;
}


/* Additional translations from bank_FF.asm; each models the listed entry's RAM effects. */
void nesSubCFDBClearUpperFlags(uint8_t ram0094, uint8_t *ram005C)
{ if (ram005C != NULL && ram0094 != 0u) *ram005C = (uint8_t)(*ram005C & 0x0Fu); }

void nesSubCFCBCompareObjectPosition(uint8_t ram0094, uint8_t ram0078,
                                     uint8_t ram0090, uint8_t ram0074, uint8_t *ram005C)
{
    if (ram005C == NULL) return;
    if (ram0094 < ram0078 || (ram0094 == ram0078 && ram0090 < ram0074))
        *ram005C = (uint8_t)(*ram005C | 0x80u);
    else if (ram0094 != 0u)
        *ram005C = (uint8_t)(*ram005C & 0x0Fu);
}

/* sub_CFE3: LDA #$F0; AND ram_005C_obj,X; STA ram_005C_obj,X. */
void nesSubCFE3MaskFlags(uint8_t *ram005C)
{
    if (ram005C != NULL) *ram005C = (uint8_t)(*ram005C & 0xF0u);
}

/* sub_CFEA calls sub_CFE3 before adding the unsigned comparison marker. */
void nesSubCFEAUpdateComparisonFlags(uint8_t ram007C, uint8_t ram00AC, uint8_t *ram005C)
{
    if (ram005C == NULL) return;
    nesSubCFE3MaskFlags(ram005C);
    if (ram007C != 0u && ram007C != ram00AC)
        *ram005C = (uint8_t)(*ram005C | (ram007C < ram00AC ? 1u : 2u));
}

/*
 * sub_C7DE: source-level menu cursor/input helper.
 * NES controller bits: Select=$20, Start=$10, Up=$08, Down=$04.
 * The exclusive bound matches the ASM: valid selections are 1..count-1.
 * This models this helper's RAM effects, not the complete menu state machine.
 */
uint8_t nesSubC7DEMenuInput(uint8_t buttonPress, uint8_t count,
                            uint8_t *selection, uint8_t *repeatGuard,
                            uint8_t *timer30, uint8_t *timer31,
                            uint8_t *selectionChanged)
{
    uint8_t y;
    if (selection == NULL || repeatGuard == NULL || timer30 == NULL ||
        timer31 == NULL || selectionChanged == NULL || count < 2u)
        return 0u;

    y = *selection;
    *selectionChanged = 0u;
    /* AND #$3C; no relevant held buttons clears ram_0049 and returns X=0. */
    if ((buttonPress & 0x3Cu) == 0u) {
        *repeatGuard = 0u;
        return 0u;
    }

    *timer31 = 0x5Du;
    if (*repeatGuard != 0u) return 0u;

    /*
     * 6502 order matters: Select has priority, then Down, then Up.
     * Any other bit in the $3C mask follows the timer30/action-X=4 path.
     */
    if ((buttonPress & 0x20u) != 0u || (buttonPress & 0x04u) != 0u) {
        y = (uint8_t)(y + 1u);
        if (y >= count) y = 1u;
        *selectionChanged = 1u;
    } else if ((buttonPress & 0x08u) != 0u) {
        y = (uint8_t)(y - 1u);
        if ((y & 0x80u) != 0u) y = (uint8_t)(count - 1u);
        *selectionChanged = 1u;
    } else {
        *timer30 = 4u;
    }

    *selection = y;
    *repeatGuard = (uint8_t)(*repeatGuard + 1u);
    return ((buttonPress & 0x20u) != 0u ||
            (buttonPress & 0x04u) != 0u ||
            (buttonPress & 0x08u) != 0u) ? 0u : 4u;
}

/* sub_C3AF: INC ram_0041 (8-bit wrap is intentional). */
void nesSubC3AFIncrementState(uint8_t *ram0041)
{
    if (ram0041 != NULL) *ram0041 = (uint8_t)(*ram0041 + 1u);
}

/* sub_C3B1: clear ram_0040 and ram_0044, and nothing else. */
void nesSubC3B1ResetFrameFlags(uint8_t *ram0040, uint8_t *ram0044)
{
    if (ram0040 != NULL) *ram0040 = 0u;
    if (ram0044 != NULL) *ram0044 = 0u;
}

/*
 * ofs_000_C3BD_08 confirmation branches after sub_C7DE returns X != 0.
 * Directly mirrors the branches and calls sub_C3AF/sub_C3B1 where ASM does.
 */
void nesSubC3BDConfirmSelection(uint8_t actionX, uint8_t *ram0041,
                                uint8_t *ram0042, uint8_t *ram03F2,
                                uint8_t *clearRamRequested,
                                uint8_t *ram0040, uint8_t *ram0044)
{
    uint8_t selection;
    if (actionX == 0u || ram0041 == NULL || ram0042 == NULL ||
        ram03F2 == NULL || clearRamRequested == NULL)
        return;

    selection = *ram0042;
    if (selection < 2u) {
        *ram03F2 = selection;
        nesSubC3AFIncrementState(ram0041);
        nesSubC3B1ResetFrameFlags(ram0040, ram0044);
    } else if (selection == 2u) {
        nesSubC3AFIncrementState(ram0041);
        nesSubC3B1ResetFrameFlags(ram0040, ram0044);
    } else if (selection == 5u) {
        *ram0042 = (uint8_t)(selection >> 1);
        *clearRamRequested = 1u;
    } else {
        *ram0041 = (uint8_t)(selection + 9u);
        *ram0042 = 0u;
        nesSubC3B1ResetFrameFlags(ram0040, ram0044);
    }
}

uint8_t nesSubCE58ObjectIndexMatches(uint8_t x, uint8_t ram004C)
{ return (uint8_t)(x == ram004C); }

uint8_t nesLocCAFAFillCommand(uint8_t x, uint8_t ram0304To040Au[263])
{
    unsigned int i;
    if (ram0304To040Au == NULL) return x;
    for (i=0u;i<7u;++i) ram0304To040Au[4u+(uint8_t)(x+(uint8_t)i)] = 0xFEu;
    return (uint8_t)(x+7u);
}

void nesLocCD13CopySpriteTemplate(uint8_t oam[256], const uint8_t tblC150[8])
{ unsigned int i; if(oam!=NULL&&tblC150!=NULL)for(i=0u;i<8u;++i)oam[0xC0u+i]=tblC150[i]; }

uint8_t nesLocD0A6StoreAndReturn(uint8_t x, uint8_t a, uint8_t ram000D, uint8_t ram007C[256])
{ if(ram007C!=NULL)ram007C[x]=a; return ram000D; }

void nesLocD197WriteScrollRegister(uint8_t a, uint8_t *ppuScrollWrite)
{ if(ppuScrollWrite!=NULL)*ppuScrollWrite=a; }

void nesLocEC47WriteFixedSprite(uint8_t oam[256], uint8_t ram05E0)
{ if(oam!=NULL){oam[0x34]=0xCFu;oam[0x35]=ram05E0;oam[0x36]=0u;oam[0x37]=0xE4u;} }

void nesLocCA85ClearRam(uint8_t cpuRam[256])
{
    uint8_t saved42; unsigned int i;
    if(cpuRam==NULL)return;
    saved42=cpuRam[0x42];
    for(i=0x21u;i<=0xFFu;++i)cpuRam[i]=0u;
    cpuRam[0xFC]=0u;
    cpuRam[0x49]=(uint8_t)(cpuRam[0x49]+1u);
    cpuRam[0x42]=saved42;
}

uint8_t nesSubC339WaitForVblank(const uint8_t statusReads[], unsigned int count,
                               uint8_t ramFor2000, uint8_t *ppuCtrlWrite,
                               uint8_t *ramFor2000Out, unsigned int *readsUsed)
{
    unsigned int i;
    if(readsUsed!=NULL)*readsUsed=0u;
    if(statusReads==NULL||ppuCtrlWrite==NULL||ramFor2000Out==NULL)return 0u;
    for(i=0u;i<count;++i){
        if(readsUsed!=NULL)*readsUsed=i+1u;
        if((statusReads[i]&0x80u)==0u){
            uint8_t v=(uint8_t)(ramFor2000|0x80u);
            *ppuCtrlWrite=v;*ramFor2000Out=v;return 1u;
        }
    }
    return 0u; /* The real 6502 loops forever if no terminating status occurs. */
}

uint8_t nesSubD19BWaitForStatus(const uint8_t statusReads[], unsigned int count,
                                uint8_t ramFor2000, uint8_t *ppuCtrlWrite,
                                uint8_t *ramFor2000Out, unsigned int *readsUsed)
{
    unsigned int i;
    if(readsUsed!=NULL)*readsUsed=0u;
    if(statusReads==NULL||ppuCtrlWrite==NULL||ramFor2000Out==NULL)return 0u;
    for(i=0u;i<count;++i){
        if(readsUsed!=NULL)*readsUsed=i+1u;
        if((statusReads[i]&0x40u)!=0u){
            uint8_t v=(uint8_t)(ramFor2000&0xF0u);
            *ppuCtrlWrite=v;*ramFor2000Out=v;return 1u;
        }
    }
    return 0u;
}

void nesSubD347ReadControllers(const uint8_t samples0[8], const uint8_t samples1[8],
                               uint8_t previous0, uint8_t previous1,
                               uint8_t *hold0, uint8_t *press0,
                               uint8_t *hold1, uint8_t *press1, uint8_t strobeWrites[2])
{
    if(strobeWrites!=NULL){strobeWrites[0]=1u;strobeWrites[1]=0u;}
    if(samples0!=NULL&&hold0!=NULL&&press0!=NULL)nesSubD35BDecodeController(samples0,previous0,hold0,press0);
    if(samples1!=NULL&&hold1!=NULL&&press1!=NULL)nesSubD35BDecodeController(samples1,previous1,hold1,press1);
}

uint8_t nesLocC4FCComputeScaledValue(uint8_t a, uint8_t ram0009, uint8_t ram000A, uint8_t *ram0053)
{
    if(a<ram0009){
        a=(uint8_t)(a+ram0009);
        while((ram000A=(uint8_t)(ram000A>>1))!=0u)a=(uint8_t)(a>>1);
    }else{
        a=(uint8_t)(a-ram0009);a=(uint8_t)(a>>1);a=(uint8_t)(a+4u);
    }
    if(ram0053!=NULL)*ram0053=a;
    return a;
}

/* Full sub_C92C -> sub_C53E -> sub_CB06/CB13 table-selection path. */
void nesSubC92CSelectTableEntry(uint8_t a, uint8_t ram0043, uint8_t ram03F2,
                                const uint8_t cpuRam[2048], uint8_t *ram0002,
                                uint8_t *ram0003, uint8_t *ram0005,
                                uint8_t *ram0006, uint8_t *ram0007)
{
    uint8_t index;
    if (cpuRam == NULL) return;
    if (ram0002 != NULL) *ram0002 = ram0043;
    if (ram0003 != NULL) *ram0003 = a;
    if (ram0043 == 5u) {
        index = 0x20u;
    } else {
        uint8_t counter = ram0043;
        index = 0u;
        for (;;) {
            counter = (uint8_t)(counter - 1u); /* DEY; branch on N */
            if ((counter & 0x80u) != 0u) break;
            index = (uint8_t)(index + 3u); /* CLC; ADC #$03 */
        }
    }
    if (ram03F2 != 0u) index = (uint8_t)(index + 0x23u);
    if (ram0005 != NULL) *ram0005 = cpuRam[0x0580u + index];
    if (ram0006 != NULL) *ram0006 = cpuRam[0x0581u + index];
    if (ram0007 != NULL) *ram0007 = cpuRam[0x0582u + index];
}

static void nesSubDFA2DivideByTen(uint8_t input, uint8_t *outX, uint8_t *outA)
{
    uint8_t x=0u, a=input;
    for (;;) {
        x=(uint8_t)(x+1u);
        a=(uint8_t)(a-10u); /* SEC; SBC #$0A */
        if ((a&0x80u)!=0u) break; /* BPL loop stops when N=1 */
    }
    x=(uint8_t)(x-1u); /* DEX */
    a=(uint8_t)(a+10u); /* ADC #$0A; carry is clear after negative SBC */
    if(outX!=NULL)*outX=x;
    if(outA!=NULL)*outA=a;
}

/* Full sub_C935 including sub_DF7F/sub_DFA2 and the seven-byte copy. */
uint8_t nesSubC935BuildCommand(uint8_t ram0005, uint8_t ram0006, uint8_t ram0007,
                               uint8_t x, uint8_t ram03D1To03D7[7],
                               uint8_t ram0304To040Au[263])
{
    uint8_t quotient, remainder;
    unsigned int i;
    if(ram03D1To03D7==NULL||ram0304To040Au==NULL)return x;
    nesSubDFA2DivideByTen(ram0007,&quotient,&remainder);
    ram03D1To03D7[5]=quotient; ram03D1To03D7[6]=remainder;
    nesSubDFA2DivideByTen(ram0006,&quotient,&remainder);
    ram03D1To03D7[2]=quotient; ram03D1To03D7[3]=remainder;
    nesSubDFA2DivideByTen(ram0005,&quotient,&remainder);
    ram03D1To03D7[0]=remainder; /* quotient is intentionally discarded */
    for(i=0u;i<7u;++i)
        ram0304To040Au[4u+(uint8_t)(x+(uint8_t)i)]=ram03D1To03D7[i];
    return (uint8_t)(x+7u);
}

uint8_t nesSubECC9CheckSpriteBytes(uint8_t y, const uint8_t ram0540[256],
                                   const uint8_t ram0480[256], const uint8_t ram0400[256],
                                   const uint8_t ram04C0[256])
{
    if(ram0540==NULL||ram0480==NULL||ram0400==NULL||ram04C0==NULL)return 1u;
    return (uint8_t)(ram0540[y]!=0x3Du||ram0480[y]!=0x3Du||
                     ram0400[y]!=0x3Bu||ram04C0[y]!=0x3Bu);
}
