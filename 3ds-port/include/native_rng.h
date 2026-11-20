#ifndef EXCITEBIKE_NATIVE_RNG_H
#define EXCITEBIKE_NATIVE_RNG_H

#include <stdint.h>

/* Eight-byte shift register at NES zero-page $0018-$001F (sub_D326). */
typedef struct NativeRngState {
    uint8_t bytes[8];
} NativeRngState;

/* C1CE seeds $0018=$06 after the source RAM clear; $0019-$001F remain zero. */
void nativeRngReset(NativeRngState *rng);

/* Exact carry chain and ROR order from sub_D326. */
void nativeRngAdvance(NativeRngState *rng);

#endif
