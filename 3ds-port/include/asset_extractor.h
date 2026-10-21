#ifndef EXCITEBIKE_ASSET_EXTRACTOR_H
#define EXCITEBIKE_ASSET_EXTRACTOR_H

#include <stddef.h>
#include <stdint.h>

void assetExtractorInit(const char *executablePath);
void assetExtractorUpdate(unsigned int pressedKeys);
int assetExtractorIsActive(void);
const uint8_t *assetExtractorGetPixels(size_t *tileCount);
const char *assetExtractorGetStatus(void);
unsigned int assetExtractorGetPalette(void);
size_t assetExtractorGetPage(void);
void assetExtractorSetPage(size_t page);

#endif
