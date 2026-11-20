#ifndef EXCITEBIKE_RACE_COUNTDOWN_H
#define EXCITEBIKE_RACE_COUNTDOWN_H

#include <stdint.h>
#include "bike_physics.h"

typedef struct BikeRaceCountdownState {
    uint8_t timer_003C;
    uint8_t overheat_timer_03B6;
} BikeRaceCountdownState;

typedef enum BikeRaceCountdownRenderEvent {
    BIKE_COUNTDOWN_RENDER_NONE = 0,
    BIKE_COUNTDOWN_RENDER_START_OVERLAY,
    BIKE_COUNTDOWN_RENDER_COUNTDOWN
} BikeRaceCountdownRenderEvent;

/*
 * Semantic native port of sub_DD8D.
 * OAM-only subroutines CCDD/CD13 are represented by a render event; no NES
 * OAM or PPU state is created. The source gameplay fields are updated in place.
 */
BikeRaceCountdownRenderEvent bikePhysicsUpdateRaceCountdown(
    BikeRaceCountdownState *countdown,
    BikeMovementState *movement,
    BikeStateMachineState *states);

#endif
