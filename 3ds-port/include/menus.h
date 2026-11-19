#ifndef EXCITEBIKE_MENUS_H
#define EXCITEBIKE_MENUS_H

#include "game_state.h"

void menusUpdate(GameState *state, unsigned int heldKeys, unsigned int pressedKeys);
void menusRender(const GameState *state);
/*
 * Native model of vec_C23B_NMI's PPU work for one frame (call after menusUpdate,
 * i.e. at the end of the frame, like the NMI at vblank): enable rendering when
 * ram_0045 is zero, otherwise execute the static list it selects and clear it.
 * Dynamic buffer ($00) and list $1A are not modelled and leave the PPU untouched.
 */
void menusNmiUpload(GameState *state);
/*
 * One complete vec_C23B_NMI frame in ASM order: PPU part (menusNmiUpload) first,
 * then timers, controller and the sub_C2A9 menu dispatch (menusUpdate), then the
 * drawable-state refresh. All menu logic lives inside the NMI in the original.
 */
void menusNmiFrame(GameState *state, unsigned int heldKeys, unsigned int pressedKeys);

/*
 * Famicom Data Recorder replacement. The original menu states $0C (save) and $0D
 * (load) hand the track data to a main-thread routine that bit-bangs $4016
 * (sub_C6C8 write, sub_C739 read). A 3DS has no recorder, so the port plugs in a
 * storage back end with the same two logical blocks and the same completion
 * handshake (ram_0048 = $FF):
 *   block 0 = RAM $0060-$009F (64 bytes), block 1 = RAM $05E0-$06DF (256 bytes).
 * save    : store both blocks; returns nonzero on success. The 3DS port
 *           reports a storage failure through source PPU list $19 and returns
 *           to the design menu instead of falsely completing the save.
 * readBlock: 1 = block delivered, 0 = no data yet (the original waits forever
 *           for tape signal), -1 = framing/checksum error (the original shows
 *           con_0045_19 and restarts the read).
 * With no back end set, save is reported as a storage error and load waits
 * like a real console without a recorder.
 */
typedef struct MenusTapeIo {
    int (*save)(void *ctx, const uint8_t *block0, const uint8_t *block1);
    int (*readBlock)(void *ctx, unsigned int block, uint8_t *dst, size_t size);
    void *ctx;
} MenusTapeIo;
void menusSetTapeIo(const MenusTapeIo *io);
/* Nonzero while the main thread is blocked waiting for tape data (NMI disabled). */
int menusTapeWaiting(const GameState *state);

#endif
