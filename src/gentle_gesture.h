#pragma once
#include <stdbool.h>

// Detect gentle double taps only during a bounded backlight interaction window.
void gentle_gesture_set_enabled(void (*handler)(void));
void gentle_gesture_set_light(bool on);
void gentle_gesture_deinit(void);

// End the current window; the next backlight wake may start another.
void gentle_gesture_pause(void);
