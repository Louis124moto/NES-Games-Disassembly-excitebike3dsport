#include "race_frame_control.h"

static const uint8_t k_tblC160[5] = { 0x29u, 0x01u, 0x02u, 0x00u, 0x02u };
static const uint8_t k_tblC166[4] = { 0x30u, 0x35u, 0x25u, 0x35u };
static const uint8_t k_tblC16A[5] = { 0x22u, 0x26u, 0x1Cu, 0x22u, 0x22u };
static const uint8_t k_tblC170[4] = { 0x26u, 0x06u, 0x36u, 0x26u };

void racePauseUpdate(RaceFrameControlState *s)
{
    uint8_t a;
    if (s == 0) return;
    s->apu_enable_write = 0u;
    if (s->pause_timer != 0u) {
        s->pause_timer = (uint8_t)(s->pause_timer - 1u);
        return;
    }
    a = (uint8_t)(s->button_press & 0x10u);
    if (a != 0u && s->start_latch == 0u) {
        s->sound_00FB = 0x80u;
        s->start_latch = 0x80u;
        s->sound_00FC = 0x00u;
        s->pause_flag ^= 1u;
        if (s->pause_flag == 0u) {
            s->apu_enable_write = 1u;
            s->apu_enable_value = 0x0Fu;
        }
        s->pause_timer = 0x28u;
        return;
    }
    if (a == 0u) s->start_latch = 0u; /* with Start held, LDA ram_03B0 reloads A: latch unchanged */
    if (s->pause_flag == 0u) return;
    s->apu_enable_write = 1u;
    s->apu_enable_value = 0x00u;
    if ((s->button_press & 0x0Fu) == 0u) return;
    if (s->pause_timer != 0u) return;
    s->pause_timer = 0x28u;
}

void raceHoldInput(RaceFrameControlState *s)
{
    if (s == 0) return;
    s->input_005C = s->button_hold;
}

void raceFrameInputStep(RaceFrameControlState *s)
{
    if (s == 0) return;
    s->apu_enable_write = 0u;
    if (s->flag_03F7 != 0u) { s->unsupported = 1u; return; }
    racePauseUpdate(s);
    raceHoldInput(s);
}

void raceFinishUpdate(RaceFrameControlState *s)
{
    unsigned i;
    if (s == 0) return;
    if (s->timer_0032 == 0u) {
        s->course_id = (uint8_t)(s->course_id + 1u); /* sub_C3AF falls into sub_C3B1 */
        s->frame_flag_0040 = 0u;
        s->frame_flag_0044 = 0u;
        if (s->flag_03F7 != 0u) { s->unsupported = 1u; return; }
        s->flag_0047 = 0u;
        return;
    }
    s->sound_00FC = 0u;
    s->counter_03A9 = 0u;
    s->counter_03E0 = 0u;
    s->timer_003C = 0u;
    if (s->track_finished != 1u) return;
    if (((s->finish_phase_003F & 1u) != 0u) || ((s->finish_phase_003F & 2u) != 0u)) {
        if ((s->timer_0032 & 1u) != 0u) return;
        for (i = 3u; i < 10u; ++i) s->ppu_packet[i] = 0xFEu; /* $0315-$031B */
        return;
    }
    {
        const unsigned y = (s->finish_phase_003F >> 2) & 3u;
        if (s->stream_index > 4u) { s->unsupported = 1u; return; }
        s->ppu_packet[0] = 0x3Fu;
        s->ppu_packet[1] = 0x00u;
        s->ppu_packet[7] = 0x00u;
        s->ppu_packet[2] = 0x04u;
        s->ppu_packet[4] = k_tblC166[y];
        s->ppu_packet[6] = k_tblC170[y];
        s->ppu_packet[3] = k_tblC160[s->stream_index];
        s->ppu_packet[5] = k_tblC16A[s->stream_index];
    }
}
