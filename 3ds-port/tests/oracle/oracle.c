#include "oracle.h"
#include <stdio.h>
#include <string.h>

static struct { uint16_t lo, hi; } g_ign[16];
static unsigned g_nign;

static void cpuWriteHook(Cpu6502 *c, uint16_t a, uint8_t v)
{
    Oracle *o = (Oracle *)c->user;
    c->io[a] = v;
    if (a < 0x4020u && o->cpuLog.n < 4096u) { o->cpuLog.addr[o->cpuLog.n] = a; o->cpuLog.val[o->cpuLog.n++] = v; }
}
static uint8_t cpuReadHook(Cpu6502 *c, uint16_t a) { return c->io[a]; }
static void mWrite(NesMachine *m, uint16_t a, uint8_t v)
{
    Oracle *o = (Oracle *)m->user;
    if (a < 0x4020u) o->ioPort[a] = v;
    if (o->mLog.n < 4096u) { o->mLog.addr[o->mLog.n] = a; o->mLog.val[o->mLog.n++] = v; }
}
static uint8_t mRead(NesMachine *m, uint16_t a) { Oracle *o = (Oracle *)m->user; return a < 0x4020u ? o->ioPort[a] : 0u; }

int oracleInit(Oracle *o, const char *prgPath)
{
    FILE *f = fopen(prgPath, "rb");
    memset(o, 0, sizeof(*o));
    if (!f) return 1;
    if (fread(o->prg, 1, 0x4000, f) != 0x4000) { fclose(f); return 2; }
    fclose(f);
    memcpy(o->cpu.prg, o->prg, 0x4000);
    o->cpu.read_hook = cpuReadHook; o->cpu.write_hook = cpuWriteHook; o->cpu.user = o;
    o->m.prg = o->prg; o->m.io_write = mWrite; o->m.io_read = mRead; o->m.user = o;
    o->rng = 0x12345678u;
    g_nign = 0;
    return 0;
}
uint32_t oracleRand(Oracle *o) { o->rng = o->rng * 1664525u + 1013904223u; return o->rng >> 8; }

void oracleRandomize(Oracle *o)
{
    unsigned i;
    cpu6502_reset_state(&o->cpu);
    for (i = 0; i < 0x800u; ++i) o->cpu.ram[i] = (uint8_t)oracleRand(o);
    for (i = 0x100u; i < 0x200u; ++i) o->cpu.ram[i] = 0u;
    for (i = 0x2000u; i < 0x4020u; ++i) o->cpu.io[i] = (uint8_t)oracleRand(o);
    o->cpu.a = (uint8_t)oracleRand(o); o->cpu.x = (uint8_t)oracleRand(o); o->cpu.y = (uint8_t)oracleRand(o);
    o->cpu.p = (uint8_t)((oracleRand(o) & 0xC3u) | 0x24u);   /* random N,V,Z,C; I set; D clear */
    o->cpu.sp = 0xFDu;
    o->cpuLog.n = o->mLog.n = 0;
    g_nign = 0;
    oracleSync(o);
}
void oracleSync(Oracle *o)
{
    memcpy(o->m.ram, o->cpu.ram, 0x800);
    o->m.a = o->cpu.a; o->m.x = o->cpu.x; o->m.y = o->cpu.y;
    o->m.c = (uint8_t)(o->cpu.p & 1u); o->m.z = (uint8_t)((o->cpu.p >> 1) & 1u);
    o->m.n = (uint8_t)(o->cpu.p >> 7); o->m.v = (uint8_t)((o->cpu.p >> 6) & 1u);
    o->m.sp = o->cpu.sp; o->m.pother = (uint8_t)(o->cpu.p & 0x0Cu);
    memcpy(o->ioPort, o->cpu.io + 0x2000, 0); /* placeholder */
    { unsigned k; for (k = 0x2000u; k < 0x4020u; ++k) o->ioPort[k] = o->cpu.io[k]; }
}
int oracleRunCpu(Oracle *o, uint16_t addr) { return cpu6502_call(&o->cpu, addr, 2000000ul); }
void oracleIgnore(Oracle *o, uint16_t lo, uint16_t hi) { (void)o; if (g_nign < 16u) { g_ign[g_nign].lo = lo; g_ign[g_nign].hi = hi; ++g_nign; } }

int oracleCompare(Oracle *o, unsigned cmpRegs, const char *what)
{
    unsigned i, k;
    for (i = 0; i < 0x800u; ++i) {
        int skip = (i >= 0x100u && i < 0x200u);
        for (k = 0; k < g_nign; ++k) if (i >= g_ign[k].lo && i <= g_ign[k].hi) skip = 1;
        if (!skip && o->cpu.ram[i] != o->m.ram[i]) {
            printf("DIFF %s: ram[%04X] original=%02X port=%02X\n", what, i, o->cpu.ram[i], o->m.ram[i]);
            return 1;
        }
    }
    if (cmpRegs) {
        const uint8_t cc = (uint8_t)(o->cpu.p & 1u), cz = (uint8_t)((o->cpu.p >> 1) & 1u), cn = (uint8_t)(o->cpu.p >> 7);
        if ((cmpRegs & 1u) && o->cpu.a != o->m.a) { printf("DIFF %s: A original=%02X port=%02X\n", what, o->cpu.a, o->m.a); return 1; }
        if ((cmpRegs & 2u) && o->cpu.x != o->m.x) { printf("DIFF %s: X original=%02X port=%02X\n", what, o->cpu.x, o->m.x); return 1; }
        if ((cmpRegs & 4u) && o->cpu.y != o->m.y) { printf("DIFF %s: Y original=%02X port=%02X\n", what, o->cpu.y, o->m.y); return 1; }
        if ((cmpRegs & 8u) && cc != o->m.c) { printf("DIFF %s: C original=%u port=%u\n", what, cc, o->m.c); return 1; }
        if ((cmpRegs & 16u) && cz != o->m.z) { printf("DIFF %s: Z original=%u port=%u\n", what, cz, o->m.z); return 1; }
        if ((cmpRegs & 64u) && ((o->cpu.p >> 6) & 1u) != o->m.v) { printf("DIFF %s: V original=%u port=%u\n", what, (o->cpu.p >> 6) & 1u, o->m.v); return 1; }
        if ((cmpRegs & 32u) && cn != o->m.n) { printf("DIFF %s: N original=%u port=%u\n", what, cn, o->m.n); return 1; }
    }
    if (o->cpuLog.n != o->mLog.n) { printf("DIFF %s: io write count original=%u port=%u\n", what, o->cpuLog.n, o->mLog.n); return 1; }
    for (i = 0; i < o->cpuLog.n; ++i)
        if (o->cpuLog.addr[i] != o->mLog.addr[i] || o->cpuLog.val[i] != o->mLog.val[i]) {
            printf("DIFF %s: io write #%u original=%04X<-%02X port=%04X<-%02X\n", what, i, o->cpuLog.addr[i], o->cpuLog.val[i], o->mLog.addr[i], o->mLog.val[i]);
            return 1;
        }
    return 0;
}


extern int nesIsInstructionStart(uint16_t addr);
static int oracleIsCode(uint16_t a) { return nesIsInstructionStart(a); }

int oracleDiffRoutine(Oracle *o, uint16_t addr, int (*port)(NesMachine *, uint16_t), const char *name, unsigned iters,
                      void (*prep)(Oracle *), unsigned cmpRegs, unsigned *skipped)
{
    unsigned i, sk = 0;
    for (i = 0; i < iters; ++i) {
        char what[96];
        int pr;
        oracleRandomize(o);
        if (prep) prep(o);
        oracleSync(o);
        if (oracleRunCpu(o, addr) != 0) { ++sk; continue; }
        pr = port(&o->m, addr);
        snprintf(what, sizeof(what), "%s iter %u", name, i);
        if (pr == 2 && !oracleIsCode(o->m.pc)) { ++sk; continue; }   /* random jump into data / mid-instruction: unreachable in the real game */
        if (pr != 0) { printf("DIFF %s: port returned %d (pc=%04X) but the original finished\n", what, pr, o->m.pc); return 1; }
        if (o->m.sp != o->cpu.sp) { printf("DIFF %s: sp original=%02X port=%02X\n", what, o->cpu.sp, o->m.sp); return 1; }
        if (oracleCompare(o, cmpRegs, what)) return 1;
    }
    if (skipped) *skipped = sk;
    if (sk == iters) { printf("FAIL %s: every iteration hit the step limit\n", name); return 1; }
    return 0;
}
