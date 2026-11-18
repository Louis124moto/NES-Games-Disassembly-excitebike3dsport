#ifndef EXCITEBIKE_MENU_TAPE_FILE_H
#define EXCITEBIKE_MENU_TAPE_FILE_H

#include <stddef.h>
#include <stdint.h>
#include "menus.h"

/*
 * File-backed stand-in for the Famicom Data Recorder used by the design menu's
 * save ($0C) and load ($0D) states. Layout, in this order:
 *   block 0: 64 bytes (RAM $0060-$009F)  + 16-bit little-endian checksum
 *   block 1: 256 bytes (RAM $05E0-$06DF) + 16-bit little-endian checksum
 * The checksum is the total number of set bits, the same quantity the original
 * accumulates in ram_0008/ram_0009 (sub_C68A is called once per 1 bit).
 */
#define MENU_TAPE_FILE_DEFAULT_PATH "sdmc:/3ds/excitebike3ds_tracks.bin"

uint16_t menuTapeChecksum(const uint8_t *data, size_t size);
void menuTapeFileSetPath(const char *path);
const MenusTapeIo *menuTapeFileIo(void);

#endif
