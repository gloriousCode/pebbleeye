#pragma once
#include <stdbool.h>

// Enable the optional double-tap rhythm on Round 2.
void theme_vibes_set_enabled(bool enabled);
// Release this watchface's gesture subscription and active vibration.
void theme_vibes_deinit(void);
