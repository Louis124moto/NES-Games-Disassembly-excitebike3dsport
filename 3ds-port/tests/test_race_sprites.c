#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "race_sprites.h"
#include "cpu6502.h"

static uint32_t rng = 0xE17FE3F4u;
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

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *file;
    size_t size;
    unsigned vector, i;

    if (argc != 2) {
        puts("usage: test_race_sprites <original-16KiB-prg>");
        return 2;
    }
    file = fopen(argv[1], "rb");
    if (file == NULL) return 2;
    size = fread(cpu.prg, 1u, sizeof(cpu.prg), file);
    if (fclose(file) != 0 || size != sizeof(cpu.prg)) return 2;
    cpu6502_reset_state(&cpu);

    for (vector = 0u; vector < 50000u; ++vector) {
        BikeMovementState movement;
        BikeStateMachineState states;
        NativeOamState native_oam;
        uint8_t expected_oam[256];
        int rc;

        memset(&movement, 0, sizeof(movement));
        memset(&states, 0, sizeof(states));
        memset(cpu.ram, 0, sizeof(cpu.ram));
        memset(cpu.ram + 0x0200u, 0xF8, sizeof(expected_oam));

        movement.track_finished = 0u;
        cpu.ram[0x0052u] = 0u;
        for (i = 0u; i < 4u; ++i) {
            movement.object_active[i] = (uint8_t)(next_byte() & 1u);
            movement.object_y[i] = next_byte();
            movement.x_low[i] = next_byte();
            movement.y_low[i] = next_byte();
            movement.approach[i] = 0x06u;
            movement.object_action_state[i] = 0u;
            movement.speed_fraction[i] = 0u;
            movement.speed_whole[i] = 0u;
            movement.object_status[i] = 0u;
            movement.collision_state[i] = 0u;
            states.state[i] = 0u;

            cpu.ram[0x00A8u+i] = movement.object_active[i];
            cpu.ram[0x00B8u+i] = movement.object_y[i];
            cpu.ram[0x0080u+i] = movement.x_low[i];
            cpu.ram[0x008Cu+i] = movement.y_low[i];
            cpu.ram[0x00ACu+i] = movement.approach[i];
            cpu.ram[0x0058u+i] = movement.object_action_state[i];
            cpu.ram[0x0090u+i] = movement.speed_fraction[i];
            cpu.ram[0x0094u+i] = movement.speed_whole[i];
            cpu.ram[0x00B0u+i] = movement.object_status[i];
            cpu.ram[0x0098u+i] = movement.collision_state[i];
            cpu.ram[0x009Cu+i] = states.state[i];
        }

        states.rider_present[3] = (uint8_t)(next_byte() & 1u);
        states.vertical_collision_ref[3] = next_byte();
        cpu.ram[0x0373u] = states.rider_present[3];
        cpu.ram[0x006Fu] = states.vertical_collision_ref[3];

        prepare_cpu(&cpu);
        rc = cpu6502_call(&cpu, 0xE17Fu, 2048u);
        if (rc != 0) {
            printf("FAIL E17F oracle rc=%d vector=%u opcode=%02X\n",
                   rc, vector, cpu.bad_opcode);
            return 1;
        }
        memcpy(expected_oam, cpu.ram + 0x0200u, sizeof(expected_oam));

        if (nativeRaceBuildInitialBikeOam(&movement, &states, &native_oam) != 0u) {
            printf("FAIL native OAM rejected initial pose vector=%u\n", vector);
            return 1;
        }
        if (memcmp(native_oam.bytes, expected_oam, sizeof(expected_oam)) != 0) {
            for (i = 0u; i < sizeof(expected_oam); ++i) {
                if (native_oam.bytes[i] != expected_oam[i]) {
                    printf("FAIL E17F vector=%u OAM[%02X] native=%02X asm=%02X\n",
                           vector, i, native_oam.bytes[i], expected_oam[i]);
                    break;
                }
            }
            return 1;
        }
    }

    puts("PASS: 50000 E17F/E3F4/E1A3/E1F9/D1C7 initial-bike OAM frames match original ASM.");
    return 0;
}
