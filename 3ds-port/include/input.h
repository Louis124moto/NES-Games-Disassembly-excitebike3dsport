#ifndef EXCITEBIKE_INPUT_H
#define EXCITEBIKE_INPUT_H

#include <3ds.h>
#include "nes_pad.h"

typedef struct InputState {
    u32 held;
    u32 pressed;      /* 3DS key edges (UI only, e.g. the extractor key) */
    NesPad pad;       /* source controller layer: $14/$15 hold and press */
} InputState;

void inputUpdate(InputState *input);

/* Map libctru's physical key bits to the original NES controller byte.
 * inputNesHeld/inputNesPressed return the source $14/$15 values computed by
 * sub_D347 semantics (nes_pad.c), once per inputUpdate. */
uint8_t inputMapNesButtons(u32 keys);
uint8_t inputNesHeld(const InputState *input);
uint8_t inputNesPressed(const InputState *input);

#endif
