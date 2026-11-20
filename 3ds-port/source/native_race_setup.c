#include <string.h>
#include "native_race_setup.h"

/* bank_FF.asm: tbl_C0B0, tbl_C0B4, tbl_C0B8 and ofs_001_C875_02_06. */
static const uint8_t s_object_class[4] = { 0x09u, 0x0Bu, 0x0Du, 0x0Fu };
static const uint8_t s_start_y[4] = { 0x0Eu, 0x1Au, 0x26u, 0x32u };
static const uint8_t s_start_x[4] = { 0x38u, 0x48u, 0x58u, 0x68u };

int nativeRaceSetupFirstCourse(NativeRaceSetupState *out,
                               const NativeRaceSetupInput *in,
                               BikeMovementState *movement,
                               BikeStateMachineState *states)
{
    unsigned int i;
    unsigned int last;
    unsigned int half_lock_slot;

    if (out == 0 || in == 0 || movement == 0 || states == 0)
        return 0;

    memset(out, 0, sizeof(*out));
    out->setup_counter_0040 = (uint8_t)(in->setup_counter_0040 + 1u);
    out->setup_counter_0047 = (uint8_t)(in->setup_counter_0047 + 1u);
    out->timer_0034 = in->timer_0034 != 0u ? in->timer_0034 : 0x05u;
    out->global_vertical_reference = 0xA0u;
    out->player_slot = 0u;
    out->player_rider_present = 1u;

    /* C8A5 selects X=3..0 when ram_03F2 != 0, otherwise only X=0. */
    last = in->multiplayer_flag != 0u ? 3u : 0u;
    for (i = 0u; i <= last; ++i) {
        out->bike[i].y = s_start_y[i];
        out->bike[i].x_low = s_start_x[i];
        out->bike[i].x_high = 0x01u;
        out->bike[i].object_class = s_object_class[i];
        out->bike[i].approach = 0x06u;
        out->bike[i].lock_timer = 0x06u;
        out->bike[i].active = 0x01u;
        out->bike[i].animation_seed = 0x08u;
        out->bike[i].speed_phase = (uint8_t)(in->object_param_0019[i] >> 1);

        movement->object_y[i] = s_start_y[i];
        movement->x_low[i] = s_start_x[i];
        movement->x_high[i] = 0x01u;
        movement->object_class[i] = s_object_class[i];
        movement->approach[i] = 0x06u;
        movement->start_lock_timer[i] = 0x06u;
        movement->object_active[i] = 0x01u;

        states->y[i] = s_start_y[i];
    }

    /* C8D3: X wrapped to $FF, so ram_0019,X reads zero-page $0018. */
    half_lock_slot = (unsigned int)(in->wrapped_param_0018 & 0x03u);
    if (half_lock_slot == 0u)
        half_lock_slot = 2u;
    out->half_lock_slot = (uint8_t)half_lock_slot;
    out->bike[half_lock_slot].lock_timer = movement->start_lock_timer[half_lock_slot];
    out->bike[half_lock_slot].lock_timer >>= 1;
    movement->start_lock_timer[half_lock_slot] >>= 1;

    /* C89B-C8A1 writes only the player rider-presence byte ($0370). */
    states->rider_present[0] = 0x01u;
    movement->global_vertical_reference = 0xA0u;
    return 1;
}
