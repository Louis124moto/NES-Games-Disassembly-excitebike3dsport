#include <stdio.h>
#include <string.h>
#include "race_ai.h"
#include "cpu6502.h"

static uint32_t rng = 0x2F6B91D3u;
static uint8_t nb(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }

#define F4(X) \
    X(spawn.active, 0xA8) X(spawn.collision_state, 0x98) X(spawn.movement_state, 0x9C) X(spawn.object_status, 0xB0) \
    X(movement_flags, 0x5C) X(desired_007C, 0x7C) X(spawn.approach, 0xAC) X(desired_0368, 0x368) \
    X(spawn.start_lock, 0x78) X(spawn.setup_parameter, 0x74) X(spawn.speed_whole, 0x94) X(spawn.speed_fraction, 0x90) \
    X(spawn.action_state, 0x58) X(spawn.x_high, 0x84) X(spawn.x_low, 0x80) X(spawn.position_0070, 0x70) \
    X(spawn.lane_delta, 0xDC) X(spawn.object_class, 0xE0) X(vertical_band, 0x360) X(spawn.script_x_high, 0xC0)
#define F1(X) X(spawn.track_finished, 0x52) X(flag_03A6, 0x3A6) X(first_slot_004A, 0x4A)

static uint8_t pickv(const uint8_t *v, unsigned n) { return v[nb() % n]; }

static void randomize(RaceAiState *s)
{
    static const uint8_t samples[] = { 0xC0u, 0xC1u, 0x70u, 0x73u, 0x74u, 0x48u, 0x4Au, 0x4Bu, 0x3Bu, 0x20u, 0x00u, 0xFFu };
    static const uint8_t pages[] = { 0x3Bu, 0x3Cu, 0x3Du, 0x3Eu, 0x3Au, 0xFAu, 0xE4u, 0xE6u, 0x00u };
    static const uint8_t c0[] = { 0xE6u, 0x00u, 0x3Du, 0xE4u };
    unsigned i, j;
    memset(s, 0, sizeof(*s));
    for (i = 0u; i < 4u; ++i) {
        s->spawn.active[i] = (nb() & 3u) ? 1u : 0u;
        s->spawn.collision_state[i] = (nb() & 7u) ? 0u : nb();
        s->spawn.movement_state[i] = (nb() & 7u) ? 0u : nb();
        s->spawn.object_status[i] = (nb() & 3u) ? 0u : (uint8_t)(nb() & 3u);
        s->movement_flags[i] = nb(); s->desired_007C[i] = (nb() & 1u) ? (uint8_t)(nb() & 0x0Fu) : 0u;
        s->spawn.approach[i] = (uint8_t)(nb() & 0x0Fu); s->desired_0368[i] = nb();
        s->spawn.start_lock[i] = (uint8_t)(nb() % 6u); s->spawn.setup_parameter[i] = nb();
        s->spawn.speed_whole[i] = (uint8_t)(nb() % 6u); s->spawn.speed_fraction[i] = nb();
        s->spawn.action_state[i] = (nb() & 1u) ? 0u : (uint8_t)(nb() % 22u);
        s->spawn.x_high[i] = (nb() & 3u) ? 1u : (uint8_t)(nb() & 3u);
        s->spawn.x_low[i] = nb(); s->spawn.position_0070[i] = (uint8_t)(nb() % 5u);
        s->spawn.lane_delta[i] = (nb() & 3u) ? 0u : nb();
        s->spawn.object_class[i] = (uint8_t)(nb() & 0x3Fu); s->vertical_band[i] = (uint8_t)(nb() % 6u);
        s->spawn.script_x_high[i] = pickv(c0, sizeof(c0));
        for (j = 0u; j < 4u; ++j) s->terrain_samples[i][j] = (nb() & 1u) ? pickv(samples, sizeof(samples)) : nb();
        s->spawn.origin_x[i] = nb();
    }
    for (i = 0u; i < 8u; ++i) s->spawn.rng[i] = (nb() & 1u) ? (uint8_t)(0xC0u + (nb() & 0x3Fu)) : nb();
    for (i = 0u; i < 6u; ++i) for (j = 0u; j < 64u; ++j) s->spawn.track_pages[i][j] = (nb() & 7u) ? pickv(pages, sizeof(pages)) : nb();
    s->spawn.track_finished = (nb() & 7u) == 0u; s->flag_03A6 = (nb() & 3u) ? 1u : 0u;
    s->first_slot_004A = (uint8_t)(nb() & 3u);
}

static void toRam(const RaceAiState *s, uint8_t *r)
{
    unsigned i, j;
    memset(r, 0, 0x800u);
#define W4(F, B) for (i = 0u; i < 4u; ++i) r[(B) + i] = s->F[i];
    F4(W4)
#undef W4
#define W1(F, B) r[B] = s->F;
    F1(W1)
#undef W1
    for (i = 0u; i < 8u; ++i) r[0x18 + i] = s->spawn.rng[i];
    for (i = 0u; i < 6u; ++i) for (j = 0u; j < 64u; ++j) r[0x400 + i * 64u + j] = s->spawn.track_pages[i][j];
    for (i = 0u; i < 4u; ++i) for (j = 0u; j < 4u; ++j) r[0x3C0 + i * 4u + j] = s->terrain_samples[i][j];
}

static int compare(const char *w, unsigned v, const RaceAiState *s, const uint8_t *r)
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
    unsigned v;
    unsigned long moves = 0u;
    for (v = 0u; v < count; ++v) {
        RaceAiState s;
        const unsigned x = nb() & 3u;
        randomize(&s);
        toRam(&s, cpu->ram);
        cpu->ram[0x0F] = (uint8_t)x;
        cpu->a = 0u; cpu->x = (uint8_t)x; cpu->y = 0u; cpu->sp = 0xFDu; cpu->p = 0x24u; cpu->cycles = 0u; cpu->bad_opcode = -1;
        if (cpu6502_call(cpu, mode == 0 ? 0xCED0u : 0xD018u, 100000u) != 0) { printf("FAIL v=%u oracle error\n", v); return 1; }
        if (mode == 0) raceAiUpdate(&s);
        else {
            const uint8_t n = raceAiClassifyTerrain(&s, x);
            if (n != cpu->a) { printf("FAIL D018 v=%u count native=%u ASM=%u\n", v, n, cpu->a); return 1; }
        }
        if (compare(mode == 0 ? "sub_CED0" : "sub_D018", v, &s, cpu->ram) != 0) return 1;
        moves += (cpu->ram[0xDC] | cpu->ram[0xDD] | cpu->ram[0xDE] | cpu->ram[0xDF]) != 0u;
    }
    printf("PASS: %u %s vectors match original ASM (%lu with lane movement).\n", count, mode == 0 ? "sub_CED0 AI" : "sub_D018 terrain", moves);
    return 0;
}

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *fp;
    if (argc != 2) { puts("usage: test_race_ai <prg>"); return 2; }
    fp = fopen(argv[1], "rb");
    if (fp == NULL || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) return 2;
    fclose(fp);
    cpu6502_reset_state(&cpu);
    return run(&cpu, 1, 200000u) || run(&cpu, 0, 400000u);
}
