#include "bike_physics.h"

/* Source: Excitebike/bank_FF.asm, tbl_C0BC ($C0BC), tbl_C0CE ($C0CE),
 * tbl_C0D1 ($C0D1), and sub_CE29 ($CE29-$CE57).
 * The values below are copied from the source tables, not tuned estimates. */
static const uint8_t kAccelerationIncrement[2] = { 0x18u, 0x3Fu };
static const uint8_t kPlayerSpeedFractionCap[2] = { 0x20u, 0x40u };
static const uint8_t kPlayerSpeedWholeCap[2] = { 0x03u, 0x03u };

void bikePhysicsAcceleratePlayer(BikeSpeedState *speed,
                                 uint8_t frame_phase,
                                 uint8_t acceleration_mode)
{
    uint16_t sum;

    if (speed == 0 || frame_phase != 0u || acceleration_mode > 1u) {
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

void bikePhysicsUpdate(BikeState *bike, const InputState *input)
{
    (void)bike;
    (void)input;
}
