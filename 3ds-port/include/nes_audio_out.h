#ifndef EXCITEBIKE_NES_AUDIO_OUT_H
#define EXCITEBIKE_NES_AUDIO_OUT_H
#include <stdint.h>
/* 3DS audio output (ndsp, 32 kHz mono). Silent when the DSP firmware is unavailable. */
void nesAudioOutInit(void);
void nesAudioOutPush(const int16_t *samples, unsigned count);
void nesAudioOutExit(void);
#endif
