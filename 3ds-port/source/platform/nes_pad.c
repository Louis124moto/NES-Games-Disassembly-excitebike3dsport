#include "nes_pad.h"

void nesPadReset(NesPad *pad)
{
    pad->hold = 0u;
    pad->press = 0u;
}

void nesPadScan(NesPad *pad, uint8_t raw)
{
    const uint8_t previous = pad->hold;
    pad->hold = raw;
    pad->press = raw;
    if ((raw & 0x80u) != 0u && (previous & 0x80u) != 0u)
        pad->press = (uint8_t)(raw & 0x7Fu);    /* D37A-D384 */
}
