#include "nes_routines.h"

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
