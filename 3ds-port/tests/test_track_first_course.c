#include <stdio.h>
#include <string.h>
#include "track.h"

#define CHECK(c, n) do { if (!(c)) { printf("FAIL %d\n", n); return n; } } while (0)

int main(void)
{
    static const uint8_t ys[] = {
        0x00u, 0x0Fu, 0x10u, 0x17u, 0x18u, 0x1Fu,
        0x20u, 0x27u, 0x28u, 0x2Fu, 0x30u, 0x37u,
        0x38u, 0xFFu
    };
    static const uint8_t bands[] = {
        5u, 5u, 4u, 4u, 3u, 3u, 2u, 2u, 1u, 1u, 0u, 0u, 0u, 5u
    };
    static const uint8_t start_y[4] = {0x0Eu, 0x1Au, 0x26u, 0x32u};
    static const uint8_t start_class[4] = {0x09u, 0x0Bu, 0x0Du, 0x0Fu};
    static const uint8_t expected_band[4] = {5u, 3u, 1u, 0u};
    static const uint8_t expected_code[4] = {0x3Du, 0x3Du, 0x3Bu, 0x3Bu};
    NativeTrackState track;
    BikeMovementState movement;
    unsigned int i, j;

    for (i = 0u; i < sizeof(ys); ++i)
        CHECK(trackGetFirstCourseBand(ys[i]) == bands[i], 1);

    memset(&movement, 0, sizeof(movement));
    for (i = 0u; i < 4u; ++i) {
        movement.object_y[i] = start_y[i];
        movement.object_class[i] = start_class[i];
    }

    trackInitFirstCourse(&track);
    CHECK(track.initialized == 1u, 2);
    trackUpdateFirstCourse(&track, &movement);

    printf("bands: %u %u %u %u\\n", track.vertical_band[0], track.vertical_band[1], track.vertical_band[2], track.vertical_band[3]);
    for (i = 0u; i < 4u; ++i) {
        CHECK(track.vertical_band[i] == expected_band[i], 10);
        CHECK(track.terrain_code[i] == expected_code[i], 11);
        CHECK(trackFirstCourseHasEvents(&track, (uint8_t)i) == 0u, 12);
        for (j = 0u; j < 4u; ++j)
            CHECK(track.terrain_samples[i][j] == expected_code[i], 13);
    }

    /* Runtime Y changes update only the semantic course lookup, no PPU state. */
    movement.object_y[0] = 0x18u;
    trackUpdateFirstCourse(&track, &movement);
    CHECK(track.vertical_band[0] == 3u && track.terrain_code[0] == 0x3Du, 20);

    CHECK(trackFirstCourseHasEvents(0, 0u) == 0u, 21);
    CHECK(trackFirstCourseHasEvents(&track, 4u) == 0u, 22);
    trackInitFirstCourse(0);
    trackUpdateFirstCourse(0, &movement);

    puts("PASS: native first-course C820/D0AB/D0C6 track profile.");
    return 0;
}
