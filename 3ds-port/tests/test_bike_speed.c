#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bike_physics.h"
#include "cpu6502.h"

/*
 * Differential test for native sub_CE29 translation.
 * The original 6502 routine is executed directly from the PRG by the
 * host-only reference CPU. No generated recompiler or NES hardware model
 * participates in this test.
 */
static int load_prg(Cpu6502 *cpu, const char *path)
{
    FILE *f = fopen(path, "rb");
    size_t n;
    if (f == NULL) return -1;
    n = fread(cpu->prg, 1u, sizeof(cpu->prg), f);
    if (fclose(f) != 0 || n != sizeof(cpu->prg)) return -1;
    cpu6502_reset_state(cpu);
    return 0;
}

int main(int argc, char **argv)
{
    static Cpu6502 reference;
    unsigned mode, phase, whole, fraction;
    unsigned long vectors = 0;

    if (argc != 2 || load_prg(&reference, argv[1]) != 0) {
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

                    memset(reference.ram, 0, sizeof(reference.ram));
                    reference.ram[0x004Cu] = (uint8_t)phase;
                    reference.ram[0x0090u] = (uint8_t)fraction;
                    reference.ram[0x0094u] = (uint8_t)whole;
                    reference.a = 0u;
                    reference.x = 0u; /* player object */
                    reference.y = (uint8_t)mode;
                    reference.sp = 0xFDu;
                    reference.p = 0x24u;
                    reference.cycles = 0u;
                    reference.bad_opcode = -1;

                    rc = cpu6502_call(&reference, 0xCE29u, 64u);
                    if (rc != 0) {
                        printf("FAIL: original sub_CE29 returned %d at mode=%u phase=%u speed=%02X:%02X\n",
                               rc, mode, phase, whole, fraction);
                        return 1;
                    }

                    bikePhysicsAcceleratePlayer(&native_speed, (uint8_t)phase, (uint8_t)mode);
                    if (native_speed.fraction != reference.ram[0x0090u] ||
                        native_speed.whole != reference.ram[0x0094u]) {
                        printf("FAIL: mode=%u phase=%u input=%02X:%02X native=%02X:%02X asm=%02X:%02X\n",
                               mode, phase, whole, fraction,
                               native_speed.whole, native_speed.fraction,
                               reference.ram[0x0094u], reference.ram[0x0090u]);
                        return 1;
                    }
                    ++vectors;
                }
            }
        }
    }

    /* Differentially exercise sub_CE58 for every deceleration-table entry. */
    {
        static const uint8_t fractions[] = { 0u, 1u, 0x0Bu, 0x20u, 0x3Fu, 0x40u, 0x7Fu, 0xB0u, 0xC0u, 0xFFu };
        static const uint8_t thresholds[] = { 0x01u, 0xB0u };
        unsigned threshold, mode2, phase2, whole2, fi;
        for (threshold = 0; threshold < 2u; ++threshold) {
            for (mode2 = 0; mode2 < 7u; ++mode2) {
                for (phase2 = 0; phase2 < 4u; ++phase2) {
                    for (whole2 = 0; whole2 < 256u; ++whole2) {
                        for (fi = 0; fi < sizeof(fractions); ++fi) {
                            BikeSpeedState native_speed;
                            const uint8_t fraction2 = fractions[fi];
                            int rc;
                            native_speed.fraction = fraction2;
                            native_speed.whole = (uint8_t)whole2;
                            memset(reference.ram, 0, sizeof(reference.ram));
                            reference.ram[0x004Cu] = (uint8_t)phase2;
                            reference.ram[0x0090u] = fraction2;
                            reference.ram[0x0094u] = (uint8_t)whole2;
                            reference.ram[0x00B0u] = (uint8_t)(threshold * 2u);
                            reference.a = 0u;
                            reference.x = 0u;
                            reference.y = (uint8_t)mode2;
                            reference.sp = 0xFDu;
                            reference.p = 0x24u;
                            reference.cycles = 0u;
                            reference.bad_opcode = -1;
                            rc = cpu6502_call(&reference, 0xCE58u, 64u);
                            if (rc != 0) {
                                printf("FAIL: original sub_CE58 returned %d at threshold=%02X mode=%u phase=%u speed=%02X:%02X\n",
                                       rc, thresholds[threshold], mode2, phase2, whole2, fraction2);
                                return 1;
                            }
                            bikePhysicsDeceleratePlayer(&native_speed, (uint8_t)phase2,
                                                        (uint8_t)mode2, thresholds[threshold]);
                            if (native_speed.fraction != reference.ram[0x0090u] ||
                                native_speed.whole != reference.ram[0x0094u]) {
                                printf("FAIL: decel threshold=%02X mode=%u phase=%u input=%02X:%02X native=%02X:%02X asm=%02X:%02X\n",
                                       thresholds[threshold], mode2, phase2, whole2, fraction2,
                                       native_speed.whole, native_speed.fraction,
                                       reference.ram[0x0094u], reference.ram[0x0090u]);
                                return 1;
                            }
                            ++vectors;
                        }
                    }
                }
            }
        }
    }

    printf("PASS: %lu native player speed/deceleration vectors match original ASM.\n", vectors);
    return 0;
}
