#pragma once
#include <stdbool.h>

// Enable the optional double-tap rhythm on Time 2 and Round 2.
void theme_vibes_set_enabled(bool enabled);
// Release this watchface's gesture subscription and active vibration.
void theme_vibes_deinit(void);

// Play once after an already recognised gesture, if enabled and permitted.
void theme_vibes_trigger(void);
