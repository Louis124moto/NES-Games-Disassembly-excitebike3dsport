#include <stdio.h>
#include <string.h>
#include "bike_physics.h"
#include "cpu6502.h"

/* Differential test of loc_CDBD..loc_CE83..loc_DCC7 (called at $CDBD) for all four slots. */
static uint32_t rng = 0x5EE12A77u;
static uint8_t nb(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }
static uint8_t pick(const uint8_t *v, unsigned n) { return v[nb() % n]; }

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    static const uint8_t small[] = { 0u, 1u, 2u, 3u };
    static const uint8_t approach[] = { 0u, 1u, 2u, 5u, 6u, 9u, 0x0Au, 0x0Bu, 0x0Cu, 0x0Du, 0x0Eu, 0xFFu };
    static const uint8_t flags[] = { 0u, 1u, 2u, 3u, 0x40u, 0x41u, 0x42u, 0x80u, 0x81u, 0xC0u, 0xC3u, 0xFFu };
    static const uint8_t speed[] = { 0u, 1u, 0x9Fu, 0xA0u, 0xA1u, 0xFFu };
    static const uint8_t timers[] = { 0u, 0u, 1u, 5u, 0x0Du, 0xFFu };
    unsigned slot, v, i;
    unsigned long vectors = 0u, steer_paths = 0u;
    FILE *fp;
    if (argc != 2) { puts("usage: test_bike_steering_tail <prg>"); return 2; }
    fp = fopen(argv[1], "rb");
    if (fp == NULL || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) return 2;
    fclose(fp);
    cpu6502_reset_state(&cpu);

    for (slot = 0u; slot < 4u; ++slot) {
        for (v = 0u; v < 60000u; ++v) {
            BikeSteeringState n;
            uint8_t *r = cpu.ram;
            n.collision_state_0098 = (nb() & 3u) == 0u ? pick(small, 4u) : 0u;
            n.object_status_00B0 = pick(small, 4u);
            n.inactive_0058 = (nb() & 3u) == 0u ? 1u : 0u;
            n.track_finished_0052 = (nb() & 3u) == 0u ? 1u : 0u;
            n.speed_fraction_0090 = pick(speed, sizeof(speed));
            n.speed_whole_0094 = (nb() & 1u) ? 0u : pick(speed, sizeof(speed));
            n.movement_flags_005C = pick(flags, sizeof(flags));
            n.approach_00AC = pick(approach, sizeof(approach));
            n.animation_state_0368 = pick(approach, sizeof(approach));
            n.movement_mode_0388 = nb();
            n.steer_timer_0026 = pick(timers, sizeof(timers));
            n.approach_timer_002A = pick(timers, sizeof(timers));

            memset(r, 0, sizeof(cpu.ram));
            r[0x98 + slot] = n.collision_state_0098; r[0xB0 + slot] = n.object_status_00B0;
            r[0x58 + slot] = n.inactive_0058; r[0x52] = n.track_finished_0052;
            r[0x90 + slot] = n.speed_fraction_0090; r[0x94 + slot] = n.speed_whole_0094;
            r[0x5C + slot] = n.movement_flags_005C; r[0xAC + slot] = n.approach_00AC;
            r[0x368 + slot] = n.animation_state_0368; r[0x388 + slot] = n.movement_mode_0388;
            r[0x26 + slot] = n.steer_timer_0026; r[0x2A + slot] = n.approach_timer_002A;
            r[0x0A] = n.movement_flags_005C & 3u; /* set by the sub_CD59 prefix */
            cpu.a = cpu.y = 0u; cpu.x = (uint8_t)slot; cpu.sp = 0xFDu; cpu.p = 0x24u;
            cpu.cycles = 0u; cpu.bad_opcode = -1;
            if (cpu6502_call(&cpu, 0xCDBDu, 512u) != 0) { printf("FAIL: ASM did not return slot=%u v=%u\n", slot, v); return 1; }

            bikePhysicsSteeringTail(&n);
            if (n.collision_state_0098 != r[0x98 + slot] || n.approach_00AC != r[0xAC + slot] ||
                n.movement_mode_0388 != r[0x388 + slot] || n.steer_timer_0026 != r[0x26 + slot] ||
                n.approach_timer_002A != r[0x2A + slot]) {
                printf("FAIL slot=%u v=%u native 98=%02X AC=%02X 388=%02X 26=%02X 2A=%02X | ASM 98=%02X AC=%02X 388=%02X 26=%02X 2A=%02X\n",
                       slot, v, n.collision_state_0098, n.approach_00AC, n.movement_mode_0388,
                       n.steer_timer_0026, n.approach_timer_002A, r[0x98 + slot], r[0xAC + slot],
                       r[0x388 + slot], r[0x26 + slot], r[0x2A + slot]);
                return 1;
            }
            if (n.steer_timer_0026 != 0u) ++steer_paths;
            ++vectors;
        }
    }
    (void)i;
    printf("PASS: %lu native sub_CD59 steering-tail vectors (loc_CDBD/CE83/DCC7) match original ASM (%lu with steering timer set).\n", vectors, steer_paths);
    return 0;
}
