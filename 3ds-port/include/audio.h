#ifndef EXCITEBIKE_AUDIO_H
#define EXCITEBIKE_AUDIO_H

#include <stdint.h>

void audioInit(void);
/* Submit the source game's semantic $00FC/$00FB/$00FD/$00FE/$00FF requests. */
void audioSubmitSourceEvents(uint8_t engine, uint8_t request_fb,
                             uint8_t request_fd, uint8_t request_fe,
                             uint8_t request_ff);
/* Forward the source's last $4015 write (pause mutes with 0, unpause/menu use $0F).
 * Only changes are forwarded, exactly once, ahead of the next driver frame. */
void audioSubmitApuEnable(uint8_t value);
void audioUpdate(void);
void audioExit(void);

#endif
