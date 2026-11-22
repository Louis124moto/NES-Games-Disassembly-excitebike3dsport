#include <stdio.h>
#include <string.h>
#include "race_session_world.h"

static RaceWorld world;
static NativeRaceSession session;

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
    if (!nativeRaceSessionInitFirstCourse(&session, &in, &rng)) { puts("FAIL init"); return 1; }
    nativeRaceSessionBuildWorld(&session, 0u, &world);

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

    /* Resume the test in a fresh session so pause timing cannot mask movement. */
    if (!nativeRaceSessionInitFirstCourse(&session, &in, &rng)) { puts("FAIL reinit"); return 1; }
    nativeRaceSessionBuildWorld(&session, 0u, &world);
    for (f = 0u; f < 3000u; ++f) {
        mask |= nativeRaceSessionStepFrame(&session, &world, f > 400u ? 0x80u : 0u, 0u);
        if (*raceWorldByte(&world, 0x4Fu) != 0u) ++started;
        if (*raceWorldByte(&world, 0x94u) != 0u) ++moved;
    }
    if (mask != 0u) { printf("FAIL unsupported mask %08X\n", (unsigned)mask); return 1; }
    if (started == 0u || moved == 0u) { printf("FAIL race never started/moved (%u/%u)\n", started, moved); return 1; }
    printf("PASS: native race world seeded; pressed input works; 3000 frames (started=%u moved=%u)\n", started, moved);
    return 0;
}
