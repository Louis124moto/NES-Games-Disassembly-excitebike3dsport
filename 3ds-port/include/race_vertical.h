#ifndef EXCITEBIKE_RACE_VERTICAL_H
#define EXCITEBIKE_RACE_VERTICAL_H

#include <stdint.h>
#include "bike_physics.h"

/*
 * Native port of the first-course E96C vertical pass and EA07-EA43 input tail.
 * Handles phase-zero bounds/drift, nonzero $00A4 phases and DD06 jump setup.
 * Returns zero when all slots are supported, or $0F for invalid arguments.
 */
/* E836/DC97: convert world Y and vertical reference to sprite-screen Y. */
void bikePhysicsUpdateScreenY(BikeMovementState *movement);

uint8_t bikePhysicsUpdateFirstCourseVertical(BikeMovementState *movement,
                                             BikeStateMachineState *states);

#endif
