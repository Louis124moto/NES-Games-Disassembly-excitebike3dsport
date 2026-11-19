#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bike_physics.h"
#include "cpu6502.h"

static uint32_t rng = 0xDBFE2026u;
static uint8_t nextByte(void)
{
    rng = rng * 1664525u + 1013904223u;
    return (uint8_t)(rng >> 24);
}

static void prepareCpu(Cpu6502 *cpu)
{
    cpu->a = 0u;
    cpu->x = 0u;
    cpu->y = 0u;
    cpu->sp = 0xFDu;
    cpu->p = 0x24u;
    cpu->cycles = 0u;
    cpu->bad_opcode = -1;
}

int main(int argc, char **argv)
{
    static Cpu6502 reference;
    FILE *file;
    size_t bytes;
    unsigned vector;
    const unsigned vectors = 100000u;

    if (argc != 2) {
        puts("usage: test_bike_secondary_scroll <original-16KiB-prg>");
        return 2;
    }
    file = fopen(argv[1], "rb");
    if (file == NULL) return 2;
    bytes = fread(reference.prg, 1u, sizeof(reference.prg), file);
    if (fclose(file) != 0 || bytes != sizeof(reference.prg)) return 2;
    cpu6502_reset_state(&reference);

    for (vector = 0u; vector < vectors; ++vector) {
        BikeMovementState native = { 0 };
        int rc;
        native.movement_delta[0] = nextByte();
        native.secondary_scroll_fraction = nextByte();
        native.secondary_scroll_x = nextByte();
        native.secondary_base_nametable = (uint8_t)(nextByte() & 1u);

        memset(reference.ram, 0, sizeof(reference.ram));
        reference.ram[0x0060u] = native.movement_delta[0];
        reference.ram[0x03BFu] = native.secondary_scroll_fraction;
        reference.ram[0x0050u] = native.secondary_scroll_x;
        reference.ram[0x004Eu] = native.secondary_base_nametable;
        prepareCpu(&reference);
        rc = cpu6502_call(&reference, 0xDBFEu, 64u);
        if (rc != 0) {
            printf("FAIL: sub_DBFE returned %d on vector %u\n", rc, vector);
            return 1;
        }

        bikePhysicsAdvanceSecondaryScroll(&native);
        if (native.secondary_scroll_fraction != reference.ram[0x03BFu] ||
            native.secondary_scroll_x != reference.ram[0x0050u] ||
            native.secondary_base_nametable != reference.ram[0x004Eu]) {
            printf("FAIL: vector=%u delta=%02X native=%02X/%02X/%02X ASM=%02X/%02X/%02X\n",
                   vector, native.movement_delta[0],
                   native.secondary_scroll_fraction, native.secondary_scroll_x,
                   native.secondary_base_nametable,
                   reference.ram[0x03BFu], reference.ram[0x0050u],
                   reference.ram[0x004Eu]);
            return 1;
        }
    }

    printf("PASS: %u secondary-scroll vectors match sub_DBFE ASM.\n", vectors);
    return 0;
}
