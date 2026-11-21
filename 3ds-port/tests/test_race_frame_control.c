#include <stdio.h>
#include <string.h>
#include "race_frame_control.h"
#include "cpu6502.h"

static uint32_t rng = 0x2B7E1516u;
static uint8_t nb(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }

#define F1(X) X(pause_timer, 0x3B2) X(start_latch, 0x3B0) X(pause_flag, 0x3B1) X(button_hold, 0x14) X(button_press, 0x15) \
    X(input_005C, 0x5C) X(sound_00FB, 0xFB) X(sound_00FC, 0xFC) X(flag_03F7, 0x3F7) X(timer_0032, 0x32) \
    X(track_finished, 0x52) X(finish_phase_003F, 0x3F) X(stream_index, 0x43) X(course_id, 0x41) \
    X(frame_flag_0040, 0x40) X(frame_flag_0044, 0x44) X(flag_0047, 0x47) X(counter_03A9, 0x3A9) \
    X(counter_03E0, 0x3E0) X(timer_003C, 0x3C)

static void toRam(const RaceFrameControlState *s, Cpu6502 *c)
{
    unsigned i;
    memset(c->ram, 0, sizeof(c->ram));
#define W(F, B) c->ram[B] = s->F;
    F1(W)
#undef W
    for (i = 0u; i < 10u; ++i) c->ram[0x312 + i] = s->ppu_packet[i];
    c->io[0x4015] = 0xAAu;
}

static int compare(const char *w, unsigned v, const RaceFrameControlState *s, const Cpu6502 *c)
{
    unsigned i;
    const uint8_t *r = c->ram;
#define C(F, B) if (s->F != r[B]) { printf("FAIL %s v=%u %s native=%02X ASM=%02X\n", w, v, #F, s->F, r[B]); return 1; }
    F1(C)
#undef C
    for (i = 0u; i < 10u; ++i) if (s->ppu_packet[i] != r[0x312 + i]) { printf("FAIL %s v=%u packet[%u] native=%02X ASM=%02X\n", w, v, i, s->ppu_packet[i], r[0x312 + i]); return 1; }
    if (s->apu_enable_write) { if (c->io[0x4015] != s->apu_enable_value) { printf("FAIL %s v=%u $4015 native=%02X ASM=%02X\n", w, v, s->apu_enable_value, c->io[0x4015]); return 1; } }
    else if (c->io[0x4015] != 0xAAu) { printf("FAIL %s v=%u unexpected $4015 write %02X\n", w, v, c->io[0x4015]); return 1; }
    return 0;
}

static void randomize(RaceFrameControlState *s)
{
    memset(s, 0, sizeof(*s));
    s->pause_timer = (nb() & 3u) ? 0u : (uint8_t)(nb() % 0x30u);
    s->start_latch = (nb() & 1u) ? 0u : (nb() & 1u) ? 0x80u : 0x10u;
    s->pause_flag = (uint8_t)(nb() & 1u);
    s->button_hold = nb();
    s->button_press = (nb() & 1u) ? (uint8_t)(nb() & 0x10u) : (nb() & 1u) ? (uint8_t)(nb() & 0x0Fu) : nb();
    s->input_005C = nb(); s->sound_00FB = nb(); s->sound_00FC = nb();
    s->timer_0032 = (nb() & 3u) ? (uint8_t)(nb() & 0x1Fu) : 0u;
    s->track_finished = (nb() & 1u) ? 1u : (uint8_t)(nb() % 3u);
    s->finish_phase_003F = nb();
    s->stream_index = (uint8_t)(nb() % 5u);
    s->course_id = nb(); s->frame_flag_0040 = nb(); s->frame_flag_0044 = nb(); s->flag_0047 = nb();
    s->counter_03A9 = nb(); s->counter_03E0 = nb(); s->timer_003C = nb();
    { unsigned i; for (i = 0u; i < 10u; ++i) s->ppu_packet[i] = nb(); }
}

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *fp;
    unsigned mode, v, skipped = 0u;
    static const char *names[] = { "sub_CA38", "sub_CA1B", "sub_CA13 tail", "sub_CA9B" };
    static const uint16_t entry[] = { 0xCA38u, 0xCA1Bu, 0xCA13u, 0xCA9Bu };
    if (argc != 2) { puts("usage: test_race_frame_control <prg>"); return 2; }
    fp = fopen(argv[1], "rb");
    if (fp == NULL || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) return 2;
    fclose(fp);
    cpu6502_reset_state(&cpu);
    for (mode = 0u; mode < 4u; ++mode) {
        for (v = 0u; v < 200000u; ++v) {
            RaceFrameControlState s;
            randomize(&s);
            s.flag_03F7 = (mode == 3u && (nb() & 15u) == 0u) ? 1u : 0u;
            toRam(&s, &cpu);
            switch (mode) {
            case 0: racePauseUpdate(&s); break;
            case 1: raceHoldInput(&s); break;
            case 2: raceFrameInputStep(&s); break;
            default: raceFinishUpdate(&s); break;
            }
            if (s.unsupported) { ++skipped; continue; }
            cpu.a = cpu.x = cpu.y = 0u; cpu.sp = 0xFDu; cpu.p = 0x24u; cpu.cycles = 0u; cpu.bad_opcode = -1;
            if (cpu6502_call(&cpu, entry[mode], 100000u) != 0) { printf("FAIL %s v=%u oracle error\n", names[mode], v); return 1; }
            if (compare(names[mode], v, &s, &cpu) != 0) return 1;
        }
        printf("PASS: 200000 %s vectors match original ASM\n", names[mode]);
    }
    printf("(%u unsupported 03F7 restart vectors skipped)\n", skipped);
    return 0;
}
