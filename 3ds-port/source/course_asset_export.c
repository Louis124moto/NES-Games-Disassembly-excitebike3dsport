#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "course_asset_export.h"
#include "race_course_stream.h"
#include "race_course_stream_data.h"

#define COURSE_MAX_COLUMNS 4096u
#define COURSE_MAX_FRAMES 65536u
#define COURSE_HEADER_BYTES 16u
#define COURSE_RECORD_BYTES 43u

static uint8_t romRead(unsigned address)
{
    if (address < COURSE_STREAM_ROM_BASE ||
        address >= COURSE_STREAM_ROM_BASE + COURSE_STREAM_ROM_SIZE) return 0u;
    return kCourseStreamRom[address - COURSE_STREAM_ROM_BASE];
}

static uint16_t streamAddress(unsigned index)
{
    return (uint16_t)(romRead(0xED3Au + index) |
                      ((uint16_t)romRead(0xED40u + index) << 8u));
}

static uint16_t templateAddress(unsigned index)
{
    return (uint16_t)(romRead(0xF063u + index) |
                      ((uint16_t)romRead(0xF087u + index) << 8u));
}

static void put16(uint8_t *out, uint16_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8u);
}

static void put32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)value;
    out[1] = (uint8_t)(value >> 8u);
    out[2] = (uint8_t)(value >> 16u);
    out[3] = (uint8_t)(value >> 24u);
}

static int writeColumnRecord(FILE *file, const RaceCourseStreamState *stream,
                             const RaceSpawnState *spawn, uint8_t courseClass,
                             uint8_t oldColumn, uint8_t oldRow,
                             uint8_t oldNametable, uint8_t oldStreamOffset)
{
    uint8_t record[COURSE_RECORD_BYTES];
    size_t i, at = 0u;
    const uint16_t base = streamAddress(0u);
    const uint16_t tmpl = templateAddress(stream->template_00EE);
    for (i = 0u; i < 6u; ++i) record[at++] = spawn->track_pages[i][oldColumn];
    record[at++] = courseClass;
    record[at++] = oldColumn;
    record[at++] = oldRow;
    record[at++] = oldNametable;
    record[at++] = oldStreamOffset;
    record[at++] = stream->stream_offset_00ED;
    record[at++] = stream->template_00EE;
    record[at++] = stream->template_offset_00EF;
    record[at++] = stream->next_column_00EB;
    record[at++] = stream->counter_0057;
    record[at++] = stream->counter_03A4;
    record[at++] = stream->counter_03A5;
    record[at++] = stream->flag_03A2;
    record[at++] = stream->flag_03BC;
    record[at++] = spawn->spawn_request_03A8;
    put16(record + at, (uint16_t)(base + oldStreamOffset)); at += 2u;
    put16(record + at, tmpl); at += 2u;
    memcpy(record + at, stream->buffer, sizeof(stream->buffer)); at += sizeof(stream->buffer);
    return at == sizeof(record) && fwrite(record, 1u, sizeof(record), file) == sizeof(record);
}

int courseAssetExportWrite(const char *directory, char *status, size_t statusCapacity)
{
    char path[512];
    struct stat st;
    FILE *file;
    uint8_t header[COURSE_HEADER_BYTES] = {
        'E','B','C','G', 1u,0u, COURSE_HEADER_BYTES,0u,
        0u,0u,0u,0u, 6u, COURSE_RECORD_BYTES, 0u,0u
    };
    RaceCourseStreamState stream;
    RaceSpawnState spawn;
    unsigned frame, page, emitted = 0u;
    int complete = 0, ok = 1;

    if (status != NULL && statusCapacity > 0u) status[0] = '\0';
    if (directory == NULL) goto fail;
    if (stat(directory, &st) != 0 && mkdir(directory, 0777) != 0 && errno != EEXIST)
        goto fail;
    if (snprintf(path, sizeof(path), "%s/course1_geometry.ebc", directory) <= 0 ||
        strlen(path) >= sizeof(path)) goto fail;

    memset(&stream, 0, sizeof(stream));
    memset(&spawn, 0, sizeof(spawn));
    for (page = 0u; page < 6u; ++page)
        memset(spawn.track_pages[page], page < 2u ? 0x3Bu : 0x3Du, 64u);
    stream.stream_index = 0u;
    stream.course_id = 2u;
    stream.column_00E8 = 0x34u;
    stream.row_00E9 = 0x14u;
    stream.nametable_00EA = 0x25u;
    stream.next_column_00EB = 0x0Au;

    file = fopen(path, "wb+");
    if (file == NULL) goto fail;
    if (fwrite(header, 1u, sizeof(header), file) != sizeof(header)) ok = 0;

    for (frame = 0u; ok && frame < COURSE_MAX_FRAMES && emitted < COURSE_MAX_COLUMNS; ++frame) {
        const uint8_t courseClass = stream.next_column_00EB;
        const uint8_t oldColumn = stream.column_00E8;
        const uint8_t oldRow = stream.row_00E9;
        const uint8_t oldNametable = stream.nametable_00EA;
        const uint8_t oldOffset = stream.stream_offset_00ED;
        spawn.object_class[0] = courseClass;
        raceCourseStreamUpdate(&stream, &spawn);
        if (stream.unsupported != 0u) { ok = 0; break; }
        if (stream.column_00E8 != oldColumn || stream.row_00E9 != oldRow ||
            stream.nametable_00EA != oldNametable) {
            if (!writeColumnRecord(file, &stream, &spawn, courseClass,
                                   oldColumn, oldRow, oldNametable, oldOffset)) {
                ok = 0; break;
            }
            ++emitted;
        }
        if (stream.flag_03BC != 0u && stream.counter_0057 == 0u) {
            complete = 1;
            break;
        }
    }
    if (emitted == 0u || emitted >= COURSE_MAX_COLUMNS || !complete) ok = 0;
    if (ok) {
        if (fseek(file, 8L, SEEK_SET) != 0) ok = 0;
        put32(header, emitted);
        if (ok && fwrite(header, 1u, 4u, file) != 4u) ok = 0;
    }
    if (fclose(file) != 0) ok = 0;
    if (!ok) { (void)remove(path); goto fail; }
    if (status != NULL && statusCapacity > 0u)
        (void)snprintf(status, statusCapacity, "First course exported: %u columns (EBCG v1).", emitted);
    return 1;

fail:
    if (status != NULL && statusCapacity > 0u)
        (void)snprintf(status, statusCapacity, "First-course export failed or source stream is incomplete.");
    return 0;
}
