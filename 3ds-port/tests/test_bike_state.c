#include <stdio.h>
#include <string.h>
#include "bike_physics.h"
#include "cpu6502.h"

static uint32_t rng = 0xD918DA15u;
static uint8_t rnd(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }
static void prepare(Cpu6502 *c)
{
    c->a=0u; c->x=0u; c->y=0u; c->sp=0xFDu; c->p=0x24u;
    c->cycles=0u; c->bad_opcode=-1;
}

#define CHECK_ARRAY(field, address) do { \
    for (slot=0; slot<4u; ++slot) { \
        if (native.field[slot] != reference.ram[(address)+slot]) { \
            printf("FAIL %s v=%u slot=%u n=%02X asm=%02X state=%02X/%02X y=%02X/%02X bc=%02X/%02X dc=%02X/%02X rider=%02X/%02X target=%02X/%02X timers=%02X/%02X frame=%02X/%02X\\n", #field, vector, slot, native.field[slot], reference.ram[(address)+slot], native.state[slot], reference.ram[0x009Cu+slot], native.y[slot], reference.ram[0x00B8u+slot], native.vertical_offset[slot], reference.ram[0x00BCu+slot], native.vertical_delta[slot], reference.ram[0x00DCu+slot], native.rider_present[slot], reference.ram[0x0370u+slot], native.target_index[slot], reference.ram[0x03E4u+slot], native.timer_state[slot], reference.ram[0x002Au+slot], native.frame_counter, reference.ram[0x003Fu]); \
            printf("native state/timer: %02X/%02X %02X/%02X %02X/%02X %02X/%02X\\n",native.state[0],native.timer_state[0],native.state[1],native.timer_state[1],native.state[2],native.timer_state[2],native.state[3],native.timer_state[3]); \
            printf("ASM state/timer: %02X/%02X %02X/%02X %02X/%02X %02X/%02X\\n",reference.ram[0x009C],reference.ram[0x002A],reference.ram[0x009D],reference.ram[0x002B],reference.ram[0x009E],reference.ram[0x002C],reference.ram[0x009F],reference.ram[0x002D]); \
            return 1; \
        } \
    } \
} while (0)

int main(int argc, char **argv)
{
    static Cpu6502 reference;
    unsigned vector, slot;
    if (argc != 2) { puts("usage: test_bike_state <original-16KiB-prg>"); return 2; }
    {
        FILE *f=fopen(argv[1],"rb"); size_t n;
        if (!f) return 2;
        n=fread(reference.prg,1u,sizeof(reference.prg),f);
        if (fclose(f)!=0 || n!=sizeof(reference.prg)) return 2;
        cpu6502_reset_state(&reference);
    }

    for (vector=0; vector<50000u; ++vector) {
        BikeStateMachineState native;
        int rc;
        memset(&native,0,sizeof(native));
        for(slot=0;slot<4u;++slot) {
            native.state[slot]=(uint8_t)(rnd()%6u);
            native.rider_present[slot]=(uint8_t)(rnd()&1u);
            native.x[slot]=rnd(); native.previous_x[slot]=rnd();
            native.target_index[slot]=(uint8_t)(rnd()%5u);
            native.y[slot]=rnd(); native.vertical_delta[slot]=rnd();
            native.speed_whole[slot]=rnd(); native.timer_state[slot]=rnd();
            native.timer_recovery[slot]=rnd(); native.vertical_state[slot]=rnd();
            native.vertical_offset[slot]=rnd(); native.overheat[slot]=rnd();
            native.vertical_collision_ref[slot]=rnd();
        }
        native.global_overheat=rnd();
        native.global_timer_0022=rnd();
        native.frame_counter=rnd();
        native.frame_phase=(uint8_t)(rnd()&3u);
        native.controller_hold=rnd();
        native.remembered_controller=rnd();
        native.player_event=rnd();
        native.opponent_spawn_event=rnd();

        memset(reference.ram,0,sizeof(reference.ram));
        for(slot=0;slot<4u;++slot) {
            reference.ram[0x009Cu+slot]=native.state[slot];
            reference.ram[0x0370u+slot]=native.rider_present[slot];
            reference.ram[0x0080u+slot]=native.x[slot];
            reference.ram[0x0390u+slot]=native.previous_x[slot];
            reference.ram[0x03E4u+slot]=native.target_index[slot];
            reference.ram[0x00B8u+slot]=native.y[slot];
            reference.ram[0x00DCu+slot]=native.vertical_delta[slot];
            reference.ram[0x0094u+slot]=native.speed_whole[slot];
            reference.ram[0x002Au+slot]=native.timer_state[slot];
            reference.ram[0x0036u+slot]=native.timer_recovery[slot];
            reference.ram[0x0398u+slot]=native.vertical_state[slot];
            reference.ram[0x00BCu+slot]=native.vertical_offset[slot];
            reference.ram[0x03E0u+slot]=native.overheat[slot];
            reference.ram[0x006Cu+slot]=native.vertical_collision_ref[slot];
        }
        reference.ram[0x03F7u]=native.global_overheat;
        reference.ram[0x0022u]=native.global_timer_0022;
        reference.ram[0x003Fu]=native.frame_counter;
        reference.ram[0x004Cu]=native.frame_phase;
        reference.ram[0x0014u]=native.controller_hold;
        reference.ram[0x0049u]=native.remembered_controller;
        reference.ram[0x00FDu]=native.player_event;
        reference.ram[0x03A8u]=native.opponent_spawn_event;

        prepare(&reference);
        rc=cpu6502_call(&reference,0xD918u,2048u);
        if(rc!=0) { printf("FAIL ASM sub_D918 rc=%d vector=%u opcode=%02X\\n",rc,vector,(unsigned)reference.bad_opcode); return 1; }
        bikePhysicsUpdateBikeStates(&native);

        CHECK_ARRAY(state,0x009Cu);
        CHECK_ARRAY(x,0x0080u);
        CHECK_ARRAY(y,0x00B8u);
        CHECK_ARRAY(speed_whole,0x0094u);
        CHECK_ARRAY(vertical_offset,0x00BCu);
        CHECK_ARRAY(rider_present,0x0370u);
        CHECK_ARRAY(previous_x,0x0390u);
        CHECK_ARRAY(vertical_delta,0x00DCu);
        CHECK_ARRAY(timer_state,0x002Au);
        CHECK_ARRAY(timer_recovery,0x0036u);
        CHECK_ARRAY(vertical_state,0x0398u);
        CHECK_ARRAY(vertical_collision_ref,0x006Cu);
        if(native.player_event!=reference.ram[0x00FDu] ||
           native.opponent_spawn_event!=reference.ram[0x03A8u] ||
           native.remembered_controller!=reference.ram[0x0049u] ||
           native.global_timer_0022!=reference.ram[0x0022u] ||
           native.frame_counter!=reference.ram[0x003Fu] ||
           native.frame_phase!=reference.ram[0x004Cu]) {
            printf("FAIL globals vector=%u\\n",vector); return 1;
        }
    }
    puts("PASS: 50,000 four-bike D918/DA15 state-machine frames match original ASM.");
    return 0;
}
