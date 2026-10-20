#include "collision.h"

/* Placeholder collision boundary; track geometry is not implemented yet. */
void collisionResolve(BikeState *bike)
{
    if (bike == 0) {
        return;
    }

    if (bike->track_position < 0) {
        bike->track_position = 0;
    }
}
