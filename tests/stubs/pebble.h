#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#define ARRAY_LENGTH(a) (sizeof(a) / sizeof((a)[0]))
#define APP_LOG_LEVEL_WARNING 1
#define APP_LOG_LEVEL_ERROR 2
static inline void pebble_test_log(int level, const char *format, ...) {
  (void)level; (void)format;
}
#define APP_LOG(...) pebble_test_log(__VA_ARGS__)
#define GColorBlack 0
#define APP_MSG_OK 0
typedef int AppMessageResult;
typedef enum { SECOND_UNIT = 1, MINUTE_UNIT = 2 } TimeUnits;
typedef struct { int16_t x, y; } GPoint;
typedef struct { int16_t w, h; } GSize;
typedef struct { GPoint origin; GSize size; } GRect;
typedef struct Layer { int unused; } Layer;
typedef struct Window { int unused; } Window;
typedef struct { void (*load)(Window *); void (*unload)(Window *); } WindowHandlers;
typedef struct { uint8_t charge_percent; bool is_charging, is_plugged; } BatteryChargeState;
typedef enum { TUPLE_BYTE_ARRAY, TUPLE_CSTRING, TUPLE_UINT, TUPLE_INT } TupleType;
typedef union { int32_t int32; uint32_t uint32; } TupleValue;
typedef struct { uint32_t key; TupleType type; uint16_t length; TupleValue *value; } Tuple;
typedef struct { Tuple *tuples; size_t count, position; } DictionaryIterator;
void tick_timer_service_subscribe(TimeUnits units, void (*handler)(struct tm *, TimeUnits));
void tick_timer_service_unsubscribe(void);
BatteryChargeState battery_state_service_peek(void);
bool bluetooth_connection_service_peek(void);
bool quiet_time_is_active(void);
bool clock_is_24h_style(void);
bool light_is_on(void);
Tuple *dict_read_first(DictionaryIterator *iterator);
Tuple *dict_read_next(DictionaryIterator *iterator);
bool persist_exists(int key);
bool persist_read_bool(int key);
int persist_read_int(int key);
int persist_write_bool(int key, bool value);
int persist_write_int(int key, int value);
Window *window_create(void);
void window_destroy(Window *window);
void window_set_background_color(Window *window, int colour);
void window_set_window_handlers(Window *window, WindowHandlers handlers);
void window_stack_push(Window *window, bool animated);
Layer *window_get_root_layer(Window *window);
GRect layer_get_bounds(Layer *layer);
void layer_add_child(Layer *parent, Layer *child);
void app_message_register_inbox_received(void (*handler)(DictionaryIterator *, void *));
void app_message_register_inbox_dropped(void (*handler)(AppMessageResult, void *));
AppMessageResult app_message_open(uint32_t inbox, uint32_t outbox);
void app_message_deregister_callbacks(void);
void battery_state_service_subscribe(void (*handler)(BatteryChargeState));
void battery_state_service_unsubscribe(void);
void bluetooth_connection_service_subscribe(void (*handler)(bool));
void bluetooth_connection_service_unsubscribe(void);
void backlight_service_subscribe(void (*handler)(bool));
void backlight_service_unsubscribe(void);
void app_event_loop(void);
#define APP_LOG_LEVEL_INFO 3
#define RESOURCE_ID_THEME_AUDIO 100
struct AppTimer { int unused; };
typedef struct AppTimer AppTimer;
typedef void (*AppTimerCallback)(void *);
typedef enum { ACCEL_AXIS_X, ACCEL_AXIS_Y, ACCEL_AXIS_Z } AccelAxisType;
typedef void (*AccelTapHandler)(AccelAxisType, int32_t);
typedef void *ResHandle;
typedef enum { SpeakerPcmFormat_16kHz_8bit = 1 } SpeakerPcmFormat;
typedef enum { SpeakerStatusIdle, SpeakerStatusPlaying, SpeakerStatusDraining } SpeakerStatus;
typedef enum { SpeakerFinishReasonDone, SpeakerFinishReasonStopped, SpeakerFinishReasonPreempted, SpeakerFinishReasonError } SpeakerFinishReason;
typedef void (*SpeakerFinishedCallback)(SpeakerFinishReason, void *);
void app_timer_cancel(AppTimer *timer);
AppTimer *app_timer_register(uint32_t delay, AppTimerCallback callback, void *context);
void accel_tap_service_subscribe(AccelTapHandler handler);
void accel_tap_service_unsubscribe(void);
uint16_t time_ms(time_t *seconds, uint16_t *milliseconds);
ResHandle resource_get_handle(uint32_t id);
size_t resource_size(ResHandle resource);
size_t resource_load_byte_range(ResHandle resource, uint32_t offset, uint8_t *buffer, size_t count);
bool speaker_is_muted(void);
SpeakerStatus speaker_get_status(void);
bool speaker_stream_open(SpeakerPcmFormat format, uint8_t volume);
uint32_t speaker_stream_write(const void *data, uint32_t count);
void speaker_stream_close(void);
void speaker_stop(void);
void speaker_set_finish_callback(SpeakerFinishedCallback callback, void *context);
