#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "race_vertical.h"
#include "cpu6502.h"

static uint32_t rng = 0xE836DC97u;
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
        puts("usage: test_race_screen_projection <original-16KiB-prg>");
        return 2;
    }
    file = fopen(argv[1], "rb");
    if (file == NULL) return 2;
    size = fread(cpu.prg, 1u, sizeof(cpu.prg), file);
    if (fclose(file) != 0 || size != sizeof(cpu.prg)) return 2;
    cpu6502_reset_state(&cpu);

    for (vector = 0u; vector < 50000u; ++vector) {
        BikeMovementState native;
        uint8_t expected[4];
        int rc;

        memset(&native, 0, sizeof(native));
        memset(cpu.ram, 0, sizeof(cpu.ram));
        native.global_vertical_reference = next_byte();
        cpu.ram[0x03F1u] = native.global_vertical_reference;
        for (i = 0u; i < 4u; ++i) {
            native.object_status[i] = (uint8_t)(next_byte() & 3u);
            native.vertical_reference[i] = next_byte();
            native.object_y[i] = next_byte();
            native.y_low[i] = next_byte();
            cpu.ram[0x00B0u + i] = native.object_status[i];
            cpu.ram[0x00BCu + i] = native.vertical_reference[i];
            cpu.ram[0x00B8u + i] = native.object_y[i];
            cpu.ram[0x008Cu + i] = native.y_low[i];
        }

        prepare_cpu(&cpu);
        rc = cpu6502_call(&cpu, 0xE836u, 256u);
        if (rc != 0) {
            printf("FAIL E836 oracle rc=%d vector=%u opcode=%02X\n",
                   rc, vector, cpu.bad_opcode);
            return 1;
        }
        for (i = 0u; i < 4u; ++i) expected[i] = cpu.ram[0x008Cu + i];

        bikePhysicsUpdateScreenY(&native);
        for (i = 0u; i < 4u; ++i) {
            if (native.y_low[i] != expected[i]) {
                printf("FAIL E836 vector=%u slot=%u native=%02X asm=%02X "
                       "global=%02X ref=%02X y=%02X status=%02X\n",
                       vector, i, native.y_low[i], expected[i],
                       native.global_vertical_reference,
                       native.vertical_reference[i], native.object_y[i],
                       native.object_status[i]);
                return 1;
            }
        }
    }
    puts("PASS: 50000 E836/DC97 four-bike screen-Y projections match original ASM.");
    return 0;
}
