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
static uint8_t playerSpeedBelowCap(const BikeSpeedState *speed, uint8_t mode)
{
    static const uint8_t whole_cap[3] = { 0x03u, 0x03u, 0x01u };
    static const uint8_t fraction_cap[3] = { 0x20u, 0x40u, 0x7Fu };

    if (speed->whole < whole_cap[mode]) {
        return 1u;
    }
    if (speed->whole > whole_cap[mode]) {
        return 0u;
    }
    return (uint8_t)(speed->fraction < fraction_cap[mode]);
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
        if (playerSpeedBelowCap(&player->speed, 2u) != 0u) {
            bikePhysicsAcceleratePlayer(&player->speed, player->frame_phase, 2u);
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

    /* CD9A performs this cap check before calling CE29; do not clamp an
     * already-over-cap speed here, because the original caller simply skips. */
    if (playerSpeedBelowCap(&player->speed, acceleration_mode) != 0u) {
        bikePhysicsAcceleratePlayer(&player->speed, player->frame_phase,
                                    acceleration_mode);
    }
}

void bikePhysicsUpdate(BikeState *bike, const InputState *input)
{
    (void)bike;
    (void)input;
}
