#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Integer-only recogniser for 50/100 Hz samples. Thresholds need physical-watch calibration.
typedef struct {
  int16_t x, y, z;
  uint64_t previous_ms, impulse_ms, first_ms, trigger_ms;
  uint16_t quiet_ms;
  bool initialised, impulse, first_tap, cooldown;
} GentleTapState;

static inline void gentle_tap_reset(GentleTapState *state) {
  if (state) memset(state, 0, sizeof(*state));
}

static inline bool gentle_tap_sample(GentleTapState *state, int16_t x, int16_t y,
                                     int16_t z, uint64_t timestamp, bool did_vibrate) {
  if (!state) return false;
  // Do not bridge motor vibration, missing, duplicate or reversed samples into a gesture.
  if (!state->initialised || timestamp <= state->previous_ms ||
      timestamp - state->previous_ms > 60 || did_vibrate) {
    gentle_tap_reset(state);
    state->initialised = true;
    state->x = x; state->y = y; state->z = z;
    state->previous_ms = timestamp;
    return false;
  }
  uint16_t interval = (uint16_t)(timestamp - state->previous_ms);
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
      state->quiet_ms = 0;
      return false;
    }
    state->cooldown = false;
  }
  if (state->impulse) {
    uint64_t duration = timestamp - state->impulse_ms;
    if (duration > 180) {
      state->impulse = false;
      state->first_tap = false;
      state->quiet_ms = 0;
      return false;
    }
    if (change > 55) state->quiet_ms = 0;
    else if (state->quiet_ms < 30) state->quiet_ms += interval;
    // Fold the first 100 ms of ringing into one impulse. Require 30 ms of settling,
    // but allow a changed wrist orientation instead of requiring the old gravity vector.
    if (duration < 100 || state->quiet_ms < 30) return false;
    state->impulse = false;
    uint64_t separation = state->impulse_ms - state->first_ms;
    if (state->first_tap && separation >= 140 && separation <= 650) {
      state->first_tap = false;
      state->cooldown = true;
      state->trigger_ms = timestamp;
      return true;
    }
    if (!state->first_tap || separation > 650) {
      state->first_tap = true;
      state->first_ms = state->impulse_ms;
    }
    return false;
  }
  if (state->first_tap && timestamp - state->first_ms > 650) state->first_tap = false;
  if (change >= 120 && state->quiet_ms >= 30) {
    state->impulse = true;
    state->impulse_ms = timestamp;
    state->quiet_ms = 0;
  } else if (change <= 55) {
    if (state->quiet_ms < 30) state->quiet_ms += interval;
  } else {
    // Unsettled motion must settle again before arming another impulse.
    state->first_tap = false;
    state->quiet_ms = 0;
  }
  return false;
}
