#ifndef EXCITEBIKE_ASSET_EXTRACTOR_H
#define EXCITEBIKE_ASSET_EXTRACTOR_H

#include <stddef.h>
#include <stdint.h>

void assetExtractorInit(const char *executablePath);
void assetExtractorUpdate(unsigned int pressedKeys);
int assetExtractorIsActive(void);
/* Eager decoded buffer; *tileCount = min(total source tiles, 960). */
const uint8_t *assetExtractorGetPixels(size_t *tileCount);
/*
 * On-demand decoded page for the atlas view (up to 375 tiles starting at the
 * current page). Decodes from the retained raw CHR when the source exceeds the
 * eager buffer; returns NULL when no source is loaded. *pageFirstTile reports
 * the absolute index of the first tile in the returned buffer.
 */
const uint8_t *assetExtractorGetPagePixels(size_t *pageTileCount, size_t *pageFirstTile);
const char *assetExtractorGetStatus(void);
size_t assetExtractorGetPage(void);
void assetExtractorSetPage(size_t page);
unsigned int assetExtractorGetViewMode(void); /* 0: CHR atlas, 1: source PPU palette diagnostics, 2: PPU snapshots, 3: framebuffer pattern, 4: active stream/OAM trace */
size_t assetExtractorGetPaletteIndex(void); /* 0..6 maps to source tables 12..18 */
size_t assetExtractorGetPpuStreamIndex(void); /* 0..16 maps to static PPU lists 1..17 */
/* Complete source tile count (rawChrBytes/16); may exceed the eager 960 cap. */
size_t assetExtractorGetTotalTiles(void);
const char *assetExtractorGetRomIdentitySummary(void);
/* 1: header matches Excitebike; 0: valid header, no match; -1: unknown/raw source. */
int assetExtractorRomMatchesExcitebikeHeader(void);

#endif
