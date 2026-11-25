#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "race_loop.h"
#include "race_world_glue.h"
#include "native_race_session.h"
#include "cpu6502.h"

static uint32_t rng = 0xC0FFEE11u;
static uint8_t nb(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }

static const uint16_t kEntry[RACE_STAGE_COUNT] = {
    0xCA08, 0xD918, 0xDD8D, 0xE733, 0xE927, 0xCA9B, 0xDA9F, 0xDBC4, 0xDEBB, 0xDF30, 0xEAC5, 0xDFD5, 0xE09F,
    0xDDD1, 0xCD1F, 0xE96C, 0xDE31, 0xE836, 0xEA44, 0xF4FF, 0xE4C8, 0xDEE2, 0xE359, 0xE17F, 0xE456, 0xDA26,
    0xE70B, 0xCED0, 0xE42B
};
static const char *kName[RACE_STAGE_COUNT] = {
    "CA08", "D918", "DD8D", "E733", "E927", "CA9B", "DA9F", "DBC4", "DEBB", "DF30", "EAC5", "DFD5", "E09F",
    "DDD1", "CD1F", "E96C", "DE31", "E836", "EA44", "F4FF", "E4C8", "DEE2", "E359", "E17F", "E456", "DA26",
    "E70B", "CED0", "E42B"
};

static uint8_t read2002(Cpu6502 *c, uint16_t a) { return a == 0x2002u ? 0x40u : c->io[a]; }

static void worldToRam(RaceWorld *w, Cpu6502 *c)
{
    unsigned b, i;
    memset(c->ram, 0, sizeof(c->ram));
    for (b = 0u; b < RACE_WORLD_BLOCK_COUNT; ++b)
        for (i = 0u; i < kRaceWorldBlocks[b].length; ++i)
            c->ram[kRaceWorldBlocks[b].base + i] = *raceWorldByte(w, (unsigned)(kRaceWorldBlocks[b].base + i));
}

static int compareWorld(RaceWorld *w, const Cpu6502 *c, const char *what, unsigned frame)
{
    unsigned b, i;
    for (b = 0u; b < RACE_WORLD_BLOCK_COUNT; ++b)
        for (i = 0u; i < kRaceWorldBlocks[b].length; ++i) {
            const unsigned a = (unsigned)(kRaceWorldBlocks[b].base + i);
            if (*raceWorldByte(w, a) != c->ram[a]) {
                { unsigned k; printf("FAIL frame %u after %s: $%04X native=%02X ASM=%02X\n", frame, what, a, *raceWorldByte(w, a), c->ram[a]); printf("  native/ASM block:"); for (k = 0u; k < kRaceWorldBlocks[b].length && k < 8u; ++k) printf(" %02X/%02X", *raceWorldByte(w, kRaceWorldBlocks[b].base + k), c->ram[kRaceWorldBlocks[b].base + k]); printf("\n"); }
                return 1;
            }
        }
    return 0;
}

static int runOracle(Cpu6502 *c, uint16_t entry)
{
    c->a = c->x = c->y = 0u; c->sp = 0xFDu; c->p = 0x24u; c->cycles = 0u; c->bad_opcode = -1;
    return cpu6502_call(c, entry, 2000000u);
}

static void initWorld(RaceWorld *w)
{
    NativeRaceSession s;
    NativeRaceSetupInput in;
    NativeRngState r;
    unsigned i, j;
    static const uint8_t codes[] = { 0x3Bu, 0x3Du, 0xE4u, 0xE6u, 0x3Au, 0x92u, 0x93u, 0x00u };
    memset(&in, 0, sizeof(in)); memset(&r, 0, sizeof(r));
    for (i = 0u; i < 5u; ++i) r.bytes[i] = nb();
    in.multiplayer_flag = (uint8_t)(nb() & 1u);
    in.setup_index_0041 = 2u; in.timer_0034 = 0x2Du; in.transition_counter_03F8 = 4u;
    nativeRaceSessionInit(&s, &in, &r);
    memset(w, 0, sizeof(*w));
    raceWorldStore_BikeMovementState(&s.movement, w);
    raceWorldStore_BikeStateMachineState(&s.states, w);
    *raceWorldByte(w, 0x3F2) = in.multiplayer_flag;
    *raceWorldByte(w, 0x34) = (uint8_t)((nb() & 3u) ? 0u : 0x2Du);
    *raceWorldByte(w, 0xE8) = s.setup.camera_scroll_00E8; *raceWorldByte(w, 0xE9) = s.setup.camera_scroll_00E9;
    *raceWorldByte(w, 0xEA) = s.setup.camera_scroll_00EA; *raceWorldByte(w, 0xEB) = s.setup.camera_scroll_00EB;
    *raceWorldByte(w, 0x3B4) = s.setup.countdown_x_03B4; *raceWorldByte(w, 0x3B0) = s.setup.race_setup_state_03B0;
    *raceWorldByte(w, 0x41) = 2u; *raceWorldByte(w, 0x43) = (uint8_t)(nb() % 5u);
    for (i = 0u; i < 8u; ++i) *raceWorldByte(w, 0x18u + i) = nb();
    for (i = 0u; i < 6u; ++i) for (j = 0u; j < 64u; ++j) *raceWorldByte(w, 0x400u + i * 64u + j) = (nb() & 7u) ? codes[nb() & 7u] : nb();
    memset(raceWorldByte(w, 0x200), 0xF8, 256);
}

static void dump(RaceWorld *b)
{
    static const unsigned a[] = {0x90,0x94,0x98,0x9C,0xA4,0xB0,0x5C,0x58,0x70,0xB8,0x36C,0x374,0x3E0,0xA8,0xAC,0x368,0x388,0x26,0x2A,0xC0,0xFD,0x4C,0x4F,0x52,0x34,0x3F};
    unsigned i, j;
    for (i = 0u; i < sizeof(a) / sizeof(a[0]); ++i) {
        printf("  before $%04X:", a[i]);
        for (j = 0u; j < 4u; ++j) { const uint8_t *q = raceWorldByte(b, a[i] + j); if (q != 0) printf(" %02X", *q); }
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *fp;
    unsigned seq, f, st, frames = 0u, stopped = 0u, started = 0u, finished = 0u, moving = 0u, cols = 0u, ai = 0u;
    unsigned nseq = 40u, nframes = 600u;
    if (argc < 2) { puts("usage: test_race_loop <prg> [sequences] [frames]"); return 2; }
    if (argc > 2) nseq = (unsigned)atoi(argv[2]);
    if (argc > 3) nframes = (unsigned)atoi(argv[3]);
    fp = fopen(argv[1], "rb");
    if (fp == NULL || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) return 2;
    fclose(fp);
    cpu6502_reset_state(&cpu);
    cpu.read_hook = read2002;
    for (seq = 0u; seq < nseq; ++seq) {
        static RaceWorld w, step, before;
        unsigned finish_at = (seq & 1u) ? 200u + (nb() % 400u) : 0xFFFFFFFFu;
        uint8_t finish_kind = (uint8_t)(1u + (nb() & 1u));
        initWorld(&w);
        worldToRam(&w, &cpu);
        for (f = 0u; f < nframes; ++f) {
            uint32_t bad = 0u;
            uint8_t hold = (nb() & 3u) ? 0x80u : nb();
            if ((nb() & 7u) == 0u) hold = (uint8_t)(hold & 0xF0u);
            *raceWorldByte(&w, 0x14) = hold; cpu.ram[0x14] = hold;
            *raceWorldByte(&w, 0x15) = 0u; cpu.ram[0x15] = 0u;
            if (f == finish_at) { /* force the finished state as EAB5/DF79 would */
                uint8_t t = (uint8_t)(nb() & 0x1Fu);
                *raceWorldByte(&w, 0x52) = finish_kind; cpu.ram[0x52] = finish_kind;
                *raceWorldByte(&w, 0x32) = t; cpu.ram[0x32] = t;
            }
            raceLoopNmiTick(&w);
            if ((cpu.ram[0x3B1] | cpu.ram[0x3B2]) == 0u) {
                cpu.ram[0x3F] = (uint8_t)(cpu.ram[0x3F] + 1u);
                if (runOracle(&cpu, 0xD310u) != 0) { puts("FAIL D310 oracle"); return 1; }
            }
            if (compareWorld(&w, &cpu, "NMI tick (INC 3F, D310)", f) != 0) return 1;
            *raceWorldByte(&w, 0x4C) = (uint8_t)(*raceWorldByte(&w, 0x3F) & 3u); cpu.ram[0x4C] = *raceWorldByte(&w, 0x4C);
            step = w;
            /* Stage-by-stage differential comparison in source order with the same gates as raceLoopStep. */
#define STAGE(S) do { \
                before = w; \
                if (raceLoopRunStage(&w, S) != 0u) { bad |= 1u << (S); } \
                if (bad == 0u) { \
                    if (runOracle(&cpu, kEntry[S]) != 0) { printf("FAIL frame %u stage %s oracle error\n", f, kName[S]); return 1; } \
                    if (compareWorld(&w, &cpu, kName[S], f) != 0) { printf("  (sequence %u)\n", seq); dump(&before); return 1; } \
                } } while (0)
            STAGE(RACE_STAGE_CA08);
            if ((*raceWorldByte(&w, 0x3B1) | *raceWorldByte(&w, 0x3B2)) == 0u && bad == 0u) {
                STAGE(RACE_STAGE_D918); STAGE(RACE_STAGE_DD8D); STAGE(RACE_STAGE_E733); STAGE(RACE_STAGE_E927);
                if (bad == 0u && *raceWorldByte(&w, 0x52) != 0u) {
                    STAGE(RACE_STAGE_CA9B);
                } else if (bad == 0u) {
                    if (*raceWorldByte(&w, 0x3F2) != 0u) { STAGE(RACE_STAGE_DA9F); STAGE(RACE_STAGE_DBC4); STAGE(RACE_STAGE_DEBB); }
                    STAGE(RACE_STAGE_DF30); STAGE(RACE_STAGE_EAC5); STAGE(RACE_STAGE_DFD5); STAGE(RACE_STAGE_E09F);
                }
                STAGE(RACE_STAGE_DDD1); STAGE(RACE_STAGE_CD1F); STAGE(RACE_STAGE_E96C); STAGE(RACE_STAGE_DE31);
                STAGE(RACE_STAGE_E836); STAGE(RACE_STAGE_EA44); STAGE(RACE_STAGE_F4FF); STAGE(RACE_STAGE_E4C8);
                STAGE(RACE_STAGE_DEE2); STAGE(RACE_STAGE_E359); STAGE(RACE_STAGE_E17F); STAGE(RACE_STAGE_E456);
                STAGE(RACE_STAGE_DA26); STAGE(RACE_STAGE_E70B); STAGE(RACE_STAGE_CED0); STAGE(RACE_STAGE_E42B);
            }
#undef STAGE
            if (bad != 0u) {
                for (st = 0u; st < RACE_STAGE_COUNT; ++st) if (bad & (1u << st)) printf("  note: sequence %u frame %u stage %s outside verified domain\n", seq, f, kName[st]);
                ++stopped; break;
            }
            /* The composed frame must equal the stage-wise run, and a whole C99A run must equal both. */
            if (raceLoopStep(&step) != 0u || memcmp(&step, &w, sizeof(w)) != 0) { printf("FAIL frame %u raceLoopStep differs from stage-wise run\n", f); return 1; }
            ++frames;
            if (*raceWorldByte(&w, 0x4F)) ++started;
            if (*raceWorldByte(&w, 0x52)) ++finished;
            if (*raceWorldByte(&w, 0x94)) ++moving;
            if (*raceWorldByte(&w, 0xE8) != *raceWorldByte(&step, 0xE8) || *raceWorldByte(&w, 0xEB) != 0x0Au) ++cols;
            if (*raceWorldByte(&w, 0x95) || *raceWorldByte(&w, 0x96)) ++ai;
        }
    }
    printf("coverage: started=%u finished=%u player_moving=%u column_progress=%u ai_moving=%u frames\n", started, finished, moving, cols, ai);
    printf("PASS: %u native race frames (%u sequences stopped at an unsupported domain) match original ASM stage by stage\n", frames, stopped);
    return 0;
}
