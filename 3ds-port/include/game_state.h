#ifndef EXCITEBIKE_GAME_STATE_H
#define EXCITEBIKE_GAME_STATE_H

#include <stdbool.h>

typedef struct {
    int speed;
    int temperature;
    int track_position;
    int vertical_velocity;
    bool airborne;
    bool crashed;
} BikeState;

typedef struct {
    BikeState bike;
    bool paused;
    bool show_help;
} GameState;

void gameStateReset(GameState *state);

#endif
