#pragma once
#include <stdbool.h>

// Enable optional double-tap playback on watches with a speaker.
void theme_audio_set_enabled(bool enabled);
// Release this watchface's gesture subscription, timers and playback.
void theme_audio_deinit(void);
