#ifndef EXCITEBIKE_NES_ROUTINES_H
#define EXCITEBIKE_NES_ROUTINES_H

#include <stdint.h>

/*
 * Exact byte-level translation of bank_FF.asm sub_DA15 ($DA15).
 *
 * Inputs are the three RAM bytes read by the original routine:
 *   ram_0390_obj,X
 *   ram_0080_obj,X
 *   ram_obj_pos_Y_lo,X
 *
 * This helper is deliberately separate from the placeholder bike physics.
 * It does not assign a gameplay meaning to the resulting RAM byte.
 */
uint8_t nesSubDA15DerivedY(uint8_t ram0390, uint8_t ram0080, uint8_t ramY);

/* Exact byte-level translation of sub_D310 ($D310). timers[0] is RAM $0021;
 * timers[28] is RAM $003D. */
void nesSubD310DecreaseAllTimers(uint8_t *timer0020, uint8_t timers[29]);

/* Exact byte-level translation of sub_D326 ($D326). bytes[0..7] map to
 * RAM $0018..$001F and are rotated in-place. */
void nesSubD326RotateInputBits(uint8_t bytes[8]);

/* Portable translation of the RAM-level behavior of sub_D35B ($D35B).
 * Samples are the eight raw reads from $4016/$4017; outputs are NES button masks. */
void nesSubD35BDecodeController(const uint8_t serial_samples[8], uint8_t previous_hold,
                                uint8_t *hold, uint8_t *press);

/* Exact byte-level translation of sub_DA7A ($DA7A). Arrays have four slots;
 * delta[0] is the global RAM[$0060] byte; only object slots 3, 2, 1 update. */
void nesSubDA7AUpdateScrollSlots(uint8_t active[4], const uint8_t delta[4],
                                uint8_t posXLo[4], uint8_t posXHi[4],
                                uint8_t ram0390[4]);


/* Exact RAM-level translations from bank_FF.asm. */
void nesSubDBFEUpdateScroll(uint8_t ram0060, uint8_t *ram03BF,
                            uint8_t *scrollX, uint8_t *baseNametable);
uint8_t nesSubDC9A(uint8_t ram03F1, uint8_t ram00BC, uint8_t ramY);
void nesSubDCDE(uint8_t *timer0374, uint8_t ram005C);
void nesSubDD38SetupObject(uint8_t x, uint8_t ram0090[4], uint8_t ram0094[4],
                           uint8_t ram0388[4], uint8_t ram0378[4],
                           uint8_t ram037C[4], uint8_t ram0380[4],
                           uint8_t ram00B0[4], uint8_t *ram00FF);
uint8_t nesSubDD6F(uint8_t x, uint8_t carryIn, uint8_t ram0380[4],
                   uint8_t ram038C[4], uint8_t ram0384[4],
                   uint8_t ram037C[4], uint8_t temp0001);
void nesSubDB95ClearOpponentBikeData(uint8_t x, uint8_t ram00A8[4],
    uint8_t ram00B8[4], uint8_t ram00BC[4], uint8_t ram00A0[4],
    uint8_t ram00A4[4], uint8_t ram0058[4], uint8_t ram00CC[4],
    uint8_t ram036C[4], uint8_t ram0090[4], uint8_t ram0094[4],
    uint8_t ram0098[4], uint8_t ram009C[4], uint8_t ram00B0[4],
    uint8_t ram00D4[4], uint8_t ram0388[4], uint8_t ram00C0[4],
    uint8_t ram039C[4], uint8_t ram00DC[4], uint8_t ram0070[4],
    uint8_t ram0370[4]);

/* Exact translations from bank_FF.asm. */
void nesSubDA6AUpdateScroll(uint8_t *scrollX, uint8_t delta, uint8_t *baseNametable);
void nesSubDB50SetupObject(uint8_t x, uint8_t y, uint8_t trackFinished,
                           uint8_t ram0084[4], uint8_t ram001B[4],
                           uint8_t ram0070[4], uint8_t ram0078[4],
                           uint8_t ram0074[4], uint8_t ram0080[4],
                           uint8_t ram0094Player, uint8_t ram0090Player,
                           const uint8_t tblD8D5[256], const uint8_t tblD8CD[16]);
void nesSubDCA0SetupSpriteHeight(uint8_t x, uint8_t ram00D4[4],
    uint8_t trackFinished, uint8_t ram0098Player, uint8_t ram009CPlayer,
    uint8_t ram0058Player, uint8_t ram00A4[4], uint8_t tblD88B[256],
    uint8_t ram0368[4]);
/* Exact translation of shared entry loc_DCC7 ($DCC7). */
void nesLocDCC7ApproachValue(uint8_t x, uint8_t target, uint8_t duration,
                             uint8_t timers[4], uint8_t values[4]);
/* Exact translations from bank_FF.asm. */\nvoid nesSubDA58UpdateObjectFraction(uint8_t x, uint8_t ram0094[4],\n                                    uint8_t ram0060[4], uint8_t ram0090[4],\n                                    uint8_t ram0394[4]);\nuint8_t nesSubE091AdjustValue(uint8_t value);\nuint8_t nesSubE098AdjustValue(uint8_t value);\n\n#endif
