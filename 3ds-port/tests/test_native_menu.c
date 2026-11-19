#include <stdio.h>
#include <string.h>
#include "native_menu.h"
#include "cpu6502.h"

int main(int argc, char **argv)
{
    static Cpu6502 reference;
    static const uint8_t entry_counts[] = { 3u, 5u, 6u };
    unsigned count_index, selection, debounce, pressed;
    unsigned long vectors = 0;

    if (argc != 2) {
        puts("usage: test_native_menu <original-16KiB-prg>");
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

    for (count_index = 0; count_index < sizeof(entry_counts); ++count_index) {
        for (selection = 0; selection < 256u; ++selection) {
            for (debounce = 0; debounce < 2u; ++debounce) {
                for (pressed = 0; pressed < 256u; ++pressed) {
                    NativeMenuSelectionState native_state;
                    int rc;
                    native_state.selection = (uint8_t)selection;
                    native_state.debounce = (uint8_t)debounce;
                    native_state.selection_dirty = 0xA5u;
                    native_state.input_timer = 0x44u;
                    native_state.start_timer = 0x33u;

                    memset(reference.ram, 0, sizeof(reference.ram));
                    reference.ram[0x0042u] = (uint8_t)selection;
                    reference.ram[0x0049u] = (uint8_t)debounce;
                    reference.ram[0x00FFu] = 0xA5u;
                    reference.ram[0x0031u] = 0x44u;
                    reference.ram[0x0030u] = 0x33u;
                    reference.ram[0x0015u] = (uint8_t)pressed;
                    reference.a = entry_counts[count_index];
                    reference.x = 0u;
                    reference.y = 0u;
                    reference.sp = 0xFDu;
                    reference.p = 0x24u;
                    reference.cycles = 0u;
                    reference.bad_opcode = -1;

                    rc = cpu6502_call(&reference, 0xC7DEu, 64u);
                    if (rc != 0) {
                        printf("FAIL: original sub_C7DE returned %d at count=%u selection=%u debounce=%u pressed=%02X\n",
                               rc, entry_counts[count_index], selection, debounce, pressed);
                        return 1;
                    }

                    {
                        const uint8_t confirmed = nativeMenuUpdateSelection(&native_state, (uint8_t)pressed,
                                                                             entry_counts[count_index]);
                        if (reference.x != (confirmed != 0u ? 4u : 0u)) {
                            printf("FAIL: sub_C7DE confirm output count=%u selection=%u debounce=%u pressed=%02X native=%u ASM_X=%u\\n",
                                   entry_counts[count_index], selection, debounce, pressed,
                                   confirmed, reference.x);
                            return 1;
                        }
                    }
                    if (native_state.selection != reference.ram[0x0042u] ||
                        native_state.debounce != reference.ram[0x0049u] ||
                        native_state.selection_dirty != reference.ram[0x00FFu] ||
                        native_state.input_timer != reference.ram[0x0031u] ||
                        native_state.start_timer != reference.ram[0x0030u]) {
                        printf("FAIL: sub_C7DE count=%u selection=%u debounce=%u pressed=%02X native=%u/%u/%02X/%02X/%02X ASM=%u/%u/%02X/%02X/%02X\n",
                               entry_counts[count_index], selection, debounce, pressed,
                               native_state.selection, native_state.debounce,
                               native_state.selection_dirty, native_state.input_timer,
                               native_state.start_timer,
                               reference.ram[0x0042u], reference.ram[0x0049u],
                               reference.ram[0x00FFu], reference.ram[0x0031u],
                               reference.ram[0x0030u]);
                        return 1;
                    }
                    ++vectors;
                }
            }
        }
    }

    printf("PASS: %lu native menu selection vectors match original sub_C7DE ASM.\n", vectors);
    return 0;
}
