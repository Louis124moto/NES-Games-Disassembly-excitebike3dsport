#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bike_physics.h"
#include "cpu6502.h"

static uint32_t rng = 0xDC1ADCF2u;
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
static void map_object_to_cpu(Cpu6502 *cpu, const BikeMovementState *s, unsigned i)
{
    cpu->ram[0x0090u+i] = s->speed_fraction[i];
    cpu->ram[0x0094u+i] = s->speed_whole[i];
    cpu->ram[0x00A8u+i] = s->object_active[i];
    cpu->ram[0x00ACu+i] = s->approach[i];
    cpu->ram[0x00B0u+i] = s->object_status[i];
    cpu->ram[0x00B8u+i] = s->object_y[i];
    cpu->ram[0x00BCu+i] = s->vertical_reference[i];
    cpu->ram[0x0058u+i] = 0u;
    cpu->ram[0x005Cu+i] = s->movement_flags[i];
    cpu->ram[0x0060u+i] = s->movement_delta[i];
    cpu->ram[0x0080u+i] = s->x_low[i];
    cpu->ram[0x0084u+i] = s->x_high[i];
    cpu->ram[0x008Cu+i] = s->y_low[i];
    cpu->ram[0x0098u+i] = s->collision_state[i];
    cpu->ram[0x00CCu+i] = s->lateral_state[i];
    cpu->ram[0x00D4u+i] = s->brake_state[i];
    cpu->ram[0x0364u+i] = s->rider_state[i];
    cpu->ram[0x0368u+i] = s->animation_state[i];
    cpu->ram[0x0374u+i] = s->object_lock_timer[i];
    cpu->ram[0x0378u+i] = s->motion_fraction_start[i];
    cpu->ram[0x037Cu+i] = s->motion_whole_start[i];
    cpu->ram[0x0380u+i] = s->motion_fraction[i];
    cpu->ram[0x0384u+i] = s->motion_whole[i];
    cpu->ram[0x0388u+i] = s->movement_mode[i];
    cpu->ram[0x038Cu+i] = s->motion_delta[i];
}
static void map_cpu_to_state(BikeMovementState *s, const Cpu6502 *cpu, unsigned i)
{
    s->speed_fraction[i] = cpu->ram[0x0090u+i];
    s->speed_whole[i] = cpu->ram[0x0094u+i];
    s->approach[i] = cpu->ram[0x00ACu+i];
    s->object_status[i] = cpu->ram[0x00B0u+i];
    s->y_low[i] = cpu->ram[0x008Cu+i];
    s->collision_state[i] = cpu->ram[0x0098u+i];
    s->lateral_state[i] = cpu->ram[0x00CCu+i];
    s->brake_state[i] = cpu->ram[0x00D4u+i];
    s->rider_state[i] = cpu->ram[0x0364u+i];
    s->animation_state[i] = cpu->ram[0x0368u+i];
    s->object_lock_timer[i] = cpu->ram[0x0374u+i];
    s->motion_fraction_start[i] = cpu->ram[0x0378u+i];
    s->motion_whole_start[i] = cpu->ram[0x037Cu+i];
    s->motion_fraction[i] = cpu->ram[0x0380u+i];
    s->motion_whole[i] = cpu->ram[0x0384u+i];
    s->movement_mode[i] = cpu->ram[0x0388u+i];
    s->motion_delta[i] = cpu->ram[0x038Cu+i];
}
int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *f;
    size_t n;
    unsigned v, i;
    if (argc != 2) {
        puts("usage: test_bike_vertical <original-16KiB-prg>");
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
        unsigned slot = v & 3u;
        int rc;
        for (i = 0; i < 4u; ++i) {
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
            native.speed_whole[i] = next_byte();
            native.speed_fraction[i] = next_byte();
            native.movement_flags[i] = next_byte();
            native.motion_fraction[i] = next_byte();
            native.motion_whole[i] = next_byte();
            native.motion_fraction_start[i] = next_byte();
            native.motion_whole_start[i] = next_byte();
            native.motion_delta[i] = next_byte();
        }
        native.global_vertical_reference = next_byte();
        native.track_finished = (uint8_t)(next_byte() & 1u);
        native.frame_phase = (uint8_t)(next_byte() & 3u);
        native.player_event = next_byte();

        memset(cpu.ram, 0, sizeof(cpu.ram));
        for (i = 0; i < 4u; ++i) map_object_to_cpu(&cpu, &native, i);
        cpu.ram[0x03F1u] = native.global_vertical_reference;
        cpu.ram[0x0052u] = native.track_finished;
        cpu.ram[0x004Cu] = native.frame_phase;
        cpu.ram[0x00FFu] = native.player_event;
        prepare_cpu(&cpu);
        cpu.x = (uint8_t)slot;
        rc = cpu6502_call(&cpu, 0xDC1Au, 256u);
        if (rc != 0) {
            printf("FAIL DC1A oracle rc=%d vector=%u slot=%u opcode=%02X\n", rc, v, slot, cpu.bad_opcode);
            return 1;
        }
        expected = native;
        map_cpu_to_state(&expected, &cpu, slot);
        expected.player_event = cpu.ram[0x00FFu];
        bikePhysicsUpdateVerticalRecovery(&native, (uint8_t)slot);
        if (memcmp(native.object_status, expected.object_status, sizeof(native.object_status)) ||
            memcmp(native.rider_state, expected.rider_state, sizeof(native.rider_state)) ||
            memcmp(native.collision_state, expected.collision_state, sizeof(native.collision_state)) ||
            memcmp(native.movement_mode, expected.movement_mode, sizeof(native.movement_mode)) ||
            memcmp(native.approach, expected.approach, sizeof(native.approach)) ||
            memcmp(native.object_lock_timer, expected.object_lock_timer, sizeof(native.object_lock_timer)) ||
            native.y_low[slot] != expected.y_low[slot] || native.player_event != expected.player_event) {
            printf("FAIL DC1A v=%u x=%u y=%02X/%02X B0=%u rider=%u collision=%u mode=%u brake=%u approach=%u anim=%u lateral=%u lock=%u speed=%u global=%02X BC=%02X B8=%02X delta=%02X temp=%02X cpu8F=%02X cpuCF=%02X cpuD7=%02X cpuEF=%02X A=%02X X=%02X P=%02X PC=%04X\\n",
                   v, slot, native.y_low[slot], expected.y_low[slot],
                   native.object_status[slot], native.rider_state[slot],
                   native.collision_state[slot], native.movement_mode[slot],
                   native.brake_state[slot], native.approach[slot],
                   native.animation_state[slot], native.lateral_state[slot],
                   native.object_lock_timer[slot], native.speed_whole[slot],
                   native.global_vertical_reference, native.vertical_reference[slot],
                   native.object_y[slot],
                   (uint8_t)(native.global_vertical_reference - native.vertical_reference[slot] - native.object_y[slot]),
                   cpu.ram[1], cpu.ram[0x008Fu], cpu.ram[0x00CFu],
                   cpu.ram[0x00D7u], cpu.ram[0x00EFu], cpu.a, cpu.x, cpu.p, cpu.pc);
            return 1;
        }
    }

    for (v = 0; v < 50000u; ++v) {
        BikeMovementState native = {0};
        BikeMovementState expected;
        unsigned slot = v & 3u;
        int rc;
        for (i = 0; i < 4u; ++i) {
            native.object_status[i] = (uint8_t)(next_byte() & 3u);
            native.movement_flags[i] = next_byte();
            native.frame_phase = (uint8_t)(next_byte() & 3u);
            native.speed_fraction[i] = next_byte();
            native.speed_whole[i] = next_byte();
            native.y_low[i] = next_byte();
            native.movement_mode[i] = (uint8_t)(next_byte() % 4u);
            native.lateral_state[i] = next_byte();
            native.motion_fraction[i] = next_byte();
            native.motion_whole[i] = next_byte();
            native.motion_fraction_start[i] = next_byte();
            native.motion_whole_start[i] = next_byte();
            native.motion_delta[i] = next_byte();
            native.rider_state[i] = next_byte();
        }
        native.player_event = next_byte();
        memset(cpu.ram, 0, sizeof(cpu.ram));
        for (i = 0; i < 4u; ++i) map_object_to_cpu(&cpu, &native, i);
        cpu.ram[0x004Cu] = native.frame_phase;
        cpu.ram[0x00FFu] = native.player_event;
        prepare_cpu(&cpu);
        cpu.x = (uint8_t)slot;
        rc = cpu6502_call(&cpu, 0xDCF2u, 256u);
        if (rc != 0) {
            printf("FAIL DCF2 oracle rc=%d vector=%u slot=%u opcode=%02X\n", rc, v, slot, cpu.bad_opcode);
            return 1;
        }
        expected = native;
        map_cpu_to_state(&expected, &cpu, slot);
        expected.player_event = cpu.ram[0x00FFu];
        bikePhysicsUpdateVerticalMotion(&native, (uint8_t)slot);
        if (native.object_status[slot] != expected.object_status[slot] ||
            native.motion_fraction[slot] != expected.motion_fraction[slot] ||
            native.motion_whole[slot] != expected.motion_whole[slot] ||
            native.motion_fraction_start[slot] != expected.motion_fraction_start[slot] ||
            native.motion_whole_start[slot] != expected.motion_whole_start[slot] ||
            native.motion_delta[slot] != expected.motion_delta[slot] ||
            native.y_low[slot] != expected.y_low[slot] ||
            native.rider_state[slot] != expected.rider_state[slot] ||
            native.player_event != expected.player_event) {
            printf("FAIL DCF2 vector=%u slot=%u y=%02X/%02X frac=%02X/%02X whole=%02X/%02X status=%02X/%02X\n",
                   v, slot, native.y_low[slot], expected.y_low[slot],
                   native.motion_fraction[slot], expected.motion_fraction[slot],
                   native.motion_whole[slot], expected.motion_whole[slot],
                   native.object_status[slot], expected.object_status[slot]);
            return 1;
        }
    }
    puts("PASS: 50000 DC1A + 50000 DCF2 randomized vectors match original ASM.");
    return 0;
}
