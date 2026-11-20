#ifndef EXCITEBIKE_RACE_VERTICAL_H
#define EXCITEBIKE_RACE_VERTICAL_H

#include <stdint.h>
#include "bike_physics.h"

/*
 * First-course phase-zero path of sub_E96C (E96E-EA05).
 * Returns a bit mask for bike slots whose nonzero $00A4 phase is not yet
 * translated; those slots are left untouched rather than guessed.
 */
/* E836/DC97: convert world Y and vertical reference to sprite-screen Y. */
void bikePhysicsUpdateScreenY(BikeMovementState *movement);

uint8_t bikePhysicsUpdateFirstCourseVertical(BikeMovementState *movement,
                                             BikeStateMachineState *states);

#endif
