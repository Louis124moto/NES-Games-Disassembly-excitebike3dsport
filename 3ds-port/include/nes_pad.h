#ifndef EXCITEBIKE_NES_PAD_H
#define EXCITEBIKE_NES_PAD_H

#include <stdint.h>

/* Source controller layer (sub_D347/D35B): per pad, $hold = the raw serial
 * byte (A,B,Select,Start,Up,Down,Left,Right = bit7..bit0) and $press = the
 * same byte with A removed while A was already held last frame. Only A is an
 * edge; every other "press" bit is level, exactly as the source stores it. */
typedef struct NesPad {
    uint8_t hold;
    uint8_t press;
} NesPad;

void nesPadReset(NesPad *pad);
void nesPadScan(NesPad *pad, uint8_t raw);

#endif
