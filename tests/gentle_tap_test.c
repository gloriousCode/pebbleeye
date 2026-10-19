#include <assert.h>
#include <stdio.h>
#include "gentle_tap.h"

static GentleTapState state;
static uint64_t now;
static unsigned detected;
static void sample(int16_t x, int16_t y, int16_t z, bool vibration) {
  now += 20;
  if (gentle_tap_sample(&state, x, y, z, now, vibration)) ++detected;
}
static void quiet(unsigned count) { while (count--) sample(0, 0, 1000, false); }
static void reset(void) { gentle_tap_reset(&state); now = 0; detected = 0; quiet(5); }
static void pulse(int16_t strength) { sample(strength, 0, 1000, false); quiet(3); }
static void test_quiet_and_single_taps(void) {
  reset(); quiet(100); assert(!detected);
  pulse(200); quiet(50); assert(!detected);
  pulse(-200); quiet(50); assert(!detected);
  assert(!gentle_tap_sample(NULL, 0, 0, 0, 0, false)); gentle_tap_reset(NULL);
}
static void test_double_taps_and_thresholds(void) {
  reset(); pulse(180); quiet(2); pulse(180); assert(detected == 1); // 120 ms onset separation.
  reset(); pulse(179); quiet(4); pulse(179); assert(!detected);
  reset(); pulse(220); quiet(23); pulse(220); assert(detected == 1); // 540 ms at 50 Hz.
  reset(); pulse(220); quiet(23); now += 10; pulse(220); assert(detected == 1); // Inclusive 550 ms limit.
  reset(); pulse(220); quiet(24); pulse(220); assert(!detected); // 560 ms is too late.
  reset(); sample(0, 200, 1000, false); quiet(3); quiet(4);
  sample(0, 0, 800, false); quiet(3); assert(detected == 1); // Any axis, either direction.
}
static void test_recoil_and_sustained_motion(void) {
  reset(); sample(200, 0, 1000, false); sample(-200, 0, 1000, false); quiet(3);
  assert(state.first_tap); // Recoil can settle at the inclusive 80 ms impulse limit.
  reset(); sample(200, 0, 1000, false); sample(-200, 0, 1000, false); sample(200, 0, 1000, false); quiet(3);
  assert(!state.first_tap); // A 100 ms unsettled impulse is rejected.
  reset(); sample(300, 0, 1000, false);
  for (int i = 0; i < 5; ++i) sample(300, 0, 1000, false);
  assert(!state.first_tap && !detected); // Lasting orientation step is not a tap.
  reset(); pulse(200); quiet(1); pulse(-200); quiet(40); assert(!detected); // 100 ms recoil.
  reset(); sample(240, 0, 1000, false);
  for (int i = 0; i < 10; ++i) sample(i % 2 ? 240 : -240, 0, 1000, false);
  quiet(5); pulse(220); quiet(40); assert(!detected);
  reset(); pulse(220);
  for (int i = 0; i < 10; ++i) sample(i * 100, 0, 1000, false);
  quiet(5); pulse(220); assert(!detected);
}
static void test_vibration_gaps_and_cooldown(void) {
  reset(); pulse(220); quiet(2); sample(220, 0, 1000, true); quiet(4); pulse(220); assert(!detected);
  reset(); pulse(220); now += 100; quiet(5); pulse(220); assert(!detected);
  reset(); pulse(220); now -= 40; quiet(5); pulse(220); assert(!detected);
  reset(); pulse(220); quiet(2); pulse(220); assert(detected == 1);
  quiet(2); pulse(220); quiet(2); pulse(220); assert(detected == 1);
  quiet(45); pulse(220); quiet(2); pulse(220); assert(detected == 2);
}
int main(void) {
  test_quiet_and_single_taps(); test_double_taps_and_thresholds();
  test_recoil_and_sustained_motion(); test_vibration_gaps_and_cooldown();
  puts("Gentle tap synthetic impulse, recoil, motion and suppression tests passed");
  return 0;
}
