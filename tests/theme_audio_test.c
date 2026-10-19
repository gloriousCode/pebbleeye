#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/theme_audio.c"

#if defined(PBL_PLATFORM_EMERY)
static AppTimer fake_timer;
static AppTimerCallback timer_callback;
static AccelTapHandler tap_handler;
static SpeakerFinishedCallback finish_callback;
static bool mute, quiet, fail_timer, fail_read, fail_open, wrong_size, finish_on_close;
static SpeakerStatus status;
static uint32_t now_ms, write_limit, total_written, opens, closes, stops, timer_calls;
static int resource_marker;

void app_timer_cancel(AppTimer *value) { assert(value == &fake_timer); timer_callback = NULL; }
AppTimer *app_timer_register(uint32_t delay, AppTimerCallback callback, void *context) {
  assert(delay == 10 && !context && !timer_callback); ++timer_calls;
  if (fail_timer) return NULL;
  timer_callback = callback; return &fake_timer;
}
void accel_tap_service_subscribe(AccelTapHandler handler) { assert(!tap_handler); tap_handler = handler; }
void accel_tap_service_unsubscribe(void) { tap_handler = NULL; }
uint16_t time_ms(time_t *seconds, uint16_t *milliseconds) { *seconds = now_ms / 1000; *milliseconds = now_ms % 1000; return *milliseconds; }
ResHandle resource_get_handle(uint32_t id) { assert(id == RESOURCE_ID_THEME_AUDIO); return &resource_marker; }
size_t resource_size(ResHandle value) { assert(value == &resource_marker); return wrong_size ? 1 : CLIP_BYTES; }
size_t resource_load_byte_range(ResHandle value, uint32_t position, uint8_t *bytes, size_t count) {
  assert(value == &resource_marker && position == total_written && count <= BUFFER_BYTES);
  if (fail_read) return 0;
  for (size_t i = 0; i < count; ++i) bytes[i] = (uint8_t)(position + i);
  return count;
}
bool speaker_is_muted(void) { return mute; }
bool quiet_time_is_active(void) { return quiet; }
SpeakerStatus speaker_get_status(void) { return status; }
bool speaker_stream_open(SpeakerPcmFormat format, uint8_t volume) {
  assert(format == SpeakerPcmFormat_16kHz_8bit && volume == 40 && status == SpeakerStatusIdle);
  if (fail_open) return false;
  ++opens; total_written = 0; status = SpeakerStatusPlaying; return true;
}
uint32_t speaker_stream_write(const void *data, uint32_t count) {
  uint32_t accepted = count < write_limit ? count : write_limit;
  const uint8_t *bytes = data;
  for (uint32_t i = 0; i < accepted; ++i) assert(bytes[i] == (uint8_t)(total_written + i));
  total_written += accepted; return accepted;
}
void speaker_stream_close(void) {
  ++closes; status = SpeakerStatusDraining;
  if (finish_on_close) { SpeakerFinishedCallback callback = finish_callback; status = SpeakerStatusIdle; callback(SpeakerFinishReasonDone, NULL); }
}
void speaker_stop(void) { ++stops; status = SpeakerStatusIdle; }
void speaker_set_finish_callback(SpeakerFinishedCallback callback, void *context) { assert(!context); finish_callback = callback; }

static void reset(void) {
  theme_audio_deinit();
  assert(!timer_callback && !tap_handler && !playing);
  mute = quiet = fail_timer = fail_read = fail_open = wrong_size = finish_on_close = false;
  status = SpeakerStatusIdle; now_ms = total_written = opens = closes = stops = timer_calls = 0;
  write_limit = BUFFER_BYTES; finish_callback = NULL;
}
static void at(uint32_t milliseconds) { now_ms = milliseconds; assert(tap_handler); tap_handler(ACCEL_AXIS_Z, 1); }
static void pair(void) { at(now_ms + 1000); at(now_ms + 200); }
static void advance(void) {
  assert(timer_callback); AppTimerCallback callback = timer_callback; timer_callback = NULL; now_ms += 10; callback(NULL);
}
static void finish(SpeakerFinishReason reason) {
  assert(finish_callback); SpeakerFinishedCallback callback = finish_callback;
  status = SpeakerStatusIdle; callback(reason, NULL);
}
static void test_gesture_and_guards(void) {
  reset(); assert(!tap_handler); theme_audio_set_enabled(true); theme_audio_set_enabled(true);
  at(0); at(99); assert(!opens); at(100); assert(opens == 1);
  pair(); assert(opens == 1); theme_audio_set_enabled(false); assert(stops == 1 && !timer_callback && !tap_handler);
  theme_audio_set_enabled(true); at(10000); at(10501); assert(opens == 1); at(11001); assert(opens == 2);
  finish(SpeakerFinishReasonDone); assert(!timer_callback && !playing && stops == 1);
  mute = true; pair(); assert(opens == 2); mute = false;
  quiet = true; pair(); assert(opens == 2); quiet = false;
  status = SpeakerStatusPlaying; pair(); assert(opens == 2);
  theme_audio_deinit(); assert(stops == 1 && status == SpeakerStatusPlaying);
}
static void test_stream_and_partial_writes(void) {
  reset(); theme_audio_set_enabled(true); write_limit = 137; pair();
  unsigned iterations = 0;
  while (!draining && ++iterations < 1500) advance();
  assert(draining && total_written == CLIP_BYTES && closes == 1 && stops == 0);
  pair(); assert(opens == 1); finish(SpeakerFinishReasonDone);
  assert(!timer_callback && !playing && !finish_callback);
  reset(); theme_audio_set_enabled(true); finish_on_close = true; pair();
  while (timer_callback) advance();
  assert(total_written == CLIP_BYTES && !playing && closes == 1 && stops == 0);
}
static void test_failures_and_preemption(void) {
  reset(); theme_audio_set_enabled(true); fail_open = true; pair(); assert(!playing && !timer_callback && !stops);
  fail_open = false; wrong_size = true; pair(); assert(!opens); wrong_size = false;
  fail_read = true; pair(); assert(opens == 1 && stops == 1 && !timer_callback);
  reset(); theme_audio_set_enabled(true); fail_timer = true; pair(); assert(stops == 1 && !playing);
  reset(); theme_audio_set_enabled(true); write_limit = 0; pair();
  unsigned iterations = 0; while (timer_callback && ++iterations < 110) advance();
  assert(!playing && stops == 1 && total_written == 0 && timer_calls == 99);
  reset(); theme_audio_set_enabled(true); pair(); write_limit = 0; advance();
  write_limit = BUFFER_BYTES; advance(); assert(total_written == 1024 && !stalled);
  finish(SpeakerFinishReasonPreempted); status = SpeakerStatusPlaying;
  theme_audio_deinit(); assert(!timer_callback && stops == 0 && status == SpeakerStatusPlaying);
  reset(); theme_audio_set_enabled(true); pair(); quiet = true; advance(); assert(stops == 1 && !playing);
  reset(); theme_audio_set_enabled(true); pair(); mute = true; advance(); assert(stops == 1 && !playing);
  reset(); theme_audio_set_enabled(true); pair();
  while (!draining) advance();
  while (timer_callback) advance();
  assert(stops == 1 && pumps == MAX_PUMPS + 1);
}
#endif
int main(void) {
#if defined(PBL_PLATFORM_EMERY)
  test_gesture_and_guards(); test_stream_and_partial_writes(); test_failures_and_preemption(); reset();
#else
  theme_audio_set_enabled(true); theme_audio_set_enabled(false); theme_audio_deinit();
#endif
  puts("Theme audio gesture, streaming and lifecycle tests passed");
  return 0;
}
