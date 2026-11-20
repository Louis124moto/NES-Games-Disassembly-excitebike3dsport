#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bike_physics.h"
#include "cpu6502.h"

static uint32_t rng = 0xDD06DD38u;
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

static void state_to_cpu(Cpu6502 *cpu, const BikeMovementState *s, unsigned i)
{
    cpu->ram[0x0090u+i] = s->speed_fraction[i];
    cpu->ram[0x0094u+i] = s->speed_whole[i];
    cpu->ram[0x005Cu+i] = s->movement_flags[i];
    cpu->ram[0x008Cu+i] = s->y_low[i];
    cpu->ram[0x00B0u+i] = s->object_status[i];
    cpu->ram[0x00CCu+i] = s->lateral_state[i];
    cpu->ram[0x0364u+i] = s->rider_state[i];
    cpu->ram[0x0378u+i] = s->motion_fraction_start[i];
    cpu->ram[0x037Cu+i] = s->motion_whole_start[i];
    cpu->ram[0x0380u+i] = s->motion_fraction[i];
    cpu->ram[0x0384u+i] = s->motion_whole[i];
    cpu->ram[0x0388u+i] = s->movement_mode[i];
    cpu->ram[0x038Cu+i] = s->motion_delta[i];
    cpu->ram[0x00FFu] = s->player_event;
}

static void cpu_to_state(BikeMovementState *s, const Cpu6502 *cpu, unsigned i)
{
    s->object_status[i] = cpu->ram[0x00B0u+i];
    s->y_low[i] = cpu->ram[0x008Cu+i];
    s->rider_state[i] = cpu->ram[0x0364u+i];
    s->motion_fraction_start[i] = cpu->ram[0x0378u+i];
    s->motion_whole_start[i] = cpu->ram[0x037Cu+i];
    s->motion_fraction[i] = cpu->ram[0x0380u+i];
    s->motion_whole[i] = cpu->ram[0x0384u+i];
    s->movement_mode[i] = cpu->ram[0x0388u+i];
    s->motion_delta[i] = cpu->ram[0x038Cu+i];
    s->player_event = cpu->ram[0x00FFu];
}

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *file;
    size_t size;
    unsigned vector, i;

    if (argc != 2) {
        puts("usage: test_bike_vertical_init <original-16KiB-prg>");
        return 2;
    }
    file = fopen(argv[1], "rb");
    if (file == NULL) return 2;
    size = fread(cpu.prg, 1u, sizeof(cpu.prg), file);
    if (fclose(file) != 0 || size != sizeof(cpu.prg)) return 2;
    cpu6502_reset_state(&cpu);

    for (vector = 0u; vector < 50000u; ++vector) {
        BikeMovementState native;
        BikeMovementState expected;
        unsigned slot = vector & 3u;
        int rc;

        memset(&native, 0, sizeof(native));
        memset(cpu.ram, 0, sizeof(cpu.ram));
        for (i = 0u; i < 4u; ++i) {
            native.speed_fraction[i] = next_byte();
            native.speed_whole[i] = next_byte();
            native.movement_flags[i] = next_byte();
            native.y_low[i] = next_byte();
            native.object_status[i] = next_byte();
            native.lateral_state[i] = next_byte();
            native.rider_state[i] = next_byte();
            native.motion_fraction_start[i] = next_byte();
            native.motion_whole_start[i] = next_byte();
            native.motion_fraction[i] = next_byte();
            native.motion_whole[i] = next_byte();
            native.movement_mode[i] = next_byte();
            native.motion_delta[i] = next_byte();
            state_to_cpu(&cpu, &native, i);
        }
        native.player_event = next_byte();
        cpu.ram[0x00FFu] = native.player_event;

        prepare_cpu(&cpu);
        cpu.x = (uint8_t)slot;
        rc = cpu6502_call(&cpu, 0xDD06u, 512u);
        if (rc != 0) {
            printf("FAIL DD06 oracle rc=%d vector=%u slot=%u opcode=%02X\n",
                   rc, vector, slot, cpu.bad_opcode);
            return 1;
        }

        expected = native;
        cpu_to_state(&expected, &cpu, slot);
        bikePhysicsInitializeVerticalJump(&native, (uint8_t)slot);

        if (native.object_status[slot] != expected.object_status[slot] ||
            native.motion_fraction[slot] != expected.motion_fraction[slot] ||
            native.motion_whole[slot] != expected.motion_whole[slot] ||
            native.motion_fraction_start[slot] != expected.motion_fraction_start[slot] ||
            native.motion_whole_start[slot] != expected.motion_whole_start[slot] ||
            native.motion_delta[slot] != expected.motion_delta[slot] ||
            native.y_low[slot] != expected.y_low[slot] ||
            native.rider_state[slot] != expected.rider_state[slot] ||
            native.player_event != expected.player_event) {
            printf("FAIL DD06 vector=%u slot=%u Y=%02X/%02X frac=%02X/%02X "
                   "whole=%02X/%02X status=%02X/%02X event=%02X/%02X\n",
                   vector, slot, native.y_low[slot], expected.y_low[slot],
                   native.motion_fraction[slot], expected.motion_fraction[slot],
                   native.motion_whole[slot], expected.motion_whole[slot],
                   native.object_status[slot], expected.object_status[slot],
                   native.player_event, expected.player_event);
            return 1;
        }
    }

    puts("PASS: 50000 DD06/DD38/DD1A/DD6F jump initializations match original ASM.");
    return 0;
}
