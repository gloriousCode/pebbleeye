#pragma once
#include <pebble.h>

// Outer coverage mask: use this font for measuring the exact rendered metrics.
GFont smooth_font_for_size(int size);
// Draw centred text only if its unwrapped SDK measurement fits the supplied frame.
// Returns false without drawing for unsupported sizes, unavailable resources or overflow.
bool smooth_font_draw(GContext *ctx, const char *text, int size, GRect bounds,
    GColor foreground, GColor background);
void smooth_fonts_unload(void);
