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

    for (mode = 0; mode < 3u; ++mode) {
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

    /* Differentially exercise player terrain gate sub_CDEE. */
    {
        static const uint8_t object_classes[] = { 0x00u, 0xE4u };
        static const uint8_t surfaces[] = { 0x00u, 0x03u };
        unsigned object_class, surface, surface_state, vertical, posture;
        for (object_class = 0; object_class < 2u; ++object_class) {
            for (surface = 0; surface < 2u; ++surface) {
                for (surface_state = 0; surface_state < 5u; ++surface_state) {
                    for (vertical = 0; vertical < 256u; ++vertical) {
                        for (posture = 0; posture < 4u; ++posture) {
                            BikeTerrainGateState native_state;
                            uint8_t native_z;
                            int rc;
                            native_state.object_class = object_classes[object_class];
                            native_state.surface = surfaces[surface];
                            native_state.surface_state = (uint8_t)surface_state;
                            native_state.vertical_position = (uint8_t)vertical;
                            native_state.posture_state = (uint8_t)posture;
                            native_state.player_event_flag = 0xA5u;

                            memset(reference.ram, 0, sizeof(reference.ram));
                            reference.ram[0x00C0u] = native_state.object_class;
                            reference.ram[0x00A4u] = native_state.surface;
                            reference.ram[0x0070u] = native_state.surface_state;
                            reference.ram[0x00B8u] = native_state.vertical_position;
                            reference.ram[0x036Cu] = native_state.posture_state;
                            reference.ram[0x00FDu] = 0xA5u;
                            reference.a = 0u;
                            reference.x = 0u;
                            reference.y = 0u;
                            reference.sp = 0xFDu;
                            reference.p = 0x24u;
                            reference.cycles = 0u;
                            reference.bad_opcode = -1;
                            rc = cpu6502_call(&reference, 0xCDEEu, 64u);
                            if (rc != 0) {
                                printf("FAIL: original sub_CDEE returned %d at class=%02X surface=%02X state=%u y=%u posture=%u\n",
                                       rc, native_state.object_class, native_state.surface,
                                       surface_state, vertical, posture);
                                return 1;
                            }
                            native_z = bikePhysicsTerrainGate(&native_state);
                            if (native_z != (uint8_t)((reference.p & 0x02u) != 0u) ||
                                native_state.posture_state != reference.ram[0x036Cu] ||
                                native_state.player_event_flag != reference.ram[0x00FDu]) {
                                printf("FAIL: terrain class=%02X surface=%02X state=%u y=%u posture=%u native(z=%u,p=%u,fd=%02X) asm(z=%u,p=%u,fd=%02X)\n",
                                       native_state.object_class, native_state.surface,
                                       surface_state, vertical, posture,
                                       native_z, native_state.posture_state, native_state.player_event_flag,
                                       (unsigned)((reference.p & 0x02u) != 0u),
                                       reference.ram[0x036Cu], reference.ram[0x00FDu]);
                                return 1;
                            }
                            ++vectors;
                        }
                    }
                }
            }
        }
    }

    /* Differentially test the complete player speed branch sub_CD59. */
    {
        uint32_t random_state = 0x4D595DF4u;
        unsigned controller, phase3, status3, finished;
        for (controller = 0; controller < 256u; ++controller) {
            for (phase3 = 0; phase3 < 4u; ++phase3) {
                for (status3 = 0; status3 < 4u; ++status3) {
                    for (finished = 0; finished < 2u; ++finished) {
                        BikePlayerSpeedContext native_player;
                        uint8_t *ram = reference.ram;
                        int rc;
#define NEXT_BYTE() (random_state = random_state * 1664525u + 1013904223u, (uint8_t)(random_state >> 24))
                        native_player.controller = (uint8_t)controller;
                        native_player.frame_phase = (uint8_t)phase3;
                        native_player.object_status_00B0 = (uint8_t)status3;
                        native_player.race_finished = (uint8_t)finished;
                        native_player.object_lock_0374 = (uint8_t)(NEXT_BYTE() & 1u);
                        native_player.collision_lock_0098 = (uint8_t)(NEXT_BYTE() & 1u);
                        {
                            const uint8_t states[] = { 0u, 5u, 2u };
                            native_player.movement_state_009C = states[NEXT_BYTE() % 3u];
                        }
                        native_player.speed.fraction = NEXT_BYTE();
                        native_player.speed.whole = NEXT_BYTE();
                        native_player.terrain.object_class = (NEXT_BYTE() & 1u) ? 0xE4u : 0u;
                        native_player.terrain.surface = (NEXT_BYTE() & 1u) ? 3u : 0u;
                        native_player.terrain.surface_state = NEXT_BYTE();
                        native_player.terrain.vertical_position = NEXT_BYTE();
                        native_player.terrain.posture_state = (uint8_t)(NEXT_BYTE() & 3u);
                        native_player.terrain.player_event_flag = 0xA5u;

                        memset(ram, 0, sizeof(reference.ram));
                        ram[0x004Cu] = native_player.frame_phase;
                        ram[0x005Cu] = native_player.controller;
                        ram[0x0052u] = native_player.race_finished;
                        ram[0x0374u] = native_player.object_lock_0374;
                        ram[0x0098u] = native_player.collision_lock_0098;
                        ram[0x009Cu] = native_player.movement_state_009C;
                        ram[0x00B0u] = native_player.object_status_00B0;
                        ram[0x0090u] = native_player.speed.fraction;
                        ram[0x0094u] = native_player.speed.whole;
                        ram[0x00C0u] = native_player.terrain.object_class;
                        ram[0x00A4u] = native_player.terrain.surface;
                        ram[0x0070u] = native_player.terrain.surface_state;
                        ram[0x00B8u] = native_player.terrain.vertical_position;
                        ram[0x036Cu] = native_player.terrain.posture_state;
                        ram[0x00FDu] = native_player.terrain.player_event_flag;
                        ram[0x0058u] = 0u;
                        ram[0x0026u] = 1u; /* keep the later steering timer path inert */
                        ram[0x0368u] = 0x55u;
                        ram[0x00ACu] = 0x55u;

                        reference.a = 0u;
                        reference.x = 0u;
                        reference.y = 0u;
                        reference.sp = 0xFDu;
                        reference.p = 0x24u;
                        reference.cycles = 0u;
                        reference.bad_opcode = -1;
                        rc = cpu6502_call(&reference, 0xCD59u, 256u);
                        if (rc != 0) {
                            printf("FAIL: original sub_CD59 returned %d at controller=%02X phase=%u status=%u finished=%u\n",
                                   rc, controller, phase3, status3, finished);
                            return 1;
                        }

                        bikePhysicsUpdatePlayerSpeed(&native_player);
                        if (native_player.speed.fraction != ram[0x0090u] ||
                            native_player.speed.whole != ram[0x0094u] ||
                            native_player.terrain.posture_state != ram[0x036Cu] ||
                            native_player.terrain.player_event_flag != ram[0x00FDu]) {
                            printf("FAIL: sub_CD59 controller=%02X phase=%u status=%u finish=%u native speed=%02X:%02X posture=%02X fd=%02X ASM speed=%02X:%02X posture=%02X fd=%02X\n",
                                   controller, phase3, status3, finished,
                                   native_player.speed.whole, native_player.speed.fraction,
                                   native_player.terrain.posture_state, native_player.terrain.player_event_flag,
                                   ram[0x0094u], ram[0x0090u], ram[0x036Cu], ram[0x00FDu]);
                            return 1;
                        }
                        ++vectors;
#undef NEXT_BYTE
                    }
                }
            }
        }
    }

    /* Differentially test temperature/overheat state from sub_E359. */
    {
        static const uint8_t whole_values[] = {
            0u, 7u, 8u, 9u, 16u, 17u, 18u, 25u, 26u, 31u, 32u, 33u, 0xFFu
        };
        static const uint8_t fraction_values[] = { 0u, 0x0Au, 0x0Bu, 0x0Cu, 0xFFu };
        unsigned controller2, started2, finished2, overheat2, timer2, wi, fi2;
        for (controller2 = 0; controller2 < 256u; ++controller2) {
            for (started2 = 0; started2 < 2u; ++started2) {
                for (finished2 = 0; finished2 < 2u; ++finished2) {
                    for (overheat2 = 0; overheat2 < 2u; ++overheat2) {
                        for (timer2 = 0; timer2 < 2u; ++timer2) {
                            for (wi = 0; wi < sizeof(whole_values); ++wi) {
                                for (fi2 = 0; fi2 < sizeof(fraction_values); ++fi2) {
                                    BikeTemperatureState native_temperature;
                                    uint8_t native_tile;
                                    int rc;
                                    native_temperature.fraction = fraction_values[fi2];
                                    native_temperature.whole = whole_values[wi];
                                    native_temperature.overheat_state = overheat2 ? 0x20u : 0u;
                                    native_temperature.overheat_timer = (uint8_t)timer2;
                                    native_temperature.sound_flags = 0x05u;

                                    memset(reference.ram, 0, sizeof(reference.ram));
                                    reference.ram[0x0052u] = (uint8_t)finished2;
                                    reference.ram[0x03E0u] = native_temperature.overheat_state;
                                    reference.ram[0x003Cu] = native_temperature.overheat_timer;
                                    reference.ram[0x005Cu] = (uint8_t)controller2;
                                    reference.ram[0x004Fu] = (uint8_t)started2;
                                    reference.ram[0x03B5u] = native_temperature.fraction;
                                    reference.ram[0x03B6u] = native_temperature.whole;
                                    reference.ram[0x00FCu] = native_temperature.sound_flags;
                                    reference.a = 0u;
                                    reference.x = 0u;
                                    reference.y = 0u;
                                    reference.sp = 0xFDu;
                                    reference.p = 0x24u;
                                    reference.cycles = 0u;
                                    reference.bad_opcode = -1;
                                    rc = cpu6502_call(&reference, 0xE359u, 512u);
                                    if (rc != 0) {
                                        printf("FAIL: original sub_E359 returned %d at controller=%02X started=%u finished=%u overheat=%u timer=%u heat=%02X:%02X\n",
                                               rc, controller2, started2, finished2, overheat2,
                                               timer2, whole_values[wi], fraction_values[fi2]);
                                        return 1;
                                    }

                                    native_tile = bikePhysicsUpdateTemperature(&native_temperature,
                                                                               (uint8_t)controller2,
                                                                               (uint8_t)started2,
                                                                               (uint8_t)finished2);
                                    if (native_temperature.fraction != reference.ram[0x03B5u] ||
                                        native_temperature.whole != reference.ram[0x03B6u] ||
                                        native_temperature.overheat_state != reference.ram[0x03E0u] ||
                                        native_temperature.overheat_timer != reference.ram[0x003Cu] ||
                                        native_temperature.sound_flags != reference.ram[0x00FCu] ||
                                        native_tile != reference.ram[0x0001u]) {
                                        printf("FAIL: sub_E359 controller=%02X start=%u finish=%u overheat=%u timer=%u heat=%02X:%02X native=%02X:%02X state=%02X timer=%02X fc=%02X tile=%02X ASM=%02X:%02X state=%02X timer=%02X fc=%02X tile=%02X\n",
                                               controller2, started2, finished2, overheat2, timer2,
                                               whole_values[wi], fraction_values[fi2],
                                               native_temperature.whole, native_temperature.fraction,
                                               native_temperature.overheat_state, native_temperature.overheat_timer,
                                               native_temperature.sound_flags, native_tile,
                                               reference.ram[0x03B6u], reference.ram[0x03B5u],
                                               reference.ram[0x03E0u], reference.ram[0x003Cu],
                                               reference.ram[0x00FCu], reference.ram[0x0001u]);
                                        return 1;
                                    }
                                    ++vectors;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    printf("PASS: %lu native player speed-control, terrain and temperature vectors match original ASM.\n", vectors);
    return 0;
}
