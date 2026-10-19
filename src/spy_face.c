#include "spy_face.h"
#include "smooth_fonts.h"
#include "spy_face_geometry.h"
#include <stdio.h>
#include <string.h>

#define DEG_TO_TRIG(degrees) ((TRIG_MAX_ANGLE * (degrees)) / 360)

typedef enum {
  SPY_FACE_DATE_FORMAT_DD_MM_YY,
  SPY_FACE_DATE_FORMAT_MM_DD_YYYY,
  SPY_FACE_DATE_FORMAT_MON_D_AUG,
  SPY_FACE_DATE_FORMAT_DD_SLASH_MM,
  SPY_FACE_DATE_FORMAT_MM_SLASH_DD,
} SpyFaceDateFormat;

typedef struct {
  SpyFaceState state;
} SpyFaceLayerData;

typedef struct {
  GColor bright;
  GColor muted;
  GColor panel;
} SpyPalette;

static GColor spy_colour(int red, int green, int blue)
{
  return GColorFromRGB(red, green, blue);
}

static SpyPalette spy_palette(int theme)
{
  switch (theme) {
    default: return (SpyPalette) { spy_colour(170, 255, 170), spy_colour(0, 170, 0), spy_colour(0, 85, 0) };
    case 1: return (SpyPalette) { spy_colour(255, 255, 170), spy_colour(255, 255, 85), spy_colour(85, 85, 0) };
    case 2: return (SpyPalette) { spy_colour(170, 255, 255), spy_colour(0, 170, 255), spy_colour(0, 0, 85) };
    case 3: return (SpyPalette) { spy_colour(255, 170, 255), spy_colour(170, 85, 255), spy_colour(85, 0, 85) };
    case 4: return (SpyPalette) { spy_colour(170, 255, 255), spy_colour(85, 170, 170), spy_colour(0, 85, 85) };
    case 5: return (SpyPalette) { spy_colour(255, 170, 85), spy_colour(255, 85, 85), spy_colour(85, 0, 0) };
    case 6: return (SpyPalette) { spy_colour(255, 255, 255), spy_colour(170, 170, 170), spy_colour(85, 85, 85) };
    case 7: return (SpyPalette) { spy_colour(255, 170, 170), spy_colour(255, 85, 85), spy_colour(85, 0, 0) };
  }
}

static GPoint spy_polar_point(GPoint centre, int radius, int degrees)
{
  int32_t angle = DEG_TO_TRIG(degrees);
  return GPoint(centre.x + (int)((int32_t)radius * sin_lookup(angle) / TRIG_MAX_RATIO),
      centre.y - (int)((int32_t)radius * cos_lookup(angle) / TRIG_MAX_RATIO));
}

static GRect spy_rect(SpyFaceRect rect)
{
  return GRect(rect.x, rect.y, rect.w, rect.h);
}

static void spy_draw_text(GContext *ctx, const char *text, GFont font,
    GRect frame, GColor colour)
{
  graphics_context_set_text_color(ctx, colour);
  graphics_draw_text(ctx, text, font, frame, GTextOverflowModeTrailingEllipsis,
      GTextAlignmentCenter, NULL);
}

static void spy_draw_radial_segment(GContext *ctx, GPoint centre, int outer_radius,
    int ring_width, int start_degrees, int end_degrees, GColor colour)
{
  GRect ring = GRect(centre.x - outer_radius, centre.y - outer_radius,
      outer_radius * 2, outer_radius * 2);

  graphics_context_set_fill_color(ctx, colour);
  graphics_fill_radial(ctx, ring, GOvalScaleModeFitCircle, ring_width,
      DEG_TO_TRIG(start_degrees), DEG_TO_TRIG(end_degrees));
}

static void spy_draw_outlined_radial_segment(GContext *ctx, SpyFaceGeometry geometry,
    int start_degrees, int end_degrees, GColor outline_colour, GColor fill_colour)
{
  GPoint centre = GPoint(geometry.centre_x, geometry.centre_y);
  int inset = spy_face_scaled_value(geometry.min_side, 2);
  int fill_width = geometry.ring_width - (inset * 2);
  if (fill_width < 1) {
    fill_width = 1;
  }

  spy_draw_radial_segment(ctx, centre, geometry.outer_radius, geometry.ring_width,
      start_degrees, end_degrees, outline_colour);
  spy_draw_radial_segment(ctx, centre, geometry.outer_radius - inset, fill_width,
      start_degrees, end_degrees, fill_colour);
}

static void spy_draw_ring(GContext *ctx, SpyFaceGeometry geometry)
{
  static const int warm_starts[] = { 309, 288, 267, 246, 225, 207, 195, 183 };
  static const int cool_starts[] = { 39, 60, 81, 102, 123, 141, 153, 165 };
  static const int warm_colours[][3] = {
    { 255, 28, 18 },
    { 255, 66, 20 },
    { 255, 100, 24 },
    { 255, 139, 31 },
    { 255, 178, 43 },
    { 255, 214, 65 },
  };
  static const int cool_colours[][3] = {
    { 6, 12, 112 },
    { 16, 31, 145 },
    { 30, 57, 176 },
    { 49, 90, 205 },
    { 75, 124, 225 },
    { 111, 168, 238 },
  };
  const int segment_degrees = 12;
  const int ring_segment_count = 6;

  for (int i = 0; i < ring_segment_count; i++) {
    spy_draw_outlined_radial_segment(ctx, geometry, warm_starts[i],
        warm_starts[i] + segment_degrees,
        spy_colour(255, 232, 146),
        spy_colour(warm_colours[i][0], warm_colours[i][1], warm_colours[i][2]));
    spy_draw_outlined_radial_segment(ctx, geometry, cool_starts[i],
        cool_starts[i] + segment_degrees,
        spy_colour(157, 207, 255),
        spy_colour(cool_colours[i][0], cool_colours[i][1], cool_colours[i][2]));
  }
}

static void spy_format_time(char *buffer, size_t buffer_size, const SpyFaceState *state)
{
  int hour = state->hour;
  if (!state->twenty_four_hour_style) {
    hour %= 12;
    if (hour == 0) {
      hour = 12;
    }
  }

  snprintf(buffer, buffer_size, "%02d:%02d", hour, state->minute);
  buffer[buffer_size - 1] = '\0';
}

static void spy_format_ampm(char *buffer, size_t buffer_size, const SpyFaceState *state)
{
  if (state->twenty_four_hour_style) {
    buffer[0] = '\0';
    return;
  }

  snprintf(buffer, buffer_size, "%s", state->hour >= 12 ? "PM" : "AM");
  buffer[buffer_size - 1] = '\0';
}

static void spy_format_date(char *buffer, size_t buffer_size, const SpyFaceState *state)
{
  static const char *weekdays[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };
  static const char *months[] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC",
  };
  int month = state->month + 1;
  int day = state->day;
  int weekday = state->weekday;
  int year = state->year;
  if (month < 1) {
    month = 1;
  } else if (month > 12) {
    month = 12;
  }
  if (day < 1) {
    day = 1;
  } else if (day > 31) {
    day = 31;
  }
  if (weekday < 0 || weekday > 6) {
    weekday = 0;
  }
  if (year < 0) {
    year = 0;
  } else if (year > 9999) {
    year = 9999;
  }

  switch (state->date_format) {
    case SPY_FACE_DATE_FORMAT_MM_DD_YYYY:
      snprintf(buffer, buffer_size, "%02d-%02d-%04d", month, day, year);
      break;
    case SPY_FACE_DATE_FORMAT_MON_D_AUG:
      snprintf(buffer, buffer_size, "%s %d %s", weekdays[weekday], day,
          months[month - 1]);
      break;
    case SPY_FACE_DATE_FORMAT_DD_SLASH_MM:
      snprintf(buffer, buffer_size, "%02d/%02d", day, month);
      break;
    case SPY_FACE_DATE_FORMAT_MM_SLASH_DD:
      snprintf(buffer, buffer_size, "%02d/%02d", month, day);
      break;
    default:
      snprintf(buffer, buffer_size, "%02d-%02d-%02d", day, month, year % 100);
      break;
  }
  buffer[buffer_size - 1] = '\0';
}

static void spy_draw_dial(GContext *ctx, GPoint centre, int radius,
    const SpyFaceState *state, SpyPalette palette)
{
  graphics_context_set_fill_color(ctx, palette.panel);
  graphics_fill_circle(ctx, centre, radius);
  graphics_context_set_stroke_color(ctx, palette.muted);
  graphics_draw_circle(ctx, centre, radius);
  for (int tick = 0; tick < 60; tick++) {
    bool major = tick % 5 == 0;
    if (radius < 50 && !major) continue;
    graphics_context_set_stroke_color(ctx, major ? palette.bright : palette.muted);
    graphics_context_set_stroke_width(ctx, major ? 2 : 1);
    graphics_draw_line(ctx, spy_polar_point(centre, radius - (major ? 8 : 4), tick * 6),
        spy_polar_point(centre, radius - 2, tick * 6));
  }
  // Stroke primitives avoid allocating paths on every frame.
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, radius < 50 ? 4 : 6);
  graphics_draw_line(ctx, centre, spy_polar_point(centre, radius * 53 / 100,
      state->hour % 12 * 30 + state->minute / 2));
  graphics_context_set_stroke_width(ctx, radius < 50 ? 3 : 4);
  graphics_draw_line(ctx, centre, spy_polar_point(centre, radius - 12, state->minute * 6));
  graphics_context_set_stroke_width(ctx, 1);
  if (state->backlight_on && state->analogue_seconds_enabled) {
    graphics_context_set_stroke_color(ctx, GColorSunsetOrange);
    graphics_draw_line(ctx, centre, spy_polar_point(centre, radius - 8, state->second * 6));
  }
  graphics_context_set_fill_color(ctx, GColorSunsetOrange);
  graphics_fill_circle(ctx, centre, 3);
}

static void spy_draw_time(GContext *ctx, GRect frame, SpyFaceGeometry geometry,
    SpyFaceLayerData *data, SpyPalette palette)
{
  char text[8];
  spy_format_time(text, sizeof(text), &data->state);
  int size = geometry.min_side >= 220 ? 42 : 34;
  if (smooth_font_draw(ctx, text, size, frame, palette.bright, palette.panel)) return;
  GFont font = fonts_get_system_font(geometry.min_side >= 220
      ? FONT_KEY_BITHAM_42_BOLD : FONT_KEY_BITHAM_34_MEDIUM_NUMBERS);
  spy_draw_text(ctx, text, font, frame, palette.bright);
}

static void spy_face_layer_update_proc(Layer *layer, GContext *ctx)
{
  SpyFaceLayerData *data = layer_get_data(layer);
  const SpyFaceState *state = &data->state;
  GRect bounds = layer_get_bounds(layer);
  SpyFaceGeometry geometry = spy_face_geometry_for_bounds(bounds.size.w,
      bounds.size.h, PBL_IF_ROUND_ELSE(true, false));
  SpyPalette palette = spy_palette(state->theme);
  graphics_context_set_antialiased(ctx, true);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  spy_draw_ring(ctx, geometry);
  GFont label_font = fonts_get_system_font(geometry.min_side >= 220
      ? FONT_KEY_GOTHIC_18_BOLD : FONT_KEY_GOTHIC_14_BOLD);
  char date[16];
  spy_format_date(date, sizeof(date), state);
  spy_draw_text(ctx, date, label_font, spy_rect(geometry.date), palette.bright);
  if (state->show_status) {
    char status[24];
    snprintf(status, sizeof(status), "%s  %d%%%s", state->bluetooth_connected
        ? "LINK" : "NO LINK", state->battery_percent, state->quiet_time_active ? "  Q" : "");
    spy_draw_text(ctx, status, label_font, spy_rect(geometry.status), palette.bright);
  }
  if (state->display_mode == 1) {
    spy_draw_dial(ctx, GPoint(geometry.centre_x, geometry.centre_y),
        geometry.analogue_radius, state, palette);
  } else if (state->display_mode == 2) {
    spy_draw_dial(ctx, GPoint(geometry.centre_x, geometry.centre_y),
        geometry.analogue_radius, state, palette);
    // An opaque inset keeps the time legible while the full-size hands pass behind it.
    GRect readout = spy_rect(geometry.combined_time);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, readout, 4, GCornersAll);
    graphics_context_set_stroke_color(ctx, palette.muted);
    graphics_draw_round_rect(ctx, readout, 4);
    char time[8];
    spy_format_time(time, sizeof(time), state);
    GRect time_frame = GRect(readout.origin.x + 2, readout.origin.y - 1,
        readout.size.w - 4, readout.size.h);
    int size = geometry.min_side >= 220 ? 28 : 24;
    if (!smooth_font_draw(ctx, time, size, time_frame, palette.bright, GColorBlack)) {
      GFont overlay_font = fonts_get_system_font(geometry.min_side >= 220
          ? FONT_KEY_GOTHIC_28_BOLD : FONT_KEY_GOTHIC_24_BOLD);
      spy_draw_text(ctx, time, overlay_font, time_frame, palette.bright);
    }
  } else {
    GRect display = spy_rect(geometry.display);
    graphics_context_set_fill_color(ctx, palette.panel);
    graphics_fill_rect(ctx, display, 6, GCornersAll);
    graphics_context_set_stroke_color(ctx, palette.muted);
    graphics_draw_round_rect(ctx, display, 6);
    spy_draw_text(ctx, "PEBBLEEYE", label_font,
        GRect(display.origin.x, display.origin.y + 3, display.size.w, 22), palette.bright);
    spy_draw_time(ctx, GRect(display.origin.x, geometry.centre_y - 26,
        display.size.w, 54), geometry, data, palette);
    char ampm[4];
    spy_format_ampm(ampm, sizeof(ampm), state);
    spy_draw_text(ctx, state->twenty_four_hour_style ? "24H" : ampm,
        label_font, GRect(display.origin.x, geometry.centre_y + 28,
        display.size.w, 22), palette.bright);
  }
}

Layer *spy_face_layer_create(GRect frame)
{
  Layer *layer = layer_create_with_data(frame, sizeof(SpyFaceLayerData));
  if (layer == NULL) return NULL;
  SpyFaceLayerData *data = layer_get_data(layer);
  memset(data, 0, sizeof(*data));
  data->state.battery_percent = 100;
  data->state.bluetooth_connected = true;
  data->state.show_status = true;
  layer_set_update_proc(layer, spy_face_layer_update_proc);
  return layer;
}

void spy_face_layer_destroy(Layer *layer)
{
  if (layer == NULL) return;
  smooth_fonts_unload();
  layer_destroy(layer);
}

void spy_face_layer_set_state(Layer *layer, const SpyFaceState *state)
{
  if (layer == NULL || state == NULL) return;
  SpyFaceLayerData *data = layer_get_data(layer);
  data->state = *state;
  layer_mark_dirty(layer);
}
