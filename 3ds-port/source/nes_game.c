#include "nes_game.h"
#include <stdio.h>
#include <string.h>
#include "nes_system.h"
#include <3ds.h>

#define ROM_MAX (16u + 512u + 0x4000u + 0x2000u)
static NesSystem s_sys;
static uint8_t s_rom[ROM_MAX];
static size_t s_romSize;
static int s_ready;
static NesGameStatus s_status = NES_GAME_NO_ROM;
static char s_text[96] = "rom.nes not loaded";

NesGameStatus nesGameInit(const char *executablePath)
{
    char path[512];
    const char *slash = NULL;
    size_t i, dir = 0u;
    FILE *f;
    s_ready = 0;
    s_status = NES_GAME_NO_ROM;
    if (executablePath != NULL)
        for (i = 0u; executablePath[i] != '\0'; ++i)
            if (executablePath[i] == '/' || executablePath[i] == '\\') slash = &executablePath[i];
    if (slash != NULL) dir = (size_t)(slash - executablePath) + 1u;
    if (dir + sizeof("rom.nes") > sizeof(path)) return s_status;
    if (dir != 0u) memcpy(path, executablePath, dir);
    memcpy(path + dir, "rom.nes", sizeof("rom.nes"));
    f = fopen(path, "rb");
    if (f == NULL) { snprintf(s_text, sizeof(s_text), "rom.nes not found beside the 3DSX"); return s_status; }
    s_romSize = fread(s_rom, 1u, sizeof(s_rom), f);
    fclose(f);
    switch (nesSystemInit(&s_sys, s_rom, s_romSize)) {
    case NES_SYS_OK: s_status = NES_GAME_OK; s_ready = 1; snprintf(s_text, sizeof(s_text), "rom.nes OK (PRG matches the disassembly)"); break;
    case NES_SYS_PRG_MISMATCH: s_status = NES_GAME_PRG_MISMATCH; snprintf(s_text, sizeof(s_text), "rom.nes PRG differs from the disassembled program"); break;
    default: s_status = NES_GAME_BAD_ROM; snprintf(s_text, sizeof(s_text), "rom.nes must be mapper 0, 16 KiB PRG + 8 KiB CHR"); break;
    }
    return s_status;
}

int nesGameAvailable(void) { return s_ready; }
const char *nesGameStatusText(void) { return s_text; }
const uint8_t *nesGameFramebuffer(void) { return nesSystemFramebuffer(&s_sys); }
const int16_t *nesGameAudio(unsigned *count) { if (count) *count = s_sys.audioCount; return s_sys.audio; }
uint32_t nesGameFrames(void) { return (uint32_t)s_sys.frames; }

void nesGameBoot(void)
{
    if (s_ready) (void)nesSystemInit(&s_sys, s_rom, s_romSize);
}

int nesGameFrame(uint32_t held)
{
    uint8_t pad = 0u;
    if (!s_ready) return 0;
    if (held & KEY_A) pad |= 0x01u;
    if (held & KEY_B) pad |= 0x02u;
    if (held & KEY_SELECT) pad |= 0x04u;
    if (held & KEY_START) pad |= 0x08u;
    if (held & KEY_UP) pad |= 0x10u;
    if (held & KEY_DOWN) pad |= 0x20u;
    if (held & KEY_LEFT) pad |= 0x40u;
    if (held & KEY_RIGHT) pad |= 0x80u;
    nesSystemSetPad(&s_sys, pad);
    return nesSystemRunFrame(&s_sys) == NES_SYS_OK;
}
