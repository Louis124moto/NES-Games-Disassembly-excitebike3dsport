#include <stdio.h>
#include <string.h>
#include "native_rng.h"

#define CHECK(c, n) do { if (!(c)) { printf("FAIL %d\n", n); return n; } } while (0)

static void reference_sub_D326(uint8_t bytes[8])
{
    uint8_t carry = (uint8_t)(((bytes[0] & 2u) ^ (bytes[1] & 2u)) != 0u);
    unsigned int i;
    for (i = 0u; i < 8u; ++i) {
        uint8_t old = bytes[i];
        bytes[i] = (uint8_t)((old >> 1) | (carry << 7));
        carry = (uint8_t)(old & 1u);
    }
}

int main(void)
{
    NativeRngState rng;
    uint8_t expected[8];
    unsigned int n, i;

    nativeRngReset(&rng);
    CHECK(rng.bytes[0] == 0x06u, 1);
    for (i = 1u; i < 8u; ++i) CHECK(rng.bytes[i] == 0u, 2);

    nativeRngAdvance(&rng);
    CHECK(rng.bytes[0] == 0x83u, 3);
    for (i = 1u; i < 8u; ++i) CHECK(rng.bytes[i] == 0u, 4);

    nativeRngAdvance(&rng);
    CHECK(rng.bytes[0] == 0xC1u && rng.bytes[1] == 0x80u, 5);

    /* Deterministic randomized state vectors against a separately written
     * byte/carry reference for the exact source ROR chain. */
    for (n = 0u; n < 100000u; ++n) {
        uint32_t x = 0x9E3779B9u ^ (n * 0x45D9F3Bu);
        for (i = 0u; i < 8u; ++i) {
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            rng.bytes[i] = (uint8_t)x;
        }
        memcpy(expected, rng.bytes, sizeof(expected));
        reference_sub_D326(expected);
        nativeRngAdvance(&rng);
        CHECK(memcmp(expected, rng.bytes, sizeof(expected)) == 0, 10);
    }

    nativeRngAdvance(0);
    nativeRngReset(0);
    puts("PASS: native sub_D326 eight-byte RNG shift register.");
    return 0;
}
