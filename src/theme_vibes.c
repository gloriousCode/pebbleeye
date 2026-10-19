#include <pebble.h>
#include "theme_vibes.h"

#if defined(PBL_PLATFORM_GABBRO)
enum { PATTERN_MS = 3000, COOLDOWN_MS = 400, CHECK_MS = 100 };
// Approximate onset rhythm from the approved clip, shifted to start at zero.
// Alternating pulses/rests preserve the longer 600/640 ms pauses; this is not pitch transcription.
static const uint32_t rhythm[] = {100, 280, 80, 100, 80, 300, 160, 600, 120, 240, 120, 640, 180};
static bool enabled, active, waiting_tap, cooling;
static uint32_t first_tap_ms, elapsed_ms, cooldown_started_ms;
static AppTimer *timer;

static uint32_t milliseconds_now(void) {
  time_t seconds;
  uint16_t milliseconds;
  time_ms(&seconds, &milliseconds);
  return (uint32_t)seconds * 1000u + milliseconds;
}

static void stop_owned_pattern(void) {
  if (timer) app_timer_cancel(timer);
  timer = NULL;
  if (!active) return;
  active = false;
  cooling = true;
  cooldown_started_ms = milliseconds_now();
  vibes_cancel();
  APP_LOG(APP_LOG_LEVEL_INFO, "Theme rhythm finished: cancelled");
}

static void monitor(void *context) {
  (void)context;
  timer = NULL;
  if (!active) return;
  uint32_t now = milliseconds_now();
  // Timer intervals avoid ending ownership early when the phone corrects wall-clock time.
  elapsed_ms += CHECK_MS;
  if (elapsed_ms >= PATTERN_MS) {
    active = false;
    cooling = true;
    cooldown_started_ms = now;
    APP_LOG(APP_LOG_LEVEL_INFO, "Theme rhythm finished");
    return;
  }
  if (quiet_time_is_active()) {
    stop_owned_pattern();
    return;
  }
  timer = app_timer_register(CHECK_MS, monitor, NULL);
  if (!timer) stop_owned_pattern();
}

static void tap(AccelAxisType axis, int32_t direction) {
  (void)axis; (void)direction;
  if (!enabled || active) return;
  uint32_t now = milliseconds_now();
  if (cooling) {
    if (now - cooldown_started_ms < COOLDOWN_MS) return;
    cooling = false;
  }
  uint32_t elapsed = now - first_tap_ms;
  if (!waiting_tap || elapsed > 500) {
    waiting_tap = true;
    first_tap_ms = now;
    return;
  }
  if (elapsed < 100) return;
  waiting_tap = false;
  if (quiet_time_is_active()) return;
  active = true;
  elapsed_ms = 0;
  vibes_enqueue_custom_pattern((VibePattern){.durations = rhythm, .num_segments = ARRAY_LENGTH(rhythm)});
  APP_LOG(APP_LOG_LEVEL_INFO, "Theme rhythm started");
  timer = app_timer_register(CHECK_MS, monitor, NULL);
  if (!timer) stop_owned_pattern();
}

void theme_vibes_set_enabled(bool requested) {
  if (enabled == requested) return;
  enabled = requested;
  waiting_tap = false;
  if (enabled) {
    accel_tap_service_subscribe(tap);
  } else {
    accel_tap_service_unsubscribe();
    stop_owned_pattern();
    cooling = false;
  }
}

void theme_vibes_deinit(void) {
  theme_vibes_set_enabled(false);
}
#else
void theme_vibes_set_enabled(bool enabled) { (void)enabled; }
void theme_vibes_deinit(void) {}
#endif
