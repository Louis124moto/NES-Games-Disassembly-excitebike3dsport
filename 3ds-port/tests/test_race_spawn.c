#include <stdio.h>
#include <string.h>
#include "race_spawn.h"
#include "cpu6502.h"

static uint32_t rng = 0x7A1D3C55u;
static uint8_t nb(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }

#define F4(X) \
    X(active, 0xA8) X(y_low, 0xB8) X(vertical_reference, 0xBC) X(slot_00A0, 0xA0) X(vertical_phase, 0xA4) \
    X(action_state, 0x58) X(lateral_state, 0xCC) X(dust_flags, 0x36C) X(speed_fraction, 0x90) X(speed_whole, 0x94) \
    X(collision_state, 0x98) X(movement_state, 0x9C) X(object_status, 0xB0) X(brake_state, 0xD4) \
    X(movement_mode, 0x388) X(script_x_high, 0xC0) X(block_039C, 0x39C) X(lane_delta, 0xDC) \
    X(position_0070, 0x70) X(rider_present, 0x370) X(x_high, 0x84) X(x_low, 0x80) X(screen_y, 0x8C) \
    X(approach, 0xAC) X(object_class, 0xE0) X(origin_x, 0x64) X(start_lock, 0x78) X(setup_parameter, 0x74)

#define F1(X) X(track_finished, 0x52) X(timer_0023, 0x23) X(timer_003B, 0x3B) X(frame_phase, 0x4C) X(spawn_request_03A8, 0x3A8)

static void randomize(RaceSpawnState *s)
{
    unsigned i, j;
    static const uint8_t codes[] = { 0x3Bu, 0x3Du, 0xE4u, 0xE6u, 0x3Au, 0x92u, 0x93u, 0x00u };
    memset(s, 0, sizeof(*s));
#define R4(F, B) for (i = 0u; i < 4u; ++i) s->F[i] = (nb() & 3u) ? 0u : nb();
    F4(R4)
#undef R4
    for (i = 0u; i < 4u; ++i) {
        s->x_high[i] = (nb() & 1u) ? (uint8_t)((nb() & 1u) * 2u) : (uint8_t)(nb() % 3u);
        s->x_low[i] = (nb() & 1u) ? (uint8_t)(0x3Cu + nb() % 8u) : nb();
        s->object_class[i] = (uint8_t)(nb() & 0x3Fu);
        s->position_0070[i] = (uint8_t)(nb() % 6u);
        s->speed_whole[i] = (nb() & 1u) ? 0u : (uint8_t)(nb() & 3u);
        s->active[i] = (nb() & 3u) ? 0u : 1u;
        s->rider_present[i] = (nb() & 3u) ? 0u : nb();
        s->y_low[i] = nb(); s->speed_fraction[i] = nb(); s->origin_x[i] = nb(); s->approach[i] = nb();
        s->start_lock[i] = (uint8_t)(nb() % 7u);
    }
    for (i = 0u; i < 8u; ++i) s->rng[i] = (nb() & 1u) ? (uint8_t)(0xA0u + (nb() & 0x5Fu)) : nb();
    for (i = 0u; i < 6u; ++i) for (j = 0u; j < 64u; ++j) s->track_pages[i][j] = (nb() & 7u) ? codes[nb() & 7u] : nb();
    s->track_finished = (nb() & 7u) == 0u;
    s->timer_0023 = (nb() & 3u) ? 0u : nb();
    s->timer_003B = (nb() & 3u) ? 0u : nb();
    s->frame_phase = (nb() & 1u) ? (uint8_t)(nb() & 3u) : nb();
    s->spawn_request_03A8 = (nb() & 3u) ? 0u : nb();
}

static void toRam(const RaceSpawnState *s, uint8_t *r)
{
    unsigned i, j;
    memset(r, 0, 0x800u);
#define W4(F, B) for (i = 0u; i < 4u; ++i) r[(B) + i] = s->F[i];
    F4(W4)
#undef W4
#define W1(F, B) r[B] = s->F;
    F1(W1)
#undef W1
    for (i = 0u; i < 8u; ++i) r[0x18 + i] = s->rng[i];
    for (i = 0u; i < 6u; ++i) for (j = 0u; j < 64u; ++j) r[0x400 + i * 64u + j] = s->track_pages[i][j];
}

static int compare(const char *w, unsigned v, const RaceSpawnState *s, const uint8_t *r)
{
    unsigned i;
#define C4(F, B) for (i = 0u; i < 4u; ++i) if (s->F[i] != r[(B) + i]) { \
    printf("FAIL %s v=%u %s[%u] native=%02X ASM=%02X\n", w, v, #F, i, s->F[i], r[(B) + i]); return 1; }
    F4(C4)
#undef C4
#define C1(F, B) if (s->F != r[B]) { printf("FAIL %s v=%u %s native=%02X ASM=%02X\n", w, v, #F, s->F, r[B]); return 1; }
    F1(C1)
#undef C1
    return 0;
}

static int run(Cpu6502 *cpu, int mode, unsigned count)
{
    static const char *names[] = { "sub_DA9F", "sub_DBC4", "sub_DAE3", "sub_DB50", "sub_DB95" };
    unsigned v, placed = 0u;
    for (v = 0u; v < count; ++v) {
        RaceSpawnState s;
        const unsigned x = 1u + nb() % 3u;
        const unsigned y = 1u + nb() % 4u;
        uint8_t zflag = 0u;
        randomize(&s);
        if (mode == 1 && (nb() & 1u)) { s.spawn_request_03A8 = 1u; s.active[3] = (nb() & 1u); }
        if (mode == 0 && (nb() & 1u)) { s.timer_003B = 0u; s.timer_0023 = 0u; s.spawn_request_03A8 = 0u; s.rider_present[3] = 0u; }
        if (mode == 0 && (nb() & 1u)) { unsigned i; for (i = 1u; i < 4u; ++i) { s.active[i] = 0u; s.frame_phase = (uint8_t)x; } }
        toRam(&s, cpu->ram);
        cpu->a = 0u; cpu->x = (uint8_t)x; cpu->y = (uint8_t)(mode == 3 ? s.x_high[x] : y); cpu->sp = 0xFDu; cpu->p = 0x24u; cpu->cycles = 0u; cpu->bad_opcode = -1;
        {
            static const uint16_t entry[] = { 0xDA9Fu, 0xDBC4u, 0xDAE3u, 0xDB50u, 0xDB95u };
            if (cpu6502_call(cpu, entry[mode], 50000u) != 0) { printf("FAIL %s v=%u oracle error\n", names[mode], v); return 1; }
            zflag = (uint8_t)((cpu->p & 2u) != 0u);
        }
        switch (mode) {
        case 0: raceSpawnUpdate(&s); break;
        case 1: raceSpawnHandleSlot3(&s); break;
        case 2: {
            const uint8_t placed_now = raceSpawnPlace(&s, x, y);
            if (placed_now == zflag) { printf("FAIL %s v=%u return flag native=%u Z=%u\n", names[mode], v, placed_now, zflag); return 1; }
            placed += placed_now;
            break; }
        case 3: raceSpawnSetupSpeed(&s, x, s.x_high[x]); break;
        default: raceSpawnClearSlot(&s, x); break;
        }
        if (compare(names[mode], v, &s, cpu->ram) != 0) return 1;
    }
    printf("PASS: %u %s vectors match original ASM%s\n", count, names[mode], mode == 2 ? " (placed counted)" : "");
    (void)placed;
    return 0;
}

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *fp;
    int m;
    if (argc != 2) { puts("usage: test_race_spawn <prg>"); return 2; }
    fp = fopen(argv[1], "rb");
    if (fp == NULL || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) return 2;
    fclose(fp);
    cpu6502_reset_state(&cpu);
    for (m = 0; m < 5; ++m) if (run(&cpu, m, m == 4 ? 20000u : 200000u) != 0) return 1;
    return 0;
}
