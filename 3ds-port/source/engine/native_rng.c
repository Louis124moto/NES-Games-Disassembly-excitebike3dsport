#include <string.h>
#include "native_rng.h"

void nativeRngReset(NativeRngState *rng)
{
    if (rng == 0) return;
    memset(rng, 0, sizeof(*rng));
    rng->bytes[0] = 0x06u;
}

void nativeRngAdvance(NativeRngState *rng)
{
    uint8_t carry;
    unsigned int i;

    if (rng == 0) return;

    /* D326-D335: XOR bit 1 of $0018 and $0019 becomes the initial carry. */
    carry = (uint8_t)(((rng->bytes[0] & 0x02u) ^
                       (rng->bytes[1] & 0x02u)) != 0u);

    /* D336-D344: ROR $18 through $1F, propagating each outgoing bit 0. */
    for (i = 0u; i < 8u; ++i) {
        uint8_t next_carry = (uint8_t)(rng->bytes[i] & 0x01u);
        rng->bytes[i] = (uint8_t)((rng->bytes[i] >> 1) | (carry << 7));
        carry = next_carry;
    }
}
