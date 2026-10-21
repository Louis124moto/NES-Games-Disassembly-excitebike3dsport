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
    uint8_t menu_timer20;
    uint8_t menu_timer30;
    uint8_t menu_timer31;
    uint8_t menu_timer34;
    uint8_t menu_sequence_preface_pending;
    uint8_t menu_scroll_x;
    uint8_t menu_scroll_y;
    uint8_t menu_ppu_ctrl;
    uint8_t menu_selection_changed;
    uint8_t menu_action_x;
    uint8_t menu_ram0040;
    uint8_t menu_ram0043;
    uint8_t menu_ram0044;
    uint8_t menu_ram004A;
    uint8_t menu_ram03F7;
    uint8_t menu_sprite_y;
    uint8_t menu_sprite_x;
    uint8_t menu_sprite_attribute;
    uint8_t menu_ram03F0;
    uint8_t menu_ram03F3;
    uint8_t menu_ram0046;
    uint8_t menu_ram03F8;
    uint8_t menu_ram0045;
    uint8_t menu_ram00FB;
    uint8_t menu_sprite_tile;
} GameState;

void gameStateReset(GameState *state);

#endif
