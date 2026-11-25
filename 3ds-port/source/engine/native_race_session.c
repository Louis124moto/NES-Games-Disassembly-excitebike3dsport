#include <string.h>
#include "native_race_session.h"

int nativeRaceSessionInit(NativeRaceSession *session,
                                     const NativeRaceSetupInput *input,
                                     const NativeRngState *rng)
{
    if (session == 0 || input == 0 || rng == 0) return 0;

    memset(session, 0, sizeof(*session));
    session->rng = *rng;

    /* C869 + C820 semantic gameplay clears; art uploads are owned by the
     * native asset pipeline and are intentionally not emulated here. */
    nativeRaceSetupClearFirstCourseState(&session->movement, &session->states);
    trackInitFirstCourse(&session->track);

    if (nativeRaceSetupFirstCourse(&session->setup, input, &session->rng,
                                   &session->movement, &session->states) == 0) {
        memset(session, 0, sizeof(*session));
        return 0;
    }

    session->course = input->course_index_0043;
    session->flag_004B = input->flag_004B;
    session->counter_03F3 = input->counter_03F3;
    session->counter_03F8 = input->transition_counter_03F8;
    session->countdown.timer_003C = input->countdown_timer_003C;
    /* C820 clears $0340-$03EF, including $03B6. */
    session->countdown.overheat_timer_03B6 = 0u;
    trackUpdateFirstCourse(&session->track, &session->movement);
    bikePhysicsUpdateScreenY(&session->movement); /* E836/DC97 */
    session->sprite_unsupported_mask =
        nativeRaceBuildInitialBikeOam(&session->movement, &session->states, &session->oam);
    session->countdown_render_event = BIKE_COUNTDOWN_RENDER_NONE;
    session->vertical_unsupported_mask = 0u;
    session->initialized = 1u;
    return 1;
}

uint8_t nativeRaceSessionUpdateSprites(NativeRaceSession *session)
{
    if (session == 0 || session->initialized == 0u) return 0x0Fu;
    bikePhysicsUpdateScreenY(&session->movement);
    session->sprite_unsupported_mask =
        nativeRaceBuildInitialBikeOam(&session->movement, &session->states, &session->oam);
    return session->sprite_unsupported_mask;
}

void nativeRaceSessionUpdateBikeStates(NativeRaceSession *session)
{
    if (session == 0 || session->initialized == 0u) return;
    bikePhysicsUpdateBikeStates(&session->states);
}

BikeRaceCountdownRenderEvent nativeRaceSessionUpdateCountdown(NativeRaceSession *session)
{
    if (session == 0 || session->initialized == 0u)
        return BIKE_COUNTDOWN_RENDER_NONE;

    session->countdown_render_event =
        bikePhysicsUpdateRaceCountdown(&session->countdown,
                                       &session->movement,
                                       &session->states);
    return session->countdown_render_event;
}

uint8_t nativeRaceSessionUpdateVertical(NativeRaceSession *session)
{
    if (session == 0 || session->initialized == 0u) return 0x0Fu;

    session->vertical_unsupported_mask =
        bikePhysicsUpdateFirstCourseVertical(&session->movement,
                                             &session->states);
    trackUpdateFirstCourse(&session->track, &session->movement);
    return session->vertical_unsupported_mask;
}

