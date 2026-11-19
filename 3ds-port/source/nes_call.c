#include "nes_gen_code.h"

int nesCall(NesMachine *m, uint16_t addr, long budget)
{
    /* RTS pops the address and adds 1: push $FFFE so it returns to $FFFF (the sentinel case in nesRunCode). */
    nesPush(m, 0xFFu);
    nesPush(m, 0xFEu);
    return nesRunCode(m, addr, &budget);
}
