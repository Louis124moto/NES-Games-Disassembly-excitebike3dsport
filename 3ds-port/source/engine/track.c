#include <string.h>
#include "track.h"

/* CPU-side course lookup pages established by C820. These are not PPU
 * nametables: they are the data consumed by D0C6/E7FC for track physics. */
static const uint8_t s_first_course_page_code[6] = {
    0x3Bu, 0x3Bu, 0x3Du, 0x3Du, 0x3Du, 0x3Du
};

void trackInitFirstCourse(NativeTrackState *track)
{
    if (track == 0) return;
    memset(track, 0, sizeof(*track));
    track->initialized = 1u;
}

/* sub_D0AB:
 * A = Y - $10; Y = 5; if negative return 5.
 * Otherwise decrement Y before each SBC #$08, stopping on underflow or Y=0.
 */
uint8_t trackGetFirstCourseBand(uint8_t object_y)
{
    uint8_t a = (uint8_t)(object_y - 0x10u);
    uint8_t band = 5u;

    if ((a & 0x80u) != 0u) return 5u;

    for (;;) {
        --band;
        if (band == 0u) return 0u;
        a = (uint8_t)(a - 0x08u);
        if ((a & 0x80u) != 0u) return band;
    }
}

void trackUpdateFirstCourse(NativeTrackState *track,
                            const BikeMovementState *movement)
{
    unsigned int bike;

    if (track == 0 || movement == 0 || track->initialized == 0u) return;

    /* D0C6 processes X=3..0. The final values are independent per bike. */
    for (bike = 0u; bike < 4u; ++bike) {
        uint8_t band = trackGetFirstCourseBand(movement->object_y[bike]);
        uint8_t code = s_first_course_page_code[band];
        unsigned int sample;

        track->vertical_band[bike] = band;
        track->terrain_code[bike] = code;
        for (sample = 0u; sample < 4u; ++sample) {
            /* D0D9 increments the object-class index and wraps it with $3F.
             * Every byte in the first-course CPU lookup page has the same
             * source fill value, so the resulting four samples are identical. */
            track->terrain_samples[bike][sample] = code;
        }
    }
}

uint8_t trackFirstCourseHasEvents(const NativeTrackState *track,
                                  uint8_t object_index)
{
    if (track == 0 || track->initialized == 0u || object_index >= 4u)
        return 0u;
    /* E74A subtracts $40 and returns immediately on a negative result. */
    return track->terrain_code[object_index] >= 0x40u ? 1u : 0u;
}
