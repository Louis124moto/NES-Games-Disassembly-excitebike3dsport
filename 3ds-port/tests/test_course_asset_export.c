#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "course_asset_export.h"

int main(void)
{
    const char *directory = "/tmp/excitebike-course-asset-export-test";
    char path[256], status[160];
    uint8_t header[16];
    FILE *file;
    long size;
    uint32_t columns;

    (void)snprintf(path, sizeof(path), "%s/course1_geometry.ebc", directory);
    (void)remove(path);
    (void)rmdir(directory);
    if (!courseAssetExportWrite(directory, status, sizeof(status))) {
        fprintf(stderr, "course export failed: %s\n", status);
        return 1;
    }
    file = fopen(path, "rb");
    if (file == NULL) return 2;
    if (fread(header, 1u, sizeof(header), file) != sizeof(header)) {
        fclose(file); return 3;
    }
    if (memcmp(header, "EBCG", 4u) != 0 ||
        header[4] != 1u || header[5] != 0u ||
        header[6] != 16u || header[7] != 0u ||
        header[12] != 6u || header[13] != 43u) {
        fclose(file); return 4;
    }
    columns = (uint32_t)header[8] | ((uint32_t)header[9] << 8u) |
              ((uint32_t)header[10] << 16u) | ((uint32_t)header[11] << 24u);
    if (columns != 1335u) { fclose(file); return 5; }
    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0L) {
        fclose(file); return 6;
    }
    fclose(file);
    if ((unsigned long)size != 16ul + (unsigned long)columns * 43ul) return 7;
    if (remove(path) != 0 || rmdir(directory) != 0) return 8;
    printf("PASS: integrated EBCG asset export wrote %lu columns (%ld bytes).\n",
           (unsigned long)columns, size);
    return 0;
}
