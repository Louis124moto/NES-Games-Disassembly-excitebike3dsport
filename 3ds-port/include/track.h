#ifndef EXCITEBIKE_TRACK_H
#define EXCITEBIKE_TRACK_H

#include <stdint.h>
#include "bike_physics.h"

typedef struct NativeTrackState {
    uint8_t vertical_band[4];       /* ram_0360_obj */
    uint8_t terrain_code[4];        /* ram_00C0_obj */
    uint8_t terrain_samples[4][4];  /* ram_03C0, four consecutive bytes per bike */
    uint8_t initialized;
} NativeTrackState;

/* C820 native course profile: six $40-byte pages are filled with
 * $3B,$3B,$3D,$3D,$3D,$3D after the original art upload. */
void trackInitFirstCourse(NativeTrackState *track);

/* Exact D0AB vertical-band calculation, including 8-bit underflow. */
uint8_t trackGetFirstCourseBand(uint8_t object_y);

/* D0C6/E7FC semantic lookup from the first-course profile. */
void trackUpdateFirstCourse(NativeTrackState *track,
                            const BikeMovementState *movement);

/* First-course values below $40 do not enter the E733 event-stream path. */
uint8_t trackFirstCourseHasEvents(const NativeTrackState *track,
                                  uint8_t object_index);

#endif
