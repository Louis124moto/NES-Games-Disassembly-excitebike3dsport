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

        movement.track_finished = (uint8_t)(next_byte() & 1u);
        cpu.ram[0x0052u] = movement.track_finished;
        states.frame_counter = next_byte();
        cpu.ram[0x003Fu] = states.frame_counter;

        for (i = 0u; i < 4u; ++i) {
            static const uint8_t state_values[4] = {0u, 1u, 5u, 6u};
            static const uint8_t action_values[3] = {0u, 0x0Du, 1u};
            static const uint8_t delta_values[3] = {0u, 1u, 0xFFu};

            movement.object_active[i] = (uint8_t)(next_byte() & 1u);
            movement.object_y[i] = next_byte();
            movement.x_low[i] = next_byte();
            movement.x_high[i] = (uint8_t)(next_byte() & 3u);
            movement.previous_x[i] = next_byte();
            movement.y_low[i] = next_byte();
            movement.approach[i] = (uint8_t)(next_byte() % 0x17u);
            movement.object_action_state[i] = action_values[next_byte() % 3u];
            movement.speed_fraction[i] = next_byte();
            movement.speed_whole[i] = next_byte();
            movement.object_status[i] = (uint8_t)(next_byte() & 1u);
            movement.collision_state[i] = (uint8_t)(next_byte() & 1u);
            movement.vertical_phase[i] = (uint8_t)(next_byte() & 1u);
            movement.vertical_reference[i] = next_byte();

            states.state[i] = state_values[next_byte() & 3u];
            states.rider_present[i] = (uint8_t)(next_byte() & 1u);
            states.vertical_delta[i] = delta_values[next_byte() % 3u];
            states.timer_recovery[i] = (uint8_t)(next_byte() % 7u);
            states.vertical_collision_ref[i] = next_byte();
            states.vertical_state[i] = (uint8_t)(next_byte() % 3u);

            cpu.ram[0x00A8u+i] = movement.object_active[i];
            cpu.ram[0x00B8u+i] = movement.object_y[i];
            cpu.ram[0x0080u+i] = movement.x_low[i];
            cpu.ram[0x0084u+i] = movement.x_high[i];
            cpu.ram[0x0390u+i] = movement.previous_x[i];
            cpu.ram[0x008Cu+i] = movement.y_low[i];
            cpu.ram[0x00ACu+i] = movement.approach[i];
            cpu.ram[0x0058u+i] = movement.object_action_state[i];
            cpu.ram[0x0090u+i] = movement.speed_fraction[i];
            cpu.ram[0x0094u+i] = movement.speed_whole[i];
            cpu.ram[0x00B0u+i] = movement.object_status[i];
            cpu.ram[0x0098u+i] = movement.collision_state[i];
            cpu.ram[0x009Cu+i] = states.state[i];
            cpu.ram[0x00A4u+i] = movement.vertical_phase[i];
            cpu.ram[0x00BCu+i] = movement.vertical_reference[i];
            cpu.ram[0x00DCu+i] = states.vertical_delta[i];
            cpu.ram[0x0036u+i] = states.timer_recovery[i];
            cpu.ram[0x0370u+i] = states.rider_present[i];
            cpu.ram[0x0398u+i] = states.vertical_state[i];
            cpu.ram[0x006Cu+i] = states.vertical_collision_ref[i];
        }

        prepare_cpu(&cpu);
        rc = cpu6502_call(&cpu, 0xE17Fu, 4096u);
        if (rc != 0) {
            printf("FAIL E17F oracle rc=%d vector=%u opcode=%02X\n",
                   rc, vector, cpu.bad_opcode);
            return 1;
        }
        memcpy(expected_oam, cpu.ram + 0x0200u, sizeof(expected_oam));

        if (nativeRaceBuildInitialBikeOam(&movement, &states, &native_oam) != 0u) {
            printf("FAIL native E17F rejected supported pose vector=%u\n", vector);
            return 1;
        }
        if (memcmp(native_oam.bytes, expected_oam, sizeof(expected_oam)) != 0) {
            for (i = 0u; i < sizeof(expected_oam); ++i) {
                if (native_oam.bytes[i] != expected_oam[i]) {
                    printf("FAIL E17F vector=%u OAM[%02X] native=%02X asm=%02X\n",
                           vector, i, native_oam.bytes[i], expected_oam[i]);
                    printf("track=%u frame=%02X rider3=%u ref3=%02X\n",
                           movement.track_finished, states.frame_counter,
                           states.rider_present[3], states.vertical_collision_ref[3]);
                    for (unsigned int b = 0u; b < 4u; ++b) {
                        printf("bike%u active=%u state=%02X approach=%02X action=%02X "
                               "speed=%02X:%02X delta=%02X timer=%02X rider=%u "
                               "vref=%02X vstate=%u x=%02X/%02X prev=%02X phase=%u\n",
                               b, movement.object_active[b], states.state[b],
                               movement.approach[b], movement.object_action_state[b],
                               movement.speed_whole[b], movement.speed_fraction[b],
                               states.vertical_delta[b], states.timer_recovery[b],
                               states.rider_present[b], movement.vertical_reference[b],
                               states.vertical_state[b], movement.x_high[b],
                               movement.x_low[b], movement.previous_x[b],
                               movement.vertical_phase[b]);
                    }
                    break;
                }
            }
            return 1;
        }
    }

    puts("PASS: 50000 dynamic E17F/E3F4/E1A3/E1F9/D1C7/E26A OAM frames match original ASM.");
    return 0;
}
