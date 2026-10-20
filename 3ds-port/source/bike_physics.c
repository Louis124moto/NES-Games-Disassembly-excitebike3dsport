#include "bike_physics.h"

/*
 * Deliberately disabled until the player's movement/acceleration state path
 * has been traced from bank_FF.asm and translated with regression coverage.
 * The previous speed/temperature increments were scaffolding, not NES physics;
 * running them would violate the port's fidelity requirement.
 */
void bikePhysicsUpdate(BikeState *bike, const InputState *input)
{
    (void)bike;
    (void)input;
}
