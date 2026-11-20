#include <stdio.h>
#include <string.h>
#include "race_frame.h"
#include "cpu6502.h"
#include "race_spawn.h"

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

static int test_e4c8(Cpu6502 *cpu)
{
    unsigned v, i;
    unsigned long hits = 0u;
    static const uint8_t lows[] = { 0u, 0xF7u, 0xF8u, 0xFFu, 0x10u };
    for (v = 0u; v < 400000u; ++v) {
        RaceCollisionState s;
        uint8_t *r = cpu->ram;
        const uint8_t base = nb();
        memset(&s, 0, sizeof(s));
        for (i = 0u; i < 4u; ++i) {
            s.x_high[i] = (nb() & 3u) ? (uint8_t)(nb() & 1u) : (uint8_t)(nb() & 3u);
            s.x_low[i] = (nb() & 1u) ? pick(lows, sizeof(lows)) : nb();
            s.y_low[i] = (nb() & 3u) ? (uint8_t)(base + (nb() & 0x1Fu) - 0x10u) : nb();
            if ((v & 1u) != 0u) { s.x_high[i] = 1u; s.y_low[i] = (uint8_t)(base + ((nb() % 3u) ? (nb() % 0x30u) : (nb() % 0x12u)) - 0x10u); }
        }
        s.collision_flag_03A3 = nb();
        memset(r, 0, sizeof(cpu->ram));
        MAP4(x_high, 0x84); MAP4(x_low, 0x80); MAP4(y_low, 0x8C);
        r[0x3A3] = s.collision_flag_03A3;
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xE4C8u, 20000u) != 0) return 1;
        raceBikeCollision(&s);
        if (s.collision_flag_03A3 != r[0x3A3]) { printf("FAIL E4C8 v=%u flag native=%u ASM=%u\n", v, s.collision_flag_03A3, r[0x3A3]); return 1; }
        hits += r[0x3A3];
    }
    printf("PASS: 400000 sub_E4C8 collision vectors match original ASM (%lu collisions).\n", hits);
    return 0;
}

static int test_e42b(Cpu6502 *cpu)
{
    unsigned v, i;
    for (v = 0u; v < 200000u; ++v) {
        RaceHideSpritesState s;
        uint8_t *r = cpu->ram;
        memset(&s, 0, sizeof(s));
        s.collision_flag_03A3 = (nb() & 1u) ? 0u : nb();
        for (i = 0u; i < 4u; ++i) s.slot_active_0088[i] = (nb() & 3u) ? (uint8_t)(nb() & 3u) : nb();
        s.rider_present_0373 = nb() & 1u; s.toggle_03B3 = (uint8_t)(nb() & 7u);
        for (i = 0u; i < 256u; ++i) s.oam.bytes[i] = nb();
        memset(r, 0, sizeof(cpu->ram));
        MAP4(slot_active_0088, 0x88);
        r[0x3A3] = s.collision_flag_03A3; r[0x373] = s.rider_present_0373; r[0x3B3] = s.toggle_03B3;
        for (i = 0u; i < 256u; ++i) r[0x200 + i] = s.oam.bytes[i];
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xE42Bu, 20000u) != 0) return 1;
        raceHideCollidedSprites(&s);
        if (s.toggle_03B3 != r[0x3B3]) { printf("FAIL E42B v=%u toggle\n", v); return 1; }
        for (i = 0u; i < 256u; ++i) if (s.oam.bytes[i] != r[0x200 + i]) { printf("FAIL E42B v=%u oam[%u]\n", v, i); return 1; }
    }
    puts("PASS: 200000 loc_E42B/sub_E4AD collided-sprite hiding vectors match original ASM.");
    return 0;
}


static int test_e456(Cpu6502 *cpu)
{
    unsigned v, i;
    for (v = 0u; v < 200000u; ++v) {
        RaceOffscreenSpriteState s;
        RaceSpawnState opponents;
        uint8_t *r = cpu->ram;
        memset(&s, 0, sizeof(s));
        memset(&opponents, 0, sizeof(opponents));
        s.opponent_cleared_03A6 = nb();
        for (i = 0u; i < 4u; ++i) {
            s.slot_object_index[i] = (uint8_t)(nb() % 4u);
            opponents.x_high[i] = (uint8_t)(nb() & 3u);
            opponents.x_low[i] = nb();
            opponents.active[i] = nb(); opponents.y_low[i] = nb();
            opponents.vertical_reference[i] = nb(); opponents.slot_00A0[i] = nb();
            opponents.vertical_phase[i] = nb(); opponents.action_state[i] = nb();
            opponents.lateral_state[i] = nb(); opponents.dust_flags[i] = nb();
            opponents.speed_fraction[i] = nb(); opponents.speed_whole[i] = nb();
            opponents.collision_state[i] = nb(); opponents.movement_state[i] = nb();
            opponents.object_status[i] = nb(); opponents.brake_state[i] = nb();
            opponents.movement_mode[i] = nb(); opponents.script_x_high[i] = nb();
            opponents.block_039C[i] = nb(); opponents.lane_delta[i] = nb();
            opponents.position_0070[i] = nb(); opponents.rider_present[i] = nb();
        }
        for (i = 0u; i < 256u; ++i) s.oam.bytes[i] = nb();

        memset(r, 0, sizeof(cpu->ram));
        for (i = 0u; i < 4u; ++i) {
            r[0x88u + i] = s.slot_object_index[i];
            r[0x84u + i] = opponents.x_high[i];
            r[0x80u + i] = opponents.x_low[i];
#define MAP_OPP(F, B) r[(B) + i] = opponents.F[i]
            MAP_OPP(active, 0xA8); MAP_OPP(y_low, 0xB8); MAP_OPP(vertical_reference, 0xBC);
            MAP_OPP(slot_00A0, 0xA0); MAP_OPP(vertical_phase, 0xA4); MAP_OPP(action_state, 0x58);
            MAP_OPP(lateral_state, 0xCC); MAP_OPP(dust_flags, 0x36C); MAP_OPP(speed_fraction, 0x90);
            MAP_OPP(speed_whole, 0x94); MAP_OPP(collision_state, 0x98); MAP_OPP(movement_state, 0x9C);
            MAP_OPP(object_status, 0xB0); MAP_OPP(brake_state, 0xD4); MAP_OPP(movement_mode, 0x388);
            MAP_OPP(script_x_high, 0xC0); MAP_OPP(block_039C, 0x39C); MAP_OPP(lane_delta, 0xDC);
            MAP_OPP(position_0070, 0x70); MAP_OPP(rider_present, 0x370);
#undef MAP_OPP
        }
        r[0x3A6] = s.opponent_cleared_03A6;
        for (i = 0u; i < 256u; ++i) r[0x200u + i] = s.oam.bytes[i];
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xE456u, 30000u) != 0) {
            printf("FAIL E456 oracle v=%u\\n", v); return 1;
        }
        raceHideOffscreenOpponentSprites(&s, &opponents);
        if (s.opponent_cleared_03A6 != r[0x3A6]) {
            printf("FAIL E456 v=%u 03A6 native=%02X ASM=%02X\\n", v, s.opponent_cleared_03A6, r[0x3A6]); return 1;
        }
        for (i = 0u; i < 256u; ++i) if (s.oam.bytes[i] != r[0x200u + i]) {
            printf("FAIL E456 v=%u OAM[%u] native=%02X ASM=%02X\\n", v, i, s.oam.bytes[i], r[0x200u + i]); return 1;
        }
#define CHECK_OPP(F, B) for (i = 0u; i < 4u; ++i) if (opponents.F[i] != r[(B) + i]) { \
    printf("FAIL E456 v=%u " #F "[%u] native=%02X ASM=%02X\\n", v, i, opponents.F[i], r[(B)+i]); return 1; }
        CHECK_OPP(active, 0xA8); CHECK_OPP(y_low, 0xB8); CHECK_OPP(vertical_reference, 0xBC);
        CHECK_OPP(slot_00A0, 0xA0); CHECK_OPP(vertical_phase, 0xA4); CHECK_OPP(action_state, 0x58);
        CHECK_OPP(lateral_state, 0xCC); CHECK_OPP(dust_flags, 0x36C); CHECK_OPP(speed_fraction, 0x90);
        CHECK_OPP(speed_whole, 0x94); CHECK_OPP(collision_state, 0x98); CHECK_OPP(movement_state, 0x9C);
        CHECK_OPP(object_status, 0xB0); CHECK_OPP(brake_state, 0xD4); CHECK_OPP(movement_mode, 0x388);
        CHECK_OPP(script_x_high, 0xC0); CHECK_OPP(block_039C, 0x39C); CHECK_OPP(lane_delta, 0xDC);
        CHECK_OPP(position_0070, 0x70); CHECK_OPP(rider_present, 0x370);
#undef CHECK_OPP
    }
    puts("PASS: 200000 sub_E456 off-screen sprite/OAM and DB95 clear vectors match original ASM.");
    return 0;
}


static int test_e70b(Cpu6502 *cpu)
{
    unsigned v, i;
    for (v = 0u; v < 200000u; ++v) {
        RacePositionSmoothingState s;
        uint8_t *r = cpu->ram;
        memset(&s, 0, sizeof(s));
        for (i = 0u; i < 4u; ++i) {
            s.movement_delta[i] = nb();
            s.target_x[i] = nb();
            s.phase[i] = nb();
            s.transient[i] = nb();
        }
        memset(r, 0, sizeof(cpu->ram));
        for (i = 0u; i < 4u; ++i) {
            r[0x60u + i] = s.movement_delta[i];
            r[0x64u + i] = s.target_x[i];
            r[0xE0u + i] = s.phase[i];
            r[0xD8u + i] = s.transient[i];
        }
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xE70Bu, 4096u) != 0) {
            printf("FAIL E70B oracle v=%u\\n", v); return 1;
        }
        raceSmoothBikePositions(&s);
        for (i = 0u; i < 4u; ++i) {
            if (s.target_x[i] != r[0x64u + i] ||
                s.phase[i] != r[0xE0u + i] ||
                s.transient[i] != r[0xD8u + i]) {
                printf("FAIL E70B v=%u slot=%u target=%02X/%02X phase=%02X/%02X transient=%02X/%02X\\n",
                       v, i, s.target_x[i], r[0x64u+i], s.phase[i], r[0xE0u+i],
                       s.transient[i], r[0xD8u+i]);
                return 1;
            }
        }
    }
    puts("PASS: 200000 sub_E70B four-bike position-smoothing vectors match original ASM.");
    return 0;
}


static int test_df30(Cpu6502 *cpu)
{
    unsigned v, i;
    for (v = 0u; v < 120000u; ++v) {
        RaceElapsedTimeState s;
        uint8_t *r = cpu->ram;
        memset(&s, 0, sizeof(s));
        s.race_started = (uint8_t)(nb() & 1u);
        s.track_finished = nb();
        s.minutes = (uint8_t)(nb() % 9u);
        s.seconds = (uint8_t)(nb() % 60u);
        s.hundredths = (uint8_t)(nb() % 100u);
        s.frame_fraction = (uint8_t)(nb() % 10u);
        for (i = 0u; i < 7u; ++i) {
            s.decimal_work[i] = nb();
            s.display[i] = nb();
        }

        memset(r, 0, sizeof(cpu->ram));
        r[0x4F] = s.race_started; r[0x52] = s.track_finished;
        r[0x68] = s.minutes; r[0x69] = s.seconds;
        r[0x6A] = s.hundredths; r[0x6B] = s.frame_fraction;
        for (i = 0u; i < 7u; ++i) {
            r[0x3D1u + i] = s.decimal_work[i];
            r[0x3D9u + i] = s.display[i];
        }
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xDF30u, 20000u) != 0) {
            printf("FAIL DF30 oracle v=%u\\n", v); return 1;
        }
        raceElapsedTimeUpdate(&s);
        if (s.track_finished != r[0x52] || s.minutes != r[0x68] ||
            s.seconds != r[0x69] || s.hundredths != r[0x6A] ||
            s.frame_fraction != r[0x6B]) {
            printf("FAIL DF30 v=%u timer native=%02X/%02X/%02X/%02X finish=%02X ASM=%02X/%02X/%02X/%02X finish=%02X\\n",
                   v, s.minutes, s.seconds, s.hundredths, s.frame_fraction, s.track_finished,
                   r[0x68], r[0x69], r[0x6A], r[0x6B], r[0x52]);
            return 1;
        }
        for (i = 0u; i < 7u; ++i) {
            if (s.decimal_work[i] != r[0x3D1u + i] ||
                s.display[i] != r[0x3D9u + i]) {
                printf("FAIL DF30 v=%u glyph[%u] work=%02X/%02X display=%02X/%02X\\n",
                       v, i, s.decimal_work[i], r[0x3D1u+i],
                       s.display[i], r[0x3D9u+i]);
                return 1;
            }
        }
    }
    puts("PASS: 120000 sub_DF30 elapsed-time vectors match original ASM.");
    return 0;
}

static int test_dfd5(Cpu6502 *cpu)
{
    unsigned v, i;
    unsigned long contacts = 0u;
    for (v = 0u; v < 400000u; ++v) {
        RaceBikeContactState s;
        uint8_t *r = cpu->ram;
        const uint8_t base = nb(), xb = nb();
        memset(&s, 0, sizeof(s));
        for (i = 0u; i < 4u; ++i) {
            s.active[i] = (nb() & 7u) ? 1u : 0u;
            s.x_high[i] = (nb() & 7u) ? 1u : (uint8_t)(nb() & 3u);
            s.position[i] = (nb() & 7u) ? 2u : (uint8_t)(nb() % 4u);
            s.collision_state[i] = (nb() & 7u) ? 0u : nb();
            s.overheat_03E0[i] = (nb() & 7u) ? 0u : nb();
            s.object_status[i] = (nb() & 1u) ? 0u : (uint8_t)(nb() & 3u);
            s.y_low[i] = (nb() & 3u) ? (uint8_t)(base + (nb() & 0x1Fu) - 0x0Fu) : nb();
            s.x_low[i] = (nb() & 3u) ? (uint8_t)(xb + (nb() & 0x1Fu) - 0x0Fu) : ((nb() & 1u) ? (uint8_t)(0xE0u + (nb() & 0x1Fu)) : nb());
        }
        s.timer_003B = nb(); s.sound_request_00FD = nb();
        memset(r, 0, sizeof(cpu->ram));
        MAP4(active, 0xA8); MAP4(x_high, 0x84); MAP4(position, 0x70); MAP4(collision_state, 0x98);
        MAP4(overheat_03E0, 0x3E0); MAP4(object_status, 0xB0); MAP4(y_low, 0x8C); MAP4(x_low, 0x80);
        r[0x3B] = s.timer_003B; r[0xFD] = s.sound_request_00FD;
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xDFD5u, 50000u) != 0) return 1;
        raceBikeContacts(&s);
        for (i = 0u; i < 4u; ++i) if (s.collision_state[i] != r[0x98 + i]) { printf("FAIL DFD5 v=%u collision[%u] native=%02X ASM=%02X\n", v, i, s.collision_state[i], r[0x98 + i]); return 1; }
        if (s.timer_003B != r[0x3B] || s.sound_request_00FD != r[0xFD]) { printf("FAIL DFD5 v=%u timer/sound\n", v); return 1; }
        contacts += (r[0xFD] == 1u);
    }
    printf("PASS: 400000 sub_DFD5 bike-contact vectors match original ASM (%lu contacts).\n", contacts);
    return 0;
}

static int test_e09f(Cpu6502 *cpu)
{
    unsigned v, i;
    unsigned long hits = 0u;
    for (v = 0u; v < 400000u; ++v) {
        RaceObstacleHitState s;
        uint8_t *r = cpu->ram;
        memset(&s, 0, sizeof(s));
        for (i = 0u; i < 4u; ++i) s.movement_state[i] = (nb() & 3u) ? 0u : nb();
        s.x_high_slot3 = (nb() & 3u) ? 1u : nb(); s.rider_present_0373 = (nb() & 7u) ? 1u : 0u;
        s.object_y_player = nb(); s.obstacle_y_006F = (nb() & 1u) ? (uint8_t)(s.object_y_player + (nb() & 0x0Fu) - 4u) : nb();
        s.x_low_player = nb(); s.obstacle_x_0393 = (nb() & 1u) ? (uint8_t)(s.x_low_player + (nb() & 0x1Fu) - 4u) : nb();
        s.global_vertical_reference_03F1 = nb(); s.vertical_reference_player = (nb() & 1u) ? 0u : nb();
        s.y_low_player = nb(); s.collision_player = nb(); s.lane_delta_slot3 = nb(); s.sound_request_00FD = nb(); s.state_039B = nb();
        memset(r, 0, sizeof(cpu->ram));
        MAP4(movement_state, 0x9C);
        r[0x87] = s.x_high_slot3; r[0x373] = s.rider_present_0373; r[0xB8] = s.object_y_player; r[0x6F] = s.obstacle_y_006F;
        r[0x80] = s.x_low_player; r[0x393] = s.obstacle_x_0393; r[0x3F1] = s.global_vertical_reference_03F1;
        r[0xBC] = s.vertical_reference_player; r[0x8C] = s.y_low_player; r[0x98] = s.collision_player; r[0xDF] = s.lane_delta_slot3;
        r[0xFD] = s.sound_request_00FD; r[0x39B] = s.state_039B;
        prep(cpu, 0u);
        if (cpu6502_call(cpu, 0xE09Fu, 20000u) != 0) return 1;
        raceObstacleHit(&s);
        for (i = 0u; i < 4u; ++i) if (s.movement_state[i] != r[0x9C + i]) { printf("FAIL E09F v=%u movement_state[%u]\n", v, i); return 1; }
        if (s.collision_player != r[0x98] || s.lane_delta_slot3 != r[0xDF] || s.sound_request_00FD != r[0xFD] || s.state_039B != r[0x39B]) {
            printf("FAIL E09F v=%u outputs\n", v); return 1; }
        hits += (r[0x39B] == 2u);
    }
    printf("PASS: 400000 sub_E09F obstacle-hit vectors match original ASM (%lu hits).\n", hits);
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
    return test_df30(&cpu) || test_e70b(&cpu) || test_e456(&cpu) || test_ddd1(&cpu) || test_debb(&cpu) || test_dee2(&cpu) || test_de31(&cpu) || test_ea44(&cpu) || test_e4c8(&cpu) || test_e42b(&cpu) || test_dfd5(&cpu) || test_e09f(&cpu);
}
