/*
 * Differential test: raceResultsPhaseStep (sub_CB2C handlers CB3B/CB77/CBA4/
 * CC6B with CC15/CC7D/CCCA) against the original code on the 6502 oracle.
 * Random worlds are compared byte for byte (all modelled RAM, OAM included).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "race_results.h"
#include "race_world.h"
#include "cpu6502.h"

static uint32_t rng = 0xBADC0DE5u;
static uint8_t nb(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }
#define W(a) (*raceWorldByte(&w, (a)))

static void worldToRam(RaceWorld *w, Cpu6502 *c)
{
    unsigned b, i;
    memset(c->ram, 0, sizeof(c->ram));
    for (b = 0u; b < RACE_WORLD_BLOCK_COUNT; ++b)
        for (i = 0u; i < kRaceWorldBlocks[b].length; ++i)
            c->ram[kRaceWorldBlocks[b].base + i] = *raceWorldByte(w, (unsigned)(kRaceWorldBlocks[b].base + i));
}

static int compareWorld(RaceWorld *w, const Cpu6502 *c, unsigned iter)
{
    unsigned b, i;
    for (b = 0u; b < RACE_WORLD_BLOCK_COUNT; ++b)
        for (i = 0u; i < kRaceWorldBlocks[b].length; ++i) {
            const unsigned a = (unsigned)(kRaceWorldBlocks[b].base + i);
            if (a == 0x4015u) continue;
            if (*raceWorldByte(w, a) != c->ram[a]) {
                printf("FAIL iter %u: $%04X native=%02X ASM=%02X\n", iter, a, *raceWorldByte(w, a), c->ram[a]);
                return 1;
            }
        }
    return 0;
}

static void randomWorld(RaceWorld *w0, unsigned phase)
{
    RaceWorld w;
    unsigned b, i;
    uint8_t wait;
    for (b = 0u; b < RACE_WORLD_BLOCK_COUNT; ++b)
        for (i = 0u; i < kRaceWorldBlocks[b].length; ++i)
            *raceWorldByte(&w, (unsigned)(kRaceWorldBlocks[b].base + i)) = nb();
    W(0x44) = (uint8_t)phase;
    W(0x300) = (uint8_t)(nb() % 0x28u);
    W(0x41) = (uint8_t)(nb() % 14u);
    W(0x4B) = (uint8_t)(nb() & 1u);
    W(0x46) = (uint8_t)(nb() & 1u);
    W(0x52) = (uint8_t)(nb() % 3u);
    W(0x53) = (uint8_t)(nb() % 7u);
    W(0x43) = (uint8_t)(nb() % 6u);
    W(0x3F3) = (uint8_t)(nb() % 4u);
    W(0x3F8) = (uint8_t)(nb() % 6u);
    W(0x3F) = (uint8_t)((nb() & 1u) ? 0u : nb());
    W(0x20) = (uint8_t)((nb() & 3u) ? 0u : (nb() % 10u));
    W(0x2F) = (uint8_t)((nb() & 1u) ? 0u : (nb() % 12u));
    W(0x56) = (uint8_t)((nb() & 3u) ? 0u : 3u);
    W(0x55) = (uint8_t)(0x20u + nb() % 64u);
    W(0x3F1) = (uint8_t)(0x20u + nb() % 64u);
    W(0x3AD) = (uint8_t)((nb() & 1u) ? 0x20u : 0x31u);
    wait = W(0x3AD);
    switch (nb() % 6u) {
    case 0: W(0x33) = 0u; break;
    case 1: W(0x33) = wait; break;
    case 2: W(0x33) = (uint8_t)(wait - 0x10u); break;
    case 3: W(0x33) = (uint8_t)(wait + 1u + nb() % 8u); break;
    case 4: W(0x33) = (uint8_t)(1u + nb() % 0x30u); break;
    default: W(0x33) = (uint8_t)(wait - 0x10u + (nb() & 1u)); break;
    }
    if (nb() % 5u == 0u) W(0x14) = 0u;
    for (i = 0u; i < 7u; ++i) W(0x3D9 + i) = (uint8_t)(nb() % 10u);
    *w0 = w;
}

static void randomEnterWorld(RaceWorld *w0)
{
    RaceWorld w;
    unsigned b, i;
    for (b = 0u; b < RACE_WORLD_BLOCK_COUNT; ++b)
        for (i = 0u; i < kRaceWorldBlocks[b].length; ++i)
            *raceWorldByte(&w, (unsigned)(kRaceWorldBlocks[b].base + i)) = nb();
    W(0x41) = (uint8_t)((nb() & 3u) ? 3u : nb() % 14u);
    W(0x43) = (uint8_t)(nb() % 6u);
    W(0x46) = (uint8_t)(nb() & 1u);
    W(0x3F2) = (uint8_t)((nb() % 5u) == 0u ? 1u : 0u);
    W(0x3F8) = (uint8_t)(nb() % 9u);
    W(0x68) = (uint8_t)(nb() % 4u);
    W(0x69) = (uint8_t)(nb() % 0x40u);
    W(0x6A) = (uint8_t)(nb() % 0x40u);
    for (i = 0u; i < RACE_RECORD_BYTES; ++i)
        W(0x580 + i) = (uint8_t)((nb() & 3u) ? (i % 3u == 0u ? nb() % 4u : nb() % 0x40u) : 0xFFu);
    /* make equal / near records likely */
    if ((nb() & 3u) == 0u) {
        unsigned y = (unsigned)(W(0x43) % 5u) * 3u;
        if (W(0x46)) y |= 0x10u;
        if (W(0x3F2)) y += 0x23u;
        if (y + 3u <= RACE_RECORD_BYTES) { W(0x580 + y) = W(0x68); W(0x581 + y) = W(0x69); W(0x582 + y) = (uint8_t)(W(0x6A) + (nb() % 3u) - 1u); }
    }
    *w0 = w;
}

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    static RaceWorld w, before;
    FILE *fp;
    unsigned it, n = 200000u, covered[4] = {0, 0, 0, 0}, unsupported = 0u;
    if (argc < 2) { puts("usage: test_race_results <prg> [iterations]"); return 2; }
    if (argc > 2) n = (unsigned)atoi(argv[2]);
    fp = fopen(argv[1], "rb");
    if (fp == NULL || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) return 2;
    fclose(fp);
    cpu6502_reset_state(&cpu);
    {
        uint8_t def[RACE_RECORD_BYTES];
        memset(cpu.ram, 0, sizeof(cpu.ram));
        cpu.a = cpu.x = cpu.y = 0u; cpu.sp = 0xFDu; cpu.p = 0x24u; cpu.cycles = 0u; cpu.bad_opcode = -1;
        if (cpu6502_call(&cpu, 0xC1FDu, 200000u) != 0) { puts("FAIL oracle C1FD"); return 1; }
        raceRecordsDefault(def);
        if (memcmp(def, &cpu.ram[0x580], RACE_RECORD_BYTES) != 0) { puts("FAIL raceRecordsDefault != C1FD/C229"); return 1; }
        puts("PASS: raceRecordsDefault matches C1FD/C229");
    }
    for (it = 0u; it < n; ++it) {
        const unsigned phase = nb() & 3u;
        randomWorld(&w, phase);
        before = w;
        worldToRam(&w, &cpu);
        cpu.a = cpu.x = cpu.y = 0u; cpu.sp = 0xFDu; cpu.p = 0x24u; cpu.cycles = 0u; cpu.bad_opcode = -1;
        if (cpu6502_call(&cpu, 0xCB2Cu, 2000000u) != 0) { puts("FAIL oracle run"); return 1; }
        if (raceResultsPhaseStep(&w) != 0u) { ++unsupported; continue; }
        if (compareWorld(&w, &cpu, it) != 0) {
            printf("  phase %u: $33=%02X $3AD=%02X $20=%02X $4B=%02X $14=%02X $52=%02X $53=%02X $56=%02X $2F=%02X $41=%02X\n",
                   phase, before.b0020[0x13], before.b03AA[3], before.b0020[0], before.b004B[0],
                   before.b0014[0], before.b0052[0], before.b0053[0], before.b0056[0], before.b0020[0x0F], before.b0041[0]);
            return 1;
        }
        ++covered[phase];
    }
    {
        unsigned ok = 0u, uns = 0u, ranks[256];
        memset(ranks, 0, sizeof(ranks));
        for (it = 0u; it < n; ++it) {
            randomEnterWorld(&w);
            before = w;
            worldToRam(&w, &cpu);
            cpu.a = cpu.x = cpu.y = 0u; cpu.sp = 0xFDu; cpu.p = 0x24u; cpu.cycles = 0u; cpu.bad_opcode = -1;
            if (cpu6502_call(&cpu, 0xC485u, 2000000u) != 0) { puts("FAIL oracle enter run"); return 1; }
            if (raceResultsEnter(&w) != 0u) { ++uns; continue; }
            if (compareWorld(&w, &cpu, it) != 0) {
                printf("  enter: $41=%02X $43=%02X $46=%02X $3F2=%02X $3F8=%02X cur=%02X%02X%02X\n",
                       *raceWorldByte(&before,0x41), *raceWorldByte(&before,0x43), *raceWorldByte(&before,0x46), *raceWorldByte(&before,0x3F2), *raceWorldByte(&before,0x3F8),
                       *raceWorldByte(&before,0x68), *raceWorldByte(&before,0x69), *raceWorldByte(&before,0x6A));
                return 1;
            }
            ++ranks[W(0x53)]; ++ok;
        }
        printf("enter: %u matched (unsupported=%u) rank1=%u rank2=%u rank3+=%u\n", ok, uns, ranks[1], ranks[2], ok - ranks[1] - ranks[2]);
    }
    printf("coverage: CB3B=%u CB77=%u CBA4=%u CC6B=%u (unsupported=%u)\n", covered[0], covered[1], covered[2], covered[3], unsupported);
    printf("PASS: %u results-phase steps match the original code\n", it - unsupported);
    return 0;
}
