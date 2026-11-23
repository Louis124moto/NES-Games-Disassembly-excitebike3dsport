#ifndef HOST_STUB_3DS_H
#define HOST_STUB_3DS_H
#include <stdint.h>
typedef uint32_t u32;
/*
 * Host-only stand-in for <3ds.h>: just the libctru key bits that menus.c uses, so
 * the menu state machine can be compiled and tested with plain gcc. Values are
 * libctru's KEY_* bit positions. Never used by the devkitARM build (-Itests is not
 * on its include path).
 */
#define KEY_A (1u << 0)
#define KEY_B (1u << 1)
#define KEY_SELECT (1u << 2)
#define KEY_START (1u << 3)
#define KEY_RIGHT (1u << 4)
#define KEY_LEFT (1u << 5)
#define KEY_UP (1u << 6)
#define KEY_DOWN (1u << 7)
void hidScanInput(void);
u32 hidKeysHeld(void);
u32 hidKeysDown(void);

typedef uint16_t u16;
#define GFX_TOP 0
#define GFX_LEFT 0
uint8_t *gfxGetFramebuffer(int screen, int side, u16 *width, u16 *height);

#endif
