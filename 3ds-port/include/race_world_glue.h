#ifndef EXCITEBIKE_RACE_WORLD_GLUE_H
#define EXCITEBIKE_RACE_WORLD_GLUE_H
#include "race_world.h"
#include "bike_physics.h"
#include "race_countdown.h"
#include "race_frame.h"
#include "race_frame_control.h"
#include "race_spawn.h"
#include "race_ai.h"
#include "race_script.h"
#include "race_sprites.h"
#include "race_course_stream.h"

void raceWorldLoad_BikeMovementState(BikeMovementState *s, const RaceWorld *w);
void raceWorldStore_BikeMovementState(const BikeMovementState *s, RaceWorld *w);
void raceWorldLoad_BikeStateMachineState(BikeStateMachineState *s, const RaceWorld *w);
void raceWorldStore_BikeStateMachineState(const BikeStateMachineState *s, RaceWorld *w);
void raceWorldLoad_BikeRaceCountdownState(BikeRaceCountdownState *s, const RaceWorld *w);
void raceWorldStore_BikeRaceCountdownState(const BikeRaceCountdownState *s, RaceWorld *w);
void raceWorldLoad_RaceScriptState(RaceScriptState *s, const RaceWorld *w);
void raceWorldStore_RaceScriptState(const RaceScriptState *s, RaceWorld *w);
void raceWorldLoad_RaceFrameControlState(RaceFrameControlState *s, const RaceWorld *w);
void raceWorldStore_RaceFrameControlState(const RaceFrameControlState *s, RaceWorld *w);
void raceWorldLoad_RaceSpawnState(RaceSpawnState *s, const RaceWorld *w);
void raceWorldStore_RaceSpawnState(const RaceSpawnState *s, RaceWorld *w);
void raceWorldLoad_RaceOpponentCueState(RaceOpponentCueState *s, const RaceWorld *w);
void raceWorldStore_RaceOpponentCueState(const RaceOpponentCueState *s, RaceWorld *w);
void raceWorldLoad_RaceElapsedTimeState(RaceElapsedTimeState *s, const RaceWorld *w);
void raceWorldStore_RaceElapsedTimeState(const RaceElapsedTimeState *s, RaceWorld *w);
void raceWorldLoad_RaceBannerSpriteState(RaceBannerSpriteState *s, const RaceWorld *w);
void raceWorldStore_RaceBannerSpriteState(const RaceBannerSpriteState *s, RaceWorld *w);
void raceWorldLoad_RaceBikeContactState(RaceBikeContactState *s, const RaceWorld *w);
void raceWorldStore_RaceBikeContactState(const RaceBikeContactState *s, RaceWorld *w);
void raceWorldLoad_RaceObstacleHitState(RaceObstacleHitState *s, const RaceWorld *w);
void raceWorldStore_RaceObstacleHitState(const RaceObstacleHitState *s, RaceWorld *w);
void raceWorldLoad_RaceCrashRecoveryState(RaceCrashRecoveryState *s, const RaceWorld *w);
void raceWorldStore_RaceCrashRecoveryState(const RaceCrashRecoveryState *s, RaceWorld *w);
void raceWorldLoad_RaceStartBannerState(RaceStartBannerState *s, const RaceWorld *w);
void raceWorldStore_RaceStartBannerState(const RaceStartBannerState *s, RaceWorld *w);
void raceWorldLoad_RaceEffectSpriteState(RaceEffectSpriteState *s, const RaceWorld *w);
void raceWorldStore_RaceEffectSpriteState(const RaceEffectSpriteState *s, RaceWorld *w);
void raceWorldLoad_RaceCourseStreamState(RaceCourseStreamState *s, const RaceWorld *w);
void raceWorldStore_RaceCourseStreamState(const RaceCourseStreamState *s, RaceWorld *w);
void raceWorldLoad_RaceCollisionState(RaceCollisionState *s, const RaceWorld *w);
void raceWorldStore_RaceCollisionState(const RaceCollisionState *s, RaceWorld *w);
void raceWorldLoad_RaceEngineSoundState(RaceEngineSoundState *s, const RaceWorld *w);
void raceWorldStore_RaceEngineSoundState(const RaceEngineSoundState *s, RaceWorld *w);
void raceWorldLoad_BikeTemperatureState(BikeTemperatureState *s, const RaceWorld *w);
void raceWorldStore_BikeTemperatureState(const BikeTemperatureState *s, RaceWorld *w);
void raceWorldLoad_RaceOffscreenSpriteState(RaceOffscreenSpriteState *s, const RaceWorld *w);
void raceWorldStore_RaceOffscreenSpriteState(const RaceOffscreenSpriteState *s, RaceWorld *w);
void raceWorldLoad_RacePositionSmoothingState(RacePositionSmoothingState *s, const RaceWorld *w);
void raceWorldStore_RacePositionSmoothingState(const RacePositionSmoothingState *s, RaceWorld *w);
void raceWorldLoad_RaceAiState(RaceAiState *s, const RaceWorld *w);
void raceWorldStore_RaceAiState(const RaceAiState *s, RaceWorld *w);
void raceWorldLoad_RaceHideSpritesState(RaceHideSpritesState *s, const RaceWorld *w);
void raceWorldStore_RaceHideSpritesState(const RaceHideSpritesState *s, RaceWorld *w);
#endif
