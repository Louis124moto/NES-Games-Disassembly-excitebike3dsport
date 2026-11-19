#include <stdio.h>
#include <string.h>
#include "native_menu.h"
#include "cpu6502.h"

static void prepare_cpu(Cpu6502 *cpu)
{
    cpu->a = 0u;
    cpu->x = 0u;
    cpu->y = 0u;
    cpu->sp = 0xFDu;
    cpu->p = 0x24u;
    cpu->cycles = 0u;
    cpu->bad_opcode = -1;
}

static void seed_menu_ram(Cpu6502 *cpu, uint8_t state, uint8_t selection,
                          uint8_t debounce, uint8_t pressed, uint8_t idle_timer)
{
    memset(cpu->ram, 0, sizeof(cpu->ram));
    cpu->ram[0x0041u] = state;
    cpu->ram[0x0042u] = selection;
    cpu->ram[0x0043u] = 0xA3u;
    cpu->ram[0x0049u] = debounce;
    cpu->ram[0x00FFu] = 0xA5u;
    cpu->ram[0x0031u] = idle_timer;
    cpu->ram[0x0030u] = 0x33u;
    cpu->ram[0x0040u] = 0xB4u;
    cpu->ram[0x0044u] = 0xB5u;
    cpu->ram[0x004Au] = 0xA4u;
    cpu->ram[0x03F0u] = 0xA0u;
    cpu->ram[0x03F2u] = 0xA2u;
    cpu->ram[0x03F3u] = 0xA3u;
    cpu->ram[0x03F7u] = 0xA7u;
    cpu->ram[0x0015u] = pressed;
}

static void seed_native(NativeMenuState *menu, uint8_t state, uint8_t selection,
                        uint8_t debounce, uint8_t idle_timer)
{
    memset(menu, 0, sizeof(*menu));
    menu->source_state = state;
    menu->input.selection = selection;
    menu->input.debounce = debounce;
    menu->input.selection_dirty = 0xA5u;
    menu->input.input_timer = idle_timer;
    menu->input.start_timer = 0x33u;
    menu->selected_course = 0xA3u;
    menu->selected_mode = 0xA2u;
    menu->setup_command = 0xB4u;
    menu->transition_phase = 0xB5u;
    menu->cursor_state = 0xA4u;
    menu->title_flag_03F0 = 0xA0u;
    menu->title_flag_03F3 = 0xA3u;
    menu->title_flag_03F7 = 0xA7u;
}

static int compare_menu(const NativeMenuState *native, const Cpu6502 *cpu,
                        const char *case_name, unsigned state,
                        unsigned selection, unsigned debounce, unsigned pressed)
{
#define CHECK_FIELD(n, a) do { if ((n) != (a)) { \
    printf("FAIL: %s state=%u selection=%u debounce=%u pressed=%02X field %s native=%02X ASM=%02X\\n", \
           case_name, state, selection, debounce, pressed, #n, (unsigned)(n), (unsigned)(a)); \
    return 1; } } while (0)
    CHECK_FIELD(native->source_state, cpu->ram[0x0041u]);
    CHECK_FIELD(native->input.selection, cpu->ram[0x0042u]);
    CHECK_FIELD(native->selected_course, cpu->ram[0x0043u]);
    CHECK_FIELD(native->input.debounce, cpu->ram[0x0049u]);
    CHECK_FIELD(native->input.selection_dirty, cpu->ram[0x00FFu]);
    CHECK_FIELD(native->input.input_timer, cpu->ram[0x0031u]);
    CHECK_FIELD(native->input.start_timer, cpu->ram[0x0030u]);
    CHECK_FIELD(native->setup_command, cpu->ram[0x0040u]);
    CHECK_FIELD(native->transition_phase, cpu->ram[0x0044u]);
    CHECK_FIELD(native->cursor_state, cpu->ram[0x004Au]);
    CHECK_FIELD(native->title_flag_03F0, cpu->ram[0x03F0u]);
    CHECK_FIELD(native->selected_mode, cpu->ram[0x03F2u]);
    CHECK_FIELD(native->title_flag_03F3, cpu->ram[0x03F3u]);
    CHECK_FIELD(native->title_flag_03F7, cpu->ram[0x03F7u]);
#undef CHECK_FIELD
    return 0;
}

int main(int argc, char **argv)
{
    static Cpu6502 reference;
    static const uint8_t states[] = { 0u, 1u, 8u };
    static const uint16_t entry_addresses[] = { 0xC346u, 0xC395u, 0xC3BDu };
    unsigned si, selection, debounce, pressed;
    unsigned long vectors = 0;

    if (argc != 2) {
        puts("usage: test_native_menu_state <original-16KiB-prg>");
        return 2;
    }
    {
        FILE *f = fopen(argv[1], "rb");
        size_t n;
        if (f == NULL) return 2;
        n = fread(reference.prg, 1u, sizeof(reference.prg), f);
        if (fclose(f) != 0 || n != sizeof(reference.prg)) return 2;
        cpu6502_reset_state(&reference);
    }

    for (si = 0; si < 3u; ++si) {
        for (selection = 0; selection < 8u; ++selection) {
            for (debounce = 0; debounce < 2u; ++debounce) {
                for (pressed = 0; pressed < 256u; ++pressed) {
                    NativeMenuState native_menu;
                    int rc;
                    seed_menu_ram(&reference, states[si], (uint8_t)selection,
                                  (uint8_t)debounce, (uint8_t)pressed, 1u);
                    prepare_cpu(&reference);
                    rc = cpu6502_call(&reference, entry_addresses[si], 2048u);
                    if (rc != 0) {
                        printf("FAIL: original menu handler %02X returned %d state=%u selection=%u debounce=%u pressed=%02X\n",
                               entry_addresses[si], rc, states[si], selection, debounce, pressed);
                        return 1;
                    }

                    seed_native(&native_menu, states[si], (uint8_t)selection,
                                (uint8_t)debounce, 1u);
                    nativeMenuUpdate(&native_menu, (uint8_t)pressed);
                    if (compare_menu(&native_menu, &reference, "menu handler",
                                     states[si], selection, debounce, pressed) != 0) {
                        return 1;
                    }
                    ++vectors;
                }
            }
        }
    }

    /* Title idle timeout bypasses input and enters the race state. */
    for (pressed = 0; pressed < 256u; ++pressed) {
        NativeMenuState native_menu;
        int rc;
        seed_menu_ram(&reference, 0u, 0xA2u, 1u, (uint8_t)pressed, 0u);
        prepare_cpu(&reference);
        rc = cpu6502_call(&reference, 0xC346u, 2048u);
        if (rc != 0) return 1;
        seed_native(&native_menu, 0u, 0xA2u, 1u, 0u);
        native_menu.input.input_timer = 0u;
        nativeMenuUpdate(&native_menu, (uint8_t)pressed);
        if (compare_menu(&native_menu, &reference, "title idle", 0u,
                         0xA2u, 1u, pressed) != 0) return 1;
        ++vectors;
    }

    /* Differentially test the 11-frame timer bank sub_D310. */
    {
        uint32_t random_state = 0xD310C0DEu;
        unsigned frame_timer, sample;
        for (frame_timer = 0; frame_timer < 256u; ++frame_timer) {
            for (sample = 0; sample < 32u; ++sample) {
                NativeMenuState native_menu;
                unsigned timer_index;
                int rc;
                memset(&native_menu, 0, sizeof(native_menu));
                native_menu.frame_timer = (uint8_t)frame_timer;
                for (timer_index = 0; timer_index < 29u; ++timer_index) {
                    random_state = random_state * 1664525u + 1013904223u;
                    native_menu.timer_values[timer_index] = (uint8_t)(random_state >> 24);
                }
                native_menu.input.start_timer = native_menu.timer_values[15];
                native_menu.input.input_timer = native_menu.timer_values[16];

                memset(reference.ram, 0, sizeof(reference.ram));
                reference.ram[0x0020u] = (uint8_t)frame_timer;
                for (timer_index = 0; timer_index < 29u; ++timer_index) {
                    reference.ram[0x0021u + timer_index] = native_menu.timer_values[timer_index];
                }
                prepare_cpu(&reference);
                rc = cpu6502_call(&reference, 0xD310u, 128u);
                if (rc != 0) {
                    printf("FAIL: original sub_D310 returned %d at frame timer=%u sample=%u\n",
                           rc, frame_timer, sample);
                    return 1;
                }

                nativeMenuTickTimers(&native_menu);
                if (native_menu.frame_timer != reference.ram[0x0020u] ||
                    native_menu.input.start_timer != reference.ram[0x0030u] ||
                    native_menu.input.input_timer != reference.ram[0x0031u]) {
                    printf("FAIL: sub_D310 divider=%u sample=%u native=%02X/%02X/%02X ASM=%02X/%02X/%02X\n",
                           frame_timer, sample, native_menu.frame_timer,
                           native_menu.input.start_timer, native_menu.input.input_timer,
                           reference.ram[0x0020u], reference.ram[0x0030u],
                           reference.ram[0x0031u]);
                    return 1;
                }
                for (timer_index = 0; timer_index < 29u; ++timer_index) {
                    if (native_menu.timer_values[timer_index] != reference.ram[0x0021u + timer_index]) {
                        printf("FAIL: sub_D310 divider=%u sample=%u timer=%02X native=%02X ASM=%02X\n",
                               frame_timer, sample, 0x21u + timer_index,
                               native_menu.timer_values[timer_index],
                               reference.ram[0x0021u + timer_index]);
                        return 1;
                    }
                }
                ++vectors;
            }
        }
    }

    printf("PASS: %lu native title/selector/design and timer vectors match original ASM.\n", vectors);
    return 0;
}
