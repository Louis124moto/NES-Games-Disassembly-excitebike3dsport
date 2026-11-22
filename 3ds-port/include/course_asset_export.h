#ifndef EXCITEBIKE_COURSE_ASSET_EXPORT_H
#define EXCITEBIKE_COURSE_ASSET_EXPORT_H

#include <stddef.h>

/* Write the source-derived first-course stream as a compact native EBCG pack. */
int courseAssetExportWrite(const char *directory, char *status, size_t statusCapacity);

#endif
