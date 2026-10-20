#include "bike_physics.h"

/*
 * Placeholder only. Authentic Excitebike acceleration, heat, ramps, airtime,
 * posture and crash timing must be derived from the NES disassembly before
 * this module is considered gameplay-complete.
 */
void bikePhysicsUpdate(BikeState *bike, const InputState *input)
{
    if (bike == 0 || input == 0) {
        return;
    }

    if (input->held & KEY_A) {
        if (bike->speed < 100) {
            bike->speed++;
        }
    } else if (bike->speed > 0) {
        bike->speed--;
    }

    if (input->held & KEY_B) {
        if (bike->speed < 100) {
            bike->speed += 2;
            if (bike->speed > 100) {
                bike->speed = 100;
            }
        }
        if (bike->temperature < 100) {
            bike->temperature++;
        }
    } else if (bike->temperature > 0) {
        bike->temperature--;
    }

    if (input->held & KEY_LEFT) {
        bike->track_position--;
    }
    if (input->held & KEY_RIGHT) {
        bike->track_position++;
    }
}
