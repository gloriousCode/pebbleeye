#include "smooth_fonts.h"

static GFont fonts[4][3];
static const int sizes[] = {24, 28, 34, 42};
static const uint32_t resources[4][3] = {
    {RESOURCE_ID_FONT_SMOOTH_BOLD_24_1, RESOURCE_ID_FONT_SMOOTH_BOLD_24_2, RESOURCE_ID_FONT_SMOOTH_BOLD_24_3},
    {RESOURCE_ID_FONT_SMOOTH_BOLD_28_1, RESOURCE_ID_FONT_SMOOTH_BOLD_28_2, RESOURCE_ID_FONT_SMOOTH_BOLD_28_3},
    {RESOURCE_ID_FONT_SMOOTH_BOLD_34_1, RESOURCE_ID_FONT_SMOOTH_BOLD_34_2, RESOURCE_ID_FONT_SMOOTH_BOLD_34_3},
    {RESOURCE_ID_FONT_SMOOTH_BOLD_42_1, RESOURCE_ID_FONT_SMOOTH_BOLD_42_2, RESOURCE_ID_FONT_SMOOTH_BOLD_42_3},
};

static int size_index(int size)
{
    for (unsigned index = 0; index < sizeof(sizes) / sizeof(sizes[0]); index++) {
        if (size == sizes[index]) return index;
    }
    return -1;
}

GFont smooth_font_for_size(int size)
{
    int index = size_index(size);
    if (index < 0) return NULL;
    for (int level = 0; level < 3; level++) {
        if (!fonts[index][level]) {
            fonts[index][level] = fonts_load_custom_font(resource_get_handle(resources[index][level]));
        }
        if (!fonts[index][level]) return NULL;
    }
    return fonts[index][0];
}

bool smooth_font_draw(GContext *ctx, const char *text, int size, GRect bounds,
    GColor foreground, GColor background)
{
    if (!text || bounds.size.w <= 0 || bounds.size.h <= 0) return false;
    for (const char *character = text; *character; character++) {
        if ((*character < '0' || *character > '9') && *character != ':' && *character != ' ') return false;
    }
    GFont outer = smooth_font_for_size(size);
    if (!outer) return false;
    GSize measured = graphics_text_layout_get_content_size(text, outer,
        GRect(0, 0, 1000, 1000), GTextOverflowModeWordWrap, GTextAlignmentCenter);
    if (measured.w > bounds.size.w || measured.h > bounds.size.h) return false;
    int index = size_index(size);
    // Quantise coverage against the actual backing colour on Pebble's two-bit channels.
    for (int level = 0; level < 3; level++) {
        int coverage = level + 1;
        GColor colour = foreground;
        colour.r = (foreground.r * coverage + background.r * (3 - coverage) + 1) / 3;
        colour.g = (foreground.g * coverage + background.g * (3 - coverage) + 1) / 3;
        colour.b = (foreground.b * coverage + background.b * (3 - coverage) + 1) / 3;
        graphics_context_set_text_color(ctx, colour);
        graphics_draw_text(ctx, text, fonts[index][level], bounds,
            GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }
    return true;
}

void smooth_fonts_unload(void)
{
    for (int index = 0; index < 4; index++) {
        for (int level = 0; level < 3; level++) {
            if (fonts[index][level]) {
                fonts_unload_custom_font(fonts[index][level]);
                fonts[index][level] = NULL;
            }
        }
    }
}
