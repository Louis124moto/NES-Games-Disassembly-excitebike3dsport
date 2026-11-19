#include "nes_audio_out.h"
#include <string.h>
#include <3ds.h>

#define BUFS 6
#define BUF_SAMPLES 640
static ndspWaveBuf s_wave[BUFS];
static int16_t *s_data;
static int s_ok;
static unsigned s_next;

void nesAudioOutInit(void)
{
    unsigned i;
    s_ok = 0;
    if (R_FAILED(ndspInit())) return;                 /* needs dspfirm.cdc on the SD card (standard homebrew requirement) */
    s_data = (int16_t *)linearAlloc(sizeof(int16_t) * BUF_SAMPLES * BUFS);
    if (s_data == NULL) { ndspExit(); return; }
    memset(s_data, 0, sizeof(int16_t) * BUF_SAMPLES * BUFS);
    ndspSetOutputMode(NDSP_OUTPUT_MONO);
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
    ndspChnSetRate(0, 32000.0f);
    ndspChnSetFormat(0, NDSP_FORMAT_MONO_PCM16);
    memset(s_wave, 0, sizeof(s_wave));
    for (i = 0; i < BUFS; ++i) { s_wave[i].data_vaddr = &s_data[i * BUF_SAMPLES]; s_wave[i].status = NDSP_WBUF_DONE; }
    s_ok = 1;
}

void nesAudioOutPush(const int16_t *samples, unsigned count)
{
    ndspWaveBuf *w;
    if (!s_ok || samples == NULL || count == 0u) return;
    w = &s_wave[s_next];
    if (w->status != NDSP_WBUF_DONE) return;          /* the DSP is behind: drop this frame's audio instead of stalling */
    if (count > BUF_SAMPLES) count = BUF_SAMPLES;
    memcpy((void *)w->data_vaddr, samples, count * sizeof(int16_t));
    w->nsamples = count;
    DSP_FlushDataCache((void *)w->data_vaddr, count * sizeof(int16_t));
    ndspChnWaveBufAdd(0, w);
    s_next = (s_next + 1u) % BUFS;
}

void nesAudioOutExit(void)
{
    if (!s_ok) return;
    ndspChnWaveBufClear(0);
    linearFree(s_data);
    ndspExit();
    s_ok = 0;
}
