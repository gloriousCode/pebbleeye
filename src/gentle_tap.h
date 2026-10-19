#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Integer-only recogniser for 50 Hz samples. Thresholds require physical-watch calibration.
typedef struct {
  int16_t x, y, z, origin_x, origin_y, origin_z;
  uint64_t previous_ms, impulse_ms, first_ms, trigger_ms;
  uint8_t quiet_samples;
  bool initialised, impulse, first_tap, cooldown;
} GentleTapState;

static inline void gentle_tap_reset(GentleTapState *state) {
  if (state) memset(state, 0, sizeof(*state));
}

static inline bool gentle_tap_sample(GentleTapState *state, int16_t x, int16_t y,
                                     int16_t z, uint64_t timestamp, bool did_vibrate) {
  if (!state) return false;
  // Never bridge missing, duplicate or reversed samples into a gesture.
  if (!state->initialised || timestamp <= state->previous_ms ||
      timestamp - state->previous_ms > 40 || did_vibrate) {
    gentle_tap_reset(state);
    state->initialised = true;
    state->x = x; state->y = y; state->z = z;
    state->previous_ms = timestamp;
    return false;
  }
  int16_t previous_x = state->x, previous_y = state->y, previous_z = state->z;
  int dx = (int)x - state->x, dy = (int)y - state->y, dz = (int)z - state->z;
  if (dx < 0) dx = -dx;
  if (dy < 0) dy = -dy;
  if (dz < 0) dz = -dz;
  int change = dx > dy ? dx : dy;
  if (dz > change) change = dz;
  state->x = x; state->y = y; state->z = z;
  state->previous_ms = timestamp;
  if (state->cooldown) {
    if (timestamp - state->trigger_ms < 800) {
      state->quiet_samples = 0;
      return false;
    }
    state->cooldown = false;
  }
  if (state->impulse) {
    if (timestamp - state->impulse_ms > 80) {
      state->impulse = false;
      state->first_tap = false;
      state->quiet_samples = 0;
      return false;
    }
    state->quiet_samples = change <= 60 ? state->quiet_samples + 1 : 0;
    if (state->quiet_samples < 2) return false;
    state->impulse = false;
    // A lasting orientation change is movement, not a returning tap impulse.
    if ((int)x - state->origin_x < -120 || (int)x - state->origin_x > 120 ||
        (int)y - state->origin_y < -120 || (int)y - state->origin_y > 120 ||
        (int)z - state->origin_z < -120 || (int)z - state->origin_z > 120) {
      state->first_tap = false;
      return false;
    }
    uint64_t separation = state->impulse_ms - state->first_ms;
    if (state->first_tap && separation >= 120 && separation <= 550) {
      state->first_tap = false;
      state->cooldown = true;
      state->trigger_ms = timestamp;
      return true;
    }
    // A fast recoil belongs to the first tap, rather than becoming a second one.
    if (!state->first_tap || separation > 550) {
      state->first_tap = true;
      state->first_ms = state->impulse_ms;
    }
    return false;
  }
  if (state->first_tap && timestamp - state->first_ms > 550) state->first_tap = false;
  if (change >= 180 && state->quiet_samples >= 3) {
    state->impulse = true;
    state->impulse_ms = timestamp;
    state->origin_x = previous_x; state->origin_y = previous_y; state->origin_z = previous_z;
    state->quiet_samples = 0;
  } else if (change <= 60) {
    if (state->quiet_samples < 3) ++state->quiet_samples;
  } else {
    // Unsettled motion invalidates a pending tap and must settle before re-arming.
    state->first_tap = false;
    state->quiet_samples = 0;
  }
  return false;
}
