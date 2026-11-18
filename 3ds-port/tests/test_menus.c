/*
 * Host test of the menu frame model (menus.c): vec_C23B_NMI order, setup
 * sequencer, cursor OAM writes, title-menu navigation and the dynamic PPU buffer.
 * Every expected value below was derived by hand from Excitebike/bank_FF.asm
 * (addresses in the comments), not by running the code under test.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "game_state.h"
#include "menus.h"
#include "nes_active_stream.h"
#include "nes_ppu_render.h"
#include "menu_tape_file.h"

/* libctru key bits (see tests/host_stub/3ds.h) */
#define K_START (1u << 3)
#define K_RIGHT (1u << 4)
#define K_UP (1u << 6)
#define K_DOWN (1u << 7)

static GameState *newState(void)
{
    GameState *s = (GameState *)calloc(1u, sizeof(GameState));
    if (s != NULL) gameStateReset(s);
    return s;
}

static void frames(GameState *s, unsigned int n, unsigned int pressed)
{
    unsigned int i;
    for (i = 0u; i < n; ++i) menusNmiFrame(s, pressed, i == 0u ? pressed : 0u);
}

/* One input edge frame followed by two idle frames (clears the sub_C7DE repeat guard). */
static void tap(GameState *s, unsigned int key)
{
    menusNmiFrame(s, key, key);
    menusNmiFrame(s, 0u, 0u);
    menusNmiFrame(s, 0u, 0u);
}

#define CHECK(cond, code) do { if (!(cond)) { fprintf(stderr, "test_menus: check %d failed (line %d)\n", (code), __LINE__); free(s); return (code); } } while (0)

static int testBootSequence(void)
{
    GameState *s = newState();
    if (s == NULL) return 1;

    /* NMI 1: ram_0045 == 0 -> $2001 = $1A; then bra_C2ED: sub_D24F ($2001 = 0) and sub_C318 prelude. */
    menusNmiFrame(s, 0u, 0u);
    CHECK(s->menu_ram_for_2001 == 0x1Au, 2);
    CHECK(s->menu_ppu_mask == 0u, 3);
    CHECK(s->menu_ram0044 == 1u, 4);
    CHECK(s->menu_ppu_ctrl == 0x10u, 5);
    CHECK(s->menu_oam[0] == 0xF8u && s->menu_oam[3] == 0xF8u, 6);
    CHECK(s->menu_ppu_nametables[0] == 0xFCu && s->menu_ppu_nametables[959] == 0xFCu, 7);

    /* NMI 2: command 0 = ofs_001_C2BD_00_01: ram_0045=2, cursor sprite $FA at X $50, Y $80, timer $31 = $38. */
    menusNmiFrame(s, 0u, 0u);
    CHECK(s->menu_ram0045 == 2u, 8);
    CHECK(s->menu_oam[0] == 0x80u && s->menu_oam[1] == 0xFAu && s->menu_oam[2] == 0u && s->menu_oam[3] == 0x50u, 9);
    CHECK(s->menu_ram03F8 == 4u && s->menu_ram03F0 == 1u && s->menu_ram00FB == 1u, 10);
    CHECK(s->menu_ppu_background_ready == 0u, 11);

    /* NMI 3: list $02 is uploaded and acknowledged; command 1 = ofs_001_C514_00_02 sets $18 and ram_0040. */
    menusNmiFrame(s, 0u, 0u);
    CHECK(s->menu_ppu_revision == 1u, 12);
    CHECK(s->menu_ppu_nametables[0x3E3] == 0xAAu && s->menu_ppu_nametables[0x083] == 0x27u, 13);
    CHECK(s->menu_ram0045 == 0x18u && s->menu_ram0040 == 1u && s->menu_ram0044 == 0u, 14);
    CHECK(s->menu_ppu_background_ready == 0u, 15);

    /* NMI 4: palette list $18 uploaded; rendering still off ($2001 was zeroed in NMI 3, ram_0045 != 0 at NMI start). */
    menusNmiFrame(s, 0u, 0u);
    CHECK(s->menu_ppu_palette_loaded == 1u && s->menu_ram0045 == 0u, 16);
    CHECK(s->menu_ppu_palette[0] == 0x02u && s->menu_ppu_palette[1] == 0x0Fu && s->menu_ppu_palette[3] == 0x21u, 17);
    CHECK(s->menu_ppu_palette[16] == s->menu_ppu_palette[0], 18);
    CHECK(s->menu_ppu_background_ready == 0u, 19);

    /* NMI 5: ram_0045 == 0 again -> $2001 = $1A and the setup path is finished (ram_0040 = 1): picture on. */
    menusNmiFrame(s, 0u, 0u);
    CHECK(s->menu_ppu_mask == 0x1Au && s->menu_ppu_background_ready == 1u, 20);
    CHECK(s->menu_oam[0] == 0x80u && s->menu_oam[3] == 0x50u, 21);
    free(s);
    return 0;
}

static int testTitleNavigation(void)
{
    GameState *s = newState();
    if (s == NULL) return 1;
    frames(s, 5u, 0u);
    CHECK(s->menu_ppu_background_ready == 1u && s->menu_asm_state == 0u, 30);

    /* tbl_C080_spr_Y = $80,$90,$A0; Down moves 0 -> 1 -> 2 and wraps to 0 (LDY #1 falls into DEY). */
    tap(s, K_DOWN);  CHECK(s->menu_selection == 1u && s->menu_oam[0] == 0x90u, 31);
    tap(s, K_DOWN);  CHECK(s->menu_selection == 2u && s->menu_oam[0] == 0xA0u, 32);
    tap(s, K_DOWN);  CHECK(s->menu_selection == 0u && s->menu_oam[0] == 0x80u, 33);
    tap(s, K_UP);    CHECK(s->menu_selection == 2u && s->menu_oam[0] == 0xA0u, 34);
    /* a held direction is one move only (ram_0049 guard) */
    menusNmiFrame(s, K_UP, K_UP);
    menusNmiFrame(s, K_UP, 0u);
    menusNmiFrame(s, K_UP, 0u);
    CHECK(s->menu_selection == 1u, 35);
    /* cursor X/tile/attribute stay as written by sub_C409 */
    CHECK(s->menu_oam[1] == 0xFAu && s->menu_oam[2] == 0u && s->menu_oam[3] == 0x50u, 36);
    free(s);
    return 0;
}

static int testSelectionToTrackScreen(void)
{
    GameState *s = newState();
    if (s == NULL) return 1;
    frames(s, 5u, 0u);
    /* Start on entry 0: sub_C7DE returns X=4 -> ofs_000_C346_00 bra_C36A: state 1, ram_03F2 = 0, ram_03F0/03F3/004A = 1. */
    menusNmiFrame(s, K_START, K_START);
    CHECK(s->menu_asm_state == 1u && s->menu_ram03F2 == 0u, 40);
    CHECK(s->menu_ram03F0 == 1u && s->menu_ram03F3 == 1u && s->menu_ram004A == 1u && s->menu_ram03F7 == 0u, 41);
    CHECK(s->menu_ram0040 == 0u && s->menu_ram0044 == 0u, 42);
    /*
     * The setup path blanks the picture again. sub_C7DE armed ram_0030_timer = 4 (X=4 path); timers
     * $30-$3D only count down once per 11 frames (sub_D310: ram_0020 reloads $0A), so the prelude
     * waits about 44 frames before the three setup commands run.
     */
    menusNmiFrame(s, 0u, 0u);
    CHECK(s->menu_ppu_mask == 0u && s->menu_timer30 == 4u, 43);
    frames(s, 200u, 0u);
    /* After sub_C318 + ofs_001_C55E_01_01 ($06) + ofs_001_C3F3_01_02 + ofs_001_C514_01_03 ($18): picture on, cursor from C3F3. */
    CHECK(s->menu_ram0040 == 1u && s->menu_ppu_mask == 0x1Au, 44);
    CHECK(s->menu_oam[1] == 0xFAu && s->menu_oam[3] == 0x48u && s->menu_oam[0] == 0xB0u && s->menu_oam[2] == 0u, 45);
    CHECK(s->menu_ram00FB == 4u, 46);

    /* State 1 navigation: Right is remapped to Down (ofs_000_C395_01); X from tbl_C083 = $48,$60,$78,$90,$A8. */
    tap(s, K_RIGHT); CHECK(s->menu_selection == 1u && s->menu_oam[3] == 0x60u, 47);
    tap(s, K_RIGHT); CHECK(s->menu_selection == 2u && s->menu_oam[3] == 0x78u, 48);
    tap(s, K_RIGHT); tap(s, K_RIGHT);
    CHECK(s->menu_selection == 4u && s->menu_oam[3] == 0xA8u, 49);
    tap(s, K_RIGHT); CHECK(s->menu_selection == 0u && s->menu_oam[3] == 0x48u, 50); /* wraps to 0 */
    free(s);
    return 0;
}

static int testDynamicBuffer(void)
{
    GameState *s = newState();
    uint8_t nt[NES_MENU_NAMETABLE_BYTES], pal[NES_MENU_PALETTE_BYTES], buf[NES_MENU_DYNAMIC_BUFFER_BYTES];
    uint8_t nt0[NES_MENU_NAMETABLE_BYTES], buf0[NES_MENU_DYNAMIC_BUFFER_BYTES];
    if (s == NULL) return 1;
    memset(nt, 0, sizeof(nt)); memset(pal, 0, sizeof(pal)); memset(buf, 0, sizeof(buf));

    /* The record tbl_C13C builds for state 4: $0300=4, PPU $216F, 1 byte, data, end token. */
    buf[0] = 4u; buf[1] = 0x21u; buf[2] = 0x6Fu; buf[3] = 0x01u; buf[4] = 0x55u; buf[5] = 0u; buf[6] = 0xEEu;
    CHECK(nesExcitebikeNmiUploadDynamic(buf, nt, pal) == NES_NMI_UPLOAD_APPLIED, 60);
    CHECK(nt[0x16F] == 0x55u, 61);                 /* $216F - $2000 */
    CHECK(buf[0] == 0u && buf[1] == 0u && buf[6] == 0xEEu, 62); /* only $0300/$0301 cleared */

    /* An empty buffer (first addr-hi = 0 = end token) changes nothing. */
    memset(buf, 0, sizeof(buf));
    CHECK(nesExcitebikeNmiUploadDynamic(buf, nt, pal) == NES_NMI_UPLOAD_NONE, 63);

    /* A buffer without a terminator inside $0301-$031F is refused and leaves the PPU model untouched. */
    memset(buf, 0xFF, sizeof(buf));
    memcpy(nt0, nt, sizeof(nt)); memcpy(buf0, buf, sizeof(buf));
    CHECK(nesExcitebikeNmiUploadDynamic(buf, nt, pal) == NES_NMI_UPLOAD_UNSUPPORTED, 64);
    CHECK(memcmp(nt, nt0, sizeof(nt)) == 0 && memcmp(buf, buf0, sizeof(buf)) == 0, 65);

    /* Through the frame model: ram_0045 == 0 runs the buffer; ram_0045 != 0 leaves it for later. */
    memcpy(s->menu_ram0300, (const uint8_t[]){4u, 0x21u, 0x6Fu, 0x01u, 0x66u, 0u}, 6u);
    s->menu_ram0045 = 0u;
    menusNmiUpload(s);
    CHECK(s->menu_ppu_nametables[0x16F] == 0x66u && s->menu_ram0300[0] == 0u, 66);
    free(s);
    return 0;
}

static int testState4DynamicWrite(void)
{
    GameState *s = newState();
    if (s == NULL) return 1;
    s->menu_asm_state = 4u;
    s->menu_ram0043 = 2u;
    s->menu_ram0040 = 0u;
    s->menu_ram0044 = 0u;
    /* prelude, command 0 ($06), command 1 ($09), command 2 (buffer) */
    menusNmiFrame(s, 0u, 0u);
    menusNmiFrame(s, 0u, 0u); CHECK(s->menu_ram0045 == 6u, 70);
    menusNmiFrame(s, 0u, 0u); CHECK(s->menu_ram0045 == 9u, 71);
    menusNmiFrame(s, 0u, 0u);
    CHECK(s->menu_ram0300[0] == 4u && s->menu_ram0300[1] == 0x21u && s->menu_ram0300[2] == 0x6Fu, 72);
    CHECK(s->menu_ram0300[3] == 1u && s->menu_ram0300[4] == 3u && s->menu_ram0300[5] == 0u, 73); /* ram_0043 + 1 */
    CHECK(s->menu_timer34 == 0x12u, 74);
    /* the next NMI finds ram_0045 == 0 and executes the buffer */
    menusNmiFrame(s, 0u, 0u);
    CHECK(s->menu_ppu_nametables[0x16F] == 3u, 75);
    free(s);
    return 0;
}

static int testState7Setup(void)
{
    GameState *s = newState();
    unsigned int i;
    if (s == NULL) return 1;
    s->menu_asm_state = 12u; /* tbl_C01C_index[$0C] = 7 -> off_C07A_07 */
    s->menu_ram05A0[0] = 1u; s->menu_ram05A0[1] = 2u; s->menu_ram05A0[2] = 3u;
    s->menu_ram05C3[0] = 4u; s->menu_ram05C3[1] = 5u; s->menu_ram05C3[2] = 6u;
    for (i = 0u; i < 256u; ++i) s->menu_ram06E0[i] = (uint8_t)(i * 3u + 1u);
    s->menu_ram004D_EF[5] = 0x99u;
    menusUpdate(s, 0u, 0u);                 /* prelude */
    menusUpdate(s, 0u, 0u);                 /* ofs_001_C869_07_01: clears $004D-$00EF */
    CHECK(s->menu_ram004D_EF[5] == 0u, 80);
    menusUpdate(s, 0u, 0u);                 /* ofs_001_C586_07_02 + sub_C621 */
    CHECK(s->menu_ram0060[1] == 0x17u && s->menu_ram0060[2] == 0x01u && s->menu_ram0060[3] == 0x00u && s->menu_ram0060[4] == 0x01u, 81);
    CHECK(s->menu_ram0060[5] == 1u && s->menu_ram0060[6] == 2u && s->menu_ram0060[7] == 3u, 82);
    CHECK(s->menu_ram0060[8] == 4u && s->menu_ram0060[9] == 5u && s->menu_ram0060[10] == 6u, 83);
    CHECK(memcmp(s->menu_ram05E0, s->menu_ram06E0, 256u) == 0, 84);
    CHECK(s->menu_ram0045 == 0x10u, 85);
    free(s);
    return 0;
}


/* Drive a fresh GameState to the design menu through the real title -> state 8 path. */
static GameState *designMenu(void)
{
    GameState *s = newState();
    if (s == NULL) return NULL;
    frames(s, 5u, 0u);
    tap(s, K_DOWN); tap(s, K_DOWN);          /* title entry 2 */
    menusNmiFrame(s, K_START, K_START);      /* -> state 8, ram_03F2 = 2 */
    frames(s, 200u, 0u);                     /* timer $30 (4 x 11 frames) + prelude + list 6 */
    return s;
}

static int testDesignMenu(void)
{
    static const uint8_t cursorY[6] = {0x30u, 0x48u, 0x78u, 0x90u, 0xA8u, 0xC0u};
    GameState *s = designMenu();
    unsigned int i;
    if (s == NULL) return 1;
    CHECK(s->menu_asm_state == 8u && s->menu_ram03F2 == 2u && s->menu_ram0040 == 1u, 90);
    /* tbl_C01C_index[8] = 6: off_C070_06 = C55E_06_01 ($06), C575_06_02 (cursor X $4C, ram_0043 = 5, $0F), C514 ($18). */
    CHECK(s->menu_ppu_mask == 0x1Au && s->menu_ppu_background_ready == 1u, 91);
    CHECK(s->menu_ram0043 == 5u, 92);
    CHECK(s->menu_oam[1] == 0xFAu && s->menu_oam[3] == 0x4Cu && s->menu_oam[2] == 0u, 93);
    CHECK(s->menu_oam[0] == cursorY[0], 94);
    /* Down walks the six entries (tbl_C08B_spr_Y) and wraps to 0 */
    for (i = 1u; i < 6u; ++i) {
        tap(s, K_DOWN);
        CHECK(s->menu_selection == i && s->menu_oam[0] == cursorY[i], 95);
    }
    tap(s, K_DOWN);
    CHECK(s->menu_selection == 0u && s->menu_oam[0] == cursorY[0], 96);
    free(s);
    return 0;
}

/* sub_C3BD confirm branches (hand-derived from $C3C8-$C3E4). */
static int confirmFrom(unsigned int selection, GameState **out)
{
    GameState *s = designMenu();
    if (s == NULL) return 1;
    s->menu_selection = (uint8_t)selection;
    menusNmiFrame(s, K_START, K_START);
    *out = s;
    return 0;
}

static int testDesignConfirm(void)
{
    GameState *s = NULL;
    if (confirmFrom(0u, &s)) return 1;
    CHECK(s->menu_asm_state == 10u && s->menu_ram03F2 == 0u && s->menu_ram0040 == 0u && s->menu_ram0044 == 0u, 100);
    free(s);
    if (confirmFrom(1u, &s)) return 1;
    CHECK(s->menu_asm_state == 10u && s->menu_ram03F2 == 1u, 101);
    free(s);
    if (confirmFrom(2u, &s)) return 1;
    CHECK(s->menu_asm_state == 9u && s->menu_ram03F2 == 2u, 102);   /* 03F2 untouched (still 2 from the title) */
    free(s);
    if (confirmFrom(3u, &s)) return 1;
    CHECK(s->menu_asm_state == 12u && s->menu_selection == 0u && s->menu_ram0040 == 0u, 103);
    free(s);
    if (confirmFrom(4u, &s)) return 1;
    CHECK(s->menu_asm_state == 13u && s->menu_selection == 0u, 104);
    free(s);
    /* entry 5 = loc_CA85: clear $0021-$00FF, state back to 0, ram_0042 restored to 2 */
    if (confirmFrom(5u, &s)) return 1;
    CHECK(s->menu_asm_state == 0u && s->menu_selection == 2u && s->menu_ram0045 == 0u, 105);
    CHECK(s->menu_ram0040 == 0u && s->menu_ram0044 == 0u && s->menu_repeat_guard == 1u, 106);
    CHECK(s->menu_timer31 == 0u && s->menu_ram00FB == 0u, 107);
    free(s);
    return 0;
}

typedef struct { uint8_t b0[64], b1[256]; int haveData; unsigned reads; int failNext; int saveCalls; } FakeTape;

static int fakeSave(void *ctx, const uint8_t *b0, const uint8_t *b1)
{
    FakeTape *t = (FakeTape *)ctx;
    memcpy(t->b0, b0, 64u); memcpy(t->b1, b1, 256u); t->haveData = 1; ++t->saveCalls;
    return 1;
}

static int fakeRead(void *ctx, unsigned int block, uint8_t *dst, size_t size)
{
    FakeTape *t = (FakeTape *)ctx;
    ++t->reads;
    if (!t->haveData) return 0;
    if (t->failNext) { t->failNext = 0; return -1; }
    memcpy(dst, block == 0u ? t->b0 : t->b1, size);
    return 1;
}

/* Save (state 12): handler posts ram_0048 = 1, the main thread writes both blocks, handler leaves via loc_CC6B. */
static int testTapeSave(void)
{
    FakeTape tape;
    MenusTapeIo io;
    GameState *s = NULL;
    unsigned int i;
    memset(&tape, 0, sizeof(tape));
    io.save = fakeSave; io.readBlock = fakeRead; io.ctx = &tape;
    menusSetTapeIo(&io);
    if (confirmFrom(3u, &s)) return 1;
    for (i = 0u; i < 256u; ++i) s->menu_ram06E0[i] = (uint8_t)(i * 7u + 3u);
    frames(s, 200u, 0u);   /* timer $30, prelude, list 7: clear, sub_C586 + sub_C621 (06E0 -> 05E0), list $18 */
    CHECK(tape.saveCalls == 1, 110);
    CHECK(tape.b0[1] == 0x17u && tape.b0[2] == 0x01u && tape.b0[3] == 0x00u && tape.b0[4] == 0x01u, 111); /* tbl_C180 header */
    CHECK(memcmp(tape.b1, s->menu_ram06E0, 256u) == 0, 112);
    /* completion: ram_00FB = $40 then loc_CC6B (INC -> 13, ram_004B == 0 -> clamp to 8) */
    CHECK(s->menu_asm_state == 8u && s->menu_ram0048 == 0xFFu, 113);
    CHECK(tape.reads == 0u, 114);              /* a save must never go through the read path */
    CHECK(s->menu_ram00FB == 0x40u, 115);      /* loc_C447 */
    menusSetTapeIo(NULL);
    free(s);
    return 0;
}

/* Load (state 13): header check, error lists $1A/$19, retry, then the $0065/$0068 copies and 05E0 -> 06E0. */
static int testTapeLoad(void)
{
    FakeTape tape;
    MenusTapeIo io;
    GameState *s = NULL;
    unsigned int i;
    uint8_t ntBefore;
    memset(&tape, 0, sizeof(tape));
    io.save = fakeSave; io.readBlock = fakeRead; io.ctx = &tape;
    menusSetTapeIo(&io);
    if (confirmFrom(4u, &s)) return 1;
    frames(s, 200u, 0u);
    /* No data: the main thread waits forever with NMI off (frozen picture). */
    CHECK(menusTapeWaiting(s) == 1 && s->menu_nmi_enabled == 0u && s->menu_ram0048 == 2u, 120);
    {
        const uint8_t rev = s->menu_ppu_revision;
        const uint8_t t20 = s->menu_timer20;
        frames(s, 30u, 0u);
        /* NMI is off, so not even the sub_D310 timer divider advances */
        CHECK(s->menu_ppu_revision == rev && menusTapeWaiting(s) == 1 && s->menu_timer20 == t20, 121);
    }

    /* Wrong header byte: 1A list shows the four bytes at $224F, then the read restarts. */
    memset(tape.b0, 0, sizeof(tape.b0));
    tape.b0[1] = 0x17u; tape.b0[2] = 0x01u; tape.b0[3] = 0x00u; tape.b0[4] = 0x02u; /* != tbl_C180[3] = 1 */
    for (i = 0u; i < 256u; ++i) tape.b1[i] = (uint8_t)(255u - i);
    tape.b0[5] = 9u; tape.b0[6] = 8u; tape.b0[7] = 7u; tape.b0[8] = 6u; tape.b0[9] = 5u; tape.b0[10] = 4u;
    tape.haveData = 1;
    ntBefore = s->menu_ppu_nametables[0x24Fu];
    frames(s, 3u, 0u);
    CHECK(s->menu_ppu_nametables[0x24Fu] == 0x17u && s->menu_ppu_nametables[0x252u] == 0x02u && ntBefore != 0x17u, 122);
    CHECK(s->menu_ram0048 == 2u, 123);               /* still waiting: restarted, not finished */

    /* Checksum failure on the retry: list $19 is posted, then another restart. */
    tape.b0[4] = 0x01u;                              /* header fixed */
    tape.failNext = 1;
    {
        const uint8_t rev = s->menu_ppu_revision;
        frames(s, 6u, 0u);
        CHECK(s->menu_ppu_revision != rev, 124);     /* $19 and/or $1A lists reached the PPU */
    }
    frames(s, 10u, 0u);
    /* Success: block 0 -> $0060.., block 1 -> $05E0..; handler copies and leaves for state 8. */
    CHECK(s->menu_ram0048 == 0xFFu || s->menu_asm_state == 8u || s->menu_asm_state == 13u, 125);
    frames(s, 200u, 0u);
    CHECK(s->menu_asm_state == 8u, 126);
    CHECK(s->menu_ram0060[5] == 9u && s->menu_ram0060[8] == 6u, 127);
    CHECK(s->menu_ram05A0[0] == 9u && s->menu_ram05A0[1] == 8u && s->menu_ram05A0[2] == 7u, 128);   /* $0065+X */
    CHECK(s->menu_ram05C3[0] == 6u && s->menu_ram05C3[1] == 5u && s->menu_ram05C3[2] == 4u, 129);   /* $0068+X */
    CHECK(memcmp(s->menu_ram06E0, tape.b1, 256u) == 0, 130);                                         /* sub_EC3B */
    CHECK(s->menu_nmi_enabled == 1u, 131);
    CHECK(s->menu_ram00FB == 0x40u, 132);
    menusSetTapeIo(NULL);
    free(s);
    return 0;
}

static int testTapeFileBackend(void)
{
    uint8_t b0[64], b1[256], r0[64], r1[256];
    const MenusTapeIo *io = menuTapeFileIo();
    unsigned int i;
    FILE *f;
    const char *path = "test_menus_tape.bin";
    GameState *s = NULL; /* CHECK frees it */
    for (i = 0u; i < 64u; ++i) b0[i] = (uint8_t)(i * 5u);
    for (i = 0u; i < 256u; ++i) b1[i] = (uint8_t)(i ^ 0x5Au);
    menuTapeFileSetPath(path);
    remove(path);
    CHECK(io->readBlock(NULL, 0u, r0, 64u) == 0, 140);               /* no file: no signal */
    CHECK(io->save(NULL, b0, b1) == 1, 141);
    CHECK(io->readBlock(NULL, 0u, r0, 64u) == 1 && memcmp(r0, b0, 64u) == 0, 142);
    CHECK(io->readBlock(NULL, 1u, r1, 256u) == 1 && memcmp(r1, b1, 256u) == 0, 143);
    /* checksum = number of set bits: 0x00 -> 0, 0xFF -> 8, 0xA5 -> 4 */
    { const uint8_t v[3] = {0x00u, 0xFFu, 0xA5u}; CHECK(menuTapeChecksum(v, 3u) == 12u, 144); }
    f = fopen(path, "r+b");
    CHECK(f != NULL, 145);
    fseek(f, 3L, SEEK_SET); fputc(b0[3] ^ 0x01, f); fclose(f);        /* corrupt one bit in block 0 */
    CHECK(io->readBlock(NULL, 0u, r0, 64u) == -1, 146);
    CHECK(io->readBlock(NULL, 1u, r1, 256u) == 1, 147);              /* block 1 is still intact */
    remove(path);
    menuTapeFileSetPath(NULL);
    return 0;
}

int main(void)
{
    int rc;
    if ((rc = testBootSequence()) != 0) return rc;
    if ((rc = testTitleNavigation()) != 0) return rc;
    if ((rc = testSelectionToTrackScreen()) != 0) return rc;
    if ((rc = testDynamicBuffer()) != 0) return rc;
    if ((rc = testState4DynamicWrite()) != 0) return rc;
    if ((rc = testState7Setup()) != 0) return rc;
    if ((rc = testDesignMenu()) != 0) return rc;
    if ((rc = testDesignConfirm()) != 0) return rc;
    if ((rc = testTapeSave()) != 0) return rc;
    if ((rc = testTapeLoad()) != 0) return rc;
    if ((rc = testTapeFileBackend()) != 0) return rc;
    puts("PASS: menu frame model (NMI order, setup sequencer, cursor OAM, navigation, dynamic buffer).");
    return 0;
}
