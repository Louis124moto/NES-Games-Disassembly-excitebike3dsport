#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "bike_physics.h"
#include "cpu6502.h"

static uint32_t rng = 0x7A31D5C9u;
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
    unsigned slot, vector;
    unsigned long vectors = 0u;
    FILE *file;
    size_t bytes;

    if (argc != 2) {
        puts("usage: test_bike_object_speed <original-16KiB-prg>");
        return 2;
    }
    file = fopen(argv[1], "rb");
    if (file == NULL) return 2;
    bytes = fread(reference.prg, 1u, sizeof(reference.prg), file);
    if (fclose(file) != 0 || bytes != sizeof(reference.prg)) return 2;
    cpu6502_reset_state(&reference);

    /* Compare the source speed-control routine for all four object indices.
     * The original routine is called at $CD59; only its speed and terrain
     * outputs through the $CDBD join are asserted here. */
    for (slot = 0u; slot < 4u; ++slot) {
        for (vector = 0u; vector < 30000u; ++vector) {
            BikeObjectSpeedContext native;
            uint8_t *ram = reference.ram;
            unsigned i;
            int rc;

            memset(&native, 0, sizeof(native));
            native.object_index = (uint8_t)slot;
            native.frame_phase = (uint8_t)(nextByte() & 3u);
            native.speed.fraction = nextByte();
            native.speed.whole = nextByte();
            native.controller = nextByte();
            native.race_finished = nextByte() & 1u;
            native.object_lock_0374 = nextByte();
            native.collision_lock_0098 = nextByte();
            {
                static const uint8_t movementStates[] = { 0u, 1u, 2u, 4u, 5u, 6u };
                native.movement_state_009C = movementStates[nextByte() % 6u];
            }
            native.object_status_00B0 = (uint8_t)(nextByte() & 3u);
            native.terrain.object_class = (nextByte() & 1u) ? 0xE4u : 0u;
            native.terrain.surface = (nextByte() & 1u) ? 3u : 0u;
            native.terrain.surface_state = nextByte();
            native.terrain.vertical_position = nextByte();
            native.terrain.posture_state = (uint8_t)(nextByte() & 3u);
            native.terrain.player_event_flag = nextByte();

            memset(ram, 0, sizeof(reference.ram));
            ram[0x004Cu] = native.frame_phase;
            ram[0x0052u] = native.race_finished;
            ram[0x005Cu + slot] = native.controller;
            ram[0x0374u + slot] = native.object_lock_0374;
            ram[0x0098u + slot] = native.collision_lock_0098;
            ram[0x009Cu + slot] = native.movement_state_009C;
            ram[0x00B0u + slot] = native.object_status_00B0;
            ram[0x0090u + slot] = native.speed.fraction;
            ram[0x0094u + slot] = native.speed.whole;
            ram[0x00C0u + slot] = native.terrain.object_class;
            ram[0x00A4u + slot] = native.terrain.surface;
            ram[0x0070u + slot] = native.terrain.surface_state;
            ram[0x00B8u + slot] = native.terrain.vertical_position;
            ram[0x036Cu + slot] = native.terrain.posture_state;
            ram[0x00FDu] = native.terrain.player_event_flag;

            prepareCpu(&reference);
            reference.x = (uint8_t)slot;
            rc = cpu6502_call(&reference, 0xCD59u, 256u);
            if (rc != 0) {
                printf("FAIL: sub_CD59 returned %d slot=%u vector=%u\n", rc, slot, vector);
                return 1;
            }

            bikePhysicsUpdateObjectSpeed(&native);
            if (native.speed.fraction != ram[0x0090u + slot] ||
                native.speed.whole != ram[0x0094u + slot] ||
                native.terrain.posture_state != ram[0x036Cu + slot] ||
                native.terrain.player_event_flag != ram[0x00FDu]) {
                printf("FAIL: sub_CD59 slot=%u vector=%u phase=%u ctl=%02X speed=%02X:%02X/%02X:%02X posture=%02X/%02X event=%02X/%02X\n",
                       slot, vector, native.frame_phase, native.controller,
                       native.speed.whole, native.speed.fraction,
                       ram[0x0094u + slot], ram[0x0090u + slot],
                       native.terrain.posture_state, ram[0x036Cu + slot],
                       native.terrain.player_event_flag, ram[0x00FDu]);
                return 1;
            }
            for (i = 0u; i < 1u; ++i) ++vectors;
        }
    }

    /* sub_CD1F: active-bike dispatch, controller suppression and calls to
     * the speed-control routine in descending X order. A nonzero $0034 keeps
     * the separate race-start handshake out of this focused test. */
    for (vector = 0u; vector < 12000u; ++vector) {
        BikeRaceSpeedState race;
        uint8_t *ram = reference.ram;
        unsigned index;
        int rc;
        memset(&race, 0, sizeof(race));
        race.frame_phase = (uint8_t)(nextByte() & 3u);
        race.race_started = (uint8_t)(nextByte() & 1u);
        race.race_finished = (uint8_t)(nextByte() & 1u);
        for (index = 0u; index < 4u; ++index) {
            BikeObjectSpeedContext *object = &race.bikes[index];
            race.active[index] = (uint8_t)(nextByte() & 1u);
            object->controller = nextByte();
            object->speed.fraction = nextByte();
            object->speed.whole = nextByte();
            object->object_lock_0374 = nextByte();
            object->collision_lock_0098 = nextByte();
            object->overheat_lock_03E0 = nextByte();
            {
                static const uint8_t states[] = { 0u, 1u, 2u, 4u, 5u, 6u };
                object->movement_state_009C = states[nextByte() % 6u];
            }
            object->object_status_00B0 = (uint8_t)(nextByte() & 3u);
            object->terrain.object_class = (nextByte() & 1u) ? 0xE4u : 0u;
            object->terrain.surface = (nextByte() & 1u) ? 3u : 0u;
            object->terrain.surface_state = nextByte();
            object->terrain.vertical_position = nextByte();
            object->terrain.posture_state = (uint8_t)(nextByte() & 3u);
            object->terrain.player_event_flag = 0xA5u;
        }

        memset(ram, 0, sizeof(reference.ram));
        ram[0x0034u] = 1u;
        ram[0x004Fu] = race.race_started;
        ram[0x004Cu] = race.frame_phase;
        ram[0x0052u] = race.race_finished;
        for (index = 0u; index < 4u; ++index) {
            const BikeObjectSpeedContext *object = &race.bikes[index];
            ram[0x00A8u + index] = race.active[index];
            ram[0x005Cu + index] = object->controller;
            ram[0x0090u + index] = object->speed.fraction;
            ram[0x0094u + index] = object->speed.whole;
            ram[0x0374u + index] = object->object_lock_0374;
            ram[0x0098u + index] = object->collision_lock_0098;
            ram[0x03E0u + index] = object->overheat_lock_03E0;
            ram[0x009Cu + index] = object->movement_state_009C;
            ram[0x00B0u + index] = object->object_status_00B0;
            ram[0x00C0u + index] = object->terrain.object_class;
            ram[0x00A4u + index] = object->terrain.surface;
            ram[0x0070u + index] = object->terrain.surface_state;
            ram[0x00B8u + index] = object->terrain.vertical_position;
            ram[0x036Cu + index] = object->terrain.posture_state;
        }
        ram[0x00FDu] = 0xA5u;

        prepareCpu(&reference);
        rc = cpu6502_call(&reference, 0xCD1Fu, 4096u);
        if (rc != 0) {
            printf("FAIL: sub_CD1F returned %d on race-speed vector %u\n", rc, vector);
            return 1;
        }

        bikePhysicsUpdateRaceSpeeds(&race);
        for (index = 0u; index < 4u; ++index) {
            const BikeObjectSpeedContext *object = &race.bikes[index];
            if (object->controller != ram[0x005Cu + index] ||
                object->speed.fraction != ram[0x0090u + index] ||
                object->speed.whole != ram[0x0094u + index] ||
                object->terrain.posture_state != ram[0x036Cu + index]) {
                printf("FAIL: sub_CD1F vector=%u slot=%u active=%u started=%u phase=%u ctl=%02X/%02X speed=%02X:%02X/%02X:%02X posture=%02X/%02X\n",
                       vector, index, race.active[index], race.race_started,
                       race.frame_phase, object->controller, ram[0x005Cu + index],
                       object->speed.whole, object->speed.fraction,
                       ram[0x0094u + index], ram[0x0090u + index],
                       object->terrain.posture_state, ram[0x036Cu + index]);
                return 1;
            }
        }
        ++vectors;
    }

    printf("PASS: %lu native speed-control vectors match sub_CD59 and sub_CD1F.\n",
           vectors);
    return 0;
}
