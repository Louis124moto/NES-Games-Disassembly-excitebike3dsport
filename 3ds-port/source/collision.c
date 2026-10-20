#include "collision.h"

/*
 * Deliberately disabled until the relevant NES object/track collision path is
 * traced and translated. Clamping track_position at zero was an invented
 * placeholder rule, not a behavior established by the original disassembly.
 */
void collisionResolve(BikeState *bike)
{
    (void)bike;
}
