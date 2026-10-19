#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef union { uint8_t argb; struct { uint8_t b:2, g:2, r:2, a:2; }; } GColor;
typedef struct { int16_t w, h; } GSize;
typedef struct { int16_t x, y; } GPoint;
typedef struct { GPoint origin; GSize size; } GRect;
#define GRect(x,y,w,h) ((GRect){{x,y},{w,h}})
typedef void *GFont;
typedef int GContext;
typedef enum { GTextAlignmentLeft, GTextAlignmentCenter, GTextAlignmentRight } GTextAlignment;
enum { GTextOverflowModeTrailingEllipsis, GTextOverflowModeWordWrap };
GSize graphics_text_layout_get_content_size(const char *text, GFont font, GRect bounds, int overflow, GTextAlignment alignment);
uint32_t resource_get_handle(uint32_t id);
GFont fonts_load_custom_font(uint32_t id);
void fonts_unload_custom_font(GFont font);
void graphics_context_set_text_color(GContext *ctx, GColor colour);
void graphics_draw_text(GContext *ctx, const char *text, GFont font, GRect bounds, int overflow, GTextAlignment alignment, void *attributes);
#define RESOURCE_ID_FONT_SMOOTH_BOLD_24_1 1
#define RESOURCE_ID_FONT_SMOOTH_BOLD_24_2 2
#define RESOURCE_ID_FONT_SMOOTH_BOLD_24_3 3
#define RESOURCE_ID_FONT_SMOOTH_BOLD_28_1 4
#define RESOURCE_ID_FONT_SMOOTH_BOLD_28_2 5
#define RESOURCE_ID_FONT_SMOOTH_BOLD_28_3 6
#define RESOURCE_ID_FONT_SMOOTH_BOLD_34_1 7
#define RESOURCE_ID_FONT_SMOOTH_BOLD_34_2 8
#define RESOURCE_ID_FONT_SMOOTH_BOLD_34_3 9
#define RESOURCE_ID_FONT_SMOOTH_BOLD_42_1 10
#define RESOURCE_ID_FONT_SMOOTH_BOLD_42_2 11
#define RESOURCE_ID_FONT_SMOOTH_BOLD_42_3 12
