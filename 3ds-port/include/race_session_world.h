#ifndef EXCITEBIKE_RACE_SESSION_WORLD_H
#define EXCITEBIKE_RACE_SESSION_WORLD_H

#include <stdint.h>
#include "native_race_session.h"
#include "race_world.h"

/* Seed the unified race world from the composed first-course session.
 * Known gap (documented in FIRST_COURSE_TODO.md): the $0400-$0446 HUD/column
 * leftovers of ECE4/D13F/D0C6/D0F4 are not yet produced natively and start zeroed. */
void nativeRaceSessionBuildWorld(const NativeRaceSession *session, uint8_t multiplayer_flag,
                                 RaceWorld *world);
/* One native race frame: NES-layout held buttons in, OAM copied back to the session
 * and movement/state structs refreshed. Returns the raceLoopStep unsupported mask. */
uint32_t nativeRaceSessionStepFrame(NativeRaceSession *session, RaceWorld *world,
                                    uint8_t nes_hold);

#endif
