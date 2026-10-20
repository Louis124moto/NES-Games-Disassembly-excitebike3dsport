#ifndef EXCITEBIKE_INPUT_H
#define EXCITEBIKE_INPUT_H

#include <3ds.h>

typedef struct {
    u32 held;
    u32 pressed;
} InputState;

void inputUpdate(InputState *input);

#endif
