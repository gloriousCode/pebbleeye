#include <pebble.h>
#include "gentle_gesture.h"
#include "gentle_tap.h"

enum { INTERACTION_MS = 8000, BATCH_SAMPLES = 5 };
static GentleTapState detector;
static void (*on_double_tap)(void);
static AppTimer *expiry;
static bool lit, sampling;

static void stop_sampling(void) {
  if (expiry) app_timer_cancel(expiry);
  expiry = NULL;
  if (sampling) accel_data_service_unsubscribe();
  sampling = false;
  gentle_tap_reset(&detector);
}

static void expired(void *context) {
  (void)context;
  expiry = NULL;
  stop_sampling();
}

static void samples(AccelData *data, uint32_t count) {
  if (!sampling || !on_double_tap) return;
  for (uint32_t i = 0; i < count; ++i) {
    if (gentle_tap_sample(&detector, data[i].x, data[i].y, data[i].z,
                          data[i].timestamp, data[i].did_vibrate)) {
      void (*handler)(void) = on_double_tap;
      // One action per light window also prevents speaker/vibration feedback.
      stop_sampling();
      handler();
      return;
    }
  }
}

static void start_sampling(void) {
  if (!lit || !on_double_tap || sampling) return;
  gentle_tap_reset(&detector);
  accel_data_service_subscribe(BATCH_SAMPLES, samples);
  sampling = true;
  if (accel_service_set_sampling_rate(ACCEL_SAMPLING_50HZ) != 0) {
    stop_sampling();
    return;
  }
  expiry = app_timer_register(INTERACTION_MS, expired, NULL);
  if (!expiry) stop_sampling();
}

void gentle_gesture_set_enabled(void (*handler)(void)) {
  if (on_double_tap == handler) return;
  stop_sampling();
  on_double_tap = handler;
  start_sampling();
}

void gentle_gesture_set_light(bool on) {
  if (lit == on) return;
  lit = on;
  if (lit) start_sampling();
  else stop_sampling();
}

void gentle_gesture_deinit(void) {
  stop_sampling();
  on_double_tap = NULL;
  lit = false;
}
