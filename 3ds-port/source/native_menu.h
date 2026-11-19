#ifndef EXCITEBIKE_NATIVE_MENU_H
#define EXCITEBIKE_NATIVE_MENU_H

#include <stdint.h>

/*
 * Native menu presentation hook.
 *
 * The old PPU/6502-driven menu implementation has been removed from the
 * executable. This hook intentionally does not emulate PPU state. The native
 * menu state machine and source-derived menu assets will be implemented here
 * in separately tested slices.
 */
typedef struct NativeMenuSelectionState {
    uint8_t selection;       /* source ram_0042 */
    uint8_t debounce;        /* source ram_0049 */
    uint8_t selection_dirty;  /* source ram_00FF */
    uint8_t input_timer;      /* source ram_0031_timer */
    uint8_t start_timer;      /* source ram_0030_timer */
} NativeMenuSelectionState;

/* Exact native translation of sub_C7DE. pressed uses NES controller bit
 * positions; the 3DS-to-NES input mapping is a separate platform adapter. */
uint8_t nativeMenuUpdateSelection(NativeMenuSelectionState *menu,
                               uint8_t pressed,
                               uint8_t entry_count);

typedef struct NativeMenuState {
    uint8_t source_state;       /* ram_0041 */
    NativeMenuSelectionState input;
    uint8_t selected_course;    /* ram_0043 */
    uint8_t selected_mode;      /* ram_03F2 */
    uint8_t setup_command;      /* ram_0040 */
    uint8_t transition_phase;   /* ram_0044 */
    uint8_t cursor_state;       /* ram_004A */
    uint8_t title_flag_03F0;
    uint8_t title_flag_03F3;
    uint8_t title_flag_03F7;
    uint8_t transition_gate_0053;
    uint8_t transition_flag_0046;
    uint8_t transition_flag_004B;
    uint8_t transition_counter_03F8;
    uint8_t transition_duration; /* source ram_03AD */
    uint8_t frame_timer;       /* ram_0020 */
    uint8_t timer_values[29]; /* ram_0021-$003D */
} NativeMenuState;

/* Native state transitions for the original title, course selector and
 * design menu handlers (source states 0, 1 and 8). */
void nativeMenuInit(NativeMenuState *menu);
void nativeMenuUpdate(NativeMenuState *menu, uint8_t pressed);
void nativeMenuTickTimers(NativeMenuState *menu);
void nativeMenuTransitionPhase0(NativeMenuState *menu);
/* Phase-1 timing/state slice only; opaque visual payload is not yet translated. */
void nativeMenuTransitionPhase1(NativeMenuState *menu);

/* Exact timer-expired branch of ofs_000_C465_04 / _07 (sub_C3AF). */
void nativeMenuCompleteTimedTransition(NativeMenuState *menu);

/* Exact phase-3 native translation of ofs_CC6B_03. */
void nativeMenuTransitionPhase3(NativeMenuState *menu);

#endif
