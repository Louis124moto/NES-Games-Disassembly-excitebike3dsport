#include <stdio.h>
#include <string.h>
#include "bike_physics.h"
#include "cpu6502.h"

static uint32_t rng_state = 0x91E10DA5u;
static uint8_t next_byte(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return (uint8_t)(rng_state >> 24);
}

static void prepare_cpu(Cpu6502 *cpu)
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
    unsigned long vectors = 0;
    unsigned i, whole, fraction;

    if (argc != 2) {
        puts("usage: test_bike_movement <original-16KiB-prg>");
        return 2;
    }
    {
        FILE *f = fopen(argv[1], "rb");
        size_t n;
        if (f == NULL) return 2;
        n = fread(reference.prg, 1u, sizeof(reference.prg), f);
        if (fclose(f) != 0 || n != sizeof(reference.prg)) return 2;
        cpu6502_reset_state(&reference);
    }

    /* sub_DA58: fixed-point speed -> per-frame movement delta. */
    for (i = 0; i < 4u; ++i) {
        for (whole = 0; whole < 256u; ++whole) {
            for (fraction = 0; fraction < 256u; ++fraction) {
                BikeMovementState native_state = { 0 };
                const uint8_t accumulator = (uint8_t)(whole * 29u + fraction * 73u + i * 11u);
                int rc;
                native_state.speed_whole[i] = (uint8_t)whole;
                native_state.speed_fraction[i] = (uint8_t)fraction;
                native_state.fraction_accumulator[i] = accumulator;

                memset(reference.ram, 0, sizeof(reference.ram));
                reference.ram[0x0094u + i] = (uint8_t)whole;
                reference.ram[0x0090u + i] = (uint8_t)fraction;
                reference.ram[0x0394u + i] = accumulator;
                prepare_cpu(&reference);
                reference.x = (uint8_t)i;
                rc = cpu6502_call(&reference, 0xDA58u, 64u);
                if (rc != 0) {
                    printf("FAIL: sub_DA58 returned %d at slot=%u speed=%02X:%02X accumulator=%02X\n",
                           rc, i, whole, fraction, accumulator);
                    return 1;
                }
                bikePhysicsBuildMovementDeltas(&native_state);
                if (native_state.movement_delta[i] != reference.ram[0x0060u + i] ||
                    native_state.fraction_accumulator[i] != reference.ram[0x0394u + i]) {
                    printf("FAIL: sub_DA58 slot=%u speed=%02X:%02X accumulator=%02X native=%02X/%02X ASM=%02X/%02X\n",
                           i, whole, fraction, accumulator,
                           native_state.movement_delta[i], native_state.fraction_accumulator[i],
                           reference.ram[0x0060u + i], reference.ram[0x0394u + i]);
                    return 1;
                }
                ++vectors;
            }
        }
    }

    /* sub_DA6A: player scroll and nametable toggle on 8-bit carry. */
    {
        static const uint8_t base_values[] = { 0u, 1u, 2u, 0xFFu };
        unsigned base, scroll, delta;
        for (base = 0; base < sizeof(base_values); ++base) {
            for (scroll = 0; scroll < 256u; ++scroll) {
                for (delta = 0; delta < 256u; ++delta) {
                    BikeMovementState native_state = { 0 };
                    int rc;
                    native_state.player_scroll_x = (uint8_t)scroll;
                    native_state.movement_delta[0] = (uint8_t)delta;
                    native_state.base_nametable = base_values[base];

                    memset(reference.ram, 0, sizeof(reference.ram));
                    reference.ram[0x0012u] = (uint8_t)scroll;
                    reference.ram[0x0060u] = (uint8_t)delta;
                    reference.ram[0x004Du] = base_values[base];
                    prepare_cpu(&reference);
                    rc = cpu6502_call(&reference, 0xDA6Au, 64u);
                    if (rc != 0) {
                        printf("FAIL: sub_DA6A returned %d at scroll=%u delta=%u base=%02X\n",
                               rc, scroll, delta, base_values[base]);
                        return 1;
                    }
                    bikePhysicsAdvancePlayerScroll(&native_state);
                    if (native_state.player_scroll_x != reference.ram[0x0012u] ||
                        native_state.base_nametable != reference.ram[0x004Du]) {
                        printf("FAIL: sub_DA6A scroll=%02X delta=%02X base=%02X native=%02X/%02X ASM=%02X/%02X\n",
                               scroll, delta, base_values[base],
                               native_state.player_scroll_x, native_state.base_nametable,
                               reference.ram[0x0012u], reference.ram[0x004Du]);
                        return 1;
                    }
                    ++vectors;
                }
            }
        }
    }

    /* sub_DA7A: opponent movement relative to player; inactive slots still
       have their previous-X history shifted by the player delta. */
    for (i = 0; i < 50000u; ++i) {
        BikeMovementState native_state = { 0 };
        unsigned slot;
        int rc;
        for (slot = 0; slot < 4u; ++slot) {
            native_state.movement_delta[slot] = next_byte();
            native_state.object_active[slot] = (uint8_t)(next_byte() & 1u);
            native_state.x_low[slot] = next_byte();
            native_state.x_high[slot] = next_byte();
            native_state.previous_x[slot] = next_byte();
        }

        memset(reference.ram, 0, sizeof(reference.ram));
        for (slot = 0; slot < 4u; ++slot) {
            reference.ram[0x0060u + slot] = native_state.movement_delta[slot];
            reference.ram[0x00A8u + slot] = native_state.object_active[slot];
            reference.ram[0x0080u + slot] = native_state.x_low[slot];
            reference.ram[0x0084u + slot] = native_state.x_high[slot];
            reference.ram[0x0390u + slot] = native_state.previous_x[slot];
        }
        prepare_cpu(&reference);
        rc = cpu6502_call(&reference, 0xDA7Au, 128u);
        if (rc != 0) {
            printf("FAIL: sub_DA7A returned %d on vector %u\n", rc, i);
            return 1;
        }
        bikePhysicsAdvanceOpponentPositions(&native_state);
        for (slot = 0; slot < 4u; ++slot) {
            if (native_state.x_low[slot] != reference.ram[0x0080u + slot] ||
                native_state.x_high[slot] != reference.ram[0x0084u + slot] ||
                native_state.previous_x[slot] != reference.ram[0x0390u + slot]) {
                printf("FAIL: sub_DA7A vector=%u slot=%u delta0=%02X asm60=%02X active=%u native=%02X:%02X prev=%02X ASM=%02X:%02X prev=%02X\n",
                       i, slot, native_state.movement_delta[0], reference.ram[0x0060u], native_state.object_active[slot],
                       native_state.x_high[slot], native_state.x_low[slot], native_state.previous_x[slot], reference.ram[0x0084u + slot],
                       reference.ram[0x0080u + slot], reference.ram[0x0390u + slot]);
                return 1;
            }
        }
        ++vectors;
    }

    printf("PASS: %lu native movement vectors match sub_DA58/sub_DA6A/sub_DA7A ASM.\n", vectors);
    return 0;
}
