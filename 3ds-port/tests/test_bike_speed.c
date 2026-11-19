#include <stdio.h>
#include <string.h>
#include "bike_physics.h"
#include "oracle.h"

/*
 * Differential test for native sub_CE29 translation.
 * The original 6502 routine is executed directly from the PRG by cpu6502_call;
 * no generated recompiler or NES hardware model participates in this test.
 */
int main(int argc, char **argv)
{
    static Oracle oracle;
    unsigned mode, phase, whole, fraction;
    unsigned long vectors = 0;

    if (argc != 2 || oracleInit(&oracle, argv[1]) != 0) {
        puts("usage: test_bike_speed <original-16KiB-prg>");
        return 2;
    }

    for (mode = 0; mode < 2u; ++mode) {
        for (phase = 0; phase < 4u; ++phase) {
            for (whole = 0; whole < 256u; ++whole) {
                for (fraction = 0; fraction < 256u; ++fraction) {
                    BikeSpeedState native_speed;
                    int rc;

                    native_speed.fraction = (uint8_t)fraction;
                    native_speed.whole = (uint8_t)whole;

                    memset(oracle.cpu.ram, 0, sizeof(oracle.cpu.ram));
                    oracle.cpu.ram[0x004Cu] = (uint8_t)phase;
                    oracle.cpu.ram[0x0090u] = (uint8_t)fraction;
                    oracle.cpu.ram[0x0094u] = (uint8_t)whole;
                    oracle.cpu.a = 0u;
                    oracle.cpu.x = 0u; /* player object */
                    oracle.cpu.y = (uint8_t)mode;
                    oracle.cpu.sp = 0xFDu;
                    oracle.cpu.p = 0x24u;
                    oracle.cpu.cycles = 0u;
                    oracle.cpu.bad_opcode = 0;

                    rc = cpu6502_call(&oracle.cpu, 0xCE29u, 64u);
                    if (rc != 0) {
                        printf("FAIL: original sub_CE29 returned %d at mode=%u phase=%u speed=%02X:%02X\n",
                               rc, mode, phase, whole, fraction);
                        return 1;
                    }

                    bikePhysicsAcceleratePlayer(&native_speed, (uint8_t)phase, (uint8_t)mode);
                    if (native_speed.fraction != oracle.cpu.ram[0x0090u] ||
                        native_speed.whole != oracle.cpu.ram[0x0094u]) {
                        printf("FAIL: mode=%u phase=%u input=%02X:%02X native=%02X:%02X asm=%02X:%02X\n",
                               mode, phase, whole, fraction,
                               native_speed.whole, native_speed.fraction,
                               oracle.cpu.ram[0x0094u], oracle.cpu.ram[0x0090u]);
                        return 1;
                    }
                    ++vectors;
                }
            }
        }
    }

    printf("PASS: %lu exhaustive sub_CE29 player-speed vectors match original ASM.\n", vectors);
    return 0;
}
