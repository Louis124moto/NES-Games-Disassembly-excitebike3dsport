#ifndef EXCITEBIKE_ASSET_EXTRACTOR_H
#define EXCITEBIKE_ASSET_EXTRACTOR_H

#include <stddef.h>
#include <stdint.h>

void assetExtractorInit(const char *executablePath);
/* Semantic controls (the platform layer maps physical keys): confirm opens the
 * extractor/re-exports, cancel closes it or retries extraction. */
void assetExtractorUpdate(int confirm, int cancel);
int assetExtractorIsActive(void);
const char *assetExtractorGetStatus(void);
/* Complete source tile count (rawChrBytes/16). */
size_t assetExtractorGetTotalTiles(void);
const char *assetExtractorGetRomIdentitySummary(void);
/* 1: header matches Excitebike; 0: valid header, no match; -1: unknown/raw source. */
int assetExtractorRomMatchesExcitebikeHeader(void);

#endif
