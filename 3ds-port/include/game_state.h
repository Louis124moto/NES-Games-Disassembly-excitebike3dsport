#ifndef EXCITEBIKE_GAME_STATE_H
#define EXCITEBIKE_GAME_STATE_H

#include <stdbool.h>
#include <stdint.h>

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

    /* Partial, source-derived menu input model for ASM state 8 / sub_C7DE. */
    uint8_t menu_asm_state;
    uint8_t menu_ram03F2;
    uint8_t menu_clear_ram_requested;
    uint8_t menu_selection;
    uint8_t menu_repeat_guard;
    uint8_t menu_timer30;
    uint8_t menu_timer31;
    uint8_t menu_selection_changed;
    uint8_t menu_action_x;
} GameState;

void gameStateReset(GameState *state);

#endif
