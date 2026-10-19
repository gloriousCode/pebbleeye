#include <assert.h>
#include <stdio.h>
#include "../src/gentle_gesture.c"

static AppTimer fake_timer;
static void (*sample_handler)(AccelData *, uint32_t);
static void (*timer_handler)(void *);
static unsigned subscriptions, actions;
static bool fail_timer, fail_rate;
static uint64_t now;

void app_timer_cancel(AppTimer *timer) { assert(timer == &fake_timer); timer_handler = NULL; }
AppTimer *app_timer_register(uint32_t ms, void (*callback)(void *), void *context) {
  assert(ms == 8000 && !context && !timer_handler);
  if (fail_timer) return NULL;
  timer_handler = callback;
  return &fake_timer;
}
void accel_data_service_subscribe(uint32_t count, void (*callback)(AccelData *, uint32_t)) {
  assert(count == 10 && !sample_handler); sample_handler = callback; ++subscriptions;
}
void accel_data_service_unsubscribe(void) { assert(sample_handler); sample_handler = NULL; }
int accel_service_set_sampling_rate(int rate) { assert(rate == 100); return fail_rate ? -1 : 0; }
static void action(void) { assert(!sample_handler && !timer_handler); ++actions; }
static void feed(int16_t x) {
  AccelData data = {.x = x, .z = 1000, .timestamp = (now += 20)};
  assert(sample_handler); sample_handler(&data, 1);
}
static void quiet(unsigned n) { while (n--) feed(0); }
static void reset(void) {
  gentle_gesture_deinit(); assert(!sample_handler && !timer_handler);
  subscriptions = actions = 0; now = 0; fail_timer = fail_rate = false;
}

static void test_batched_100hz_samples(void) {
  reset(); gentle_gesture_set_light(true); gentle_gesture_set_enabled(action);
  for (int batch = 0; batch < 5; ++batch) {
    AccelData data[10];
    for (int i = 0; i < 10; ++i) {
      int index = batch * 10 + i;
      int phase = index >= 36 ? index - 36 : index - 12;
      int16_t x = phase == 0 ? 200 : phase == 1 ? -100 : phase == 2 ? 60 : 0;
      data[i] = (AccelData){.x = x, .z = 1000, .timestamp = (uint64_t)(index + 1) * 10};
    }
    assert(sample_handler); sample_handler(data, 10);
    assert(actions == (batch == 4 ? 1u : 0u));
  }
  assert(!sample_handler && !timer_handler);
  reset();
}

int main(void) {
  test_batched_100hz_samples();
  reset(); gentle_gesture_set_enabled(action); assert(!sample_handler && !timer_handler);
  gentle_gesture_set_light(true); assert(subscriptions == 1 && sample_handler && timer_handler);
  gentle_gesture_set_enabled(action); gentle_gesture_set_light(true); assert(subscriptions == 1);
  quiet(5); feed(200); quiet(7); feed(200); quiet(5);
  assert(actions == 1 && !sample_handler && !timer_handler);
  gentle_gesture_set_light(true); gentle_gesture_set_enabled(action); assert(!sample_handler);
  gentle_gesture_set_light(false); gentle_gesture_set_light(true); assert(subscriptions == 2);
  void (*expired_callback)(void *) = timer_handler; timer_handler = NULL; expired_callback(NULL);
  assert(!sample_handler && !timer_handler);
  gentle_gesture_set_light(true); assert(!sample_handler);
  gentle_gesture_set_light(false); gentle_gesture_set_light(true);
  gentle_gesture_set_light(false); assert(!sample_handler && !timer_handler);
  gentle_gesture_set_light(true); gentle_gesture_set_enabled(NULL);
  assert(!sample_handler && !timer_handler);
  reset(); gentle_gesture_set_light(true); fail_timer = true; gentle_gesture_set_enabled(action);
  assert(!sample_handler && !timer_handler);
  reset(); gentle_gesture_set_light(true); fail_rate = true; gentle_gesture_set_enabled(action);
  assert(!sample_handler && !timer_handler);
  reset(); gentle_gesture_set_light(true); gentle_gesture_set_enabled(action);
  gentle_gesture_pause(); assert(!sample_handler && !timer_handler);
  gentle_gesture_set_light(true); assert(!sample_handler);
  gentle_gesture_set_light(false); gentle_gesture_set_light(true); assert(sample_handler);
  reset();
  puts("Gentle gesture idle, window, one-shot, disable and failure lifecycle tests passed");
}
