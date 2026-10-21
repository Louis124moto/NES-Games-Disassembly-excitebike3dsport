#ifndef EXCITEBIKE_ASSET_EXTRACTOR_H
#define EXCITEBIKE_ASSET_EXTRACTOR_H

#include <stddef.h>
#include <stdint.h>

void assetExtractorInit(const char *executablePath);
void assetExtractorUpdate(unsigned int pressedKeys);
int assetExtractorIsActive(void);
const uint8_t *assetExtractorGetPixels(size_t *tileCount);
const char *assetExtractorGetStatus(void);
size_t assetExtractorGetPage(void);
void assetExtractorSetPage(size_t page);
unsigned int assetExtractorGetViewMode(void); /* 0: CHR atlas, 1: source PPU palette diagnostics */
size_t assetExtractorGetPaletteIndex(void); /* 0..6 maps to source tables 12..18 */
size_t assetExtractorGetPpuStreamIndex(void); /* 0..16 maps to static PPU lists 1..17 */

#endif
