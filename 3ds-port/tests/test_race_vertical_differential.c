#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "race_vertical.h"
#include "cpu6502.h"

static uint32_t rng = 0xE96CE53Du;
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
        puts("usage: test_race_vertical_differential <original-16KiB-prg>");
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
        uint8_t expected_y[4], expected_delta[4];
        uint8_t expected_player_event, expected_movement_mode;
        int rc;

        memset(&movement, 0, sizeof(movement));
        memset(&states, 0, sizeof(states));
        memset(cpu.ram, 0, sizeof(cpu.ram));
        states.race_started = (uint8_t)(next_byte() & 1u);
        states.controller_hold = next_byte();
        states.player_event = next_byte();
        movement.collision_state[0] = (uint8_t)(next_byte() & 1u);
        movement.object_status[0] = (uint8_t)(next_byte() & 1u);
        movement.movement_mode[0] = (uint8_t)(next_byte() & 3u);
        states.global_overheat = (uint8_t)(next_byte() & 1u);
        cpu.ram[0x004Fu] = states.race_started;
        cpu.ram[0x0014u] = states.controller_hold;
        cpu.ram[0x00FDu] = states.player_event;
        cpu.ram[0x0098u] = movement.collision_state[0];
        cpu.ram[0x00B0u] = movement.object_status[0];
        cpu.ram[0x0388u] = movement.movement_mode[0];
        cpu.ram[0x03F7u] = states.global_overheat;
        for (i = 0u; i < 4u; ++i) {
            movement.vertical_phase[i] = 0u; /* first-course phase-zero path */
            movement.object_action_state[i] = next_byte();
            movement.object_y[i] = next_byte();
            states.vertical_delta[i] = next_byte();
            states.state[i] = (uint8_t)(next_byte() & 7u);
            states.overheat[i] = (uint8_t)(next_byte() & 1u);

            cpu.ram[0x0058u + i] = movement.object_action_state[i];
            cpu.ram[0x00A4u + i] = 0u;
            cpu.ram[0x00B8u + i] = movement.object_y[i];
            cpu.ram[0x00DCu + i] = states.vertical_delta[i];
            cpu.ram[0x009Cu + i] = states.state[i];
            cpu.ram[0x03E0u + i] = states.overheat[i];
        }

        prepare_cpu(&cpu);
        rc = cpu6502_call(&cpu, 0xE96Cu, 512u);
        if (rc != 0) {
            printf("FAIL E96C oracle rc=%d vector=%u opcode=%02X\n",
                   rc, vector, cpu.bad_opcode);
            return 1;
        }
        for (i = 0u; i < 4u; ++i) {
            expected_y[i] = cpu.ram[0x00B8u + i];
            expected_delta[i] = cpu.ram[0x00DCu + i];
        }
        expected_player_event = cpu.ram[0x00FDu];
        expected_movement_mode = cpu.ram[0x0388u];

        if (bikePhysicsUpdateFirstCourseVertical(&movement, &states) != 0u) {
            printf("FAIL native E96C rejected phase-zero vector=%u\n", vector);
            return 1;
        }
        if (states.player_event != expected_player_event ||
            movement.movement_mode[0] != expected_movement_mode) {
            printf("FAIL EA07 vector=%u event=%02X/%02X movement_mode=%02X/%02X\n",
                   vector, states.player_event, expected_player_event,
                   movement.movement_mode[0], expected_movement_mode);
            return 1;
        }
        for (i = 0u; i < 4u; ++i) {
            if (movement.object_y[i] != expected_y[i] ||
                states.vertical_delta[i] != expected_delta[i]) {
                printf("FAIL E96C vector=%u slot=%u Y=%02X/%02X delta=%02X/%02X "
                       "action=%02X state=%02X overheat=%02X\n",
                       vector, i, movement.object_y[i], expected_y[i],
                       states.vertical_delta[i], expected_delta[i],
                       movement.object_action_state[i], states.state[i],
                       states.overheat[i]);
                return 1;
            }
        }
    }

    puts("PASS: 50000 four-bike E96C phase-zero + EA07 input frames match original ASM.");
    return 0;
}
