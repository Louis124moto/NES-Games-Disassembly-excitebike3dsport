#include "nes_system.h"
#include <string.h>
#include "nes_gen_code.h"
#include "nes_prg_crc.h"
#include "nes_ppu_render.h"

static uint32_t crc32Bytes(const uint8_t *d, size_t n)
{
    uint32_t c = 0xFFFFFFFFu;
    size_t i;
    unsigned k;
    for (i = 0; i < n; ++i) {
        c ^= d[i];
        for (k = 0; k < 8u; ++k) c = (c >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(c & 1u)));
    }
    return ~c;
}

/* ---- PPU memory ---- */
static unsigned ntPhysical(const NesSystem *s, unsigned nt)
{
    return s->verticalMirroring ? (nt & 1u) : ((nt >> 1) & 1u);
}
static uint8_t ppuRead(NesSystem *s, uint16_t a)
{
    a &= 0x3FFFu;
    if (a < 0x2000u) return s->chr[a];
    if (a < 0x3F00u) return s->vram[ntPhysical(s, (a >> 10) & 3u) * 0x400u + (a & 0x3FFu)];
    {
        unsigned i = a & 0x1Fu;
        if ((i & 3u) == 0u) i &= 0x0Fu;
        return s->pal[i];
    }
}
static void ppuWrite(NesSystem *s, uint16_t a, uint8_t v)
{
    a &= 0x3FFFu;
    if (a < 0x2000u) return;                       /* CHR-ROM */
    if (a < 0x3F00u) { s->vram[ntPhysical(s, (a >> 10) & 3u) * 0x400u + (a & 0x3FFu)] = v; return; }
    {
        unsigned i = a & 0x1Fu;
        if ((i & 3u) == 0u) i &= 0x0Fu;
        s->pal[i] = (uint8_t)(v & 0x3Fu);
    }
}

/* ---- scanline renderer ---- */
static unsigned bgPixel(const NesSystem *s, unsigned x, unsigned *palIdx)
{
    const unsigned coarseX = s->v & 0x1Fu, fineY = (s->v >> 12) & 7u, row = (s->v >> 5) & 0x1Fu;
    const unsigned px = coarseX * 8u + s->fineX + x;
    const unsigned ntx = ((s->v >> 10) & 1u) ^ ((px >> 8) & 1u);
    const unsigned nt = ntx | (((s->v >> 11) & 1u) << 1);
    const unsigned col = (px >> 3) & 0x1Fu, fx = px & 7u;
    const unsigned base = ntPhysical(s, nt) * 0x400u;
    const unsigned tile = s->vram[base + row * 32u + col];
    const unsigned attr = s->vram[base + 0x3C0u + (row >> 2) * 8u + (col >> 2)];
    const unsigned shift = ((row & 2u) << 1) | (col & 2u);
    const unsigned pat = (s->ctrl & 0x10u) ? 0x1000u : 0u;
    const unsigned lo = s->chr[pat + tile * 16u + fineY], hi = s->chr[pat + tile * 16u + 8u + fineY];
    const unsigned pix = ((lo >> (7u - fx)) & 1u) | (((hi >> (7u - fx)) & 1u) << 1);
    *palIdx = ((attr >> shift) & 3u);
    return pix;
}

static void renderScanline(NesSystem *s, unsigned y)
{
    uint8_t line[256], bgop[256];
    const int showBg = (s->mask & 0x08u) != 0u, showSp = (s->mask & 0x10u) != 0u;
    uint8_t *out = &s->fb[y * NES_SYS_WIDTH * 3u];
    unsigned x;

    if (!showBg && !showSp) {
        const uint8_t *c = s->rgbPalette[s->pal[0] & 0x3Fu];
        for (x = 0; x < 256u; ++x) { out[x * 3u] = c[0]; out[x * 3u + 1u] = c[1]; out[x * 3u + 2u] = c[2]; }
        return;
    }
    s->v = (uint16_t)((s->v & ~0x041Fu) | (s->t & 0x041Fu));
    for (x = 0; x < 256u; ++x) {
        unsigned pi = 0u, pix = 0u;
        if (showBg && !(x < 8u && !(s->mask & 2u))) pix = bgPixel(s, x, &pi);
        bgop[x] = (uint8_t)(pix != 0u);
        line[x] = pix ? s->pal[(pi << 2) | pix] : s->pal[0];
    }
    if (showSp) {
        const unsigned h = (s->ctrl & 0x20u) ? 16u : 8u;
        int sel[8], n = 0, i;
        uint8_t filled[256];
        memset(filled, 0, sizeof(filled));
        for (i = 0; i < 64 && n < 8; ++i) {
            const int top = (int)s->oam[i * 4] + 1;
            if ((int)y >= top && (int)y < top + (int)h) sel[n++] = i;
        }
        for (i = 0; i < n; ++i) {
            const unsigned idx = (unsigned)sel[i];
            const unsigned attr = s->oam[idx * 4 + 2], sx = s->oam[idx * 4 + 3];
            unsigned fy = y - ((unsigned)s->oam[idx * 4] + 1u), tile = s->oam[idx * 4 + 1], pat;
            unsigned k;
            if (attr & 0x80u) fy = h - 1u - fy;
            if (h == 16u) { pat = (tile & 1u) ? 0x1000u : 0u; tile = (tile & 0xFEu) + (fy >= 8u ? 1u : 0u); fy &= 7u; }
            else pat = (s->ctrl & 0x08u) ? 0x1000u : 0u;
            {
                const unsigned lo = s->chr[pat + tile * 16u + fy], hi = s->chr[pat + tile * 16u + 8u + fy];
                for (k = 0; k < 8u; ++k) {
                    const unsigned bit = (attr & 0x40u) ? k : 7u - k;
                    const unsigned pix = ((lo >> bit) & 1u) | (((hi >> bit) & 1u) << 1);
                    const unsigned px = sx + k;
                    if (pix == 0u || px > 255u) continue;
                    if (px < 8u && !(s->mask & 4u)) continue;
                    if (idx == 0u && bgop[px] && px != 255u && s->s0Line < 0) { s->s0Line = (int)y; s->s0X = (int)px; }
                    if (filled[px]) continue;
                    filled[px] = 1u;
                    if ((attr & 0x20u) && bgop[px]) continue;
                    line[px] = s->pal[0x10u + (((attr & 3u) << 2) | pix)];
                }
            }
        }
    }
    for (x = 0; x < 256u; ++x) {
        uint8_t ci = line[x];
        const uint8_t *c;
        if (s->mask & 1u) ci &= 0x30u;
        c = s->rgbPalette[ci & 0x3Fu];
        out[x * 3u] = c[0]; out[x * 3u + 1u] = c[1]; out[x * 3u + 2u] = c[2];
    }
    /* end of line: increment vertical position in v */
    if (((s->v >> 12) & 7u) != 7u) s->v = (uint16_t)(s->v + 0x1000u);
    else {
        unsigned cy = (s->v >> 5) & 0x1Fu;
        s->v = (uint16_t)(s->v & ~0x7000u);
        if (cy == 29u) { cy = 0u; s->v ^= 0x0800u; } else if (cy == 31u) cy = 0u; else ++cy;
        s->v = (uint16_t)((s->v & ~0x03E0u) | (cy << 5));
    }
}

static unsigned currentLine(const NesSystem *s)
{
    const unsigned long c = s->m.cyc - s->frameStart;
    return (unsigned)((c * 3ul) / 341ul);
}
/* Render lines < upTo (exclusive), limited to the visible area. */
static void catchUp(NesSystem *s, unsigned upTo)
{
    if (upTo > 240u) upTo = 240u;
    while (s->renderLine < upTo) { renderScanline(s, s->renderLine); ++s->renderLine; }
}

/* ---- hardware registers ---- */
static void ioWrite(NesMachine *m, uint16_t a, uint8_t d)
{
    NesSystem *s = (NesSystem *)m->user;
    if (a < 0x4000u) {
        switch (a & 7u) {
        case 0: catchUp(s, currentLine(s)); s->ctrl = d; s->t = (uint16_t)((s->t & ~0x0C00u) | ((unsigned)(d & 3u) << 10)); break;
        case 1: catchUp(s, currentLine(s)); s->mask = d; break;
        case 3: s->oamAddr = d; break;
        case 4: catchUp(s, currentLine(s)); s->oam[s->oamAddr++] = d; break;
        case 5:
            catchUp(s, currentLine(s));
            if (s->w == 0u) { s->t = (uint16_t)((s->t & ~0x1Fu) | (d >> 3)); s->fineX = (uint8_t)(d & 7u); s->w = 1u; }
            else { s->t = (uint16_t)((s->t & ~0x73E0u) | ((unsigned)(d & 7u) << 12) | ((unsigned)(d >> 3) << 5)); s->w = 0u; }
            break;
        case 6:
            catchUp(s, currentLine(s));
            if (s->w == 0u) { s->t = (uint16_t)((s->t & 0x00FFu) | ((unsigned)(d & 0x3Fu) << 8)); s->w = 1u; }
            else { s->t = (uint16_t)((s->t & 0xFF00u) | d); s->v = s->t; s->w = 0u; }
            break;
        case 7:
            catchUp(s, currentLine(s));
            ppuWrite(s, s->v, d);
            s->v = (uint16_t)(s->v + ((s->ctrl & 4u) ? 32u : 1u));
            break;
        default: break;
        }
    } else if (a == 0x4014u) {
        unsigned i;
        catchUp(s, currentLine(s));
        for (i = 0; i < 256u; ++i) s->oam[(uint8_t)(s->oamAddr + i)] = m->ram[(((unsigned)d << 8) + i) & 0x7FFu];
        m->cyc += 513u;
    } else if (a == 0x4016u) {
        s->strobe = (uint8_t)(d & 1u);
        if (s->strobe) s->padShift = s->pad;
    }
}

static uint8_t ioRead(NesMachine *m, uint16_t a)
{
    NesSystem *s = (NesSystem *)m->user;
    if (a < 0x4000u) {
        switch (a & 7u) {
        case 2: {
            const unsigned line = currentLine(s);
            const unsigned long dot = ((m->cyc - s->frameStart) * 3ul) % 341ul;
            uint8_t v;
            catchUp(s, line + 1u);                       /* lines up to and including the current one */
            v = s->status & 0x80u;
            if (s->s0Line >= 0 && ((int)line > s->s0Line || ((int)line == s->s0Line && (int)dot >= s->s0X))) v |= 0x40u;
            s->status &= 0x7Fu;
            s->w = 0u;
            return v;
        }
        case 4: return s->oam[s->oamAddr];
        case 7: {
            uint8_t r;
            if ((s->v & 0x3FFFu) >= 0x3F00u) { r = ppuRead(s, s->v); s->readBuf = ppuRead(s, (uint16_t)(s->v - 0x1000u)); }
            else { r = s->readBuf; s->readBuf = ppuRead(s, s->v); }
            s->v = (uint16_t)(s->v + ((s->ctrl & 4u) ? 32u : 1u));
            return r;
        }
        default: return 0u;
        }
    }
    if (a == 0x4016u) {
        const uint8_t bit = (uint8_t)(s->padShift & 1u);
        if (!s->strobe) s->padShift = (uint8_t)((s->padShift >> 1) | 0x80u);
        return bit;
    }
    return 0u;
}

NesSystemResult nesSystemInit(NesSystem *s, const uint8_t *rom, size_t romSize)
{
    unsigned i;
    memset(s, 0, sizeof(*s));
    if (rom == NULL || romSize < 16u + 0x4000u + 0x2000u) return NES_SYS_BAD_ROM;
    if (rom[0] != 'N' || rom[1] != 'E' || rom[2] != 'S' || rom[3] != 0x1Au) return NES_SYS_BAD_ROM;
    if (rom[4] != 1u || rom[5] != 1u || (((rom[6] >> 4) | (rom[7] & 0xF0u)) != 0u)) return NES_SYS_BAD_ROM;
    {
        const size_t off = 16u + ((rom[6] & 4u) ? 512u : 0u);
        if (romSize < off + 0x6000u) return NES_SYS_BAD_ROM;
        memcpy(s->prg, rom + off, 0x4000u);
        memcpy(s->chr, rom + off + 0x4000u, 0x2000u);
    }
    s->verticalMirroring = (rom[6] & 1u) != 0u;
    if (crc32Bytes(s->prg, sizeof(s->prg)) != (uint32_t)NES_EXPECTED_PRG_CRC32) return NES_SYS_PRG_MISMATCH;
    for (i = 0; i < 64u; ++i) {
        uint8_t rgb[3];
        if (!nesPpuColorToRgb((uint8_t)i, rgb)) { rgb[0] = rgb[1] = rgb[2] = 0u; }
        memcpy(s->rgbPalette[i], rgb, 3);
    }
    s->m.prg = s->prg;
    s->m.io_write = ioWrite;
    s->m.io_read = ioRead;
    s->m.user = s;
    s->m.sp = 0xFDu;
    s->m.pother = 0x04u;
    s->m.cyc_limit = 0ul;
    s->mainPc = (uint16_t)(s->prg[0x3FFCu] | (s->prg[0x3FFDu] << 8));
    s->s0Line = -1;
    s->padShift = 0u;
    return NES_SYS_OK;
}

void nesSystemSetPad(NesSystem *s, uint8_t pad) { s->pad = pad; if (s->strobe) s->padShift = pad; }

/*
 * One real CPU context (pc/regs/stack) executes the recompiled program continuously, like the hardware: events
 * (vblank start, end of frame) are processed whenever the program yields at a backward jump after reaching the
 * event's cycle; an NMI is injected exactly like the interrupt sequence (push pc, push P, jump to the vector), so
 * the game's long-running NMI handler (which waits for the sprite-0 hit inside the NMI) behaves as on hardware.
 */
static void injectNmi(NesSystem *s)
{
    NesMachine *m = &s->m;
    const uint16_t vec = (uint16_t)(s->prg[0x3FFAu] | (s->prg[0x3FFBu] << 8));
    nesPush(m, (uint8_t)(s->mainPc >> 8));
    nesPush(m, (uint8_t)(s->mainPc & 0xFFu));
    nesPush(m, nesGetP(m, 0u));
    m->pother |= 0x04u;
    m->cyc += 7u;
    s->mainPc = vec;
}

NesSystemResult nesSystemRunFrame(NesSystem *s)
{
    const unsigned long target = s->frames + 1u;
    long budget = 100000000L;
    while (s->frames < target) {
        const unsigned long ev = s->frameStart + (s->vblankDone ? NES_SYS_FRAME_CYCLES : NES_SYS_VBLANK_CYCLE);
        int r;
        if (s->m.cyc < ev) {
            s->m.cyc_limit = ev;
            r = nesRunCode(&s->m, s->mainPc, &budget);
            s->m.cyc_limit = 0ul;
            if (r != 1) { s->lastError = NES_SYS_CODE_ERROR; return NES_SYS_CODE_ERROR; }
            s->mainPc = s->m.pc;
            if (budget <= 0) { s->lastError = NES_SYS_CODE_ERROR; return NES_SYS_CODE_ERROR; }
        }
        if (s->m.cyc >= ev) {
            if (!s->vblankDone) {
                catchUp(s, 240u);                 /* the visible image is complete: this is the frame to present */
                s->status |= 0x80u;
                s->vblankDone = 1;
                ++s->frames;
                if (s->ctrl & 0x80u) injectNmi(s);
            } else {
                /* pre-render line: clear vblank/sprite-0/overflow, restart the picture */
                s->status &= 0x1Fu;
                s->s0Line = -1;
                if (s->mask & 0x18u) s->v = s->t;
                s->frameStart += NES_SYS_FRAME_CYCLES;
                s->renderLine = 0u;
                s->vblankDone = 0;
            }
        }
    }
    s->lastError = NES_SYS_OK;
    return NES_SYS_OK;
}
