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
