#include <stdio.h>
#include <string.h>
#include "race_session_world.h"
#include "race_results.h"
#include "race_loop.h"
#include "race_world_glue.h"
#include "race_spawn.h"
#include "race_frame.h"

static RaceWorld world;
static NativeRaceSession session;


/* End-to-end source terrain semantics:
 * F4FF writes the six CPU course pages; DB95/raceSpawnPlace uses page 5
 * (and page 2) to decide whether a slot-3 obstacle can exist; E09F then
 * consumes that spawned obstacle's coordinates for the actual hit test.
 */
static int testF4ffObstacleCollision(void)
{
    RaceSpawnState source, candidate;
    RaceObstacleHitState hit;
    unsigned base;
    int found = 0;

    raceWorldLoad_RaceSpawnState(&source, &world);
    for (base = 0u; base < 64u; ++base) {
        uint8_t cell;
        candidate = source;
        candidate.x_high[3] = 1u;
        candidate.object_class[0] = (uint8_t)base;
        if (!raceSpawnPlace(&candidate, 3u, 1u)) continue;
        cell = candidate.track_pages[5u][candidate.object_class[3] & 0x3Fu];
        if (cell < 0x3Bu || cell >= 0x93u) continue;

        memset(&hit, 0, sizeof(hit));
        hit.x_high_slot3 = candidate.x_high[3];
        hit.rider_present_0373 = 1u;
        hit.object_y_player = (uint8_t)(candidate.y_low[3] + 2u);
        hit.obstacle_y_006F = candidate.y_low[3];
        hit.x_low_player = (uint8_t)(candidate.x_low[3] - 5u);
        hit.obstacle_x_0393 = candidate.x_low[3];
        hit.global_vertical_reference_03F1 = 0xB6u;
        hit.vertical_reference_player = 0u;
        hit.y_low_player = 0xFFu;
        raceObstacleHit(&hit);
        if (hit.collision_player != 0xFFu ||
            hit.movement_state[3] != 1u ||
            hit.sound_request_00FD != 1u) {
            puts("FAIL F4FF road cell spawned an obstacle but E09F did not hit it");
            return 1;
        }
        found = 1;
        break;
    }
    if (!found) {
        puts("FAIL no source-road cell from F4FF pages could feed obstacle collision");
        return 1;
    }

    /* A non-road page byte outside the source's accepted $3B-$92 range
     * must block obstacle creation, so E09F has no phantom object to hit. */
    candidate = source;
    memset(candidate.track_pages[5], 0x20, sizeof(candidate.track_pages[5]));
    candidate.x_high[3] = 1u;
    candidate.object_class[0] = 0u;
    if (raceSpawnPlace(&candidate, 3u, 1u) != 0u) {
        puts("FAIL non-road F4FF terrain incorrectly allowed obstacle spawn");
        return 1;
    }
    puts("PASS: F4FF terrain pages gate source obstacle spawn and the spawned coordinates feed E09F collision.");
    return 0;
}

int main(void)
{
    NativeRaceSetupInput in;
    NativeRngState rng;
    unsigned f, started = 0u, moved = 0u, i;
    uint32_t mask = 0u;
    memset(&in, 0, sizeof(in));
    memset(&rng, 0, sizeof(rng));
    rng.bytes[0] = 0x5Au; rng.bytes[1] = 0x33u;
    rng.bytes[2] = 0xA7u; rng.bytes[3] = 0xC1u;
    rng.bytes[4] = 0x28u; rng.bytes[5] = 0x71u;
    rng.bytes[6] = 0x94u; rng.bytes[7] = 0xE2u;
    in.setup_index_0041 = 2u;
    in.timer_0034 = 0x2Du;
    in.transition_counter_03F8 = 4u;
    in.transition_flag_0046 = 1u;
    if (!nativeRaceSessionInit(&session, &in, &rng)) { puts("FAIL init"); return 1; }
    nativeRaceSessionBuildWorld(&session, 0u, &world);

    /* $4015: all channels on after setup; pause mutes (CA71), unpause re-enables (CA5E). */
    if (*raceWorldByte(&world, 0x4015u) != 0x0Fu) { puts("FAIL $4015 initial value"); return 1; }
    {
        RaceWorld pw = world;
        unsigned f;
        /* Start is ignored while the setup latch $03B0 is non-zero (countdown/setup). */
        for (f = 0u; f < 4000u && *raceWorldByte(&pw, 0x3B0u) != 0u; ++f)
            (void)nativeRaceSessionStepFrame(&session, &pw, 0u, 0u);
        if (*raceWorldByte(&pw, 0x3B0u) != 0u) { puts("FAIL setup latch never cleared"); return 1; }
        (void)nativeRaceSessionStepFrame(&session, &pw, 0u, 0x10u);       /* Start pressed */
        if (*raceWorldByte(&pw, 0x3B1u) != 1u || *raceWorldByte(&pw, 0xFBu) != 0x80u) {
            puts("FAIL pause request/jingle"); return 1;
        }
        /* CA63: the 0x28-frame pause timer runs first, then CA71 mutes every frame. */
        for (f = 0u; f < 0x30u; ++f) (void)nativeRaceSessionStepFrame(&session, &pw, 0u, 0u);
        if (*raceWorldByte(&pw, 0x4015u) != 0x00u) { puts("FAIL pause did not mute $4015"); return 1; }
        (void)nativeRaceSessionStepFrame(&session, &pw, 0u, 0x10u);       /* Start again */
        if (*raceWorldByte(&pw, 0x3B1u) != 0u || *raceWorldByte(&pw, 0x4015u) != 0x0Fu) {
            puts("FAIL unpause did not re-enable $4015"); return 1;
        }
    }

    if (*raceWorldByte(&world, 0x4Au) != 1u || session.setup.player_slot != 0u ||
        session.setup.first_opponent_slot != 1u ||
        session.setup.bike[0].role != NATIVE_RACE_BIKE_PLAYER ||
        session.setup.bike[1].role != NATIVE_RACE_BIKE_OPPONENT) {
        puts("FAIL player/opponent slot identity (expected player=0, AI=1..3)"); return 1;
    }
    if (*raceWorldByte(&world, 0x46u) != 1u) { puts("FAIL transition flag seed"); return 1; }
    for (i = 0u; i < 4u; ++i) {
        unsigned j;
        if (*raceWorldByte(&world, 0x360u + i) != session.track.vertical_band[i] ||
            *raceWorldByte(&world, 0xC0u + i) != session.track.terrain_code[i]) {
            printf("FAIL D0AB/D0C6 bike projection %u\\n", i); return 1;
        }
        for (j = 0u; j < 4u; ++j) {
            if (*raceWorldByte(&world, 0x3C0u + i * 4u + j) !=
                session.track.terrain_samples[i][j]) {
                printf("FAIL D0C6 sample %u/%u\\n", i, j); return 1;
            }
        }
    }
    for (i = 0u; i < 8u; ++i) {
        if (*raceWorldByte(&world, 0x18u + i) != rng.bytes[i]) {
            printf("FAIL RNG seed byte %u\n", i); return 1;
        }
    }
    for (i = 0u; i < 6u; ++i) {
        unsigned j;
        const uint8_t expected = i < 2u ? 0x3Bu : 0x3Du;
        for (j = 0u; j < 64u; ++j) {
            if (*raceWorldByte(&world, 0x400u + i * 64u + j) != expected) {
                printf("FAIL C820 course page %u byte %u\n", i, j); return 1;
            }
        }
    }

    /* The source pause handler ignores Start while the countdown state at
     * $03B0 is active; clear that gate to test the actual press-latch path. */
    *raceWorldByte(&world, 0x3B0u) = 0u;
    (void)nativeRaceSessionStepFrame(&session, &world, 0x10u, 0x10u);
    if (*raceWorldByte(&world, 0x15u) != 0x10u ||
        *raceWorldByte(&world, 0x3B1u) == 0u) {
        puts("FAIL pressed Start did not toggle pause"); return 1;
    }

    {
    unsigned course, variant;
    for (variant = 0u; variant < 10u; ++variant) {
    course = variant % 5u;
    in.multiplayer_flag = (uint8_t)(variant / 5u);
    started = moved = 0u; mask = 0u;
    /* Resume the test in a fresh session so pause timing cannot mask movement. */
    in.course_index_0043 = (uint8_t)course;
    if (!nativeRaceSessionInit(&session, &in, &rng)) { puts("FAIL reinit"); return 1; }
    nativeRaceSessionBuildWorld(&session, in.multiplayer_flag, &world);
    {
        int track_finished = 0;
        int finish_transition = 0;
        for (f = 0u; f < 100000u; ++f) {
            mask |= nativeRaceSessionStepFrame(&session, &world,
                                               f > 400u ? 0x80u : 0u, 0u);
            if (*raceWorldByte(&world, 0x4Fu) != 0u) ++started;
            if (*raceWorldByte(&world, 0x94u) != 0u) ++moved;
            if (*raceWorldByte(&world, 0x52u) != 0u) track_finished = 1;
            if (track_finished && *raceWorldByte(&world, 0x41u) != 2u) {
                finish_transition = 1;
                break;
            }
        }
        if (mask != 0u) {
            printf("(course %u) ", course); printf("FAIL unsupported mask %08X\n", (unsigned)mask); return 1; }
        if (started == 0u || moved == 0u) {
            printf("FAIL race never started/moved (%u/%u)\n", started, moved); return 1;
        }
        if (!track_finished || !finish_transition) {
            printf("FAIL course %u did not complete (finished=%d transition=%d frame=%u columns=%lu)\n",
                   course, track_finished, finish_transition, f,
                   (unsigned long)session.course_column_serial);
            return 1;
        }
        /* This serial counts live horizontal-scroll column updates, not
         * unique F4FF source records: the race can scroll through repeated
         * course data many times before the source finish gate is reached. */
        if (session.course_column_serial < (course == 0u ? 1335u : 200u)) {
            printf("FAIL first-course runtime column updates=%lu (below 1335-record source pass)\n",
                   (unsigned long)session.course_column_serial);
            return 1;
        }
    }
    /* Results flow (sub_CB2C): CB3B -> CB77 -> CBA4 -> CC6B must run to
     * completion on the world and leave state 3 (return to $41 == 0). */
    {
        unsigned r, phase_seen = 0u, podium = 0u;
        *raceWorldByte(&world, 0x53u) = 1u;
        *raceWorldByte(&world, 0x4Bu) = 0u;
        if (*raceWorldByte(&world, 0x43u) != course) { printf("FAIL course %u not in world\n", course); return 1; }
        if (*raceWorldByte(&world, 0x41u) != 3u) {
            printf("FAIL finish did not enter results state (41=%02X)\n", *raceWorldByte(&world, 0x41u));
            return 1;
        }
        for (r = 0u; r < 4000u && *raceWorldByte(&world, 0x41u) == 3u; ++r) {
            *raceWorldByte(&world, 0x14u) = (uint8_t)(r > 600u ? 0x10u : 0u);
            raceLoopNmiTick(&world);
            if (raceResultsPhaseStep(&world) != 0u) { puts("FAIL results step out of model"); return 1; }
            *raceWorldByte(&world, 0x300u) = 0u;
            phase_seen |= 1u << (*raceWorldByte(&world, 0x44u) & 7u);
            if (*raceWorldByte(&world, 0x204u) != 0u && *raceWorldByte(&world, 0x205u) != 0u) podium = 1u;
        }
        if (*raceWorldByte(&world, 0x41u) == 3u || (phase_seen & 7u) != 7u || podium == 0u) {
            printf("FAIL results flow state=%02X phases=%02X podium=%u frames=%u\n",
                   *raceWorldByte(&world, 0x41u), phase_seen, podium, r);
            return 1;
        }
    }
    printf("PASS: course %u mp=%u runs from start to source finish and results (columns=%lu frames=%u)\n", course, (unsigned)in.multiplayer_flag,
           (unsigned long)session.course_column_serial, f + 1u);
    }
    }
    if (testF4ffObstacleCollision() != 0) return 1;
    puts("PASS: all five courses complete natively.");
    return 0;
}
