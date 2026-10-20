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
/* Exact translations from bank_FF.asm. */
void nesSubDA58UpdateObjectFraction(uint8_t x, uint8_t ram0094[4],
                                    uint8_t ram0060[4], uint8_t ram0090[4],
                                    uint8_t ram0394[4]);
uint8_t nesSubE091AdjustValue(uint8_t value);
uint8_t nesSubE098AdjustValue(uint8_t value);

/* sub_DFA2/sub_DFA4: exact 6502 subtraction loop; divisor must be nonzero. */
void nesSubDFA4(uint8_t inputA, uint8_t divisor, uint8_t *outX, uint8_t *outA);
void nesSubDFA2(uint8_t inputA, uint8_t *outX, uint8_t *outA);

/* sub_D0AB/sub_DFB2: byte-exact object vertical-band calculations. */
void nesSubD0ABComputeVerticalBands(const uint8_t ram00B8[4], uint8_t ram0360[4]);
void nesSubDFB2SelectBand(uint8_t x, const uint8_t ram009C[4],
                          const uint8_t ram00B8[4], const uint8_t ram00A4[4],
                          uint8_t ram0070[4], const uint8_t tblD913[5]);


/* Additional byte-exact translations from bank_FF.asm. */
uint8_t nesSubD000ShouldSetupObject(uint8_t x, const uint8_t ram0080[4],
                                    const uint8_t ram001A[4], uint8_t *outY);
void nesSubD11FInitializeUpdateBuffer(uint8_t ram0300_to_0312[19]);
void nesSubD255HideAllSprites(uint8_t oam[256]);
void nesSubD2FFAdvanceBufferIndex(uint8_t *x, uint8_t ram0300,
                                 uint8_t ram0301_to_0400[256],
                                 uint8_t *stackBytesDiscarded);
void nesSubE70BUpdateObjectOffsets(uint8_t ram0060[4], uint8_t ram0064[4],
                                   uint8_t ram00E0[4], uint8_t ram00D8[4]);
uint8_t nesSubE80DPreparePointer(uint8_t y, const uint8_t *const dataPages[6],
                                 const uint8_t **outData);
uint8_t nesSubE803ReadData(uint8_t y, uint8_t dataIndex,
                           const uint8_t *const dataPages[6],
                           uint8_t *savedY, uint8_t *outA);
uint8_t nesSubE7FFReadObjectData(uint8_t x, uint8_t y,
                                 const uint8_t ram00E0[4],
                                 const uint8_t *const dataPages[6],
                                 uint8_t *savedY, uint8_t *outA);
uint8_t nesSubE7FCReadObjectBandData(uint8_t x, const uint8_t ram0360[4],
                                     const uint8_t ram00E0[4],
                                     const uint8_t *const dataPages[6],
                                     uint8_t *savedY, uint8_t *outA);
void nesSubE836UpdateObjectDifferences(uint8_t ram00B0[4], uint8_t ram00BC[4],
                                       uint8_t ram00B8[4], uint8_t ram008C[4],
                                       uint8_t ram03F1);


typedef struct NesPreparedSpriteCommand {
    uint8_t scratch0, scratch1, scratch2, scratch3, scratch4, scratchB, transformY;
} NesPreparedSpriteCommand;

/* Return: 0 = original RTS path, 1 = prepare countdown sprite, 2 = original branch into E2F6 tail. */
uint8_t nesSubE2AFUpdateFinishCountdown(uint8_t delta,uint8_t finished,uint8_t frameTimer,
    uint8_t *low03BD,uint8_t *high03BE,uint8_t *blink03BB,
    const uint8_t tblD8E5[2],NesPreparedSpriteCommand *out);
void nesSubE359UpdateTemperatureMeter(uint8_t finished,uint8_t ram005C,uint8_t raceStarted,
    uint8_t *timer003C,uint8_t *fraction003B5,uint8_t *coarse003B6,
    uint8_t *overheatLatch,uint8_t *ram00FC,uint8_t oam[256]);

typedef struct NesSpriteCullState {
    uint8_t ram00A8[4],ram00B8[4],ram00BC[4],ram00A0[4],ram00A4[4];
    uint8_t ram0058[4],ram00CC[4],ram036C[4],ram0090[4],ram0094[4];
    uint8_t ram0098[4],ram009C[4],ram00B0[4],ram00D4[4],ram0388[4];
    uint8_t ram00C0[4],ram039C[4],ram00DC[4],ram0070[4],ram0370[4];
    uint8_t ram0080[4],ram0084[4],ram0088[4],ram03A6_flag;
} NesSpriteCullState;

uint8_t nesSubE26APrepareSpecialSprite(uint8_t x,uint8_t ram039C3,
    const uint8_t ram0084[4],const uint8_t ram0390[4],const uint8_t ram0080[4],
    const uint8_t ram006C[4],const uint8_t ram00A4[4],const uint8_t ram0398[4],
    const uint8_t tblD90B[3],uint8_t ram000B,NesPreparedSpriteCommand *out);
void nesSubE456CullObjectSprites(uint8_t oam[256],NesSpriteCullState *state,
                                  const uint8_t oamIndex[4],const uint8_t spriteOffset[3]);
void nesSubE16CWriteSpriteStrip(uint8_t oam[256],uint8_t address,uint8_t firstTile);
void nesSubE3F4SortObjectsByY(uint8_t order[4],const uint8_t y[4],uint8_t active3,uint8_t y3);
void nesSubE4ADHideFourSprites(uint8_t oam[256],uint8_t index,uint8_t offset);
void nesSubE49BHideSelectedSpriteGroups(uint8_t oam[256],uint8_t index,const uint8_t flags[3],const uint8_t offsets[3]);
void nesSubE2AFDecrementCountdown(uint8_t *lo,uint8_t *hi,uint8_t delta);
void nesLocE2BDUpdateFinishBlink(uint8_t finished,uint8_t timer,uint8_t *blink);
uint8_t nesLocE2D0CountdownReachedOne(uint8_t countdown);
void nesSubEA44UpdateWheelSprites(uint8_t oam[256],const uint8_t ram0084[4],
    const uint8_t ram0080[4],const uint8_t ram00B0[4],const uint8_t ram00B4[4],
    const uint8_t ram00BC[4],const uint8_t ramY[4],const uint8_t tblE69A[7],
    const uint8_t ram036C[4],const uint8_t ram0094[4],const uint8_t ram0090[4],
    const uint8_t ram008C[4],uint8_t ram003F);
void nesSubE2F6DrawWheelSprites(uint8_t x,uint8_t yOffset,const uint8_t ram036C[4],
    const uint8_t ram0094[4],const uint8_t ram0090[4],const uint8_t ram00BC[4],
    const uint8_t ram0084[4],const uint8_t ram0080[4],const uint8_t ram008C[4],
    uint8_t ram003F,uint8_t oam[256]);
uint8_t nesSubE2F6ShouldDrawWheel(uint8_t x,const uint8_t r36c[4],const uint8_t r94[4],const uint8_t r90[4],const uint8_t rbc[4],const uint8_t r84[4],const uint8_t r80[4]);
uint8_t nesSubE1A3PrepareObjectSprite(uint8_t x,uint8_t oamAddress,
    const uint8_t ram0080[4],const uint8_t ram008C[4],const uint8_t ram009C[4],
    const uint8_t ram00BC[4],const uint8_t ram0370[4],const uint8_t ram0036[4],
    const uint8_t ram00AC[4],const uint8_t ram0098[4],const uint8_t ram00B0[4],
    const uint8_t ram0058[4],const uint8_t ram00B8[4],const uint8_t ram0090[4],
    const uint8_t ram0094[4],const uint8_t ram00DC[4],uint8_t finished,uint8_t frameFlags,
    const uint8_t tblD892[23],const uint8_t tblD8A9[23],uint8_t oam[256],
    uint8_t *ram039C3,uint8_t *temp,uint8_t *renderType,NesPreparedSpriteCommand *out);
uint8_t nesSubE1F9SelectObjectSprite(uint8_t x,const uint8_t r9c[4],const uint8_t rbc[4],const uint8_t r370[4],const uint8_t r36[4],const uint8_t rac[4],const uint8_t r98[4],const uint8_t rb0[4],const uint8_t r58[4],const uint8_t rb8[4],const uint8_t r90[4],const uint8_t r94[4],const uint8_t rdc[4],uint8_t finished,uint8_t frameFlags,uint8_t *ram039C3,uint8_t *temp);
void nesLocE3AAUpdateHeatFlags(uint8_t heat,uint8_t priorY,uint8_t *timer,uint8_t *latch,uint8_t *flags);

uint8_t nesSubE52CCheckVerticalOverlap(uint8_t x, uint8_t y, const uint8_t ram008c[4]);
void nesSubE4C8CheckOverlappingObjects(const uint8_t ram0084[4], const uint8_t ram0080[4],
                                       const uint8_t ram008c[4], uint8_t *ram000b, uint8_t *ram03a3);

/* Exact loc_E96E block from sub_E96C: four object slots, 8-bit ADC, skip states $14/$15. */
void nesLocE96EApplyVerticalDelta(const uint8_t state[4], uint8_t posY[4], const uint8_t delta[4]);


/* Exact RAM-level helpers from bank_FF.asm, keeping 6502 byte arithmetic. */
void nesSubE84FStorePair(uint8_t x, uint8_t value, uint8_t ram00BC[4], uint8_t ram00E4[4]);
uint8_t nesSubE86FSubtractFromE4(uint8_t x, uint8_t a, const uint8_t ram00E4[4], uint8_t ram00BC[4]);
uint8_t nesSubE893SetOffsetsAndSubtract(uint8_t x, uint8_t a,
                                       const uint8_t ram00C8[4], const uint8_t ram00D0[4],
                                       uint8_t ram00D4[4], uint8_t ram00B4[4]);
void nesSubE73BAddXLow(uint8_t x, const uint8_t ram0060[4], uint8_t posXLo[4]);
void nesSubE733UpdateThreeXLowSlots(const uint8_t ram0060[4], uint8_t posXLo[4]);


uint8_t nesSubE7FCLoadDataIndex(uint8_t x, const uint8_t ram0360[4]);
uint8_t nesSubE7FFLoadDataIndex(uint8_t x, const uint8_t ram00E0[4]);
void nesSubE80DPointerBytes(uint8_t tableIndex, uint8_t *low, uint8_t *high);
uint8_t nesSubE803ReadIndexedRam(uint8_t tableIndex, uint8_t dataIndex, const uint8_t ram0400To07FF[1024]);
void nesOfsE81807Apply(uint8_t x, uint8_t ram00B0[4], const uint8_t ram00AC[4],
                       const uint8_t ram0094[4], const uint8_t ram0090[4], uint8_t ram0098[4]);


void nesOfsE84504Apply(uint8_t x, const uint8_t ram00C8[4], const uint8_t ram00D0[4],
                       uint8_t ram00D8[4], uint8_t ram00D4[4], uint8_t ram00B4[4],
                       uint8_t ram00BC[4], uint8_t ram00E4[4]);
void nesOfsE85405Apply(uint8_t x, const uint8_t ram00C8[4], const uint8_t ram00D0[4],
                       const uint8_t ram00E4[4], uint8_t ram00D4[4], uint8_t ram00B4[4],
                       uint8_t ram00BC[4]);
void nesOfsE85D02Apply(uint8_t x, const uint8_t ram00C8[4], const uint8_t ram00D0[4],
                       uint8_t ram00D8[4], uint8_t ram00D4[4], uint8_t ram00B4[4],
                       uint8_t ram00BC[4], uint8_t ram00E4[4]);
void nesOfsE86A03Apply(uint8_t x, const uint8_t ram00C8[4], const uint8_t ram00D0[4],
                       const uint8_t ram00E4[4], uint8_t ram00D4[4], uint8_t ram00B4[4],
                       uint8_t ram00BC[4]);
void nesOfsE87909Apply(uint8_t x, const uint8_t ram00C8[4], const uint8_t ram00D0[4],
                       const uint8_t ram00A0[4], uint8_t ram00D8[4],
                       uint8_t ram00D4[4], uint8_t ram00B4[4],
                       uint8_t ram00BC[4], uint8_t ram00E4[4]);


void nesOfsE8BF0EReset(uint8_t x, uint8_t ram00A4[4], uint8_t ram00A0[4]);
void nesOfsE8C60FSetStateFromY(uint8_t x, uint8_t yLow, uint8_t ram00A4[4]);
void nesOfsE8E301SetA0(uint8_t x, uint8_t ram00A0[4]);
void nesOfsE8F514SetPair30(uint8_t x, uint8_t ram00BC[4], uint8_t ram00E4[4]);
void nesOfsE8FA10SetPair10(uint8_t x, uint8_t ram00BC[4], uint8_t ram00E4[4]);
void nesOfsE96300ClearThree(uint8_t x, uint8_t ram00CC[4], uint8_t ram00D4[4], uint8_t ram00B4[4]);


/* Portable setup fragments from bank_FF.asm; PPU writes are returned as effects. */
void nesSubD0F4PrepareCommandStream(const uint8_t ram03D9To03DF[7],
    uint8_t ram03D0To03D7[8], uint8_t *sourceLo, uint8_t *sourceHi,
    uint8_t *dataLo, uint8_t *dataHi);
void nesSubD1A9PrepareScrollWrites(uint8_t scrollWrites[2], uint8_t *statusReads);
void nesSubD1B3PrepareScrollWrites(uint8_t ramFor2000, uint8_t baseNametable,
    uint8_t scrollX, uint8_t scrollY, uint8_t *ppuCtrl,
    uint8_t scrollWrites[2], uint8_t *statusReads);


/* 2026-10-21: exact bank_FF.asm RAM/register-effect helpers. */
void nesSubC333WritePpuCtrl(uint8_t a, uint8_t *ramFor2000, uint8_t *ppuCtrl);
void nesSubC3AFIncrementRaceCounter(uint8_t *ram0041);
void nesSubC3B1ClearCounters(uint8_t *ram0040, uint8_t *ram0044);
void nesSubC621CopyPage(const uint8_t ram06E0[256], uint8_t ram05E0[256]);
void nesSubC68AIncrementPointer(uint8_t *lo, uint8_t *hi);
uint8_t nesSubC971AddCommandLength(uint8_t a, uint8_t ram0300, uint8_t *newRam0300, uint8_t *xOut);
void nesSubCA1BCopyButtonHold(uint8_t ramButtonHold, uint8_t *ram005C);
uint8_t nesSubCB13ComputeSpriteCode(uint8_t yIn, uint8_t ram0046Flag);
void nesSubCFE3ClearObjectFlags(uint8_t x, uint8_t ram005C[256]);
void nesSubD24FDisableRendering(uint8_t *ppuMask);
uint8_t nesSubCF8BCarryOut(uint8_t a);
uint8_t nesSubECBFIncrementAndWrap(uint8_t a);
void nesSubF676FillCommandPayload(uint8_t ram0300To0315[22]);
uint8_t nesSubF826AdvanceObject(uint8_t x, uint8_t *ram00B7, uint8_t *ram00C5);
void nesSubF82EPreparePointer(uint8_t x, const uint8_t tblF063Lo[256], const uint8_t tblF087Hi[256], uint8_t *ptrLo, uint8_t *ptrHi);
void nesSubF839PreparePointer(uint8_t x, const uint8_t tblF4F5Lo[256], const uint8_t tblF4FAHi[256], uint8_t *ptrLo, uint8_t *ptrHi);
void nesSubC62DControllerStrobe(uint8_t *controller4016);
void nesSubF6FBAdvanceAudioPhase(uint8_t *ram00B0, uint8_t *ram00B1);
void nesSubF871WriteAudioTimer(uint8_t a, const uint8_t tblFF00[256], uint8_t byteAt0000, uint8_t audio4000To400F[16]);
void nesSubF885WriteAudioTimer(uint8_t a, const uint8_t tblFF00[256], uint8_t byteAt0000, uint8_t audio4000To400F[16]);


/* Exact translations audited against bank_FF.asm, 2026-10-21. */
void nesSubC1FDInitializeRam(const uint8_t tblC091[256], uint8_t cpuRam[2048]);
void nesSubC229InitializeRows(uint8_t cpuRam[2048]);
void nesSubC32FMaskPpuCtrl(uint8_t *ramFor2000, uint8_t *ppuCtrl);
uint8_t nesSubCB06ComputeTableIndex(uint8_t ram0002, uint8_t ram0046Flag, uint8_t ram03F2);
void nesSubC53ESelectTableEntry(uint8_t ram0002, uint8_t ram0046Flag, uint8_t ram03F2,
                                const uint8_t cpuRam[2048],
                                uint8_t *ram0005, uint8_t *ram0006, uint8_t *ram0007);
void nesSubC53ASelectTableEntry(uint8_t ram0043, uint8_t ram0046Flag, uint8_t ram03F2,
                                uint8_t *ram0002, const uint8_t cpuRam[2048],
                                uint8_t *ram0005, uint8_t *ram0006, uint8_t *ram0007);
void nesSubC716ConfigureLoop(uint8_t *ram000D, uint8_t *ram000E, uint8_t *ram000F,
                             uint8_t *ram0005, uint8_t *ram0006);
void nesSubC726ConfigureLoop(uint8_t *ram000D, uint8_t *ram000E, uint8_t *ram000F,
                             uint8_t *ram0005, uint8_t *ram0006);
uint8_t nesSubDC97SubtractObjectY(uint8_t ram03F1, uint8_t x, const uint8_t ram00BC[4],
                                  const uint8_t ramPosYLo[4], uint8_t *carryOut);
void nesSubEC25WriteSprite(uint8_t a, uint8_t ram00C2, const uint8_t tblE6F6[256], uint8_t oam[256]);
void nesSubEC5DWriteSprite(uint8_t ram003F, uint8_t ram00C2, const uint8_t tblE6F6[256], uint8_t oam[256]);
void nesSubEC3BCopyPageDescending(uint8_t cpuRam[2048]);
void nesSubEC75InitializeObjectPositions(uint8_t ram00B0[4], uint8_t ram00C0[4],
                                         const uint8_t tblE6E1[256],
                                         uint8_t ramPosXLo[4], uint8_t ramPosYLo[4],
                                         uint8_t ram00BC[4]);
void nesSubF681CopyCommandBytes(uint16_t sourcePointer, uint8_t xIn, uint8_t yIn,
                                const uint8_t cpuMemory[65536], uint8_t cpuRam[2048],
                                uint8_t *xOut, uint8_t *yOut);
uint8_t nesSubF821InitializeBufferAndAdvance(uint8_t x, uint8_t cpuRam[2048]);
void nesSubF86BWriteAudioRegisters(uint8_t x, uint8_t y, uint8_t a,
                                   const uint8_t tblFF00[256], uint8_t byteAt0000,
                                   uint8_t audio4000To400F[16]);
void nesSubF889WriteThirdAudioTimer(uint8_t a, const uint8_t tblFF00[256],
                                    uint8_t byteAt0000, uint8_t audio4000To400F[16]);
void nesSubC94BBuildCommand(uint8_t *ram0300, uint8_t tableIndex,
                            const uint8_t tblC0EC[257], uint8_t ram0300To040A[267],
                            uint8_t *xOut);
void nesSubC64EControllerDelay(uint8_t a, uint8_t *aOut, uint8_t *ram0007,
                               uint8_t *ram000C, uint8_t controllerWrites[2], uint8_t delayIterations[2]);
void nesSubC653ControllerDelay(uint8_t a, uint8_t *aOut, uint8_t *ram0007,
                               uint8_t *ram000C, uint8_t controllerWrites[2], uint8_t delayIterations[2]);

/* Exact translation of sub_FF53 ($FF53). cpuMemory models 6502 address
 * space so tbl_FF3E,Y reads remain correct when the index crosses $FFFF. */
uint8_t nesSubFF53Lookup(uint8_t inputA, uint8_t carryIn, uint8_t ram07F0,
                         const uint8_t cpuMemory[65536], uint8_t *yOut);

/* RAM-level translation of sub_EC99 ($EC99), including its sub_F6FB and
 * sub_ECBF calls. */
void nesSubEC99AdvanceObjectPhase(uint8_t ram0060, uint8_t *ram00B9Object1,
                                  uint8_t *ram00C0Object0, uint8_t ram00B0Object2,
                                  uint8_t ram00B0Object3, uint8_t *ram00B0Global,
                                  uint8_t *ram00B1Global, uint8_t *ram00B0Object2Out,
                                  uint8_t *ram00B0Object3Out);

void nesLocC411WriteSpriteX(uint8_t a, uint8_t spriteOam[4], uint8_t *ram0046);
void nesLocC3EFWriteSpriteY(uint8_t a, uint8_t *spriteY);

uint8_t nesSubC409SetupStaticSprite(uint8_t y, const uint8_t tblC088[256],
                                    uint8_t spriteOam[4], uint8_t *ram0046);


void nesSubCFDBClearUpperFlags(uint8_t ram0094, uint8_t *ram005C);
void nesSubCFCBCompareObjectPosition(uint8_t ram0094, uint8_t ram0078, uint8_t ram0090, uint8_t ram0074, uint8_t *ram005C);
void nesSubCFEAUpdateComparisonFlags(uint8_t ram007C, uint8_t ram00AC, uint8_t *ram005C);
uint8_t nesSubCE58ObjectIndexMatches(uint8_t x, uint8_t ram004C);
uint8_t nesLocCAFAFillCommand(uint8_t x, uint8_t ram0304To040Au[263]);
void nesLocCD13CopySpriteTemplate(uint8_t oam[256], const uint8_t tblC150[8]);
uint8_t nesLocD0A6StoreAndReturn(uint8_t x, uint8_t a, uint8_t ram000D, uint8_t ram007C[256]);
void nesLocD197WriteScrollRegister(uint8_t a, uint8_t *ppuScrollWrite);
void nesLocEC47WriteFixedSprite(uint8_t oam[256], uint8_t ram05E0);
void nesLocCA85ClearRam(uint8_t cpuRam[256]);
uint8_t nesSubC339WaitForVblank(const uint8_t statusReads[], unsigned int count, uint8_t ramFor2000, uint8_t *ppuCtrlWrite, uint8_t *ramFor2000Out, unsigned int *readsUsed);
uint8_t nesSubD19BWaitForStatus(const uint8_t statusReads[], unsigned int count, uint8_t ramFor2000, uint8_t *ppuCtrlWrite, uint8_t *ramFor2000Out, unsigned int *readsUsed);
void nesSubD347ReadControllers(const uint8_t samples0[8], const uint8_t samples1[8], uint8_t previous0, uint8_t previous1, uint8_t *hold0, uint8_t *press0, uint8_t *hold1, uint8_t *press1, uint8_t strobeWrites[2]);
uint8_t nesLocC4FCComputeScaledValue(uint8_t a, uint8_t ram0009, uint8_t ram000A, uint8_t *ram0053);
uint8_t nesSubECC9CheckSpriteBytes(uint8_t y, const uint8_t ram0540[256], const uint8_t ram0480[256], const uint8_t ram0400[256], const uint8_t ram04C0[256]);

void nesSubC92CSelectTableEntry(uint8_t a, uint8_t ram0043, uint8_t ram03F2,
                                const uint8_t cpuRam[2048], uint8_t *ram0002,
                                uint8_t *ram0003, uint8_t *ram0005,
                                uint8_t *ram0006, uint8_t *ram0007);
uint8_t nesSubC935BuildCommand(uint8_t ram0005, uint8_t ram0006, uint8_t ram0007,
                               uint8_t x, uint8_t ram03D1To03D7[7],
                               uint8_t ram0304To040Au[263]);

#endif
