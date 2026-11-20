#include "race_course_stream.h"
#include "race_course_stream_data.h"

#define LOOP_GUARD 8192u

static uint8_t romRead(RaceCourseStreamState *s, unsigned addr)
{
    if (addr < COURSE_STREAM_ROM_BASE || addr >= COURSE_STREAM_ROM_BASE + COURSE_STREAM_ROM_SIZE) {
        s->unsupported = 1u;
        return 0u;
    }
    return kCourseStreamRom[addr - COURSE_STREAM_ROM_BASE];
}

static uint8_t ramRead(RaceCourseStreamState *s, const RaceSpawnState *spawn, unsigned addr)
{
    if (addr >= 0x300u && addr <= 0x311u) return s->buffer[addr - 0x300u];
    if (addr >= 0x400u && addr <= 0x57Fu) return spawn->track_pages[(addr - 0x400u) >> 6][addr & 0x3Fu];
    s->unsupported = 1u;
    return 0u;
}

/* sub_ECBF */
static uint8_t incrementMod40(uint8_t a)
{
    a = (uint8_t)(a + 1u);
    return a == 0x40u ? 0u : a;
}

/* sub_F64E: copy the buffer into the six course pages at column `column`. */
static void writeColumn(RaceCourseStreamState *s, RaceSpawnState *spawn, uint8_t column)
{
    unsigned x = 5u;
    uint8_t counter = 0x0Du;

    if (column >= 0x40u) { s->unsupported = 1u; return; }
    for (;;) {
        unsigned page = x;
        uint8_t a;
        for (;;) {
            a = ramRead(s, spawn, 0x304u + counter);
            if (s->unsupported) return;
            if (a != 0xFCu) break;
            counter = (uint8_t)(counter - 1u);
            a = counter;
            if ((a & 0x80u) != 0u) break; /* BPL not taken: A keeps the counter */
        }
        spawn->track_pages[page][column] = a;
        counter = (uint8_t)(counter - 1u);
        if (x == 0u) break;
        --x;
    }
}

void raceCourseStreamUpdate(RaceCourseStreamState *s, RaceSpawnState *spawn)
{
    unsigned stream = 0u;
    unsigned tmpl = 0u;
    unsigned y;
    unsigned x;
    uint8_t a;
    unsigned guard = 0u;

    s->unsupported = 0u;
    stream = romRead(s, 0xED3Au + s->stream_index) | ((unsigned)romRead(s, 0xED40u + s->stream_index) << 8);
    if (s->unsupported) return;

    if (s->next_column_00EB != spawn->object_class[0]) return;
    if (s->repeat_count_00EC != 0u) goto l_F591;
    if (s->template_offset_00EF != 0u) goto l_F597;

l_F51A:
    if (++guard > LOOP_GUARD) { s->unsupported = 1u; return; }
    y = s->stream_offset_00ED;
    if (y == 0u) {
        a = romRead(s, stream + y);
        if (s->unsupported) return;
        s->counter_0057 = a;
        s->counter_03A4 = (uint8_t)(s->counter_03A4 + 1u);
    }
l_F525:
    if (s->course_id == 9u && s->flag_03BC != 0u) {
        s->template_00EE = 0u;
        s->stream_offset_00ED = 0u;
        a = 0x7Fu;
        goto l_F58D;
    }
    s->stream_offset_00ED = (uint8_t)(s->stream_offset_00ED + 1u);
    y = s->stream_offset_00ED;
    a = romRead(s, stream + y);
    if (s->unsupported) return;
    if (a == 0u) return;
    if ((a & 0x80u) != 0u && s->flag_0046 == 0u) goto l_F51A;
    /* bra_F54B */
    if ((a & 0x40u) == 0u) goto l_F576;
    x = a & 0x0Fu;
    s->template_00EE = romRead(s, 0xF4C6u + x);
    if (s->unsupported) return;
    s->stream_offset_00ED = (uint8_t)(s->stream_offset_00ED + 1u);
    y = (y + 1u) & 0xFFu;
    if (s->course_id == 9u && y == 2u) {
        s->counter_0057 = 1u;
        a = romRead(s, stream + y);
        if (s->unsupported) return;
        a = (uint8_t)(a - 0x3Du);
        if (a != 0u) goto l_F58D;
    }
    a = romRead(s, stream + y);
    if (s->unsupported) return;
    if (a != 0u) goto l_F58D;
l_F576:
    x = 1u;
    a &= 0x3Fu;
    s->template_00EE = a;
    if (a < 0x30u) goto l_F597;
    if (a == 0x30u && s->flag_03BC == 0u) {
        /* X stays 1 */
    } else {
        x = 0u;
    }
    spawn->spawn_request_03A8 = (uint8_t)x;
    goto l_F51A;
l_F58D:
    s->repeat_count_00EC = a & 0x7Fu;
l_F591:
    s->template_offset_00EF = 0u;
    s->repeat_count_00EC = (uint8_t)(s->repeat_count_00EC - 1u);
l_F597:
    x = s->template_00EE;
    tmpl = romRead(s, 0xF063u + x) | ((unsigned)romRead(s, 0xF087u + x) << 8);
    if (s->unsupported) return;
    y = s->template_offset_00EF;
    a = romRead(s, tmpl + y);
    if (s->unsupported) return;
    if (a == 0u) {
        s->template_offset_00EF = 0u;
        if (s->template_00EE != 9u) goto l_F51A;
        /* bra_F638 */
        s->counter_03A4 = (uint8_t)(s->counter_03A4 + 1u);
        s->counter_0057 = (uint8_t)(s->counter_0057 - 1u);
        if (s->counter_0057 == 0u) s->flag_03BC = 1u;
        s->stream_offset_00ED = 0u;
        s->counter_03A5 = 0u;
        goto l_F525;
    }
    /* sub_F676 */
    for (x = 1u; x <= 10u; ++x) s->buffer[3u + x] = 0xFCu;
    /* sub_F681 with X = template byte */
    x = a;
    if (x > 0x0Eu) { s->unsupported = 1u; return; }
    do {
        y = (y + 1u) & 0xFFu;
        a = romRead(s, tmpl + y);
        if (s->unsupported) return;
        s->buffer[3u + x] = a;
        ++x;
    } while (x != 0x0Fu);
    a = s->template_00EE;
    if (a == 0u || a == 0x18u || a == 0x1Au) {
        s->flag_03A2 ^= 1u;
        if (s->flag_03A2 == 0u) s->buffer[0x0Au] = 0x83u;
    }
    y = (y + 1u) & 0xFFu;
    s->template_offset_00EF = (uint8_t)y;
    if (s->template_00EE == 9u && s->counter_0057 != 1u && y >= 0x25u && y < 0x58u) {
        x = s->counter_03A5;
        a = romRead(s, 0xF4CDu + x);
        if (s->unsupported) return;
        s->buffer[0x10u] = a;
        s->counter_03A5 = (uint8_t)(s->counter_03A5 + 1u);
        if (x == 5u) s->buffer[0x10u] = s->counter_03A4;
    }
    s->buffer[0x03u] = 0x8Eu;
    s->buffer[0x01u] = s->nametable_00EA;
    s->buffer[0x02u] = s->row_00E9;
    writeColumn(s, spawn, s->column_00E8);
    if (s->unsupported) return;
    s->column_00E8 = incrementMod40(s->column_00E8);
    s->row_00E9 = (uint8_t)(s->row_00E9 + 1u);
    if (s->row_00E9 == 0x20u) {
        s->row_00E9 = 0u;
        s->nametable_00EA ^= 4u;
    }
    s->next_column_00EB = incrementMod40(spawn->object_class[0]);
}
