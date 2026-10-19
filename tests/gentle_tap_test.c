#include <assert.h>
#include <stdio.h>
#include "gentle_tap.h"

static GentleTapState state;
static uint64_t now;
static unsigned detected, period;
static void sample(int16_t x, int16_t y, int16_t z, bool vibration) {
  now += period;
  if (gentle_tap_sample(&state, x, y, z, now, vibration)) ++detected;
}
static void quiet(unsigned milliseconds) {
  for (unsigned elapsed = 0; elapsed < milliseconds; elapsed += period) sample(0, 0, 1000, false);
}
static void reset(unsigned sampling_ms) {
  gentle_tap_reset(&state); now = 0; detected = 0; period = sampling_ms; quiet(100);
}
static void pulse(int16_t strength) { sample(strength, 0, 1000, false); quiet(100); }
static void pair(unsigned separation, int16_t strength) {
  pulse(strength); quiet(separation - 100 - period); pulse(strength);
}
static void test_rate_independent_bounds(void) {
  for (unsigned rate = 10; rate <= 20; rate += 10) {
    reset(rate); quiet(1000); pulse(180); quiet(1000); assert(!detected);
    reset(rate); pair(140, 120); assert(detected == 1);
    reset(rate); pair(120, 180); quiet(1000); assert(!detected);
    reset(rate); pair(640, 120); assert(detected == 1);
    reset(rate); pair(660, 180); quiet(1000); assert(!detected);
    reset(rate); pair(240, 119); assert(!detected);
    // Any axis and direction can supply the impulse.
    reset(rate); sample(0, -130, 1000, false); quiet(200);
    sample(0, 0, 1130, false); quiet(100); assert(detected == 1);
  }
  reset(10); pair(650, 120); assert(detected == 1);
  assert(!gentle_tap_sample(NULL, 0, 0, 0, 0, false)); gentle_tap_reset(NULL);
}
static void test_noise_ringing_and_orientation_drift(void) {
  for (unsigned rate = 10; rate <= 20; rate += 10) {
    reset(rate);
    for (int i = 0; i < 200; ++i) sample(i % 2 ? 20 : -20, i % 3 * 8, 1000 + i % 4 * 6, false);
    assert(!detected); quiet(100);
    // Damped ringing persists for 100 ms: the former 80 ms deadline rejected this.
    for (int tap = 0; tap < 2; ++tap) {
      for (unsigned t = 0; t <= 100; t += rate) sample((t / rate) % 2 ? -100 : 140, 0, 1000, false);
      quiet(80);
      if (!tap) quiet(100);
    }
    assert(detected == 1);
    reset(rate);
    // A tap plus wrist rotation settles around a new gravity vector, without returning
    // within the former 120 mg origin tolerance. This is synthetic, not captured hardware.
    sample(160, 0, 1000, false);
    for (unsigned t = 0; t < 160; t += rate) sample(160 + t / 10, t / 20, 990 - t / 20, false);
    assert(state.first_tap && !detected);
    sample(340, 10, 980, false);
    for (unsigned t = 0; t < 120; t += rate) sample(185 + t / 20, 10, 980, false);
    assert(detected == 1);
    reset(rate); sample(180, 0, 1000, false); quiet(40);
    sample(-150, 0, 1000, false); quiet(40); sample(130, 0, 1000, false);
    quiet(1000); assert(!detected); // Several recoil spikes remain one tap.
  }
}
static void test_sustained_motion_and_shaking(void) {
  for (unsigned rate = 10; rate <= 20; rate += 10) {
    reset(rate);
    for (unsigned t = 0; t < 1000; t += rate) sample(t / rate % 2 ? -350 : 350, 0, 1000, false);
    quiet(100); pulse(160); quiet(1000); assert(!detected);
    reset(rate); pulse(160);
    for (int i = 0; i < 12; ++i) sample(i * 80, 0, 1000, false);
    quiet(100); pulse(160); assert(!detected);
    reset(rate);
    // Repeated 220 ms shake bursts with short quiet gaps must not become taps.
    for (int burst = 0; burst < 3; ++burst) {
      for (unsigned t = 0; t < 220; t += rate) sample(t / rate % 2 ? -240 : 240, 0, 1000, false);
      quiet(60);
    }
    quiet(1000); assert(!detected);
  }
}
static void test_vibration_gaps_cooldown_and_batch_timestamps(void) {
  for (unsigned rate = 10; rate <= 20; rate += 10) {
    reset(rate); pulse(160); sample(0, 0, 1000, true); quiet(100); pulse(160); assert(!detected);
    reset(rate); pulse(160); now += 61; quiet(100); pulse(160); assert(!detected);
    reset(rate); pulse(160); now -= period; quiet(100); pulse(160); assert(!detected);
    reset(rate); pulse(160); now -= period * 2; quiet(100); pulse(160); assert(!detected);
    reset(rate); pair(200, 160); assert(detected == 1);
    pair(200, 160); assert(detected == 1); quiet(1000); pair(200, 160); assert(detected == 2);
    // SDK batches carry individual timestamps. Processing all samples consecutively
    // must give the same result regardless of delivery batching or wall-clock delay.
    reset(rate);
    uint64_t start = now;
    for (unsigned i = 1; i <= 50; ++i) {
      uint64_t stamp = start + i * rate;
      int16_t x = (i == 5 || i == 5 + 200 / rate) ? 160 : 0;
      if (gentle_tap_sample(&state, x, 0, 1000, stamp, false)) ++detected;
    }
    assert(detected == 1);
  }
}
int main(void) {
  test_rate_independent_bounds(); test_noise_ringing_and_orientation_drift();
  test_sustained_motion_and_shaking(); test_vibration_gaps_cooldown_and_batch_timestamps();
  puts("Gentle tap synthetic 50/100 Hz settling, ringing, drift and suppression tests passed");
  return 0;
}
