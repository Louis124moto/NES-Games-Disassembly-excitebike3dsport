#ifndef EXCITEBIKE_NES_PALETTE_H
#define EXCITEBIKE_NES_PALETTE_H

#include <stdint.h>

/* RGB for a source colour index 0..63. Returns 0 (rgb untouched) when out of range. */
int nesPaletteToRgb(uint8_t nesColor, uint8_t rgb[3]);

#endif
