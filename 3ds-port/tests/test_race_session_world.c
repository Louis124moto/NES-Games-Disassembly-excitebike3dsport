#include <stdio.h>
#include <string.h>
#include "race_session_world.h"

static RaceWorld world;
static NativeRaceSession session;

int main(void)
{
    NativeRaceSetupInput in;
    NativeRngState rng;
    unsigned f, started = 0u, moved = 0u;
    uint32_t mask = 0u;
    memset(&in, 0, sizeof(in));
    memset(&rng, 0, sizeof(rng));
    rng.bytes[0] = 0x5Au; rng.bytes[1] = 0x33u;
    in.setup_index_0041 = 2u; in.timer_0034 = 0x2Du; in.transition_counter_03F8 = 4u;
    if (!nativeRaceSessionInitFirstCourse(&session, &in, &rng)) { puts("FAIL init"); return 1; }
    nativeRaceSessionBuildWorld(&session, 0u, &world);
    for (f = 0u; f < 3000u; ++f) {
        mask |= nativeRaceSessionStepFrame(&session, &world, f > 400u ? 0x80u : 0u);
        if (*raceWorldByte(&world, 0x4Fu) != 0u) ++started;
        if (*raceWorldByte(&world, 0x94u) != 0u) ++moved;
    }
    if (mask != 0u) { printf("FAIL unsupported mask %08X\n", (unsigned)mask); return 1; }
    if (started == 0u || moved == 0u) { printf("FAIL race never started/moved (%u/%u)\n", started, moved); return 1; }
    printf("PASS: native race session ran 3000 frames (started=%u moved=%u)\n", started, moved);
    return 0;
}
