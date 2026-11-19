#include "bike_physics.h"

/* Source: Excitebike/bank_FF.asm, tbl_C0BC ($C0BC), tbl_C0CE ($C0CE),
 * tbl_C0D1 ($C0D1), and sub_CE29 ($CE29-$CE57).
 * The values below are copied from the source tables, not tuned estimates. */
static const uint8_t kAccelerationIncrement[3] = { 0x18u, 0x3Fu, 0x28u };
static const uint8_t kPlayerSpeedFractionCap[3] = { 0x20u, 0x40u, 0x7Fu };
static const uint8_t kPlayerSpeedWholeCap[3] = { 0x03u, 0x03u, 0x01u };

void bikePhysicsAcceleratePlayer(BikeSpeedState *speed,
                                 uint8_t frame_phase,
                                 uint8_t acceleration_mode)
{
    uint16_t sum;

    if (speed == 0 || frame_phase != 0u || acceleration_mode > 2u) {
        return;
    }

    /* CLC / ADC / STA / BCC / INC from sub_CE29. */
    sum = (uint16_t)speed->fraction + kAccelerationIncrement[acceleration_mode];
    speed->fraction = (uint8_t)sum;
    if (sum > 0xFFu) {
        speed->whole = (uint8_t)(speed->whole + 1u);
    }

    /* Only object X=0 is capped here; preserve the source CMP/BCC behavior. */
    if (speed->whole > kPlayerSpeedWholeCap[acceleration_mode] ||
        (speed->whole == kPlayerSpeedWholeCap[acceleration_mode] &&
         speed->fraction >= kPlayerSpeedFractionCap[acceleration_mode])) {
        speed->fraction = kPlayerSpeedFractionCap[acceleration_mode];
        speed->whole = kPlayerSpeedWholeCap[acceleration_mode];
    }
}


/* Source: tbl_C0C1 ($C0C1), tbl_C0CC ($C0CC), sub_CE58/sub_CE5C ($CE58-$CE82). */
static const uint8_t kDecelerationAmount[7] = {
    0x38u, 0x0Cu, 0x00u, 0x3Cu, 0x1Cu, 0xC0u, 0x7Fu
};
void bikePhysicsDeceleratePlayer(BikeSpeedState *speed,
                                 uint8_t frame_phase,
                                 uint8_t deceleration_mode,
                                 uint8_t zero_whole_threshold)
{
    uint8_t old_fraction;
    uint8_t amount;

    if (speed == 0 || frame_phase != 0u ||
        deceleration_mode >= 7u ||
        (zero_whole_threshold != 0x01u && zero_whole_threshold != 0xB0u)) {
        return;
    }

    if (speed->whole == 0u &&
        speed->fraction < zero_whole_threshold) {
        return;
    }

    old_fraction = speed->fraction;
    amount = kDecelerationAmount[deceleration_mode];
    speed->fraction = (uint8_t)(old_fraction - amount);

    /* 6502 SBC sets carry iff no borrow; a borrow lowers the whole byte.
     * If the whole byte is already zero, the source clamps the fraction to 0. */
    if (old_fraction < amount) {
        if (speed->whole != 0u) {
            speed->whole = (uint8_t)(speed->whole - 1u);
        } else {
            speed->fraction = 0u;
        }
    }
}


/* Source: sub_CDEE ($CDEE-$CE28), player object X=0. */
uint8_t bikePhysicsTerrainGate(BikeTerrainGateState *state)
{
    uint8_t high_path;

    if (state == 0) {
        return 0u;
    }

    high_path = (uint8_t)(
        state->object_class == 0xE4u ||
        (state->surface == 0x03u && state->surface_state >= 0x03u) ||
        state->vertical_position < 0x08u ||
        state->vertical_position >= 0x38u);

    if (high_path != 0u) {
        if (state->posture_state != 0x01u) {
            state->posture_state = 0x02u;
        }
        state->player_event_flag = 0x04u;
        return 0u;
    }

    if (state->posture_state != 0x01u) {
        state->posture_state = 0x00u;
    }
    return 1u;
}


/* Player branch of sub_CD59 ($CD59-$CDEB), excluding its later steering/
 * animation side effects at loc_CDBD. The exact called speed/gate helpers
 * are independently differential-tested against their original ASM. */
static int playerSpeedCompareCap(const BikeSpeedState *speed, uint8_t mode)
{
    static const uint8_t whole_cap[3] = { 0x03u, 0x03u, 0x01u };
    static const uint8_t fraction_cap[3] = { 0x20u, 0x40u, 0x7Fu };

    if (speed->whole < whole_cap[mode]) {
        return -1;
    }
    if (speed->whole > whole_cap[mode]) {
        return 1;
    }
    if (speed->fraction < fraction_cap[mode]) {
        return -1;
    }
    if (speed->fraction > fraction_cap[mode]) {
        return 1;
    }
    return 0;
}

void bikePhysicsUpdatePlayerSpeed(BikePlayerSpeedContext *player)
{
    uint8_t deceleration_mode;
    uint8_t acceleration_mode;
    uint8_t zero_whole_threshold;
    uint8_t gate_z;

    if (player == 0 || player->frame_phase > 3u ||
        player->object_status_00B0 > 3u) {
        return;
    }

    zero_whole_threshold =
        (player->object_status_00B0 & 0x02u) != 0u ? 0xB0u : 0x01u;

    /* Early CDBA path: locked/colliding or non-running object state. */
    if (player->object_lock_0374 != 0u ||
        player->collision_lock_0098 != 0u ||
        (player->movement_state_009C != 0u &&
         player->movement_state_009C != 5u)) {
        bikePhysicsDeceleratePlayer(&player->speed, player->frame_phase,
                                    0u, zero_whole_threshold);
        return;
    }

    /* X=0 special case in CD70: after finish, use Y=2 and its lower cap. */
    if (player->race_finished != 0u) {
        const int cap_order = playerSpeedCompareCap(&player->speed, 2u);
        if (cap_order < 0) {
            bikePhysicsAcceleratePlayer(&player->speed, player->frame_phase, 2u);
        } else if (cap_order > 0) {
            bikePhysicsDeceleratePlayer(&player->speed, player->frame_phase,
                                        3u, zero_whole_threshold);
        }
        return;
    }

    /* B0 != 0 selects the slow/recovery deceleration table path. */
    if (player->object_status_00B0 != 0u) {
        const uint8_t direction = (uint8_t)(player->controller & 0x03u);
        deceleration_mode = direction == 0u ? 4u : (uint8_t)(direction + 1u);
        bikePhysicsDeceleratePlayer(&player->speed, player->frame_phase,
                                    deceleration_mode, zero_whole_threshold);
        return;
    }

    /* Without either upper controller bit, the player coasts via Y=0. */
    if ((player->controller & 0xC0u) == 0u) {
        bikePhysicsDeceleratePlayer(&player->speed, player->frame_phase,
                                    0u, zero_whole_threshold);
        return;
    }

    /* ASL of the masked $C0 bits: bit 7 wins (Y=0), bit 6 alone gives Y=1. */
    acceleration_mode = (player->controller & 0x80u) != 0u ? 0u : 1u;
    gate_z = bikePhysicsTerrainGate(&player->terrain);

    /* sub_CD8F: a non-zero gate result blocks throttle only while whole speed != 0. */
    if (gate_z == 0u && player->speed.whole != 0u) {
        bikePhysicsDeceleratePlayer(&player->speed, player->frame_phase,
                                    0u, zero_whole_threshold);
        return;
    }

    /* CD9A accelerates below the cap, does nothing at the exact cap, and
     * increments Y then calls CE58 to decelerate when above the cap. */
    {
        const int cap_order = playerSpeedCompareCap(&player->speed, acceleration_mode);
        if (cap_order < 0) {
            bikePhysicsAcceleratePlayer(&player->speed, player->frame_phase,
                                        acceleration_mode);
        } else if (cap_order > 0) {
            bikePhysicsDeceleratePlayer(&player->speed, player->frame_phase,
                                        (uint8_t)(acceleration_mode + 1u),
                                        zero_whole_threshold);
        }
    }
}


/* Source: sub_E359 ($E359-$E3F3), tbl_D8F7 ($D8F7-$D8FA),
 * tbl_D8FB ($D8FB-$D8FE). This translates gameplay meter/overheat state;
 * the final 12-sprite OAM loop remains in the native renderer. */
uint8_t bikePhysicsUpdateTemperature(BikeTemperatureState *temperature,
                                     uint8_t controller,
                                     uint8_t race_started,
                                     uint8_t race_finished)
{
    static const uint8_t heat_increment[4] = { 0x3Fu, 0x0Fu, 0x07u, 0x07u };
    static const uint8_t heat_threshold[4] = { 0x08u, 0x20u, 0x11u, 0x11u };
    uint8_t mode = 0u;

    if (temperature == 0) {
        return 0xFEu;
    }

    if (race_finished != 0u) {
        return 0xFEu;
    }

    if (temperature->overheat_state != 0u) {
        return (temperature->overheat_timer & 1u) != 0u ? 0xFEu : 0xFCu;
    }

    if ((controller & 0xC0u) != 0u) {
        const uint8_t throttle = race_started != 0u
            ? (uint8_t)(controller & 0xC0u)
            : 0x80u;
        if (throttle == 0x40u) {
            mode = 1u;
        } else if (throttle == 0x80u) {
            mode = 2u;
        } else {
            mode = 3u;
        }
    }

    if (temperature->whole < heat_threshold[mode]) {
        const uint16_t sum = (uint16_t)temperature->fraction + heat_increment[mode];
        temperature->fraction = (uint8_t)sum;
        if (sum > 0xFFu) {
            temperature->whole = (uint8_t)(temperature->whole + 1u);
        }
    } else if (temperature->whole > heat_threshold[mode]) {
        const uint8_t old_fraction = temperature->fraction;
        temperature->fraction = (uint8_t)(old_fraction - 0x0Bu);
        if (old_fraction < 0x0Bu && temperature->whole != 0u) {
            temperature->whole = (uint8_t)(temperature->whole - 1u);
        }
    }

    if (temperature->whole >= 0x20u) {
        temperature->overheat_timer = temperature->whole;
        temperature->overheat_state = temperature->whole;
    }

    if (temperature->whole >= 0x1Au) {
        if (mode == 1u) {
            temperature->sound_flags = (uint8_t)(temperature->sound_flags | 0xB0u);
        }
    }

    return 0xFEu;
}


/* Source: sub_DA58 ($DA58-$DA69), all four bike slots. */
void bikePhysicsBuildMovementDeltas(BikeMovementState *movement)
{
    unsigned i;
    if (movement == 0) {
        return;
    }
    for (i = 0; i < 4u; ++i) {
        const uint16_t sum = (uint16_t)movement->speed_fraction[i] +
                             movement->fraction_accumulator[i];
        movement->movement_delta[i] = movement->speed_whole[i];
        movement->fraction_accumulator[i] = (uint8_t)sum;
        if (sum > 0xFFu) {
            movement->movement_delta[i] =
                (uint8_t)(movement->movement_delta[i] + 1u);
        }
    }
}

/* Source: sub_DA6A ($DA6A-$DA79). The base nametable toggles only on carry. */
void bikePhysicsAdvancePlayerScroll(BikeMovementState *movement)
{
    uint16_t sum;
    if (movement == 0) {
        return;
    }
    sum = (uint16_t)movement->player_scroll_x + movement->movement_delta[0];
    movement->player_scroll_x = (uint8_t)sum;
    if (sum > 0xFFu) {
        movement->base_nametable ^= 0x01u;
    }
}

/* Source: sub_DA7A ($DA7A-$DAA0). Slots 3,2,1 are opponent-relative;
 * slot 0 is the player and moves the camera instead. previous_x is shifted
 * for every opponent slot, even when its active flag is clear. */
void bikePhysicsAdvanceOpponentPositions(BikeMovementState *movement)
{
    int i;
    if (movement == 0) {
        return;
    }
    for (i = 3; i >= 1; --i) {
        if (movement->object_active[i] != 0u) {
            const uint16_t sum = (uint16_t)movement->x_low[i] +
                                 movement->movement_delta[i];
            const uint8_t moved_low = (uint8_t)sum;
            uint8_t moved_high = movement->x_high[i];
            uint8_t relative_low;
            if (sum > 0xFFu) {
                moved_high = (uint8_t)(moved_high + 1u);
            }

            relative_low = (uint8_t)(moved_low - movement->movement_delta[0]);
            if (moved_low < movement->movement_delta[0]) {
                moved_high = (uint8_t)(moved_high - 1u);
            }
            movement->x_low[i] = relative_low;
            movement->x_high[i] = moved_high;
        }
        movement->previous_x[i] =
            (uint8_t)(movement->previous_x[i] - movement->movement_delta[0]);
    }
}

