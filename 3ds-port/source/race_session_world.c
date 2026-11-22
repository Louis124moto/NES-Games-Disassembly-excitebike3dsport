#include "race_session_world.h"
#include "race_loop.h"
#include "race_world_glue.h"
#include <string.h>

void nativeRaceSessionBuildWorld(const NativeRaceSession *session, uint8_t multiplayer_flag,
                                 RaceWorld *world)
{
    if (session == 0 || world == 0) return;
    memset(world, 0, sizeof(*world));
    raceWorldStore_BikeMovementState(&session->movement, world);
    raceWorldStore_BikeStateMachineState(&session->states, world);
    *raceWorldByte(world, 0x3F2u) = (uint8_t)(multiplayer_flag != 0u);
    *raceWorldByte(world, 0x34u) = session->setup.timer_0034;
    *raceWorldByte(world, 0xE8u) = session->setup.camera_scroll_00E8;
    *raceWorldByte(world, 0xE9u) = session->setup.camera_scroll_00E9;
    *raceWorldByte(world, 0xEAu) = session->setup.camera_scroll_00EA;
    *raceWorldByte(world, 0xEBu) = session->setup.camera_scroll_00EB;
    *raceWorldByte(world, 0x3B4u) = session->setup.countdown_x_03B4;
    *raceWorldByte(world, 0x3B0u) = session->setup.race_setup_state_03B0;
    if (raceWorldByte(world, 0x3BDu) != 0) *raceWorldByte(world, 0x3BDu) = session->setup.countdown_limit_03BD;
    if (raceWorldByte(world, 0x3BEu) != 0) *raceWorldByte(world, 0x3BEu) = session->setup.countdown_index_03BE;
    *raceWorldByte(world, 0x41u) = 2u;
    memcpy(raceWorldByte(world, 0x200u), session->oam.bytes, sizeof(session->oam.bytes));
}

uint32_t nativeRaceSessionStepFrame(NativeRaceSession *session, RaceWorld *world,
                                    uint8_t nes_hold)
{
    uint32_t mask;
    if (session == 0 || world == 0) return 0xFFFFFFFFu;
    *raceWorldByte(world, 0x14u) = nes_hold;
    *raceWorldByte(world, 0x15u) = 0u;
    raceLoopNmiTick(world);
    mask = raceLoopStep(world);
    raceWorldLoad_BikeMovementState(&session->movement, world);
    raceWorldLoad_BikeStateMachineState(&session->states, world);
    memcpy(session->oam.bytes, raceWorldByte(world, 0x200u), sizeof(session->oam.bytes));
    return mask;
}
