#include <stdio.h>
#include <string.h>
#include "race_tail.h"
#include "cpu6502.h"

static uint32_t rng = 0x7A11B00Bu;
static uint8_t nb(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }

#define F(X) X(timer_003A, 0x3A) X(track_finished_0052, 0x52) X(timer_0024, 0x24) X(flag_03A0, 0x3A0) \
    X(timer_002E, 0x2E) X(rng_0018, 0x18) X(tile_03A1, 0x3A1) X(x_03B9, 0x3B9) X(scroll_0060, 0x60) \
    X(frame_003F, 0x3F) X(timer_003C, 0x3C) X(state_03BC, 0x3BC) X(scroll_03BD, 0x3BD) X(scroll_03BE, 0x3BE) \
    X(blink_03BB, 0x3BB) X(timer_0020, 0x20)

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *fp;
    unsigned v, skipped = 0u, drew = 0u, banner = 0u, sign = 0u;
    if (argc != 2) return 2;
    fp = fopen(argv[1], "rb");
    if (fp == NULL || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) return 2;
    fclose(fp);
    cpu6502_reset_state(&cpu);
    for (v = 0u; v < 400000u; ++v) {
        RaceTailState s;
        uint8_t initial[256];
        unsigned i;
        memset(&s, 0, sizeof(s));
        s.timer_003A = (nb() & 3u) ? 0u : nb();
        s.track_finished_0052 = (nb() & 3u) ? 0u : (uint8_t)(nb() % 3u);
        s.timer_0024 = (nb() & 3u) ? 0u : nb();
        s.flag_03A0 = (nb() & 1u) ? 0u : (uint8_t)(0xF8u - (nb() & 0x7Fu));
        s.timer_002E = (nb() & 1u) ? 0u : nb();
        s.rng_0018 = nb();
        s.tile_03A1 = (nb() & 1u) ? (uint8_t)(0xCFu + 4u * (nb() & 3u)) : nb();
        s.x_03B9 = (nb() & 3u) ? (uint8_t)(nb() % 0xC0u) : nb();
        s.scroll_0060 = (nb() & 3u) ? (uint8_t)(nb() & 7u) : nb();
        s.frame_003F = (nb() & 3u) ? (uint8_t)(nb() & 0xF0u) : nb();
        s.timer_003C = (nb() & 3u) ? 0u : (uint8_t)(nb() & 7u);
        s.state_03BC = (nb() & 15u) ? (uint8_t)(nb() & 1u) : nb();
        s.scroll_03BD = nb();
        s.scroll_03BE = (nb() & 1u) ? 1u : (uint8_t)(nb() & 3u);
        s.blink_03BB = (uint8_t)(nb() & 1u);
        s.timer_0020 = (nb() & 1u) ? 0u : (uint8_t)(nb() % 11u);
        for (i = 0u; i < 256u; ++i) s.oam[i] = nb();
        memset(cpu.ram, 0, sizeof(cpu.ram));
#define W(Fd, B) cpu.ram[B] = s.Fd;
        F(W)
#undef W
        memcpy(&cpu.ram[0x200], s.oam, 256);
        memcpy(initial, s.oam, 256);
        raceFrameTail(&s);
        if (s.unsupported) { ++skipped; continue; }
        cpu.a = cpu.x = cpu.y = 0u; cpu.sp = 0xFDu; cpu.p = 0x24u; cpu.cycles = 0u; cpu.bad_opcode = -1;
        if (cpu6502_call(&cpu, 0xDA44u, 100000u) != 0) { printf("FAIL v=%u oracle error\n", v); return 1; }
#define C(Fd, B) if (s.Fd != cpu.ram[B]) { printf("FAIL v=%u %s native=%02X ASM=%02X\n", v, #Fd, s.Fd, cpu.ram[B]); return 1; }
        F(C)
#undef C
        for (i = 0u; i < 256u; ++i) if (s.oam[i] != cpu.ram[0x200 + i]) { printf("FAIL v=%u oam[%02X] native=%02X ASM=%02X\n", v, i, s.oam[i], cpu.ram[0x200 + i]); return 1; }
        if (memcmp(s.oam, initial, 256) != 0) ++drew;
        if (s.timer_003C == 0u) { if (s.state_03BC == 0u) ++banner; else ++sign; }
    }
    printf("PASS: %u DA44 tail vectors match original ASM (%u unsupported skipped; %u with OAM writes; banner=%u sign=%u)\n", 400000u - skipped, skipped, drew, banner, sign);
    return drew < 10000u;
}
