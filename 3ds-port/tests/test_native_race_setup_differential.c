#include <stdio.h>
#include <string.h>

#include "native_race_setup.h"
#include "cpu6502.h"

static uint32_t random_state = 0xC8752026u;

static uint8_t next_byte(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return (uint8_t)(random_state >> 24);
}

static void prepare_cpu(Cpu6502 *cpu)
{
    cpu->a = 0u;
    cpu->x = 0u;
    cpu->y = 0u;
    cpu->sp = 0xFDu;
    cpu->p = 0x24u;
    cpu->cycles = 0u;
    cpu->bad_opcode = -1;
}

#define CHECK_EQ(label, native_value, asm_value, vector) do { \
    if ((native_value) != (asm_value)) { \
        printf("FAIL C875 vector=%u %s native=%02X ASM=%02X\n", \
               (vector), (label), (unsigned)(native_value), (unsigned)(asm_value)); \
        return 1; \
    } \
} while (0)

static int run_case(Cpu6502 *cpu, unsigned vector)
{
    NativeRaceSetupInput input;
    NativeRaceSetupState setup;
    NativeRngState rng;
    BikeMovementState movement;
    BikeStateMachineState states;
    static const uint8_t course_parameters[2][3] = {
        { 0x01u, 0x10u, 0x00u },
        { 0x01u, 0x18u, 0x00u }
    };
    unsigned i;
    uint8_t mode = (uint8_t)(next_byte() & 1u);
    uint8_t transition = (uint8_t)(next_byte() & 1u);
    uint8_t timer = (uint8_t)(next_byte() & 0x7Fu);
    uint8_t counter = next_byte();
    uint8_t counter40 = next_byte();
    uint8_t counter47 = next_byte();

    memset(&input, 0, sizeof(input));
    memset(&setup, 0, sizeof(setup));
    memset(&movement, 0, sizeof(movement));
    memset(&states, 0, sizeof(states));
    memset(&rng, 0, sizeof(rng));
    memset(cpu->ram, 0, sizeof(cpu->ram));

    for (i = 0u; i < sizeof(rng.bytes); ++i)
        rng.bytes[i] = next_byte();

    /* C1FD is the actual title/menu seed for $0580/$05A3, consumed by C53E. */
    prepare_cpu(cpu);
    if (cpu6502_call(cpu, 0xC1FDu, 100000u) != 0) {
        printf("FAIL C875 vector=%u C1FD oracle\n", vector);
        return 1;
    }

    for (i = 0u; i < sizeof(rng.bytes); ++i)
        cpu->ram[0x18u + i] = rng.bytes[i];
    /* Execute the original menu-selection tail at C36A. It derives
     * $004A=1 (first AI slot) and $03F2 from the selected race mode. */
    cpu->ram[0x42u] = mode;
    prepare_cpu(cpu);
    if (cpu6502_call(cpu, 0xC36Au, 200000u) != 0 ||
        cpu->ram[0x4Au] != 0x01u || cpu->ram[0x3F2u] != mode) {
        printf("FAIL C875 vector=%u C36A first-AI-slot/mode setup\\n", vector);
        return 1;
    }

    cpu->ram[0x41u] = 0x02u; /* first-course C875 setup list */
    cpu->ram[0x43u] = 0x00u; /* first course: tbl_ED3A stream zero */
    cpu->ram[0x46u] = transition;
    cpu->ram[0x34u] = timer;
    cpu->ram[0x40u] = counter40;
    cpu->ram[0x47u] = counter47;
    cpu->ram[0x3F8u] = counter;
    cpu->ram[0xE8u] = 0x3Fu;
    cpu->ram[0xE9u] = 0x1Fu;
    cpu->ram[0xEAu] = 0x25u;
    cpu->ram[0xEBu] = 0x0Au;

    prepare_cpu(cpu);
    if (cpu6502_call(cpu, 0xC875u, 500000u) != 0) {
        printf("FAIL C875 vector=%u original ASM call\n", vector);
        return 1;
    }

    nativeRaceSetupClearFirstCourseState(&movement, &states);
    input.multiplayer_flag = mode;
    input.course_index_0043 = 0u;
    input.setup_index_0041 = 0x02u;
    input.transition_flag_0046 = transition;
    input.timer_0034 = timer;
    input.transition_counter_03F8 = counter;
    input.setup_counter_0040 = counter40;
    input.setup_counter_0047 = counter47;
    if (!nativeRaceSetupFirstCourse(&setup, &input, &rng, &movement, &states)) {
        printf("FAIL C875 vector=%u native rejected course zero\n", vector);
        return 1;
    }

    CHECK_EQ("0040", setup.setup_counter_0040, cpu->ram[0x40u], vector);
    CHECK_EQ("0047", setup.setup_counter_0047, cpu->ram[0x47u], vector);
    CHECK_EQ("0034", setup.timer_0034, cpu->ram[0x34u], vector);
    CHECK_EQ("03F1", movement.global_vertical_reference, cpu->ram[0x3F1u], vector);
    CHECK_EQ("00E8", setup.camera_scroll_00E8, cpu->ram[0xE8u], vector);
    CHECK_EQ("00E9", setup.camera_scroll_00E9, cpu->ram[0xE9u], vector);
    CHECK_EQ("00EA", setup.camera_scroll_00EA, cpu->ram[0xEAu], vector);
    CHECK_EQ("00EB", setup.camera_scroll_00EB, cpu->ram[0xEBu], vector);
    CHECK_EQ("03B4", setup.countdown_x_03B4, cpu->ram[0x3B4u], vector);
    CHECK_EQ("03B0", setup.race_setup_state_03B0, cpu->ram[0x3B0u], vector);
    CHECK_EQ("03BD", setup.countdown_limit_03BD, cpu->ram[0x3BDu], vector);
    CHECK_EQ("03BE", setup.countdown_index_03BE, cpu->ram[0x3BEu], vector);
    CHECK_EQ("player slot", setup.player_slot, 0u, vector);
    CHECK_EQ("first AI slot", setup.first_opponent_slot, cpu->ram[0x4Au], vector);
    CHECK_EQ("player rider flag", setup.player_rider_present, cpu->ram[0x370u], vector);

    for (i = 0u; i < 3u; ++i) {
        CHECK_EQ("C522 course params", setup.course_setup_parameters[i],
                 course_parameters[transition][i], vector);
        CHECK_EQ("C53E object params", setup.race_object_parameters[i],
                 cpu->ram[0x05u + i], vector);
    }
    CHECK_EQ("HUD first digit", setup.hud_glyphs[0], cpu->ram[0x3D1u], vector);
    CHECK_EQ("HUD second tens", setup.hud_glyphs[2], cpu->ram[0x3D3u], vector);
    CHECK_EQ("HUD second units", setup.hud_glyphs[3], cpu->ram[0x3D4u], vector);
    CHECK_EQ("HUD third tens", setup.hud_glyphs[5], cpu->ram[0x3D6u], vector);
    CHECK_EQ("HUD third units", setup.hud_glyphs[6], cpu->ram[0x3D7u], vector);

    for (i = 0u; i < 4u; ++i) {
        CHECK_EQ("bike Y", setup.bike[i].y, cpu->ram[0xB8u + i], vector);
        CHECK_EQ("bike X low", setup.bike[i].x_low, cpu->ram[0x80u + i], vector);
        CHECK_EQ("bike X high", setup.bike[i].x_high, cpu->ram[0x84u + i], vector);
        CHECK_EQ("bike class", setup.bike[i].object_class, cpu->ram[0xE0u + i], vector);
        CHECK_EQ("bike approach", setup.bike[i].approach, cpu->ram[0xACu + i], vector);
        CHECK_EQ("bike lock", setup.bike[i].lock_timer, cpu->ram[0x78u + i], vector);
        CHECK_EQ("bike active", setup.bike[i].active, cpu->ram[0xA8u + i], vector);
        CHECK_EQ("bike origin", setup.bike[i].object_origin_x, cpu->ram[0x64u + i], vector);
        CHECK_EQ("bike setup parameter", setup.bike[i].setup_parameter, cpu->ram[0x74u + i], vector);
        CHECK_EQ("native Y", movement.object_y[i], cpu->ram[0xB8u + i], vector);
        CHECK_EQ("native active", movement.object_active[i], cpu->ram[0xA8u + i], vector);
        CHECK_EQ("native rider flag", states.rider_present[i], cpu->ram[0x370u + i], vector);
        if (setup.bike[i].role !=
            (i == 0u ? NATIVE_RACE_BIKE_PLAYER : NATIVE_RACE_BIKE_OPPONENT)) {
            printf("FAIL C875 vector=%u role slot=%u role=%u\n",
                   vector, i, setup.bike[i].role);
            return 1;
        }
    }

    if (setup.player_slot != 0u || setup.first_opponent_slot != 1u ||
        states.rider_present[0] != 1u ||
        states.rider_present[1] != 0u || states.rider_present[2] != 0u ||
        states.rider_present[3] != 0u) {
        printf("FAIL C875 vector=%u player/opponent identity\n", vector);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *fp;
    unsigned vector;

    if (argc != 2) {
        puts("usage: test_native_race_setup_differential <prg>");
        return 2;
    }
    fp = fopen(argv[1], "rb");
    if (fp == 0 || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) {
        if (fp != 0) fclose(fp);
        return 2;
    }
    fclose(fp);
    cpu6502_reset_state(&cpu);

    for (vector = 0u; vector < 2000u; ++vector)
        if (run_case(&cpu, vector) != 0) return 1;

    puts("PASS: 2000 complete C875 first-course setup vectors match original ASM; player=0, opponents=1..3.");
    return 0;
}
