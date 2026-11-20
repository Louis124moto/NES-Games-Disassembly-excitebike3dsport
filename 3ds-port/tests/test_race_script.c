#include <stdio.h>
#include <string.h>
#include "race_script.h"
#include "cpu6502.h"

static uint32_t rng = 0x5C219E07u;
static uint8_t nb(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }
static uint8_t pick(const uint8_t *v, unsigned n) { return v[nb() % n]; }

/* name, base address, struct member (4 slots) */
#define FIELDS4(X) \
    X(movement.movement_delta, 0x60) X(movement.object_action_state, 0x58) X(movement.vertical_phase, 0xA4) \
    X(movement.approach, 0xAC) X(movement.brake_state, 0xD4) X(movement.object_status, 0xB0) \
    X(movement.vertical_reference, 0xBC) X(movement.lateral_state, 0xCC) X(movement.object_origin_x, 0x64) \
    X(movement.speed_whole, 0x94) X(movement.speed_fraction, 0x90) X(movement.movement_flags, 0x5C) \
    X(movement.movement_mode, 0x388) X(movement.object_y, 0xB8) X(movement.y_low, 0x8C) \
    X(movement.rider_state, 0x364) X(movement.collision_state, 0x98) X(movement.motion_fraction, 0x380) \
    X(movement.motion_whole, 0x384) X(movement.motion_fraction_start, 0x378) X(movement.motion_whole_start, 0x37C) \
    X(movement.motion_delta, 0x38C) X(script_x_high, 0xC0) X(script_index, 0xC4) X(position_x, 0xC8) \
    X(slot_00A0, 0xA0) X(slot_00B4, 0xB4) X(slot_00D0, 0xD0) X(slot_00D8, 0xD8) X(slot_00E4, 0xE4) X(slot_036C, 0x36C)

#define SCALARS(X) \
    X(movement.track_finished, 0x52) X(movement.player_event, 0xFF) X(timer_003C, 0x3C) X(player_overheat_03E0, 0x3E0) \
    X(ram_03B6, 0x3B6) X(ram_0057, 0x57) X(timer_003A, 0x3A) X(timer_0032, 0x32) X(sound_request_00FD, 0xFD)

static void prep(Cpu6502 *c, uint8_t x)
{
    c->a = 0u; c->x = x; c->y = 0u; c->sp = 0xFDu; c->p = 0x24u; c->cycles = 0u; c->bad_opcode = -1;
}

static const uint8_t k_script_len[21] = { 21, 13, 13, 21, 21, 21, 19, 19, 19, 19, 13, 11, 9, 21, 29, 5, 3, 3, 3, 11, 3 };
static const uint8_t k_small[] = { 0u, 0u, 0u, 1u, 2u, 3u, 4u, 5u, 0x0Fu, 0x20u, 0x80u, 0xFFu };

static uint8_t lateralValue(void)
{
    const uint8_t v = (uint8_t)(nb() % 0x15u);
    return v == 0x11u ? 0x10u : v;
}

static void randomize(RaceScriptState *s, int lateral_only)
{
    unsigned i;
    memset(s, 0, sizeof(*s));
    for (i = 0u; i < 4u; ++i) {
        const unsigned script = (nb() & 7u) == 0u ? 0u : 1u + nb() % 21u;
        s->movement.object_action_state[i] = (uint8_t)script;
        s->script_index[i] = script == 0u ? (uint8_t)(nb() & 0xFEu)
            : (uint8_t)(2u * (nb() % ((k_script_len[script - 1u] - 1u) / 2u)));
        s->position_x[i] = (nb() & 1u) ? (uint8_t)(nb() & 0x7Fu) : nb();
        s->movement.movement_delta[i] = (nb() & 1u) ? (uint8_t)(nb() & 3u) : nb();
        s->movement.object_origin_x[i] = nb();
        s->script_x_high[i] = (nb() & 1u) ? (uint8_t)(0x40u + nb() % 0x54u) : nb();
        s->movement.vertical_phase[i] = (uint8_t)(nb() % 6u);
        s->movement.approach[i] = nb();
        s->movement.brake_state[i] = nb();
        s->movement.object_status[i] = (nb() & 3u) ? 0u : (uint8_t)(nb() & 3u);
        s->movement.vertical_reference[i] = nb();
        s->movement.lateral_state[i] = lateral_only ? lateralValue() : (nb() & 1u ? 0u : (uint8_t)(nb() & 0x0Fu));
        s->movement.speed_whole[i] = (nb() & 3u) ? (uint8_t)(nb() & 3u) : nb();
        s->movement.speed_fraction[i] = nb();
        s->movement.movement_flags[i] = nb();
        s->movement.movement_mode[i] = (uint8_t)(nb() & 3u);
        s->movement.object_y[i] = (nb() & 1u) ? pick(k_small, sizeof(k_small)) : nb();
        s->movement.y_low[i] = nb();
        s->movement.rider_state[i] = nb();
        s->movement.collision_state[i] = (nb() & 3u) ? 0u : nb();
        s->movement.motion_fraction[i] = nb(); s->movement.motion_whole[i] = nb();
        s->movement.motion_fraction_start[i] = nb(); s->movement.motion_whole_start[i] = nb();
        s->movement.motion_delta[i] = nb();
        s->slot_00A0[i] = (uint8_t)(nb() % 3u); s->slot_00B4[i] = nb(); s->slot_00D0[i] = nb();
        s->slot_00D8[i] = nb(); s->slot_00E4[i] = nb(); s->slot_036C[i] = nb();
    }
    s->movement.track_finished = nb() & 1u; s->movement.player_event = nb();
    s->timer_003C = (nb() & 1u) ? 0u : nb(); s->player_overheat_03E0 = (nb() & 3u) ? 0u : nb();
    s->ram_03B6 = nb(); s->ram_0057 = nb() & 1u; s->timer_003A = nb(); s->timer_0032 = nb();
    s->sound_request_00FD = nb();
    for (i = 0u; i < 7u; ++i) { s->cue_tiles_03D9[i] = nb(); s->gate_tile[i] = nb(); }
}

static void toRam(const RaceScriptState *s, uint8_t *r)
{
    unsigned i;
    memset(r, 0, 0x800u);
#define W4(F, B) for (i = 0u; i < 4u; ++i) r[(B) + i] = s->F[i];
    FIELDS4(W4)
#undef W4
#define W1(F, B) r[B] = s->F;
    SCALARS(W1)
#undef W1
    for (i = 0u; i < 7u; ++i) { r[0x3D9 + i] = s->cue_tiles_03D9[i]; r[0x2C1 + 4u * i] = s->gate_tile[i]; }
}

static int compare(const char *what, unsigned v, const RaceScriptState *s, const uint8_t *r)
{
    unsigned i;
#define C4(F, B) for (i = 0u; i < 4u; ++i) if (s->F[i] != r[(B) + i]) { \
    printf("FAIL %s v=%u %s[%u] native=%02X ASM=%02X\n", what, v, #F, i, s->F[i], r[(B) + i]); return 1; }
    FIELDS4(C4)
#undef C4
#define C1(F, B) if (s->F != r[B]) { printf("FAIL %s v=%u %s native=%02X ASM=%02X\n", what, v, #F, s->F, r[B]); return 1; }
    SCALARS(C1)
#undef C1
    for (i = 0u; i < 7u; ++i) if (s->gate_tile[i] != r[0x2C1 + 4u * i]) { printf("FAIL %s v=%u gate_tile[%u]\n", what, v, i); return 1; }
    return 0;
}

static int run(Cpu6502 *cpu, const char *what, uint16_t entry, int lateral, unsigned count)
{
    unsigned v, skipped = 0u, executed = 0u;
    for (v = 0u; v < count; ++v) {
        RaceScriptState s;
        randomize(&s, lateral);
        toRam(&s, cpu->ram);
        prep(cpu, 0u);
        if (cpu6502_call(cpu, entry, 50000u) != 0) return 1;
        if (lateral) raceScriptDispatchLateral(&s); else raceScriptUpdateOpponents(&s);
        if (s.unsupported_mask != 0u) { ++skipped; continue; }
        ++executed;
        if (compare(what, v, &s, cpu->ram) != 0) return 1;
    }
    printf("PASS: %u %s vectors match original ASM (%u skipped as unsupported source states).\n", executed, what, skipped);
    return skipped * 10u > count ? 1 : 0;
}

static int runOrigins(Cpu6502 *cpu)
{
    unsigned v;
    for (v = 0u; v < 200000u; ++v) {
        RaceScriptState s;
        unsigned i;
        randomize(&s, 0);
        for (i = 0u; i < 4u; ++i) {
            if (nb() & 1u) s.movement.movement_delta[i] = (uint8_t)(nb() & 7u);
            s.movement.object_class[i] = nb();
            if (nb() & 1u) s.movement.object_origin_x[i] = (uint8_t)(nb() & 0x0Fu);
        }
        toRam(&s, cpu->ram);
        for (i = 0u; i < 4u; ++i) cpu->ram[0xE0 + i] = s.movement.object_class[i];
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xE70Bu, 20000u) != 0) return 1;
        raceScriptAdvanceOrigins(&s);
        for (i = 0u; i < 4u; ++i) {
            if (s.movement.object_origin_x[i] != cpu->ram[0x64 + i] || s.movement.object_class[i] != cpu->ram[0xE0 + i] ||
                s.slot_00D8[i] != cpu->ram[0xD8 + i]) { printf("FAIL E70B v=%u slot %u\n", v, i); return 1; }
        }
    }
    puts("PASS: 200000 sub_E70B origin-advance vectors match original ASM.");
    return 0;
}

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *fp;
    if (argc != 2) { puts("usage: test_race_script <prg>"); return 2; }
    fp = fopen(argv[1], "rb");
    if (fp == NULL || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) return 2;
    fclose(fp);
    cpu6502_reset_state(&cpu);
    return run(&cpu, "sub_E733 opponent-script", 0xE733u, 0, 400000u) ||
           run(&cpu, "sub_E927 lateral-dispatch", 0xE927u, 1, 300000u) ||
           runOrigins(&cpu);
}
