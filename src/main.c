#include <pebble.h>
#include "settings.h"
#include "spy_face.h"
#include "theme_audio.h"

static Window *window;
static Layer *face;
static Settings settings;
static TimeUnits subscribed_units;
static bool backlight_on;

static void tick(struct tm *now, TimeUnits units);

static void update_tick_subscription(void) {
  TimeUnits wanted = settings_want_seconds(&settings, backlight_on)
      ? SECOND_UNIT : MINUTE_UNIT;
  if (subscribed_units != wanted) {
    tick_timer_service_subscribe(wanted, tick);
    subscribed_units = wanted;
  }
}

static void refresh(void) {
  if (!face) return;
  time_t seconds = time(NULL);
  struct tm *now = localtime(&seconds);
  if (!now) return;
  BatteryChargeState battery = battery_state_service_peek();
  SpyFaceState state = {
    .hour = now->tm_hour, .minute = now->tm_min, .second = now->tm_sec,
    .day = now->tm_mday, .month = now->tm_mon, .year = now->tm_year + 1900,
    .weekday = now->tm_wday, .date_format = settings.date_format,
    .display_mode = settings.display_mode, .theme = settings.theme,
    .show_status = settings.show_status, .battery_percent = battery.charge_percent,
    .bluetooth_connected = bluetooth_connection_service_peek(),
    .backlight_on = backlight_on, .quiet_time_active = quiet_time_is_active(),
    .analogue_seconds_enabled = settings.analogue_seconds,
    .twenty_four_hour_style = clock_is_24h_style()
  };
  spy_face_layer_set_state(face, &state);
}

static void tick(struct tm *now, TimeUnits units) {
  (void)now;
  (void)units;
  refresh();
}

static void battery_changed(BatteryChargeState state) {
  (void)state;
  refresh();
}

static void connection_changed(bool connected) {
  (void)connected;
  refresh();
}

static void backlight_changed(bool on) {
  backlight_on = on;
  update_tick_subscription();
  refresh();
}

static void receive_settings(DictionaryIterator *iterator, void *context) {
  (void)context;
  for (Tuple *tuple = dict_read_first(iterator); tuple; tuple = dict_read_next(iterator)) {
    if ((tuple->type != TUPLE_INT && tuple->type != TUPLE_UINT) || tuple->length != 4) continue;
    int value = tuple->value->int32;
    if (settings_apply(&settings, tuple->key, value)) {
      if (tuple->key == 14 || tuple->key == 17 || tuple->key == 18) {
        persist_write_bool(tuple->key, value != 0);
      } else {
        persist_write_int(tuple->key, value);
      }
    }
  }
  theme_audio_set_enabled(settings.music_enabled);
  update_tick_subscription();
  refresh();
}

static void inbox_dropped(AppMessageResult reason, void *context) {
  (void)context;
  APP_LOG(APP_LOG_LEVEL_WARNING, "Settings delivery dropped: %d", reason);
}

static void load(Window *loaded_window) {
  Layer *root = window_get_root_layer(loaded_window);
  face = spy_face_layer_create(layer_get_bounds(root));
  if (!face) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unable to allocate watchface layer");
    return;
  }
  layer_add_child(root, face);
  refresh();
}

static void unload(Window *unloaded_window) {
  (void)unloaded_window;
  spy_face_layer_destroy(face);
  face = NULL;
}

static void init(void) {
  settings = settings_defaults();
  const int keys[] = {7, 14, 15, 16, 17, 18};
  for (unsigned i = 0; i < ARRAY_LENGTH(keys); ++i) {
    int key = keys[i];
    if (persist_exists(key)) {
      int value = (key == 14 || key == 17 || key == 18) ? persist_read_bool(key) : persist_read_int(key);
      settings_apply(&settings, key, value);
    }
  }
  backlight_on = light_is_on();
  window = window_create();
  if (!window) return;
  window_set_background_color(window, GColorBlack);
  window_set_window_handlers(window, (WindowHandlers){.load = load, .unload = unload});
  window_stack_push(window, false);
  app_message_register_inbox_received(receive_settings);
  app_message_register_inbox_dropped(inbox_dropped);
  AppMessageResult result = app_message_open(256, 64);
  if (result != APP_MSG_OK) APP_LOG(APP_LOG_LEVEL_ERROR, "Cannot open settings inbox: %d", result);
  battery_state_service_subscribe(battery_changed);
  bluetooth_connection_service_subscribe(connection_changed);
  backlight_service_subscribe(backlight_changed);
  theme_audio_set_enabled(settings.music_enabled);
  update_tick_subscription();
}

static void deinit(void) {
  theme_audio_deinit();
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  backlight_service_unsubscribe();
  app_message_deregister_callbacks();
  if (window) window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}
