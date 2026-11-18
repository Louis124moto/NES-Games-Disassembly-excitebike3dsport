#include "nes_race_decl.h"
#include "nes_machine.h"

/* sub_DFA2: LDX #$0A, falls into sub_DFA4. */
void nesR_DFA2(NesMachine *m)
{
    m->x = nesLoad(m, 0x0Au);
    nesR_DFA4(m);
}

/* sub_DFA4: A = A mod X, X = A div X (repeated subtraction). In: A, X = divisor. Out: A remainder, X quotient. */
void nesR_DFA4(NesMachine *m)
{
    m->ram[0x0F] = m->x;                       /* STX ram_000F_temp */
    m->x = nesLoad(m, 0u);                     /* LDX #$00 */
    do {
        m->x = nesLoad(m, (uint8_t)(m->x + 1u));   /* INX */
        m->c = 1u;                                 /* SEC */
        nesSbc(m, m->ram[0x0F]);                   /* SBC ram_000F_temp */
    } while (m->n == 0u);                          /* BPL bra_DFA8_loop */
    m->x = nesLoad(m, (uint8_t)(m->x - 1u));       /* DEX */
    nesAdc(m, m->ram[0x0F]);                       /* ADC ram_000F_temp; RTS */
}
