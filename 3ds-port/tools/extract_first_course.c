#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "race_course_stream.h"
#include "race_course_stream_data.h"

#define MAX_FRAMES 65536u
#define INITIAL_CLASS 0x0Au

static uint8_t read_rom(unsigned address)
{
    if (address < COURSE_STREAM_ROM_BASE ||
        address >= COURSE_STREAM_ROM_BASE + COURSE_STREAM_ROM_SIZE) return 0u;
    return kCourseStreamRom[address - COURSE_STREAM_ROM_BASE];
}

static uint16_t stream_address(unsigned index)
{
    return (uint16_t)(read_rom(0xED3Au + index) |
                      ((uint16_t)read_rom(0xED40u + index) << 8));
}

static uint16_t template_address(unsigned index)
{
    return (uint16_t)(read_rom(0xF063u + index) |
                      ((uint16_t)read_rom(0xF087u + index) << 8));
}

static const char *tile_semantic(uint8_t value)
{
    if (value >= 0x3Bu && value <= 0x3Du) return "road";
    if (value == 0xE4u) return "E4_empty_or_no_spawn_marker";
    if (value == 0xE6u) return "E6_special_vertical_marker";
    if (value == 0xC0u || value == 0xC1u ||
        (value >= 0x70u && value <= 0x73u)) return "AI_terrain_event_code";
    if (value >= 0x48u && value <= 0x4Au) return "AI_approach_target_code";
    if (value == 0xFAu) return "terrain_scan_sentinel";
    if (value == 0xFCu) return "stream_padding";
    return "other_source_tile_code";
}

static unsigned parse_limit(int argc, char **argv)
{
    unsigned value = 512u;
    if (argc > 1) {
        char *end = 0;
        unsigned long parsed = strtoul(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || parsed == 0u || parsed > 4096u)
            return 0u;
        value = (unsigned)parsed;
    }
    return value;
}

int main(int argc, char **argv)
{
    RaceCourseStreamState stream;
    RaceSpawnState spawn;
    const unsigned max_columns = parse_limit(argc, argv);
    const uint16_t stream_base = stream_address(0u);
    const uint16_t next_stream_base = stream_address(1u);
    unsigned page, frame, emitted = 0u;
    int complete = 0, truncated = 0;

    if (max_columns == 0u || stream_base == 0u || next_stream_base <= stream_base) {
        fputs("invalid extractor arguments or source stream pointer\n", stderr);
        return 2;
    }
    memset(&stream, 0, sizeof(stream));
    memset(&spawn, 0, sizeof(spawn));
    for (page = 0u; page < 6u; ++page)
        memset(spawn.track_pages[page], page < 2u ? 0x3Bu : 0x3Du, 64u);

    /* Exact C820 + C875 first-course stream state. C875 sets E8/E9; C820
     * supplies EA/EB and the six CPU-side track pages. */
    stream.stream_index = 0u;       /* ram_0043: tbl_ED3A index 0 */
    stream.course_id = 2u;          /* ram_0041: first-course setup entry */
    stream.column_00E8 = 0x34u;
    stream.row_00E9 = 0x14u;
    stream.nametable_00EA = 0x25u;
    stream.next_column_00EB = INITIAL_CLASS;

    puts("{");
    puts("  \"schema\": \"excitebike.first-course-geometry.v1\",");
    puts("  \"source\": {");
    puts("    \"asm\": \"Excitebike/bank_FF.asm\",");
    puts("    \"stream_pointer_table\": \"$ED3A/$ED40\",");
    printf("    \"stream_index\": 0, \"stream_address\": \"$%04X\", \"next_stream_address\": \"$%04X\", \"stream_segment_bytes\": %u,\n", stream_base, next_stream_base, (unsigned)(next_stream_base - stream_base));
    puts("    \"course_index\": 0, \"setup_id_0041\": 2,");
    puts("    \"decoder\": \"native F4FF/F64E/F676/F681/F82E/ECBF translation; ASM differential suite\",");
    puts("    \"track_pages\": \"$0400-$057F, six 64-byte vertical-band pages\"");
    puts("  },");
    puts("  \"initial_state\": {");
    puts("    \"object_class_before_first_stream_column\": \"$09\",");
    puts("    \"stream_trigger_class\": \"$0A\",");
    printf("    \"ppu_column\": \"$34\", \"ppu_row\": \"$14\", \"nametable\": \"$25\", \"repeat_counter_first_byte\": \"$%02X\"\n", read_rom(stream_base));
    puts("  },");
    puts("  \"initial_profile_by_band\": [\"$3B\", \"$3B\", \"$3D\", \"$3D\", \"$3D\", \"$3D\"],");
    puts("  \"termination\": \"F638: template 9 completion sets $03BC when the stream lap counter reaches zero\",");
    puts("  \"tile_semantics\": {");
    puts("    \"$3B-$3D\": \"road values used by CF8B/AI lane checks\",");
    puts("    \"$E4\": \"empty/no-spawn marker checked by DAE3\",");
    puts("    \"$E6\": \"special vertical marker checked by DAE3\",");
    puts("    \"$C0-$C1,$70-$73\": \"terrain event values checked by D018\",");
    puts("    \"$48-$4A\": \"approach target values checked by D018\",");
    puts("    \"$FA\": \"terrain scan sentinel checked by D06F\"");
    puts("  },");
    puts("  \"columns\": [");

    for (frame = 0u; frame < MAX_FRAMES && emitted < max_columns; ++frame) {
        const uint8_t course_class = stream.next_column_00EB;
        const uint8_t old_column = stream.column_00E8;
        const uint8_t old_row = stream.row_00E9;
        const uint8_t old_nametable = stream.nametable_00EA;
        const uint8_t old_stream_offset = stream.stream_offset_00ED;

        spawn.object_class[0] = course_class;
        raceCourseStreamUpdate(&stream, &spawn);
        if (stream.unsupported != 0u) {
            fprintf(stderr, "unsupported F4FF state at frame %u, stream offset $%02X\n",
                    frame, stream.stream_offset_00ED);
            return 1;
        }

        if (stream.column_00E8 != old_column ||
            stream.row_00E9 != old_row ||
            stream.nametable_00EA != old_nametable) {
            const uint16_t template_addr = template_address(stream.template_00EE);
            printf("    %s{ \"ordinal\": %u, \"course_class\": \"$%02X\", \"ring_column\": \"$%02X\", \"ppu_row\": \"$%02X\", \"nametable\": \"$%02X\", ",
                   emitted == 0u ? "" : ",\n", emitted, course_class,
                   old_column, old_row, old_nametable);
            printf("\"stream_offset_before\": %u, \"stream_address_before\": \"$%04X\", \"stream_offset_after\": %u, ",
                   old_stream_offset, (unsigned)(stream_base + old_stream_offset), stream.stream_offset_00ED);
            printf("\"template_id\": %u, \"template_address\": \"$%04X\", \"template_offset_next\": %u, ",
                   stream.template_00EE, template_addr, stream.template_offset_00EF);
            printf("\"next_column_00EB\": \"$%02X\", \"repeat_counter_0057\": \"$%02X\", ",
                   stream.next_column_00EB, stream.counter_0057);
            printf("\"counter_03A4\": \"$%02X\", \"counter_03A5\": \"$%02X\", ",
                   stream.counter_03A4, stream.counter_03A5);
            printf("\"flag_03A2\": \"$%02X\", \"flag_03BC\": \"$%02X\", ",
                   stream.flag_03A2, stream.flag_03BC);
            printf("\"spawn_request_03A8\": \"$%02X\", \"stream_buffer_0300_0311\": [",
                   spawn.spawn_request_03A8);
            for (page = 0u; page < 0x12u; ++page)
                printf("%s\"$%02X\"", page == 0u ? "" : ", ", stream.buffer[page]);
            printf("], ");
            printf("\"values_by_band\": [");
            for (page = 0u; page < 6u; ++page)
                printf("%s\"$%02X\"", page == 0u ? "" : ", ",
                       spawn.track_pages[page][old_column]);
            printf("], \"semantics_by_band\": [");
            for (page = 0u; page < 6u; ++page)
                printf("%s\"%s\"", page == 0u ? "" : ", ",
                       tile_semantic(spawn.track_pages[page][old_column]));
            printf("] }");
            ++emitted;
        }

        /* F638 is the source-defined end of the configured stream: template
         * 9 decrements $0057 and sets $03BC when the lap counter reaches zero.
         * Preserve any final column written by that same F4FF call, then stop. */
        if (stream.flag_03BC != 0u && stream.counter_0057 == 0u) {
            complete = 1;
            break;
        }
    }

    puts("");
    puts("  ],");
    printf("  \"summary\": { \"column_count\": %u, \"frames_processed\": %u, ",
           emitted, frame + 1u);
    printf("\"complete\": %s, \"cycle_detected\": false, \"truncated\": %s }\n",
           complete ? "true" : "false", truncated ? "true" : "false");
    puts("}");
    return truncated ? 3 : 0;
}
