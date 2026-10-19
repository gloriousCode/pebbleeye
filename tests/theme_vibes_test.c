#include <assert.h>
#include <stdio.h>
#include "../src/theme_vibes.c"

#if defined(PBL_PLATFORM_GABBRO)
static AppTimer fake_timer;
static AppTimerCallback callback;
static AccelTapHandler tap_handler;
static bool quiet, fail_timer;
static uint32_t now_ms, enqueues, cancels, registrations;
static const uint32_t expected[] = {100, 280, 80, 100, 80, 300, 160, 600, 120, 240, 120, 640, 180};
void app_timer_cancel(AppTimer *value) { assert(value == &fake_timer); callback = NULL; }
AppTimer *app_timer_register(uint32_t delay, AppTimerCallback handler, void *context) {
  assert(delay == 100 && !callback && !context); ++registrations;
  if (fail_timer) return NULL;
  callback = handler; return &fake_timer;
}
void accel_tap_service_subscribe(AccelTapHandler handler) { assert(!tap_handler); tap_handler = handler; }
void accel_tap_service_unsubscribe(void) { tap_handler = NULL; }
uint16_t time_ms(time_t *seconds, uint16_t *milliseconds) { *seconds = now_ms / 1000; *milliseconds = now_ms % 1000; return *milliseconds; }
bool quiet_time_is_active(void) { return quiet; }
void vibes_enqueue_custom_pattern(VibePattern pattern) {
  assert(pattern.num_segments == ARRAY_LENGTH(expected)); uint32_t sum = 0;
  for (unsigned i = 0; i < ARRAY_LENGTH(expected); ++i) { assert(pattern.durations[i] == expected[i]); sum += pattern.durations[i]; }
  assert(sum == 3000); ++enqueues;
}
void vibes_cancel(void) { ++cancels; }
static void reset(void) {
  theme_vibes_deinit(); assert(!callback && !tap_handler && !active);
  quiet = fail_timer = false; now_ms = enqueues = cancels = registrations = 0;
}
static void at(uint32_t milliseconds) { assert(tap_handler); now_ms = milliseconds; tap_handler(ACCEL_AXIS_Z, 1); }
static void advance(uint32_t milliseconds) {
  assert(callback); AppTimerCallback handler = callback; callback = NULL; now_ms += milliseconds; handler(NULL);
}
static void test_taps_and_quiet_time(void) {
  reset(); theme_vibes_set_enabled(false); assert(!tap_handler && !callback && !enqueues);
  theme_vibes_set_enabled(true); theme_vibes_set_enabled(true);
  at(0); at(99); assert(!enqueues); at(100); assert(enqueues == 1);
  at(200); at(400); assert(enqueues == 1);
  quiet = true; advance(100); assert(!active && cancels == 1 && !callback);
  at(600); at(800); assert(enqueues == 1);
  at(1000); at(1200); assert(enqueues == 1); // Quiet Time consumes the pair without queuing.
  quiet = false; at(2000); at(2501); assert(enqueues == 1);
  at(3001); assert(enqueues == 2); // Inclusive upper bound.
  theme_vibes_deinit(); assert(!tap_handler && !callback && cancels == 2);
}
static void test_pattern_finish_and_cooldown(void) {
  reset(); theme_vibes_set_enabled(true); at(1000); at(1200);
  assert(active && enqueues == 1);
  for (int i = 0; i < 29; ++i) advance(100);
  assert(active && now_ms == 4100 && callback);
  advance(100); assert(!active && !callback && !cancels && registrations == 30);
  at(4200); at(4400); at(4599); assert(enqueues == 1 && !waiting_tap);
  at(4600); assert(enqueues == 1 && waiting_tap);
  at(4700); assert(enqueues == 2);
  theme_vibes_set_enabled(false); assert(cancels == 1 && !callback && !tap_handler);
  theme_vibes_deinit(); assert(cancels == 1);
}
static void test_failure_and_delayed_timer(void) {
  reset(); theme_vibes_set_enabled(true); fail_timer = true; at(0); at(100);
  assert(enqueues == 1 && cancels == 1 && !active && !callback);
  at(200); at(400); assert(enqueues == 1);
  fail_timer = false; at(500); at(600); assert(enqueues == 2);
  fail_timer = true; advance(100); assert(cancels == 2 && !callback && !active);
  reset(); theme_vibes_set_enabled(true); at(0); at(100); advance(3500);
  assert(callback && active); // Delayed timers or wall-clock corrections cannot end ownership early.
  for (int i = 0; i < 29; ++i) advance(100);
  assert(!callback && !active && !cancels);
  // An unrelated later vibration must not be cancelled after our pattern finished.
  theme_vibes_deinit(); assert(!cancels);
  reset(); theme_vibes_set_enabled(true); at(UINT32_MAX - 150); at(49);
  assert(enqueues == 1); // Unsigned elapsed time remains correct at timestamp wrap.
}
#endif
int main(void) {
#if defined(PBL_PLATFORM_GABBRO)
  test_taps_and_quiet_time(); test_pattern_finish_and_cooldown(); test_failure_and_delayed_timer(); reset();
#else
  theme_vibes_set_enabled(true); theme_vibes_set_enabled(false); theme_vibes_deinit();
#endif
  puts("Theme rhythm gesture, duration, cooldown and lifecycle tests passed");
  return 0;
}
