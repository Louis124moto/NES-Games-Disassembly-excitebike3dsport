#include <string.h>
#include "nes_rom_identify.h"

int main(void)
{
    uint8_t rom[64];
    NesRomIdentity identity;
    char summary[160];

    /* Canonical Excitebike iNES header: NROM-128, vertical mirroring. */
    memset(rom, 0, sizeof(rom));
    rom[0] = 'N'; rom[1] = 'E'; rom[2] = 'S'; rom[3] = 0x1A;
    rom[4] = 1u;  /* 16 KiB PRG-ROM */
    rom[5] = 1u;  /* 8 KiB CHR-ROM */
    rom[6] = 1u;  /* vertical mirroring, no trainer, mapper low nibble 0 */
    rom[7] = 0u;
    if (nesRomIdentifyHeader(rom, sizeof(rom), &identity) != NES_ROM_IDENTIFY_OK) return 1;
    if (identity.headerValid == 0 || identity.isNes2 != 0 || identity.mapper != 0u ||
        identity.prgRomKib != 16u || identity.chrRomKib != 8u ||
        identity.usesChrRam != 0 || identity.verticalMirroring == 0 ||
        identity.trainer != 0) return 2;
    if (nesRomIdentityMatchesExcitebike(&identity) != 1) return 3;
    if (nesRomIdentitySummary(&identity, summary, sizeof(summary)) == NULL) return 4;
    if (strstr(summary, "mapper 0") == NULL || strstr(summary, "PRG 16") == NULL ||
        strstr(summary, "CHR-ROM 8") == NULL) return 5;

    /* Wrong mapper: iNES mapper 32 needs flags 7 low nibble = 2. */
    rom[7] = 0x02u;
    if (nesRomIdentifyHeader(rom, sizeof(rom), &identity) != NES_ROM_IDENTIFY_OK) return 6;
    if (identity.mapper != 32u || identity.isNes2 != 0 ||
        nesRomIdentityMatchesExcitebike(&identity) != 0) return 7;

    /* CHR-RAM layout: no match. */
    memset(rom, 0, sizeof(rom));
    rom[0] = 'N'; rom[1] = 'E'; rom[2] = 'S'; rom[3] = 0x1A;
    rom[4] = 1u; rom[5] = 0u; rom[6] = 1u; rom[7] = 0u;
    if (nesRomIdentifyHeader(rom, sizeof(rom), &identity) != NES_ROM_IDENTIFY_OK) return 8;
    if (identity.usesChrRam == 0 || nesRomIdentityMatchesExcitebike(&identity) != 0) return 9;

    /*
     * NES 2.0 header with the same layout: identifies and matches.
     * flags 7 = $08 sets the NES 2.0 format bits; its LOW nibble must NOT
     * contribute mapper bits (regression: it previously reported mapper 128,
     * which made a valid NES 2.0 Excitebike dump fail the hardware check).
     */
    memset(rom, 0, sizeof(rom));
    rom[0] = 'N'; rom[1] = 'E'; rom[2] = 'S'; rom[3] = 0x1A;
    rom[4] = 1u; rom[5] = 1u; rom[6] = 1u; rom[7] = 0x08u;
    if (nesRomIdentifyHeader(rom, sizeof(rom), &identity) != NES_ROM_IDENTIFY_OK) return 10;
    if (identity.isNes2 == 0 || identity.mapper != 0u ||
        nesRomIdentityMatchesExcitebike(&identity) != 1) return 11;

    /* NES 2.0 extended mapper bits (flags 8 low nibble): no match. */
    rom[8] = 0x01u;
    if (nesRomIdentifyHeader(rom, sizeof(rom), &identity) != NES_ROM_IDENTIFY_OK) return 18;
    if (identity.mapper != 0x100u ||
        nesRomIdentityMatchesExcitebike(&identity) != 0) return 19;

    /* Bad magic. */
    rom[3] = 0x00u;
    if (nesRomIdentifyHeader(rom, sizeof(rom), &identity) != NES_ROM_IDENTIFY_BAD_MAGIC) return 12;

    /* Header shorter than 16 bytes. */
    if (nesRomIdentifyHeader(rom, 8u, &identity) != NES_ROM_IDENTIFY_SHORT_HEADER) return 13;

    /* NULL arguments. */
    if (nesRomIdentifyHeader(NULL, sizeof(rom), &identity) != NES_ROM_IDENTIFY_INVALID_ARGUMENT) return 14;
    if (nesRomIdentifyHeader(rom, sizeof(rom), NULL) != NES_ROM_IDENTIFY_INVALID_ARGUMENT) return 15;
    if (nesRomIdentityMatchesExcitebike(NULL) != 0) return 16;

    /* Unidentified identity reports honestly. */
    memset(&identity, 0, sizeof(identity));
    if (nesRomIdentitySummary(&identity, summary, sizeof(summary)) == NULL ||
        strstr(summary, "not identified") == NULL) return 17;

    return 0;
}
