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
    uint8_t menu_timer33;
    uint8_t menu_timer34;
    /* RAM $0300-$031F: $0300 = buffer length, $0301.. = sub_D3C1 records (dynamic PPU buffer) */
    uint8_t menu_ram0300[32];
    uint8_t menu_ram004D_EF[0xA3];
    /* Persistent 2 KiB NES CPU RAM for source-recompiled setup routines. */
    uint8_t menu_cpu_ram[0x800];
    uint8_t menu_scroll_x;
    uint8_t menu_scroll_y;
    uint8_t menu_ppu_ctrl;
    uint8_t menu_ppu_mask;
    /* Valid only after the source-derived menu path has populated these PPU values. */
    uint8_t menu_ppu_background_ready;
    uint8_t menu_sprite0_hit_found;
    uint8_t menu_sprite0_hit_last_frame;
    uint32_t menu_sprite0_hit_cycle;       /* CPU cycles from pre-render to first sprite-0 hit */
    uint32_t menu_ppu_cycle_phase;         /* PPU phase at the C97C handler after D14E */
    /* PPU palette RAM $3F00-$3F1F (BG 0-15, sprite 16-31; $10/$14/$18/$1C mirror $00/$04/$08/$0C). */
    uint8_t menu_ppu_palette[32];
    uint8_t menu_ram_for_2001;        /* ram_for_2001: written only by the NMI model */
    uint8_t menu_ppu_palette_loaded;  /* a palette list ($12-$19) has been executed */
    uint8_t menu_ppu_revision;        /* incremented per applied static upload (redraw trigger) */
    /* ram_oam ($0200): sprite 0 = ram_spr_Y/T/A/X = menu_oam[0..3] (the menu cursor). */
    uint8_t menu_oam[256];
    /* RAM $0060-$009F (tape block 1): [1..4] = tbl_C180 header, [5..7] = $0065.., [8..10] = $0068.. */
    uint8_t menu_ram0060[64];
    uint8_t menu_ramA0[16];    /* RAM $00A0-$00AF: zero-page PPU list used by con_0045_1A */
    uint8_t menu_nmi_enabled;  /* $2000 bit 7: sub_C32F clears it, sub_C339 sets it (main thread / NMI) */
    uint8_t menu_tape_phase;   /* main-thread progress of the Famicom data-recorder handoff */
    uint8_t menu_ram05A0[4];
    uint8_t menu_ram05C3[4];
    /* CPU-side mirror of PPU $2000-$27FF used by sub_D23B/sub_D260. */
    uint8_t menu_ppu_nametables[2048];
    uint8_t menu_selection_changed;
    uint8_t menu_action_x;
    uint8_t menu_ram0040;
    uint8_t menu_ram0043;
    uint8_t menu_ram0044;
    uint8_t menu_ram004A;
    uint8_t menu_ram03F7;
    uint8_t menu_ram03F0;
    uint8_t menu_ram03F3;
    uint8_t menu_ram0046;
    uint8_t menu_ram0048;
    uint8_t menu_ram004B;
    uint8_t menu_ram05E0[256];
    uint8_t menu_ram06E0[256];
    uint8_t menu_ram03F8;
    uint8_t menu_ram0045;
    uint8_t menu_ram00FB;
    uint8_t menu_ram00FF; /* set by sub_C7DE on Up/Down/Select, consumed by later state logic */
} GameState;

void gameStateReset(GameState *state);

#endif
