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
    cpu->ram[0x0200u] = 0xF8u;
    cpu->ram[0x0201u] = 0xF8u;
    cpu->ram[0x0202u] = 0xF8u;
    cpu->ram[0x0203u] = 0xF8u;
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
    menu->timer_values[15] = 0x33u;
    menu->timer_values[16] = idle_timer;
    menu->selected_course = 0xA3u;
    menu->selected_mode = 0xA2u;
    menu->setup_command = 0xB4u;
    menu->transition_phase = 0xB5u;
    menu->cursor_state = 0xA4u;
    menu->title_flag_03F0 = 0xA0u;
    menu->title_flag_03F3 = 0xA3u;
    menu->title_flag_03F7 = 0xA7u;
    menu->cursor_sprite_y = 0xF8u;
    menu->cursor_sprite_tile = 0xF8u;
    menu->cursor_sprite_attribute = 0xF8u;
    menu->cursor_sprite_x = 0xF8u;
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
    CHECK_FIELD(native->cursor_sprite_y, cpu->ram[0x0200u]);
    CHECK_FIELD(native->cursor_sprite_tile, cpu->ram[0x0201u]);
    CHECK_FIELD(native->cursor_sprite_attribute, cpu->ram[0x0202u]);
    CHECK_FIELD(native->cursor_sprite_x, cpu->ram[0x0203u]);
    CHECK_FIELD(native->frame_timer, cpu->ram[0x0020u]);
    { unsigned ti; for (ti = 0; ti < 29u; ++ti) {
        if (native->timer_values[ti] != cpu->ram[0x0021u + ti]) {
            printf("FAIL: %s timer %02X native=%02X ASM=%02X\\n", case_name,
                   0x21u + ti, native->timer_values[ti], cpu->ram[0x0021u + ti]);
            return 1;
        }
    } }
#undef CHECK_FIELD
    return 0;
}

 
static int test_native_menu_setup_commands(Cpu6502 *reference)
{
    static const struct {
        uint8_t state, command;
        uint16_t address;
    } cases[] = {
        {0u, 0u, 0xC2BDu}, {0u, 1u, 0xC514u},
        {1u, 0u, 0xC55Eu}, {1u, 1u, 0xC3F3u}, {1u, 2u, 0xC514u},
        {2u, 0u, 0xC551u}, {2u, 1u, 0xC869u}, {2u, 2u, 0xC820u},
        {2u, 3u, 0xC5A3u}, {2u, 4u, 0xC566u}, {2u, 5u, 0xC875u},
        {8u, 0u, 0xC55Eu}, {8u, 1u, 0xC575u}, {8u, 2u, 0xC514u}
    };
    unsigned n;
    for (n = 0u; n < sizeof(cases) / sizeof(cases[0]); ++n) {
        NativeMenuState native;
        unsigned rc;
        memset(reference->ram, 0, sizeof(reference->ram));
        reference->ram[0x0041u] = cases[n].state;
        reference->ram[0x0044u] = (uint8_t)(cases[n].command + 1u);
        reference->ram[0x0040u] = 0u;
        reference->ram[0x03F0u] = 1u;
        reference->ram[0x00FBu] = 0xA5u;
        reference->ram[0x03F8u] = cases[n].state == 2u ? 0x04u : 0x77u;
        reference->ram[0x03F2u] = cases[n].state == 2u ? 0x01u : 0xA2u;
        reference->ram[0x0046u] = 0u;
        reference->ram[0x004Bu] = cases[n].state == 2u ? 0xB6u : 0xB5u;
        reference->ram[0x0034u] = cases[n].state == 2u ? 0x2Du : 0u;
        if (cases[n].state == 2u) {
            static const uint8_t prior_assets[6] = {0x00u,0x12u,0x12u,0x01u,0x17u,0x0Eu};
            reference->ram[0x0045u] = prior_assets[cases[n].command];
        }
        reference->ram[0x0200u] = 0xF8u;
        reference->ram[0x0201u] = 0xF8u;
        reference->ram[0x0202u] = 0xF8u;
        reference->ram[0x0203u] = 0xF8u;
        prepare_cpu(reference);
        rc = (unsigned)cpu6502_call(reference, cases[n].address, 2048u);
        if (rc != 0u) {
            printf("FAIL: ASM menu setup command %04X returned %u\n", cases[n].address, rc);
            return 1;
        }

        memset(&native, 0, sizeof(native));
        native.source_state = cases[n].state;
        native.transition_phase = cases[n].command;
        native.setup_command = 0u;
        native.title_animation_index = 1u;
        native.title_animation_flag = 0xA5u;
        native.title_animation_counter = cases[n].state == 2u ? 0x04u : 0x77u;
        native.transition_counter_03F8 = cases[n].state == 2u ? 0x04u : 0u;
        native.transition_flag_004B = cases[n].state == 2u ? 0xB6u : 0u;
        native.transition_timer_0034 = cases[n].state == 2u ? 0x2Du : 0u;
        native.selected_mode = cases[n].state == 2u ? 0x01u : 0xA2u;
        if (cases[n].state == 2u) {
            static const uint8_t prior_assets[6] = {0x00u,0x12u,0x12u,0x01u,0x17u,0x0Eu};
            native.source_visual_asset = prior_assets[cases[n].command];
        }
        native.cursor_sprite_y = 0xF8u;
        native.cursor_sprite_tile = 0xF8u;
        native.cursor_sprite_attribute = 0xF8u;
        native.cursor_sprite_x = 0xF8u;
        if (nativeMenuStepSetup(&native) != 1u) {
            puts("FAIL: native menu setup command was not dispatched");
            return 1;
        }
#define SETUP_CHECK(native_value, ram_address, field) do { \
    if ((native_value) != reference->ram[(ram_address)]) { \
        printf("FAIL: setup %04X %s native=%02X ASM=%02X\n", cases[n].address, #field, \
               (unsigned)(native_value), reference->ram[(ram_address)]); return 1; \
    } \
} while (0)
        SETUP_CHECK(native.source_visual_asset, 0x0045u, visual_asset);
        SETUP_CHECK(native.title_animation_counter, 0x03F8u, title_counter);
        SETUP_CHECK(native.title_animation_index, 0x03F0u, title_index);
        SETUP_CHECK(native.title_animation_flag, 0x00FBu, title_flag);
        SETUP_CHECK(native.cursor_sprite_y, 0x0200u, cursor_y);
        SETUP_CHECK(native.cursor_sprite_tile, 0x0201u, cursor_tile);
        SETUP_CHECK(native.cursor_sprite_attribute, 0x0202u, cursor_attribute);
        SETUP_CHECK(native.cursor_sprite_x, 0x0203u, cursor_x);
        SETUP_CHECK(native.transition_flag_0046, 0x0046u, flag_0046);
        SETUP_CHECK(native.input.selection, 0x0042u, selection);
        SETUP_CHECK(native.selected_course, 0x0043u, course);
        SETUP_CHECK(native.input.debounce, 0x0049u, debounce);
        SETUP_CHECK(native.setup_command, 0x0040u, setup_complete);
        SETUP_CHECK(native.transition_phase, 0x0044u, setup_index);
        SETUP_CHECK(native.input.input_timer, 0x0031u, idle_timer);
        SETUP_CHECK(native.transition_timer_0034, 0x0034u, timer_0034);
        SETUP_CHECK(native.transition_flag_004B, 0x004Bu, flag_004B);
        SETUP_CHECK(native.title_animation_counter, 0x03F8u, title_counter);
        if (cases[n].state == 2u && cases[n].command == 5u && native.race_setup_complete != 1u) {
            puts("FAIL: first-course setup list did not signal completion"); return 1;
        }
        if ((cases[n].state != 2u || cases[n].command != 5u) && native.race_setup_complete != 0u) {
            puts("FAIL: race setup completion signaled too early"); return 1;
        }
#undef SETUP_CHECK
    }
    puts("PASS: title, selector and design setup commands match the original ASM.");
    return 0;
}

static int test_native_transition_phase1(void)
{
    NativeMenuState menu;

    memset(&menu, 0, sizeof(menu));
    menu.transition_phase = 1u;
    menu.transition_flag_004B = 1u;
    menu.timer_values[18] = 2u;
    nativeMenuTransitionPhase1(&menu);
    if (menu.transition_phase != 1u || menu.timer_values[18] != 2u) {
        puts("FAIL: CB77 must wait while the source transition timer is nonzero");
        return 1;
    }

    menu.timer_values[18] = 0u;
    nativeMenuTransitionPhase1(&menu);
    if (menu.transition_phase != 2u ||
        menu.transition_duration != 0x20u || menu.timer_values[18] != 0x23u) {
        puts("FAIL: CB77 native phase/timing mismatch");
        return 1;
    }

    memset(&menu, 0, sizeof(menu));
    menu.transition_phase = 1u;
    menu.timer_values[18] = 0u;
    nativeMenuTransitionPhase1(&menu);
    if (menu.transition_duration != 0x31u || menu.timer_values[18] != 0x34u) {
        puts("FAIL: CB77 alternate duration for transition flag zero mismatch");
        return 1;
    }

    menu.transition_phase = 0u;
    menu.timer_values[18] = 0u;
    nativeMenuTransitionPhase1(&menu);
    if (menu.transition_phase != 0u) {
        puts("FAIL: CB77 handler must ignore non-phase-1 states");
        return 1;
    }
    puts("PASS: native CB77 phase-1 timing/state unit tests (visual payload intentionally unimplemented).");
    return 0;
}

static int test_native_transition_phase1_differential(Cpu6502 *reference)
{
    unsigned flag, sample, i;
    unsigned long vectors = 0u;
    for (flag = 0u; flag < 2u; ++flag) {
        for (sample = 0u; sample < 32u; ++sample) {
            NativeMenuState native_menu;
            int rc;
            memset(reference->ram, 0, sizeof(reference->ram));
            reference->ram[0x0033u] = 0u;
            reference->ram[0x0044u] = 1u;
            reference->ram[0x004Bu] = (uint8_t)flag;
            reference->ram[0x0300u] = 0u;
            for (i = 0u; i < 7u; ++i) {
                reference->ram[0x03D9u + i] = (uint8_t)(sample * 7u + i * 19u);
            }
            prepare_cpu(reference);
            rc = cpu6502_call(reference, 0xCB77u, 2048u);
            if (rc != 0) {
                printf("FAIL: original CB77 returned %d flag=%u sample=%u\\n", rc, flag, sample);
                return 1;
            }

            memset(&native_menu, 0, sizeof(native_menu));
            native_menu.transition_phase = 1u;
            native_menu.transition_flag_004B = (uint8_t)flag;
            native_menu.timer_values[18] = 0u;
            nativeMenuTransitionPhase1(&native_menu);
            if (native_menu.transition_phase != reference->ram[0x0044u] ||
                native_menu.transition_duration != reference->ram[0x03ADu] ||
                native_menu.timer_values[18] != reference->ram[0x0033u]) {
                printf("FAIL: CB77 native/ASM state mismatch flag=%u sample=%u\\n", flag, sample);
                return 1;
            }
            ++vectors;
        }
    }
    printf("PASS: %lu native CB77 phase-1 timing/state vectors match original ASM (visual payload excluded).\\n", vectors);
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
    if (test_native_transition_phase1() != 0) return 1;

    {
        FILE *f = fopen(argv[1], "rb");
        size_t n;
        if (f == NULL) return 2;
        n = fread(reference.prg, 1u, sizeof(reference.prg), f);
        if (fclose(f) != 0 || n != sizeof(reference.prg)) return 2;
        cpu6502_reset_state(&reference);
    }

    if (test_native_transition_phase1_differential(&reference) != 0) return 1;
    if (test_native_menu_setup_commands(&reference) != 0) return 1;

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
                rc = cpu6502_call(&reference, 0xD310u, 512u);
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


    /* Shared transition dispatcher phase 0 (sub_CB2C -> ofs_CB3B_00). */
    {
        static const uint8_t state_values[] = { 0u, 1u, 2u, 3u, 6u, 7u, 8u, 9u, 0x0Bu, 0x0Cu, 0x0Du, 0xFFu };
        static const uint8_t gate_values[] = { 0u, 1u, 3u, 4u, 5u, 0xFFu };
        static const uint8_t flag_values[] = { 0u, 1u };
        static const uint8_t course_values[] = { 0u, 1u, 3u, 4u, 5u, 0xFFu };
        static const uint8_t blink_values[] = { 0u, 1u, 2u, 3u, 0xFEu, 0xFFu };
        unsigned st, gate, flag, course, blink, counter;
        for (st = 0; st < sizeof(state_values); ++st) {
            for (gate = 0; gate < sizeof(gate_values); ++gate) {
                for (flag = 0; flag < sizeof(flag_values); ++flag) {
                    for (course = 0; course < sizeof(course_values); ++course) {
                        for (blink = 0; blink < sizeof(blink_values); ++blink) {
                            for (counter = 0; counter < sizeof(blink_values); ++counter) {
                                NativeMenuState native_menu;
                                int rc;
                                memset(&native_menu, 0, sizeof(native_menu));
                                native_menu.source_state = state_values[st];
                                native_menu.selected_mode = (uint8_t)(0xA0u + course);
                                native_menu.input.selection = 0x55u;
                                native_menu.selected_course = course_values[course];
                                native_menu.transition_gate_0053 = gate_values[gate];
                                native_menu.transition_flag_0046 = flag_values[flag];
                                native_menu.transition_flag_004B = 0xA5u;
                                native_menu.transition_counter_03F8 = blink_values[counter];
                                native_menu.title_flag_03F3 = blink_values[blink];
                                native_menu.transition_phase = 0xFEu;
                                native_menu.timer_values[18] = 0xA7u;

                                memset(reference.ram, 0, sizeof(reference.ram));
                                reference.ram[0x0041u] = native_menu.source_state;
                                reference.ram[0x0042u] = native_menu.input.selection;
                                reference.ram[0x0043u] = native_menu.selected_course;
                                reference.ram[0x0046u] = native_menu.transition_flag_0046;
                                reference.ram[0x004Bu] = native_menu.transition_flag_004B;
                                reference.ram[0x0053u] = native_menu.transition_gate_0053;
                                reference.ram[0x03F2u] = native_menu.selected_mode;
                                reference.ram[0x03F3u] = native_menu.title_flag_03F3;
                                reference.ram[0x03F8u] = native_menu.transition_counter_03F8;
                                reference.ram[0x0033u] = native_menu.timer_values[18];
                                reference.ram[0x0044u] = native_menu.transition_phase;
                                prepare_cpu(&reference);
                                rc = cpu6502_call(&reference, 0xCB3Bu, 256u);
                                if (rc != 0) {
                                    printf("FAIL: original CB3B returned %d state=%02X gate=%02X flag=%u course=%02X blink=%02X counter=%02X\n",
                                           rc, state_values[st], gate_values[gate], flag_values[flag],
                                           course_values[course], blink_values[blink], blink_values[counter]);
                                    return 1;
                                }

                                nativeMenuTransitionPhase0(&native_menu);
                                if (native_menu.input.selection != reference.ram[0x0042u] ||
                                    native_menu.selected_course != reference.ram[0x0043u] ||
                                    native_menu.transition_flag_0046 != reference.ram[0x0046u] ||
                                    native_menu.transition_flag_004B != reference.ram[0x004Bu] ||
                                    native_menu.title_flag_03F3 != reference.ram[0x03F3u] ||
                                    native_menu.transition_counter_03F8 != reference.ram[0x03F8u] ||
                                    native_menu.timer_values[18] != reference.ram[0x0033u] ||
                                    native_menu.transition_phase != reference.ram[0x0044u]) {
                                    printf("FAIL: CB3B state=%02X gate=%02X flag=%u course=%02X blink=%02X counter=%02X\n",
                                           state_values[st], gate_values[gate], flag_values[flag],
                                           course_values[course], blink_values[blink], blink_values[counter]);
                                    return 1;
                                }
                                ++vectors;
                            }
                        }
                    }
                }
            }
        }
    }


    /* Timer-expired C465 branch: sub_C3AF followed by the $08 state clamp. */
    {
        unsigned state_value, setup, phase;
        for (state_value = 0; state_value < 256u; ++state_value) {
            for (setup = 0; setup < 8u; ++setup) {
                for (phase = 0; phase < 8u; ++phase) {
                    NativeMenuState native_menu;
                    int rc;
                    memset(&native_menu, 0, sizeof(native_menu));
                    native_menu.source_state = (uint8_t)state_value;
                    native_menu.setup_command = (uint8_t)(0xF8u + setup);
                    native_menu.transition_phase = (uint8_t)(0xF8u + phase);
                    native_menu.timer_values[19] = 0u; /* $0034 */

                    memset(reference.ram, 0, sizeof(reference.ram));
                    reference.ram[0x0041u] = native_menu.source_state;
                    reference.ram[0x0040u] = native_menu.setup_command;
                    reference.ram[0x0044u] = native_menu.transition_phase;
                    reference.ram[0x0034u] = 0u;
                    prepare_cpu(&reference);
                    rc = cpu6502_call(&reference, 0xC477u, 64u);
                    if (rc != 0) {
                        printf("FAIL: original C465 timer-zero branch returned %d state=%02X setup=%02X phase=%02X\n",
                               rc, state_value, native_menu.setup_command,
                               native_menu.transition_phase);
                        return 1;
                    }

                    nativeMenuCompleteTimedTransition(&native_menu);
                    if (native_menu.source_state != reference.ram[0x0041u] ||
                        native_menu.setup_command != reference.ram[0x0040u] ||
                        native_menu.transition_phase != reference.ram[0x0044u]) {
                        printf("FAIL: C465 timer-zero state=%02X setup=%02X phase=%02X native=%02X/%02X/%02X ASM=%02X/%02X/%02X\n",
                               state_value, native_menu.setup_command,
                               native_menu.transition_phase,
                               native_menu.source_state, native_menu.setup_command,
                               native_menu.transition_phase,
                               reference.ram[0x0041u], reference.ram[0x0040u],
                               reference.ram[0x0044u]);
                        return 1;
                    }
                    ++vectors;
                }
            }
        }
    }


    /* Shared transition dispatcher phase 3: ofs_CC6B_03. */
    {
        unsigned state_value, flag, setup, phase;
        for (state_value = 0; state_value < 256u; ++state_value) {
            for (flag = 0; flag < 2u; ++flag) {
                for (setup = 0; setup < 4u; ++setup) {
                    for (phase = 0; phase < 4u; ++phase) {
                        NativeMenuState native_menu;
                        int rc;
                        memset(&native_menu, 0, sizeof(native_menu));
                        native_menu.source_state = (uint8_t)state_value;
                        native_menu.transition_flag_004B = (uint8_t)flag;
                        native_menu.setup_command = (uint8_t)(0xFCu + setup);
                        native_menu.transition_phase = (uint8_t)(0xFCu + phase);

                        memset(reference.ram, 0, sizeof(reference.ram));
                        reference.ram[0x0041u] = native_menu.source_state;
                        reference.ram[0x004Bu] = native_menu.transition_flag_004B;
                        reference.ram[0x0040u] = native_menu.setup_command;
                        reference.ram[0x0044u] = native_menu.transition_phase;
                        prepare_cpu(&reference);
                        rc = cpu6502_call(&reference, 0xCC6Bu, 64u);
                        if (rc != 0) {
                            printf("FAIL: original CC6B returned %d state=%02X flag=%u\n",
                                   rc, state_value, flag);
                            return 1;
                        }

                        nativeMenuTransitionPhase3(&native_menu);
                        if (native_menu.source_state != reference.ram[0x0041u] ||
                            native_menu.setup_command != reference.ram[0x0040u] ||
                            native_menu.transition_phase != reference.ram[0x0044u]) {
                            printf("FAIL: CC6B state=%02X flag=%u native=%02X/%02X/%02X ASM=%02X/%02X/%02X\n",
                                   state_value, flag,
                                   native_menu.source_state, native_menu.setup_command,
                                   native_menu.transition_phase,
                                   reference.ram[0x0041u], reference.ram[0x0040u],
                                   reference.ram[0x0044u]);
                            return 1;
                        }
                        ++vectors;
                    }
                }
            }
        }
    }

    printf("PASS: %lu native menu selection, state, timer and transition vectors match original ASM.\n", vectors);
    return 0;
}
