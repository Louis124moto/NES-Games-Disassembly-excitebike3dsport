#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bike_physics.h"
#include "cpu6502.h"

static uint32_t rng = 0xDA26A11Eu;
static uint8_t next_byte(void)
{
    rng = rng * 1664525u + 1013904223u;
    return (uint8_t)(rng >> 24);
}
static void prepare_cpu(Cpu6502 *cpu)
{
    cpu->a = cpu->x = cpu->y = 0u;
    cpu->sp = 0xFDu;
    cpu->p = 0x24u;
    cpu->cycles = 0u;
    cpu->bad_opcode = -1;
}
static void state_to_cpu(Cpu6502 *cpu, const BikeMovementState *s)
{
    unsigned i;
    for (i = 0; i < 4u; ++i) {
        cpu->ram[0x0090u+i] = s->speed_fraction[i];
        cpu->ram[0x0094u+i] = s->speed_whole[i];
        cpu->ram[0x0394u+i] = s->fraction_accumulator[i];
        cpu->ram[0x0060u+i] = s->movement_delta[i];
        cpu->ram[0x00A8u+i] = s->object_active[i];
        cpu->ram[0x0080u+i] = s->x_low[i];
        cpu->ram[0x0084u+i] = s->x_high[i];
        cpu->ram[0x0390u+i] = s->previous_x[i];
        cpu->ram[0x00ACu+i] = s->approach[i];
        cpu->ram[0x00B0u+i] = s->object_status[i];
        cpu->ram[0x00B8u+i] = s->object_y[i];
        cpu->ram[0x00BCu+i] = s->vertical_reference[i];
        cpu->ram[0x008Cu+i] = s->y_low[i];
        cpu->ram[0x0364u+i] = s->rider_state[i];
        cpu->ram[0x0098u+i] = s->collision_state[i];
        cpu->ram[0x0388u+i] = s->movement_mode[i];
        cpu->ram[0x00D4u+i] = s->brake_state[i];
        cpu->ram[0x0368u+i] = s->animation_state[i];
        cpu->ram[0x00CCu+i] = s->lateral_state[i];
        cpu->ram[0x0374u+i] = s->object_lock_timer[i];
        cpu->ram[0x005Cu+i] = s->movement_flags[i];
        cpu->ram[0x0380u+i] = s->motion_fraction[i];
        cpu->ram[0x0384u+i] = s->motion_whole[i];
        cpu->ram[0x0378u+i] = s->motion_fraction_start[i];
        cpu->ram[0x037Cu+i] = s->motion_whole_start[i];
        cpu->ram[0x038Cu+i] = s->motion_delta[i];
    }
    cpu->ram[0x0012u] = s->player_scroll_x;
    cpu->ram[0x004Du] = s->base_nametable;
    cpu->ram[0x03BFu] = s->secondary_scroll_fraction;
    cpu->ram[0x0050u] = s->secondary_scroll_x;
    cpu->ram[0x004Eu] = s->secondary_base_nametable;
    cpu->ram[0x0052u] = s->track_finished;
    cpu->ram[0x03F1u] = s->global_vertical_reference;
    cpu->ram[0x004Cu] = s->frame_phase;
    cpu->ram[0x00FFu] = s->player_event;
    cpu->ram[0x004Fu] = 1u;
    cpu->ram[0x003Cu] = 1u; /* Keep DA26 on its movement-only timer path. */
}
static void cpu_to_state(BikeMovementState *s, const Cpu6502 *cpu)
{
    unsigned i;
    for (i = 0; i < 4u; ++i) {
        s->speed_fraction[i] = cpu->ram[0x0090u+i];
        s->speed_whole[i] = cpu->ram[0x0094u+i];
        s->fraction_accumulator[i] = cpu->ram[0x0394u+i];
        s->movement_delta[i] = cpu->ram[0x0060u+i];
        s->object_active[i] = cpu->ram[0x00A8u+i];
        s->x_low[i] = cpu->ram[0x0080u+i];
        s->x_high[i] = cpu->ram[0x0084u+i];
        s->previous_x[i] = cpu->ram[0x0390u+i];
        s->approach[i] = cpu->ram[0x00ACu+i];
        s->object_status[i] = cpu->ram[0x00B0u+i];
        s->y_low[i] = cpu->ram[0x008Cu+i];
        s->rider_state[i] = cpu->ram[0x0364u+i];
        s->collision_state[i] = cpu->ram[0x0098u+i];
        s->movement_mode[i] = cpu->ram[0x0388u+i];
        s->brake_state[i] = cpu->ram[0x00D4u+i];
        s->animation_state[i] = cpu->ram[0x0368u+i];
        s->lateral_state[i] = cpu->ram[0x00CCu+i];
        s->object_lock_timer[i] = cpu->ram[0x0374u+i];
        s->motion_fraction[i] = cpu->ram[0x0380u+i];
        s->motion_whole[i] = cpu->ram[0x0384u+i];
        s->motion_fraction_start[i] = cpu->ram[0x0378u+i];
        s->motion_whole_start[i] = cpu->ram[0x037Cu+i];
        s->motion_delta[i] = cpu->ram[0x038Cu+i];
    }
    s->player_scroll_x = cpu->ram[0x0012u];
    s->base_nametable = cpu->ram[0x004Du];
    s->secondary_scroll_fraction = cpu->ram[0x03BFu];
    s->secondary_scroll_x = cpu->ram[0x0050u];
    s->secondary_base_nametable = cpu->ram[0x004Eu];
    s->player_event = cpu->ram[0x00FFu];
}
#define CHECK_ARRAY(field) \
    if (memcmp(native.field, expected.field, sizeof(native.field)) != 0) { \
        printf("FAIL DA26 vector=%u field=%s\\n", v, #field); return 1; \
    }
int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *f;
    size_t n;
    unsigned v, i;
    if (argc != 2) {
        puts("usage: test_bike_race_movement <original-16KiB-prg>");
        return 2;
    }
    f = fopen(argv[1], "rb");
    if (!f) return 2;
    n = fread(cpu.prg, 1u, sizeof(cpu.prg), f);
    if (fclose(f) || n != sizeof(cpu.prg)) return 2;
    cpu6502_reset_state(&cpu);

    for (v = 0; v < 50000u; ++v) {
        BikeMovementState native = {0};
        BikeMovementState expected;
        int rc;
        for (i = 0; i < 4u; ++i) {
            native.speed_whole[i] = next_byte();
            native.speed_fraction[i] = next_byte();
            native.fraction_accumulator[i] = next_byte();
            native.movement_delta[i] = next_byte();
            native.object_active[i] = (uint8_t)(next_byte() & 1u);
            native.x_low[i] = next_byte();
            native.x_high[i] = next_byte();
            native.previous_x[i] = next_byte();
            native.object_status[i] = (uint8_t)(next_byte() & 3u);
            native.vertical_reference[i] = next_byte();
            native.object_y[i] = next_byte();
            native.y_low[i] = next_byte();
            native.rider_state[i] = (uint8_t)(next_byte() & 3u);
            native.collision_state[i] = (uint8_t)(next_byte() & 1u);
            native.movement_mode[i] = (uint8_t)(next_byte() % 3u);
            native.brake_state[i] = (uint8_t)(next_byte() % 7u);
            native.approach[i] = next_byte();
            native.animation_state[i] = next_byte();
            native.lateral_state[i] = next_byte();
            native.object_lock_timer[i] = next_byte();
            native.movement_flags[i] = next_byte();
            native.motion_fraction[i] = next_byte();
            native.motion_whole[i] = next_byte();
            native.motion_fraction_start[i] = next_byte();
            native.motion_whole_start[i] = next_byte();
            native.motion_delta[i] = next_byte();
        }
        native.player_scroll_x = next_byte();
        native.base_nametable = (uint8_t)(next_byte() & 1u);
        native.secondary_scroll_fraction = next_byte();
        native.secondary_scroll_x = next_byte();
        native.secondary_base_nametable = (uint8_t)(next_byte() & 1u);
        native.track_finished = (uint8_t)(next_byte() & 1u);
        native.global_vertical_reference = next_byte();
        native.frame_phase = (uint8_t)(next_byte() & 3u);
        native.player_event = next_byte();

        memset(cpu.ram, 0, sizeof(cpu.ram));
        state_to_cpu(&cpu, &native);
        prepare_cpu(&cpu);
        rc = cpu6502_call(&cpu, 0xDA26u, 2048u);
        if (rc != 0) {
            printf("FAIL DA26 oracle rc=%d vector=%u opcode=%02X\\n", rc, v, cpu.bad_opcode);
            return 1;
        }
        expected = native;
        cpu_to_state(&expected, &cpu);
        bikePhysicsUpdateRaceMovementPass(&native, 1u);

        CHECK_ARRAY(fraction_accumulator);
        CHECK_ARRAY(movement_delta);
        CHECK_ARRAY(object_status);
        CHECK_ARRAY(y_low);
        CHECK_ARRAY(rider_state);
        CHECK_ARRAY(collision_state);
        CHECK_ARRAY(movement_mode);
        CHECK_ARRAY(approach);
        CHECK_ARRAY(object_lock_timer);
        CHECK_ARRAY(motion_fraction);
        CHECK_ARRAY(motion_whole);
        CHECK_ARRAY(motion_fraction_start);
        CHECK_ARRAY(motion_whole_start);
        CHECK_ARRAY(motion_delta);
        CHECK_ARRAY(x_low);
        CHECK_ARRAY(x_high);
        CHECK_ARRAY(previous_x);
        if (native.player_scroll_x != expected.player_scroll_x ||
            native.base_nametable != expected.base_nametable ||
            native.secondary_scroll_fraction != expected.secondary_scroll_fraction ||
            native.secondary_scroll_x != expected.secondary_scroll_x ||
            native.secondary_base_nametable != expected.secondary_base_nametable ||
            native.player_event != expected.player_event) {
            printf("FAIL DA26 vector=%u camera=%02X/%02X secondary=%02X/%02X/%02X vs %02X/%02X/%02X\\n",
                   v, native.player_scroll_x, expected.player_scroll_x,
                   native.secondary_scroll_fraction, native.secondary_scroll_x,
                   native.secondary_base_nametable, expected.secondary_scroll_fraction,
                   expected.secondary_scroll_x, expected.secondary_base_nametable);
            return 1;
        }
    }
    puts("PASS: 50000 complete DA26 four-bike movement/camera frames match original ASM.");
    return 0;
}
