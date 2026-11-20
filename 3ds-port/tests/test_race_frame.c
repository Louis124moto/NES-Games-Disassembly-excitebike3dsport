#include <stdio.h>
#include <string.h>
#include "race_frame.h"
#include "cpu6502.h"

static uint32_t rng = 0xD0D1D2D3u;
static uint8_t nb(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }
static uint8_t pick(const uint8_t *v, unsigned n) { return v[nb() % n]; }
static const uint8_t k_small[] = { 0u, 0u, 1u, 2u, 3u, 0x0Fu, 0x80u, 0xFFu };
static const uint8_t k_byte[] = { 0u, 1u, 7u, 8u, 0x7Fu, 0x80u, 0xF7u, 0xF8u, 0xFFu };

static void prep(Cpu6502 *c, uint8_t x)
{
    c->a = 0u; c->x = x; c->y = 0u; c->sp = 0xFDu; c->p = 0x24u; c->cycles = 0u; c->bad_opcode = -1;
}

#define MAP4(field, base) for (i = 0u; i < 4u; ++i) r[(base) + i] = s.field[i]
#define CMP4(field, base, name) for (i = 0u; i < 4u; ++i) if (s.field[i] != r[(base) + i]) { \
    printf("FAIL DDD1 v=%u %s[%u] native=%02X ASM=%02X\n", v, name, i, s.field[i], r[(base) + i]); return 1; }

static int test_ddd1(Cpu6502 *cpu)
{
    unsigned v, i;
    unsigned long recovered = 0u;
    for (v = 0u; v < 120000u; ++v) {
        RaceCrashRecoveryState s;
        uint8_t *r = cpu->ram;
        memset(&s, 0, sizeof(s));
        for (i = 0u; i < 4u; ++i) {
            s.collision_state[i] = pick(k_small, sizeof(k_small));
            s.speed_whole[i] = pick(k_small, sizeof(k_small));
            s.speed_fraction[i] = nb();
            s.movement_delta[i] = nb();
            s.object_status[i] = pick(k_small, sizeof(k_small));
            s.inactive[i] = pick(k_small, sizeof(k_small));
            s.approach[i] = nb();
            s.x_low[i] = pick(k_byte, sizeof(k_byte));
            s.previous_x[i] = nb();
            s.y_low[i] = nb();
            s.derived_y[i] = nb();
            s.movement_state[i] = nb();
            s.crash_variant[i] = nb();
            s.recovery_flag[i] = nb();
        }
        s.player_crashed_03A9 = nb(); s.sound_request_00FC = nb(); s.sound_request_00FD = nb();
        s.frame_phase_004C = (uint8_t)(nb() & 3u); s.random_001B = nb();

        memset(r, 0, sizeof(cpu->ram));
        MAP4(collision_state, 0x98); MAP4(speed_whole, 0x94); MAP4(speed_fraction, 0x90);
        MAP4(movement_delta, 0x60); MAP4(object_status, 0xB0); MAP4(inactive, 0x58);
        MAP4(approach, 0xAC); MAP4(x_low, 0x80); MAP4(previous_x, 0x390); MAP4(y_low, 0xB8);
        MAP4(derived_y, 0x6C); MAP4(movement_state, 0x9C); MAP4(crash_variant, 0x3E4);
        MAP4(recovery_flag, 0x398);
        r[0x3A9] = s.player_crashed_03A9; r[0xFC] = s.sound_request_00FC; r[0xFD] = s.sound_request_00FD;
        r[0x4C] = s.frame_phase_004C; r[0x1B] = s.random_001B;
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xDDD1u, 4096u) != 0) { printf("FAIL: ASM DDD1 did not return v=%u\n", v); return 1; }

        raceCrashRecovery(&s);
        CMP4(collision_state, 0x98, "0098"); CMP4(speed_whole, 0x94, "0094"); CMP4(speed_fraction, 0x90, "0090");
        CMP4(movement_delta, 0x60, "0060"); CMP4(approach, 0xAC, "00AC"); CMP4(previous_x, 0x390, "0390");
        CMP4(derived_y, 0x6C, "006C"); CMP4(movement_state, 0x9C, "009C"); CMP4(crash_variant, 0x3E4, "03E4");
        CMP4(recovery_flag, 0x398, "0398");
        if (s.player_crashed_03A9 != r[0x3A9] || s.sound_request_00FC != r[0xFC] || s.sound_request_00FD != r[0xFD]) {
            printf("FAIL DDD1 v=%u scalars native %02X/%02X/%02X ASM %02X/%02X/%02X\n", v,
                   s.player_crashed_03A9, s.sound_request_00FC, s.sound_request_00FD, r[0x3A9], r[0xFC], r[0xFD]);
            return 1;
        }
        { unsigned any = 0u; for (i = 0u; i < 4u; ++i) any |= (s.movement_state[i] == 1u && s.recovery_flag[i] == 2u); recovered += any; }
    }
    printf("PASS: 120000 sub_DDD1 crash-recovery vectors match original ASM (%lu reach the recovery path).\n", recovered);
    return 0;
}

static int test_debb(Cpu6502 *cpu)
{
    unsigned v, i;
    for (v = 0u; v < 100000u; ++v) {
        RaceOpponentCueState s;
        uint8_t *r = cpu->ram;
        static const uint8_t lows[] = { 0u, 0x2Fu, 0x30u, 0x3Fu, 0x40u, 0x7Fu, 0xFFu };
        memset(&s, 0, sizeof(s));
        for (i = 0u; i < 4u; ++i) { s.x_high[i] = nb() & 3u; s.x_low[i] = (nb() & 1u) ? pick(lows, sizeof(lows)) : nb(); }
        for (i = 0u; i < 3u; ++i) s.cue_flag[i] = pick(k_small, sizeof(k_small));
        s.sound_request_00FF = nb();
        memset(r, 0, sizeof(cpu->ram));
        for (i = 0u; i < 4u; ++i) { r[0x84 + i] = s.x_high[i]; r[0x80 + i] = s.x_low[i]; }
        for (i = 0u; i < 3u; ++i) r[0x3AA + i] = s.cue_flag[i];
        r[0xFF] = s.sound_request_00FF;
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xDEBBu, 4096u) != 0) return 1;
        raceOpponentCue(&s);
        for (i = 0u; i < 3u; ++i) if (s.cue_flag[i] != r[0x3AA + i]) { printf("FAIL DEBB v=%u flag[%u] %02X/%02X\n", v, i, s.cue_flag[i], r[0x3AA + i]); return 1; }
        if (s.sound_request_00FF != r[0xFF]) { printf("FAIL DEBB v=%u 00FF\n", v); return 1; }
    }
    puts("PASS: 100000 sub_DEBB opponent-cue vectors match original ASM.");
    return 0;
}

static int test_dee2(Cpu6502 *cpu)
{
    unsigned v;
    for (v = 0u; v < 200000u; ++v) {
        RaceEngineSoundState s;
        uint8_t *r = cpu->ram;
        static const uint8_t sp[] = { 0u, 1u, 2u, 3u, 4u, 0x0Fu, 0x10u, 0x17u, 0x18u, 0x7Fu, 0xFFu };
        s.speed_whole = (nb() & 1u) ? (uint8_t)(nb() & 3u) : pick(sp, sizeof(sp));
        s.speed_fraction = (nb() & 1u) ? nb() : pick(sp, sizeof(sp));
        s.object_status = (nb() & 1u) ? 0u : (uint8_t)(1u + (nb() & 2u));
        s.controller = nb();
        s.player_crashed_03A9 = (nb() & 3u) ? 1u : 0u;
        s.frame_phase_004C = (uint8_t)(nb() & 3u);
        s.frame_counter_003F = nb();
        s.sound_request_00FC = nb();
        memset(r, 0, sizeof(cpu->ram));
        r[0x94] = s.speed_whole; r[0x90] = s.speed_fraction; r[0xB0] = s.object_status; r[0x5C] = s.controller;
        r[0x3A9] = s.player_crashed_03A9; r[0x4C] = s.frame_phase_004C; r[0x3F] = s.frame_counter_003F; r[0xFC] = s.sound_request_00FC;
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xDEE2u, 4096u) != 0) return 1;
        raceEngineSound(&s);
        if (s.sound_request_00FC != r[0xFC]) {
            printf("FAIL DEE2 v=%u whole=%02X frac=%02X st=%02X ctl=%02X native=%02X ASM=%02X\n", v, r[0x94], r[0x90], r[0xB0], r[0x5C], s.sound_request_00FC, r[0xFC]);
            return 1;
        }
    }
    puts("PASS: 200000 sub_DEE2 engine-sound vectors match original ASM.");
    return 0;
}

static int test_de31(Cpu6502 *cpu)
{
    unsigned v, i;
    unsigned long banner_paths = 0u;
    for (v = 0u; v < 300000u; ++v) {
        RaceStartBannerState s;
        uint8_t *r = cpu->ram;
        static const uint8_t t24[] = { 0u, 1u, 2u, 7u, 8u, 0x0Fu, 0x10u, 0x5Fu, 0x60u, 0x71u, 0x72u, 0x73u, 0x88u, 0xFFu };
        static const uint8_t xs[] = { 0u, 1u, 0x0Fu, 0x10u, 0x11u, 0x2Fu, 0x70u, 0xF0u, 0xFFu };
        memset(&s, 0, sizeof(s));
        s.timer_0024 = (nb() & 3u) ? pick(t24, sizeof(t24)) : nb();
        s.timer_0034 = pick(k_small, sizeof(k_small));
        s.race_started_004F = (nb() & 3u) == 0u ? 1u : 0u;
        s.banner_x_03B4 = (nb() & 1u) ? pick(xs, sizeof(xs)) : nb();
        s.player_delta_0060 = (nb() & 1u) ? pick(xs, sizeof(xs)) : nb();
        s.player_crashed_03A9 = (nb() & 1u) ? 1u : 0u;
        s.controller_005C = nb();
        s.sound_request_00FB = nb(); s.sound_request_00FE = nb();
        for (i = 0u; i < 8u; ++i) { s.banner[i].y = nb(); s.banner[i].tile = nb(); s.banner[i].attribute = nb(); s.banner[i].x = nb(); }
        memset(r, 0, sizeof(cpu->ram));
        r[0x24] = s.timer_0024; r[0x34] = s.timer_0034; r[0x4F] = s.race_started_004F; r[0x3B4] = s.banner_x_03B4;
        r[0x60] = s.player_delta_0060; r[0x3A9] = s.player_crashed_03A9; r[0x5C] = s.controller_005C;
        r[0xFB] = s.sound_request_00FB; r[0xFE] = s.sound_request_00FE;
        for (i = 0u; i < 8u; ++i) { r[0x2C0 + 4 * i] = s.banner[i].y; r[0x2C1 + 4 * i] = s.banner[i].tile; r[0x2C2 + 4 * i] = s.banner[i].attribute; r[0x2C3 + 4 * i] = s.banner[i].x; }
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xDE31u, 8192u) != 0) return 1;
        raceStartBanner(&s);
        if (s.timer_0024 != r[0x24] || s.race_started_004F != r[0x4F] || s.banner_x_03B4 != r[0x3B4] ||
            s.sound_request_00FB != r[0xFB] || s.sound_request_00FE != r[0xFE]) {
            printf("FAIL DE31 v=%u scalars native 24=%02X 4F=%02X B4=%02X FB=%02X FE=%02X ASM 24=%02X 4F=%02X B4=%02X FB=%02X FE=%02X\n", v,
                   s.timer_0024, s.race_started_004F, s.banner_x_03B4, s.sound_request_00FB, s.sound_request_00FE,
                   r[0x24], r[0x4F], r[0x3B4], r[0xFB], r[0xFE]);
            return 1;
        }
        for (i = 0u; i < 8u; ++i) {
            if (s.banner[i].y != r[0x2C0 + 4 * i] || s.banner[i].tile != r[0x2C1 + 4 * i] ||
                s.banner[i].attribute != r[0x2C2 + 4 * i] || s.banner[i].x != r[0x2C3 + 4 * i]) {
                printf("FAIL DE31 v=%u sprite %u native %02X %02X %02X %02X ASM %02X %02X %02X %02X\n", v, i,
                       s.banner[i].y, s.banner[i].tile, s.banner[i].attribute, s.banner[i].x,
                       r[0x2C0 + 4 * i], r[0x2C1 + 4 * i], r[0x2C2 + 4 * i], r[0x2C3 + 4 * i]);
                return 1;
            }
        }
        banner_paths += (s.banner[7].tile != 0u && s.race_started_004F == 0u);
    }
    printf("PASS: 300000 sub_DE31 start-banner vectors match original ASM (%lu draw sprites).\n", banner_paths);
    return 0;
}

static int test_ea44(Cpu6502 *cpu)
{
    unsigned v, i;
    static const uint8_t lows[] = { 0u, 0x0Fu, 0x10u, 0xF3u, 0xF4u, 0xF5u, 0xF6u, 0xFFu };
    for (v = 0u; v < 300000u; ++v) {
        RaceEffectSpriteState s;
        uint8_t *r = cpu->ram;
        memset(&s, 0, sizeof(s));
        for (i = 0u; i < 4u; ++i) {
            s.x_high[i] = nb() & 3u;
            s.x_low[i] = (nb() & 1u) ? pick(lows, sizeof(lows)) : nb();
            s.object_status[i] = (nb() & 1u) ? 0u : (uint8_t)(nb() & 3u);
            s.vertical_reference[i] = (nb() & 1u) ? 0u : nb();
            s.object_y[i] = nb(); s.marker_index[i] = (uint8_t)(nb() % 19u); s.y_low[i] = nb();
            s.dust_flags[i] = (nb() & 1u) ? 0u : nb();
            s.speed_whole[i] = (nb() & 1u) ? 0u : (uint8_t)(nb() & 3u); s.speed_fraction[i] = (nb() & 1u) ? 0u : nb();
        }
        s.frame_counter_003F = nb(); s.global_vertical_reference_03F1 = nb();
        for (i = 0u; i < 8u; ++i) { s.sprite[i].y = nb(); s.sprite[i].tile = nb(); s.sprite[i].attribute = nb(); s.sprite[i].x = nb(); }
        memset(r, 0, sizeof(cpu->ram));
        MAP4(x_high, 0x84); MAP4(x_low, 0x80); MAP4(object_status, 0xB0); MAP4(vertical_reference, 0xBC);
        MAP4(object_y, 0xB8); MAP4(marker_index, 0xB4); MAP4(y_low, 0x8C); MAP4(dust_flags, 0x36C);
        MAP4(speed_whole, 0x94); MAP4(speed_fraction, 0x90);
        r[0x3F] = s.frame_counter_003F; r[0x3F1] = s.global_vertical_reference_03F1;
        for (i = 0u; i < 8u; ++i) { r[0x294 + i * 4] = s.sprite[i].y; r[0x295 + i * 4] = s.sprite[i].tile; r[0x296 + i * 4] = s.sprite[i].attribute; r[0x297 + i * 4] = s.sprite[i].x; }
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xEA44u, 20000u) != 0) return 1;
        raceEffectSprites(&s);
        for (i = 0u; i < 8u; ++i) if (s.sprite[i].y != r[0x294 + i * 4] || s.sprite[i].tile != r[0x295 + i * 4] ||
            s.sprite[i].attribute != r[0x296 + i * 4] || s.sprite[i].x != r[0x297 + i * 4]) { printf("FAIL EA44 v=%u sprite %u\n", v, i); return 1; }
    }
    puts("PASS: 300000 sub_EA44 effect-sprite vectors match original ASM.");
    return 0;
}

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *fp;
    if (argc != 2) { puts("usage: test_race_frame <prg>"); return 2; }
    fp = fopen(argv[1], "rb");
    if (fp == NULL || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) return 2;
    fclose(fp);
    cpu6502_reset_state(&cpu);
    return test_ddd1(&cpu) || test_debb(&cpu) || test_dee2(&cpu) || test_de31(&cpu) || test_ea44(&cpu);
}
