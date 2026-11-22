#include <string.h>
#include "race_loop.h"
#include "race_world_glue.h"
#include "nes_routines.h"
#include "race_vertical.h"
#include "race_tail.h"

#define P(a) raceWorldByte(world, (a))

/* tbl_D88B ($D88B), tbl_D913 ($D913): constant gameplay tables. */
static const uint8_t k_tblD88B[32] = {
    0x06, 0x03, 0x04, 0x02, 0x0B, 0x08, 0x09, 0x58, 0x60, 0x69, 0x71, 0x79, 0x80, 0x0A, 0x18, 0x20,
    0x28, 0x30, 0x38, 0x40, 0x48, 0x50, 0x50, 0x97, 0x90, 0x89, 0x11, 0xA6, 0x9F, 0xB3, 0x01, 0x00
};
static const uint8_t k_tblD913[5] = { 0x08, 0x14, 0x20, 0x2C, 0x38 };

static uint8_t stageCA08(RaceWorld *world)
{
    RaceFrameControlState c;
    uint8_t table[256];
    int x;
    memset(table, 0, sizeof(table));
    memcpy(table, k_tblD88B, sizeof(k_tblD88B));
    for (x = 3; x >= 0; --x) {
        nesSubDCA0SetupSpriteHeight((uint8_t)x, P(0xD4), *P(0x52), *P(0x98), *P(0x9C), *P(0x58), P(0xA4), table, P(0x368));
        nesSubDFB2SelectBand((uint8_t)x, P(0x9C), P(0xB8), P(0xA4), P(0x70), k_tblD913);
    }
    raceWorldLoad_RaceFrameControlState(&c, world);
    c.unsupported = 0u;
    raceFrameInputStep(&c);
    raceWorldStore_RaceFrameControlState(&c, world);
    return c.unsupported;
}

static uint8_t stageCD1F(RaceWorld *world)
{
    BikeRaceSpeedState race;
    int x;
    memset(&race, 0, sizeof(race));
    race.race_started = *P(0x4F);
    race.frame_phase = *P(0x4C);
    race.race_finished = *P(0x52);
    for (x = 0; x < 4; ++x) {
        BikeObjectSpeedContext *o = &race.bikes[x];
        race.active[x] = P(0xA8)[x];
        o->object_index = (uint8_t)x;
        o->speed.fraction = P(0x90)[x];
        o->speed.whole = P(0x94)[x];
        o->terrain.object_class = P(0xC0)[x];
        o->terrain.surface = P(0xA4)[x];
        o->terrain.surface_state = P(0x70)[x];
        o->terrain.vertical_position = P(0xB8)[x];
        o->terrain.posture_state = P(0x36C)[x];
        o->terrain.player_event_flag = *P(0xFD);
        o->controller = P(0x5C)[x];
        o->frame_phase = race.frame_phase;
        o->race_finished = race.race_finished;
        o->object_lock_0374 = P(0x374)[x];
        o->collision_lock_0098 = P(0x98)[x];
        o->overheat_lock_03E0 = P(0x3E0)[x];
        o->movement_state_009C = P(0x9C)[x];
        o->object_status_00B0 = P(0xB0)[x];
        o->inactive_0058 = P(0x58)[x];
        o->approach_00AC = P(0xAC)[x];
        o->animation_state_0368 = P(0x368)[x];
        o->movement_mode_0388 = P(0x388)[x];
        o->steer_timer_0026 = P(0x26)[x];
        o->approach_timer_002A = P(0x2A)[x];
    }
    bikePhysicsUpdateRaceSpeeds(&race);
    for (x = 0; x < 4; ++x) {
        const BikeObjectSpeedContext *o = &race.bikes[x];
        if (race.active[x] == 0u) continue;
        P(0x90)[x] = o->speed.fraction;
        P(0x94)[x] = o->speed.whole;
        P(0x70)[x] = o->terrain.surface_state;
        P(0x36C)[x] = o->terrain.posture_state;
        *P(0xFD) = o->terrain.player_event_flag;
        P(0x5C)[x] = o->controller;
        P(0x98)[x] = o->collision_lock_0098;
        P(0xAC)[x] = o->approach_00AC;
        P(0x368)[x] = o->animation_state_0368;
        P(0x388)[x] = o->movement_mode_0388;
        P(0x26)[x] = o->steer_timer_0026;
        P(0x2A)[x] = o->approach_timer_002A;
    }
    /* CD40-CD58 handshake tail. */
    if (*P(0x34) == 0u && (*P(0x5C) & 0xC0u) != 0u && (*P(0x3A9) | *P(0x52)) == 0u) {
        *P(0x3A9) = 1u;
        *P(0xFE) = 1u;
    }
    return 0u;
}

/* NMI prefix before the state handler: INC ram_003F and sub_D310 while not paused
 * (vec_C23B_NMI $C262-$C26C). Controller reads (sub_D347) are the input layer. */
void raceLoopNmiTick(RaceWorld *world)
{
    if ((*P(0x3B1) | *P(0x3B2)) != 0u) return;
    *P(0x3F) = (uint8_t)(*P(0x3F) + 1u);
    nesSubD310DecreaseAllTimers(P(0x20), P(0x21));
}

uint8_t raceLoopRunStage(RaceWorld *world, RaceLoopStage stage)
{
    switch (stage) {
    case RACE_STAGE_CA08: return stageCA08(world);
    case RACE_STAGE_D918: { BikeStateMachineState s; raceWorldLoad_BikeStateMachineState(&s, world);
        bikePhysicsUpdateBikeStates(&s); raceWorldStore_BikeStateMachineState(&s, world); return 0u; }
    case RACE_STAGE_DD8D: { BikeRaceCountdownState c; BikeMovementState m; BikeStateMachineState s;
        raceWorldLoad_BikeRaceCountdownState(&c, world); raceWorldLoad_BikeMovementState(&m, world);
        raceWorldLoad_BikeStateMachineState(&s, world);
        (void)bikePhysicsUpdateRaceCountdown(&c, &m, &s);
        raceWorldStore_BikeRaceCountdownState(&c, world); raceWorldStore_BikeMovementState(&m, world);
        raceWorldStore_BikeStateMachineState(&s, world); return 0u; }
    case RACE_STAGE_E733: case RACE_STAGE_E927: { RaceScriptState s; raceWorldLoad_RaceScriptState(&s, world);
        s.unsupported_mask = 0u;
        if (stage == RACE_STAGE_E733) raceScriptUpdateOpponents(&s); else raceScriptDispatchLateral(&s);
        raceWorldStore_RaceScriptState(&s, world); return s.unsupported_mask != 0u; }
    case RACE_STAGE_CA9B: { RaceFrameControlState c; raceWorldLoad_RaceFrameControlState(&c, world);
        c.unsupported = 0u; raceFinishUpdate(&c); raceWorldStore_RaceFrameControlState(&c, world); return c.unsupported; }
    case RACE_STAGE_DA9F: case RACE_STAGE_DBC4: { RaceSpawnState s; raceWorldLoad_RaceSpawnState(&s, world);
        if (stage == RACE_STAGE_DA9F) raceSpawnUpdate(&s); else raceSpawnHandleSlot3(&s);
        raceWorldStore_RaceSpawnState(&s, world); return 0u; }
    case RACE_STAGE_DEBB: { RaceOpponentCueState s; raceWorldLoad_RaceOpponentCueState(&s, world);
        raceOpponentCue(&s); raceWorldStore_RaceOpponentCueState(&s, world); return 0u; }
    case RACE_STAGE_DF30: { RaceElapsedTimeState s; raceWorldLoad_RaceElapsedTimeState(&s, world);
        raceElapsedTimeUpdate(&s); raceWorldStore_RaceElapsedTimeState(&s, world); return 0u; }
    case RACE_STAGE_EAC5: { RaceBannerSpriteState s; raceWorldLoad_RaceBannerSpriteState(&s, world);
        raceBannerSprites(&s); raceWorldStore_RaceBannerSpriteState(&s, world); return 0u; }
    case RACE_STAGE_DFD5: { RaceBikeContactState s; raceWorldLoad_RaceBikeContactState(&s, world);
        raceBikeContacts(&s); raceWorldStore_RaceBikeContactState(&s, world); return 0u; }
    case RACE_STAGE_E09F: { RaceObstacleHitState s; raceWorldLoad_RaceObstacleHitState(&s, world);
        raceObstacleHit(&s); raceWorldStore_RaceObstacleHitState(&s, world); return 0u; }
    case RACE_STAGE_DDD1: { RaceCrashRecoveryState s; raceWorldLoad_RaceCrashRecoveryState(&s, world);
        raceCrashRecovery(&s); raceWorldStore_RaceCrashRecoveryState(&s, world); return 0u; }
    case RACE_STAGE_CD1F: return stageCD1F(world);
    case RACE_STAGE_E96C: { BikeMovementState m; BikeStateMachineState s;
        raceWorldLoad_BikeMovementState(&m, world); raceWorldLoad_BikeStateMachineState(&s, world);
        { const uint8_t r = bikePhysicsUpdateFirstCourseVertical(&m, &s);
          raceWorldStore_BikeMovementState(&m, world); raceWorldStore_BikeStateMachineState(&s, world); return r; } }
    case RACE_STAGE_DE31: { RaceStartBannerState s; raceWorldLoad_RaceStartBannerState(&s, world);
        raceStartBanner(&s); raceWorldStore_RaceStartBannerState(&s, world); return 0u; }
    case RACE_STAGE_E836: { BikeMovementState m; raceWorldLoad_BikeMovementState(&m, world);
        bikePhysicsUpdateScreenY(&m); raceWorldStore_BikeMovementState(&m, world); return 0u; }
    case RACE_STAGE_EA44: { RaceEffectSpriteState s; raceWorldLoad_RaceEffectSpriteState(&s, world);
        raceEffectSprites(&s); raceWorldStore_RaceEffectSpriteState(&s, world); return 0u; }
    case RACE_STAGE_F4FF: { RaceCourseStreamState s; RaceSpawnState sp;
        raceWorldLoad_RaceCourseStreamState(&s, world); s.unsupported = 0u; raceWorldLoad_RaceSpawnState(&sp, world);
        raceCourseStreamUpdate(&s, &sp);
        raceWorldStore_RaceCourseStreamState(&s, world); raceWorldStore_RaceSpawnState(&sp, world); return s.unsupported; }
    case RACE_STAGE_E4C8: { RaceCollisionState s; raceWorldLoad_RaceCollisionState(&s, world);
        raceBikeCollision(&s); raceWorldStore_RaceCollisionState(&s, world); return 0u; }
    case RACE_STAGE_DEE2: { RaceEngineSoundState s; raceWorldLoad_RaceEngineSoundState(&s, world);
        raceEngineSound(&s); raceWorldStore_RaceEngineSoundState(&s, world); return 0u; }
    case RACE_STAGE_E359: {
        uint8_t oam[256];
        memcpy(oam, P(0x200), 256);
        nesSubE359UpdateTemperatureMeter(*P(0x52), *P(0x5C), *P(0x4F), P(0x3C), P(0x3B5), P(0x3B6), P(0x3E0), P(0xFC), oam);
        memcpy(P(0x200), oam, 256);
        return 0u; }
    case RACE_STAGE_E17F: { BikeMovementState m; BikeStateMachineState s; NativeOamState oam;
        raceWorldLoad_BikeMovementState(&m, world); raceWorldLoad_BikeStateMachineState(&s, world);
        memcpy(oam.bytes, P(0x200), 256);
        { const uint8_t r = nativeRaceUpdateBikeOam(&m, &s, &oam, P(0x88), P(0x39F));
          memcpy(P(0x200), oam.bytes, 256); return r; } }
    case RACE_STAGE_E456: { RaceOffscreenSpriteState s; RaceSpawnState sp;
        raceWorldLoad_RaceOffscreenSpriteState(&s, world); raceWorldLoad_RaceSpawnState(&sp, world);
        raceHideOffscreenOpponentSprites(&s, &sp);
        raceWorldStore_RaceOffscreenSpriteState(&s, world); raceWorldStore_RaceSpawnState(&sp, world); return 0u; }
    case RACE_STAGE_DA26: { BikeMovementState m; RaceTailState t; raceWorldLoad_BikeMovementState(&m, world);
        bikePhysicsUpdateRaceMovementPass(&m, *P(0x4F)); raceWorldStore_BikeMovementState(&m, world);
        if (*P(0x4F) == 0u) return 0u;
        t.timer_003A = *P(0x3A); t.track_finished_0052 = *P(0x52); t.timer_0024 = *P(0x24); t.flag_03A0 = *P(0x3A0);
        t.timer_002E = *P(0x2E); t.rng_0018 = *P(0x18); t.tile_03A1 = *P(0x3A1); t.x_03B9 = *P(0x3B9);
        t.scroll_0060 = *P(0x60); t.frame_003F = *P(0x3F); t.timer_003C = *P(0x3C); t.state_03BC = *P(0x3BC);
        t.scroll_03BD = *P(0x3BD); t.scroll_03BE = *P(0x3BE); t.blink_03BB = *P(0x3BB); t.timer_0020 = *P(0x20);
        memcpy(t.oam, P(0x200), 256);
        raceFrameTail(&t);
        *P(0x2E) = t.timer_002E; *P(0x3A0) = t.flag_03A0; *P(0x3A1) = t.tile_03A1; *P(0x3B9) = t.x_03B9;
        *P(0x3BD) = t.scroll_03BD; *P(0x3BE) = t.scroll_03BE; *P(0x3BB) = t.blink_03BB;
        memcpy(P(0x200), t.oam, 256);
        return t.unsupported; }
    case RACE_STAGE_E70B: { RacePositionSmoothingState s; raceWorldLoad_RacePositionSmoothingState(&s, world);
        raceSmoothBikePositions(&s); raceWorldStore_RacePositionSmoothingState(&s, world); return 0u; }
    case RACE_STAGE_CED0: { RaceAiState s; raceWorldLoad_RaceAiState(&s, world);
        raceAiUpdate(&s); raceWorldStore_RaceAiState(&s, world); return 0u; }
    case RACE_STAGE_E42B: { RaceHideSpritesState s; raceWorldLoad_RaceHideSpritesState(&s, world);
        raceHideCollidedSprites(&s); raceWorldStore_RaceHideSpritesState(&s, world); return 0u; }
    default: return 1u;
    }
}

uint32_t raceLoopStep(RaceWorld *world)
{
    uint32_t bad = 0u;
#define RUN(S) do { if (raceLoopRunStage(world, S) != 0u) bad |= 1u << (S); } while (0)
    RUN(RACE_STAGE_CA08);
    if ((*P(0x3B1) | *P(0x3B2)) != 0u) return bad; /* paused: only the D19B sync runs */
    RUN(RACE_STAGE_D918); RUN(RACE_STAGE_DD8D); RUN(RACE_STAGE_E733); RUN(RACE_STAGE_E927);
    if (*P(0x52) != 0u) {
        RUN(RACE_STAGE_CA9B);
    } else {
        if (*P(0x3F2) != 0u) { RUN(RACE_STAGE_DA9F); RUN(RACE_STAGE_DBC4); RUN(RACE_STAGE_DEBB); }
        RUN(RACE_STAGE_DF30); RUN(RACE_STAGE_EAC5); RUN(RACE_STAGE_DFD5); RUN(RACE_STAGE_E09F);
    }
    RUN(RACE_STAGE_DDD1); RUN(RACE_STAGE_CD1F); RUN(RACE_STAGE_E96C); RUN(RACE_STAGE_DE31);
    RUN(RACE_STAGE_E836); RUN(RACE_STAGE_EA44); RUN(RACE_STAGE_F4FF); RUN(RACE_STAGE_E4C8);
    RUN(RACE_STAGE_DEE2); RUN(RACE_STAGE_E359); RUN(RACE_STAGE_E17F); RUN(RACE_STAGE_E456);
    RUN(RACE_STAGE_DA26); RUN(RACE_STAGE_E70B); RUN(RACE_STAGE_CED0); RUN(RACE_STAGE_E42B);
#undef RUN
    return bad;
}
