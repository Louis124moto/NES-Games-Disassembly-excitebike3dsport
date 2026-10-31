#ifndef EXCITEBIKE_NES_ROM_IDENTIFY_H
#define EXCITEBIKE_NES_ROM_IDENTIFY_H

#include <stddef.h>
#include <stdint.h>

/*
 * Header-level ROM identification for the runtime extractor.
 *
 * The checked-in source header (Excitebike/header.bin) confirms canonical
 * Excitebike as mapper 0 (NROM-128), 16 KiB PRG-ROM, and 8 KiB CHR-ROM with
 * vertical mirroring. This module reports whether a user-supplied ROM header
 * matches that layout. It is a header signature check only: PRG content
 * identity (checksums) is intentionally NOT claimed.
 */
typedef struct {
    int headerValid;          /* 1 when a valid iNES/NES 2.0 header was parsed */
    int isNes2;
    unsigned int mapper;
    unsigned int prgRomKib;
    unsigned int chrRomKib;
    int usesChrRam;
    int verticalMirroring;
    int trainer;
} NesRomIdentity;

typedef enum {
    NES_ROM_IDENTIFY_OK = 0,
    NES_ROM_IDENTIFY_INVALID_ARGUMENT,
    NES_ROM_IDENTIFY_SHORT_HEADER,
    NES_ROM_IDENTIFY_BAD_MAGIC,
    NES_ROM_IDENTIFY_UNSUPPORTED_NES2
} NesRomIdentifyResult;

NesRomIdentifyResult nesRomIdentifyHeader(const uint8_t *rom, size_t romSize,
                                          NesRomIdentity *identity);

/* 1 only for a header-valid mapper-0 / 16 KiB PRG / 8 KiB CHR-ROM layout. */
int nesRomIdentityMatchesExcitebike(const NesRomIdentity *identity);

const char *nesRomIdentitySummary(const NesRomIdentity *identity,
                                  char *buffer, size_t capacity);
const char *nesRomIdentifyError(NesRomIdentifyResult result);

#endif
