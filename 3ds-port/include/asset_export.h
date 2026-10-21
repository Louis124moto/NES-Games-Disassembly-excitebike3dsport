#ifndef EXCITEBIKE_ASSET_EXPORT_H
#define EXCITEBIKE_ASSET_EXPORT_H

#include <stddef.h>
#include <stdint.h>

/* Export verified source CHR bytes, decoded 2bpp indices, a grayscale atlas and a manifest. */
int assetExportWrite(const char *directory, const uint8_t *rawChr, size_t rawChrBytes,
                     const uint8_t *pixels, size_t tileCount,
                     char *status, size_t statusCapacity);

#endif
