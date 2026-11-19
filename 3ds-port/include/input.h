#ifndef EXCITEBIKE_INPUT_H
#define EXCITEBIKE_INPUT_H

#include <3ds.h>

typedef struct InputState {
    u32 held;
    u32 pressed;
} InputState;

void inputUpdate(InputState *input);

/* Map libctru's physical key bits to the original NES controller byte. */
uint8_t inputMapNesButtons(u32 keys);
uint8_t inputNesHeld(const InputState *input);
uint8_t inputNesPressed(const InputState *input);

#endif
