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
