#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/smooth_fonts.c"

static int loads, unloads, draws;
static uint32_t fail_resource;
static GColor colours[3];
static GFont drawn_fonts[3];
static const GRect expected_bounds = {{4, 7}, {180, 66}};
static GSize measured = {100, 42};
GSize graphics_text_layout_get_content_size(const char *text, GFont font, GRect bounds,
    int overflow, GTextAlignment alignment) {
    assert(strcmp(text, "23:59") == 0 && font);
    assert(bounds.size.w == 1000 && bounds.size.h == 1000);
    assert(overflow == GTextOverflowModeWordWrap && alignment == GTextAlignmentCenter);
    return measured;
}
uint32_t resource_get_handle(uint32_t id) { return id; }
GFont fonts_load_custom_font(uint32_t id) {
    loads++;
    return id == fail_resource ? NULL : (GFont)(uintptr_t)id;
}
void fonts_unload_custom_font(GFont font) { assert(font); unloads++; }
void graphics_context_set_text_color(GContext *ctx, GColor colour) {
    (void)ctx;
    colours[draws] = colour;
}
void graphics_draw_text(GContext *ctx, const char *text, GFont font, GRect bounds,
    int overflow, GTextAlignment alignment, void *attributes) {
    (void)ctx;
    assert(strcmp(text, "23:59") == 0);
    assert(memcmp(&bounds, &expected_bounds, sizeof(bounds)) == 0);
    assert(overflow == GTextOverflowModeTrailingEllipsis);
    assert(alignment == GTextAlignmentCenter && attributes == NULL);
    drawn_fonts[draws++] = font;
}

int main(void) {
    GContext ctx = 0;
    const int sizes[] = {24, 28, 34, 42};
    for (int index = 0; index < 4; index++) {
        int size = sizes[index];
        {
            GFont measured = smooth_font_for_size(size);
            assert(measured != NULL);
            int loaded = loads;
            assert(smooth_font_for_size(size) == measured && loads == loaded);
            for (int fg = 0; fg < 64; fg++) {
                for (int bg = 0; bg < 64; bg++) {
                    GColor foreground = { .argb = 0xc0 | fg };
                    GColor background = { .argb = 0xc0 | bg };
                    draws = 0;
                    assert(smooth_font_draw(&ctx, "23:59", size, expected_bounds, foreground, background));
                    assert(draws == 3 && drawn_fonts[0] == measured);
                    assert(colours[2].argb == foreground.argb);
                    for (int level = 0; level < 3; level++) {
                        for (int shift = 0; shift <= 4; shift += 2) {
                            int f = (fg >> shift) & 3, b = (bg >> shift) & 3;
                            int c = (colours[level].argb >> shift) & 3;
                            assert(c == (f * (level + 1) + b * (2 - level) + 1) / 3);
                        }
                    }
                }
            }
        }
    }
    assert(loads == 12);
    smooth_fonts_unload();
    assert(unloads == 12);
    smooth_fonts_unload();
    assert(unloads == 12);
    assert(smooth_font_for_size(99) == NULL);
    draws = 0;
    measured.w = 181;
    assert(!smooth_font_draw(&ctx, "23:59", 24, expected_bounds,
        (GColor){.argb=255}, (GColor){.argb=192}));
    measured.w = 100;
    measured.h = 67;
    assert(!smooth_font_draw(&ctx, "23:59", 24, expected_bounds,
        (GColor){.argb=255}, (GColor){.argb=192}));
    assert(!smooth_font_draw(&ctx, "AM", 24, expected_bounds,
        (GColor){.argb=255}, (GColor){.argb=192}));
    assert(draws == 0);
    measured.h = 42;
    smooth_fonts_unload();
    int unloaded_before_failure = unloads;
    fail_resource = RESOURCE_ID_FONT_SMOOTH_BOLD_24_2;
    draws = 0;
    assert(!smooth_font_draw(&ctx, "23:59", 24, expected_bounds, (GColor){ .argb = 255 }, (GColor){ .argb = 192 }));
    assert(draws == 0);
    smooth_fonts_unload();
    assert(unloads == unloaded_before_failure + 1);
    puts("Smooth font renderer: colour blending, shared layout, caching and failure fallback pass");
}
