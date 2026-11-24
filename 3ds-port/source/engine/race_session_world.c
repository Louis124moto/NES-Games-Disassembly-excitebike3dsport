#include "race_session_world.h"
#include "race_loop.h"
#include "race_world_glue.h"
#include <string.h>


void nativeRaceSessionBuildWorld(const NativeRaceSession *session, uint8_t multiplayer_flag,
                                 RaceWorld *world)
{
    RaceSpawnState spawn;
    unsigned page;

    if (session == 0 || world == 0 || session->initialized == 0u) return;

    memset(world, 0, sizeof(*world));
    /* D0F4's decimal-time display uses $FB for both separators. */
    *raceWorldByte(world, 0x3D2u) = 0xFBu;
    *raceWorldByte(world, 0x3D5u) = 0xFBu;
    /* C2E3 enabled all four APU channels before the race; pause ($CA71/$CA5E) rewrites $4015. */
    *raceWorldByte(world, 0x4015u) = 0x0Fu;
    raceWorldStore_BikeMovementState(&session->movement, world);
    raceWorldStore_BikeStateMachineState(&session->states, world);
    /* D0AB/D0C6 are part of the original first-course setup. Their outputs
     * feed E733's script gate, AI terrain samples and opponent placement. */
    memcpy(raceWorldByte(world, 0x360u), session->track.vertical_band,
           sizeof(session->track.vertical_band));
    memcpy(raceWorldByte(world, 0xC0u), session->track.terrain_code,
           sizeof(session->track.terrain_code));
    memcpy(raceWorldByte(world, 0x3C0u), session->track.terrain_samples,
           sizeof(session->track.terrain_samples));

    /* C1CE's eight-byte seed is shared by opponent spawning, AI and race-tail
     * routines. It must exist in the world before the first frame. */
    memcpy(raceWorldByte(world, 0x18u), session->rng.bytes, sizeof(session->rng.bytes));

    *raceWorldByte(world, 0x3F2u) = (uint8_t)(multiplayer_flag != 0u);
    *raceWorldByte(world, 0x34u) = session->setup.timer_0034;
    *raceWorldByte(world, 0xE8u) = session->setup.camera_scroll_00E8;
    *raceWorldByte(world, 0xE9u) = session->setup.camera_scroll_00E9;
    *raceWorldByte(world, 0xEAu) = session->setup.camera_scroll_00EA;
    *raceWorldByte(world, 0xEBu) = session->setup.camera_scroll_00EB;
    *raceWorldByte(world, 0x3B4u) = session->setup.countdown_x_03B4;
    *raceWorldByte(world, 0x3B0u) = session->setup.race_setup_state_03B0;
    *raceWorldByte(world, 0x3BDu) = session->setup.countdown_limit_03BD;
    *raceWorldByte(world, 0x3BEu) = session->setup.countdown_index_03BE;
    *raceWorldByte(world, 0x41u) = 2u;
    *raceWorldByte(world, 0x46u) = session->setup.transition_flag_0046;
    /* C36C's $004A=1 excludes the human player (slot 0) from CED0 AI. */
    *raceWorldByte(world, 0x4Au) = session->setup.first_opponent_slot;

    /* C820's CPU-side course profile is six consecutive 64-byte pages:
     * $0400-$047F = $3B, $0480-$057F = $3D. These are lookup/AI data,
     * not the PPU background or a fabricated visual track. */
    raceWorldLoad_RaceSpawnState(&spawn, world);
    for (page = 0u; page < 6u; ++page) {
        memset(spawn.track_pages[page], page < 2u ? 0x3Bu : 0x3Du,
               sizeof(spawn.track_pages[page]));
    }
    memcpy(spawn.rng, session->rng.bytes, sizeof(spawn.rng));
    raceWorldStore_RaceSpawnState(&spawn, world);

    memcpy(raceWorldByte(world, 0x200u), session->oam.bytes, sizeof(session->oam.bytes));
}

uint32_t nativeRaceSessionStepFrame(NativeRaceSession *session, RaceWorld *world,
                                    uint8_t nes_hold, uint8_t nes_press)
{
    uint32_t mask;
    const uint8_t previousColumn = world != 0 ? *raceWorldByte(world, 0xE8u) : 0u;
    if (session == 0 || world == 0 || session->initialized == 0u) return 0xFFFFFFFFu;
    *raceWorldByte(world, 0x14u) = nes_hold;
    *raceWorldByte(world, 0x15u) = nes_press;
    raceLoopNmiTick(world);
    mask = raceLoopStep(world);
    /* F4FF emits one source PPU column whenever $00E8 advances. Preserve the
     * complete 18-byte command so the renderer can apply it exactly once. */
    if (*raceWorldByte(world, 0xE8u) != previousColumn) {
        memcpy(session->course_column_buffer, raceWorldByte(world, 0x300u),
               sizeof(session->course_column_buffer));
        ++session->course_column_serial;
    }
    raceWorldLoad_BikeMovementState(&session->movement, world);
    raceWorldLoad_BikeStateMachineState(&session->states, world);
    memcpy(session->track.vertical_band, raceWorldByte(world, 0x360u),
           sizeof(session->track.vertical_band));
    memcpy(session->track.terrain_code, raceWorldByte(world, 0xC0u),
           sizeof(session->track.terrain_code));
    memcpy(session->track.terrain_samples, raceWorldByte(world, 0x3C0u),
           sizeof(session->track.terrain_samples));
    memcpy(session->rng.bytes, raceWorldByte(world, 0x18u), sizeof(session->rng.bytes));
    memcpy(session->oam.bytes, raceWorldByte(world, 0x200u), sizeof(session->oam.bytes));
    return mask;
}
