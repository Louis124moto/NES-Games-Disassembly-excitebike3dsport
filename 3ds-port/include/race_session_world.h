#ifndef EXCITEBIKE_RACE_SESSION_WORLD_H
#define EXCITEBIKE_RACE_SESSION_WORLD_H

#include <stdint.h>
#include "native_race_session.h"
#include "race_world.h"

/* Seed the unified race world from the composed first-course session,
 * including the C820 CPU course lookup pages and the native RNG state. */
void nativeRaceSessionBuildWorld(const NativeRaceSession *session, uint8_t multiplayer_flag,
                                 RaceWorld *world);
/* One native race frame: NES-layout held/pressed buttons in, semantic state
 * and OAM copied back to the session. Returns the raceLoopStep unsupported mask. */
uint32_t nativeRaceSessionStepFrame(NativeRaceSession *session, RaceWorld *world,
                                    uint8_t nes_hold, uint8_t nes_press);

#endif
