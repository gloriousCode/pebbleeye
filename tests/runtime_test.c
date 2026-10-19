#include <assert.h>
#include <stdio.h>
#include <string.h>
#define main watchface_main
#include "../src/main.c"
#undef main

static unsigned audio_triggers, vibe_triggers;
static void (*gesture_action)(void);
static bool gesture_light;
void gentle_gesture_set_enabled(void (*handler)(void)) { gesture_action = handler; }
void gentle_gesture_set_light(bool on) { gesture_light = on; }
void gentle_gesture_deinit(void) { gesture_action = NULL; gesture_light = false; }
void theme_audio_trigger(void) { ++audio_triggers; }
void theme_vibes_trigger(void) { ++vibe_triggers; }
static bool vibes_enabled;
void theme_vibes_set_enabled(bool enabled) { vibes_enabled = enabled; }
void theme_vibes_deinit(void) { vibes_enabled = false; }
static bool audio_enabled;
void theme_audio_set_enabled(bool enabled) { audio_enabled = enabled; }
void theme_audio_deinit(void) { audio_enabled = false; }

static Window fake_window;
static Layer root_layer, face_layer;
static WindowHandlers handlers;
static SpyFaceState rendered;
static bool saved[20], saved_bool[20], initial_light, fail_window, fail_face;
static int saved_int[20], bool_reads, int_reads, bool_writes, int_writes;
static int renders, subscriptions, destroyed_faces, destroyed_windows, children;
static TimeUnits active_units;
static void (*tick_handler)(struct tm *, TimeUnits);

void tick_timer_service_subscribe(TimeUnits units, void (*handler)(struct tm *, TimeUnits)) {
  active_units = units; tick_handler = handler; ++subscriptions;
}
void tick_timer_service_unsubscribe(void) { active_units = 0; tick_handler = NULL; }
BatteryChargeState battery_state_service_peek(void) { return (BatteryChargeState){.charge_percent = 73}; }
bool bluetooth_connection_service_peek(void) { return true; }
bool quiet_time_is_active(void) { return true; }
bool clock_is_24h_style(void) { return true; }
bool light_is_on(void) { return initial_light; }
Tuple *dict_read_first(DictionaryIterator *it) { it->position = 0; return it->count ? &it->tuples[0] : NULL; }
Tuple *dict_read_next(DictionaryIterator *it) { return ++it->position < it->count ? &it->tuples[it->position] : NULL; }
bool persist_exists(int key) { assert(key >= 0 && key < 20); return saved[key]; }
bool persist_read_bool(int key) { assert(key == 14 || key == 17 || key == 18 || key == 19); ++bool_reads; return saved_bool[key]; }
int persist_read_int(int key) { assert(key != 14 && key != 17 && key != 18 && key != 19); ++int_reads; return saved_int[key]; }
int persist_write_bool(int key, bool value) { assert(key == 14 || key == 17 || key == 18 || key == 19); ++bool_writes; saved_bool[key] = value; saved[key] = true; return 1; }
int persist_write_int(int key, int value) { assert(key != 14 && key != 17 && key != 18 && key != 19); ++int_writes; saved_int[key] = value; saved[key] = true; return 4; }
Window *window_create(void) { return fail_window ? NULL : &fake_window; }
void window_destroy(Window *w) { assert(w == &fake_window); ++destroyed_windows; handlers.unload(w); }
void window_set_background_color(Window *w, int colour) { assert(w && colour == GColorBlack); }
void window_set_window_handlers(Window *w, WindowHandlers h) { assert(w); handlers = h; }
void window_stack_push(Window *w, bool animated) { assert(!animated); handlers.load(w); }
Layer *window_get_root_layer(Window *w) { assert(w); return &root_layer; }
GRect layer_get_bounds(Layer *layer) { assert(layer == &root_layer); return (GRect){.size = {260, 260}}; }
void layer_add_child(Layer *parent, Layer *child) { assert(parent == &root_layer && child == &face_layer); ++children; }
Layer *spy_face_layer_create(GRect bounds) { assert(bounds.size.w == 260); return fail_face ? NULL : &face_layer; }
void spy_face_layer_destroy(Layer *layer) { if (layer) { assert(layer == &face_layer); ++destroyed_faces; } }
void spy_face_layer_set_state(Layer *layer, const SpyFaceState *state) { assert(layer == &face_layer); rendered = *state; ++renders; }
void app_message_register_inbox_received(void (*handler)(DictionaryIterator *, void *)) { assert(handler == receive_settings); }
void app_message_register_inbox_dropped(void (*handler)(AppMessageResult, void *)) { assert(handler == inbox_dropped); }
AppMessageResult app_message_open(uint32_t inbox, uint32_t outbox) { assert(inbox >= 128 && outbox >= 32); return APP_MSG_OK; }
void app_message_deregister_callbacks(void) {}
void battery_state_service_subscribe(void (*handler)(BatteryChargeState)) { assert(handler == battery_changed); }
void battery_state_service_unsubscribe(void) {}
void bluetooth_connection_service_subscribe(void (*handler)(bool)) { assert(handler == connection_changed); }
void bluetooth_connection_service_unsubscribe(void) {}
void backlight_service_subscribe(void (*handler)(bool)) { assert(handler == backlight_changed); }
void backlight_service_unsubscribe(void) {}
void app_event_loop(void) {}

static void reset(void) {
  gesture_action = NULL; gesture_light = false; audio_triggers = vibe_triggers = 0;
  window = NULL; face = NULL; subscribed_units = 0; backlight_on = false;
  memset(saved, 0, sizeof(saved)); memset(saved_bool, 0, sizeof(saved_bool)); memset(saved_int, 0, sizeof(saved_int));
  initial_light = fail_window = fail_face = false;
  bool_reads = int_reads = bool_writes = int_writes = 0;
  renders = subscriptions = destroyed_faces = destroyed_windows = children = 0;
  active_units = 0; tick_handler = NULL;
}

static void send_value(int key, int value) {
  TupleValue data = {.int32 = value};
  Tuple tuple = {.key = key, .type = TUPLE_INT, .length = 4, .value = &data};
  DictionaryIterator it = {.tuples = &tuple, .count = 1};
  receive_settings(&it, NULL);
}

static void test_initial_settings_and_tick_policy(void) {
  reset();
  for (int i = 0; i < 5; ++i) saved[(int[]){7, 14, 15, 16, 17}[i]] = true;
  saved_int[7] = 3; saved_int[15] = 2; saved_int[16] = 1;
  saved_bool[14] = true; saved_bool[17] = false; initial_light = true;
  init();
  assert(bool_reads == 2 && int_reads == 3 && renders == 1 && children == 1);
  assert(rendered.date_format == 3 && rendered.display_mode == 2 && rendered.theme == 1);
  assert(!rendered.show_status && rendered.analogue_seconds_enabled && rendered.backlight_on);
  assert(rendered.battery_percent == 73 && rendered.bluetooth_connected && rendered.quiet_time_active && rendered.twenty_four_hour_style);
  assert(active_units == SECOND_UNIT && subscriptions == 1);
  backlight_changed(false);
  assert(active_units == MINUTE_UNIT && subscriptions == 2 && !rendered.backlight_on);
  backlight_changed(false); assert(subscriptions == 2);
  backlight_changed(true); assert(active_units == SECOND_UNIT && subscriptions == 3);
  send_value(15, 0); assert(active_units == MINUTE_UNIT);
  backlight_changed(true); assert(active_units == MINUTE_UNIT);
  send_value(15, 1); assert(active_units == SECOND_UNIT);
  send_value(14, 0); assert(active_units == MINUTE_UNIT);
  backlight_changed(false); backlight_changed(true); assert(active_units == MINUTE_UNIT);
  int before = renders; tick_handler(NULL, MINUTE_UNIT); assert(renders == before + 1);
  deinit(); assert(!face && !active_units && destroyed_faces == 1 && destroyed_windows == 1);
  before = renders; refresh(); battery_changed((BatteryChargeState){0}); connection_changed(false); assert(renders == before);
}

static void test_batch_validation_and_persistence(void) {
  reset(); init();
  const int keys[] = {7, 14, 15, 16, 17, 7, 14, 15, 16, 17, 999, 7, 7};
  const int values[] = {4, 1, 2, 7, 0, -1, 2, 3, 8, -1, 1, 0, 0};
  TupleValue data[ARRAY_LENGTH(keys)]; Tuple tuples[ARRAY_LENGTH(keys)];
  for (unsigned i = 0; i < ARRAY_LENGTH(keys); ++i) {
    data[i].int32 = values[i];
    tuples[i] = (Tuple){.key = keys[i], .type = TUPLE_INT, .length = 4, .value = &data[i]};
  }
  tuples[11].type = TUPLE_CSTRING; tuples[12].length = 1;
  DictionaryIterator it = {.tuples = tuples, .count = ARRAY_LENGTH(keys)};
  int before = renders; receive_settings(&it, NULL);
  assert(renders == before + 1 && bool_writes == 2 && int_writes == 3);
  assert(settings.date_format == 4 && settings.analogue_seconds && settings.display_mode == 2 && settings.theme == 7 && !settings.show_status);
  assert(active_units == MINUTE_UNIT);
  deinit();
  /* Reload the persisted batch; bool values deliberately use separate storage. */
  window = NULL; subscribed_units = 0; init();
  assert(settings.date_format == 4 && settings.analogue_seconds && !settings.show_status);
  assert(settings.display_mode == 2 && settings.theme == 7);
  deinit();
  reset(); saved[7] = saved[15] = saved[16] = true;
  saved_int[7] = -9; saved_int[15] = 9; saved_int[16] = 9;
  init(); assert(settings.date_format == 0 && settings.display_mode == 0 && settings.theme == 0); deinit();
}

static void test_allocation_failures(void) {
  reset(); fail_window = true; init();
  assert(!window && !face && renders == 0 && subscriptions == 0);
  deinit(); assert(destroyed_windows == 0 && destroyed_faces == 0);
  reset(); fail_face = true; init();
  assert(!face && children == 0 && renders == 0);
  backlight_changed(true); send_value(16, 2); assert(renders == 0);
  deinit(); assert(!face && destroyed_windows == 1 && destroyed_faces == 0);
}

static void test_exclusive_gesture_actions(void) {
  reset(); initial_light = true;
  saved[18] = saved_bool[18] = saved[19] = saved_bool[19] = true;
  init();
  assert(gesture_light && gesture_action && !audio_enabled && vibes_enabled);
  assert(!settings.music_enabled && settings.vibrate_theme && !saved_bool[18]);
  gesture_action(); assert(vibe_triggers == 1 && audio_triggers == 0);
  send_value(19, 0); assert(!gesture_action);
  send_value(18, 1);
#if defined(PBL_PLATFORM_GABBRO)
  assert(!gesture_action);
#else
  assert(gesture_action); gesture_action(); assert(audio_triggers == 1);
#endif
  send_value(19, 1); assert(!audio_enabled && vibes_enabled && !saved_bool[18]);
  gesture_action(); assert(vibe_triggers == 2);
  backlight_changed(false); assert(!gesture_light);
  deinit(); assert(!gesture_action);
}

int main(void) {
  test_exclusive_gesture_actions();
  test_initial_settings_and_tick_policy();
  test_batch_validation_and_persistence();
  test_allocation_failures();
  reset(); saved[18] = saved_bool[18] = true; init();
  assert(audio_enabled && settings.music_enabled && bool_reads == 1);
  send_value(18, 0); assert(!audio_enabled && !settings.music_enabled && bool_writes == 1);
  send_value(18, 2); assert(!audio_enabled && bool_writes == 1);
  send_value(18, 1); assert(audio_enabled); deinit(); assert(!audio_enabled);
  reset(); init(); assert(!vibes_enabled && !settings.vibrate_theme); deinit();
  reset(); saved[19] = saved_bool[19] = true; init();
  assert(vibes_enabled && settings.vibrate_theme && bool_reads == 1);
  send_value(19, 0); assert(!vibes_enabled && !settings.vibrate_theme && bool_writes == 1);
  send_value(19, -1); send_value(19, 2); assert(!vibes_enabled && bool_writes == 1);
  send_value(19, 1); assert(vibes_enabled); deinit(); assert(!vibes_enabled);
  puts("Runtime settings, tick subscription and lifecycle tests passed");
  return 0;
}
