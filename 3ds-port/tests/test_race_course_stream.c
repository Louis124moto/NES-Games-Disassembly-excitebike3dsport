#include <stdio.h>
#include <string.h>
#include "race_course_stream.h"
#include "cpu6502.h"

static uint32_t rng = 0x51C0A7E3u;
static uint8_t nb(void) { rng = rng * 1664525u + 1013904223u; return (uint8_t)(rng >> 24); }

#define S1(X) X(stream_index, 0x43) X(course_id, 0x41) X(flag_0046, 0x46) X(next_column_00EB, 0xEB) \
    X(repeat_count_00EC, 0xEC) X(stream_offset_00ED, 0xED) X(template_00EE, 0xEE) X(template_offset_00EF, 0xEF) \
    X(column_00E8, 0xE8) X(row_00E9, 0xE9) X(nametable_00EA, 0xEA) X(counter_0057, 0x57) X(flag_03A2, 0x3A2) \
    X(counter_03A4, 0x3A4) X(counter_03A5, 0x3A5) X(flag_03BC, 0x3BC)

static void toRam(const RaceCourseStreamState *s, const RaceSpawnState *p, uint8_t *r)
{
    unsigned i;
    memset(r, 0, 0x800u);
#define W1(F, B) r[B] = s->F;
    S1(W1)
#undef W1
    r[0xE0] = p->object_class[0];
    r[0x3A8] = p->spawn_request_03A8;
    for (i = 0u; i < 0x12u; ++i) r[0x300 + i] = s->buffer[i];
    for (i = 0u; i < 6u * 64u; ++i) r[0x400 + i] = p->track_pages[i >> 6][i & 63u];
}

static int compare(const char *w, unsigned v, const RaceCourseStreamState *s, const RaceSpawnState *p, const uint8_t *r)
{
    unsigned i;
#define C1(F, B) if (s->F != r[B]) { printf("FAIL %s v=%u %s native=%02X ASM=%02X\n", w, v, #F, s->F, r[B]); return 1; }
    S1(C1)
#undef C1
    if (p->spawn_request_03A8 != r[0x3A8]) { printf("FAIL %s v=%u spawn_request_03A8 native=%02X ASM=%02X\n", w, v, p->spawn_request_03A8, r[0x3A8]); return 1; }
    for (i = 0u; i < 0x12u; ++i) if (s->buffer[i] != r[0x300 + i]) { printf("FAIL %s v=%u buffer[%02X] native=%02X ASM=%02X\n", w, v, i, s->buffer[i], r[0x300 + i]); return 1; }
    for (i = 0u; i < 6u * 64u; ++i) if (p->track_pages[i >> 6][i & 63u] != r[0x400 + i]) { printf("FAIL %s v=%u page[%u][%u] native=%02X ASM=%02X\n", w, v, i >> 6, i & 63u, p->track_pages[i >> 6][i & 63u], r[0x400 + i]); return 1; }
    return 0;
}

static void randomPages(RaceSpawnState *p)
{
    unsigned i;
    for (i = 0u; i < 6u * 64u; ++i) p->track_pages[i >> 6][i & 63u] = nb();
}

static void randomize(RaceCourseStreamState *s, RaceSpawnState *p)
{
    unsigned i;
    memset(s, 0, sizeof(*s));
    memset(p, 0, sizeof(*p));
    randomPages(p);
    s->stream_index = (nb() & 15u) ? (uint8_t)(nb() % 5u) : nb();
    s->course_id = (nb() & 1u) ? 9u : (uint8_t)(nb() % 10u);
    s->flag_0046 = (uint8_t)(nb() & 1u);
    p->object_class[0] = (uint8_t)(nb() & 0x3Fu);
    s->next_column_00EB = (nb() & 3u) ? p->object_class[0] : (uint8_t)(nb() & 0x3Fu);
    s->repeat_count_00EC = (nb() & 1u) ? 0u : (uint8_t)(nb() & 7u);
    s->stream_offset_00ED = (nb() & 7u) ? (uint8_t)(nb() % 0x60u) : nb();
    s->template_00EE = (nb() & 15u) ? (uint8_t)(nb() % 0x24u) : (uint8_t)(nb() & 0x3Fu);
    s->template_offset_00EF = (nb() & 1u) ? 0u : (uint8_t)(nb() % 0x60u);
    s->column_00E8 = (uint8_t)(nb() & 0x3Fu);
    s->row_00E9 = (nb() & 7u) ? (uint8_t)(0x18u + nb() % 8u) : (uint8_t)(nb() & 0x1Fu);
    s->nametable_00EA = (uint8_t)((nb() & 1u) ? 0x20u : 0x24u);
    s->counter_0057 = (uint8_t)(nb() & 3u);
    s->flag_03A2 = (uint8_t)(nb() & 1u);
    s->counter_03A4 = (uint8_t)(nb() & 7u);
    s->counter_03A5 = (uint8_t)(nb() % 8u);
    s->flag_03BC = (uint8_t)(nb() & 1u);
    p->spawn_request_03A8 = nb();
    for (i = 0u; i < 0x12u; ++i) s->buffer[i] = (nb() & 1u) ? 0xFCu : nb();
}

static int callOracle(Cpu6502 *cpu, unsigned v, const char *w)
{
    cpu->a = 0u; cpu->x = 0u; cpu->y = 0u; cpu->sp = 0xFDu; cpu->p = 0x24u; cpu->cycles = 0u; cpu->bad_opcode = -1;
    if (cpu6502_call(cpu, 0xF4FFu, 2000000u) != 0) { printf("FAIL %s v=%u oracle error\n", w, v); return 1; }
    return 0;
}

static int runRandom(Cpu6502 *cpu, unsigned count)
{
    unsigned v, skipped = 0u, hit_template = 0u;
    for (v = 0u; v < count; ++v) {
        RaceCourseStreamState s;
        RaceSpawnState p;
        randomize(&s, &p);
        toRam(&s, &p, cpu->ram);
        raceCourseStreamUpdate(&s, &p);
        if (s.unsupported) { ++skipped; continue; }
        if (callOracle(cpu, v, "sub_F4FF") != 0) return 1;
        if (compare("sub_F4FF", v, &s, &p, cpu->ram) != 0) return 1;
        if (s.buffer[3] == 0x8Eu) ++hit_template;
    }
    printf("PASS: %u sub_F4FF random vectors match original ASM (%u unsupported skipped, %u column writes)\n", count - skipped, skipped, hit_template);
    return hit_template < 1000u;
}

/* Frame-by-frame play through every stream from the real start state. */
static int runSequences(Cpu6502 *cpu)
{
    unsigned stream, course, f, calls = 0u, writes = 0u;
    for (stream = 0u; stream < 5u; ++stream) {
        for (course = 0u; course < 10u; ++course) {
            RaceCourseStreamState s;
            RaceSpawnState p;
            memset(&s, 0, sizeof(s));
            memset(&p, 0, sizeof(p));
            randomPages(&p);
            s.stream_index = (uint8_t)stream;
            s.course_id = (uint8_t)course;
            s.flag_0046 = (uint8_t)(nb() & 1u);
            for (f = 0u; f < 3000u; ++f) {
                const unsigned v = (stream * 10u + course) * 3000u + f;
                if (nb() & 7u) p.object_class[0] = s.next_column_00EB;
                else p.object_class[0] = (uint8_t)(nb() & 0x3Fu);
                if ((nb() & 63u) == 0u) s.flag_0046 ^= 1u;
                toRam(&s, &p, cpu->ram);
                raceCourseStreamUpdate(&s, &p);
                if (s.unsupported) { printf("FAIL sequence stream=%u course=%u frame=%u unexpectedly unsupported\n", stream, course, f); return 1; }
                if (callOracle(cpu, v, "sub_F4FF sequence") != 0) return 1;
                if (compare("sub_F4FF sequence", v, &s, &p, cpu->ram) != 0) return 1;
                ++calls;
                if (s.buffer[3] == 0x8Eu) ++writes;
            }
        }
    }
    printf("PASS: %u sub_F4FF sequential frames match original ASM (%u column writes)\n", calls, writes);
    return 0;
}

int main(int argc, char **argv)
{
    static Cpu6502 cpu;
    FILE *fp;
    if (argc != 2) { puts("usage: test_race_course_stream <prg>"); return 2; }
    fp = fopen(argv[1], "rb");
    if (fp == NULL || fread(cpu.prg, 1u, sizeof(cpu.prg), fp) != sizeof(cpu.prg)) return 2;
    fclose(fp);
    cpu6502_reset_state(&cpu);
    if (runRandom(&cpu, 300000u) != 0) return 1;
    return runSequences(&cpu);
}
