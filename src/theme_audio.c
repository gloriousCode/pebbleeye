#include <pebble.h>
#include "theme_audio.h"

#if defined(PBL_PLATFORM_EMERY)
enum { CLIP_BYTES = 192000, BUFFER_BYTES = 512, PUMP_MS = 10,
       MAX_STALLED_PUMPS = 100, MAX_PUMPS = 2000 };
static bool enabled, playing, draining;
static uint32_t offset, pumps, stalled;
static AppTimer *timer;
static ResHandle resource;
static uint8_t buffer[BUFFER_BYTES];

static void cancel_timer(void) {
  if (timer) app_timer_cancel(timer);
  timer = NULL;
}

static void stop_owned_playback(void) {
  cancel_timer();
  if (!playing) return;
  playing = false;
  speaker_set_finish_callback(NULL, NULL);
  speaker_stop();
}

static void finished(SpeakerFinishReason reason, void *context) {
  (void)context;
  APP_LOG(APP_LOG_LEVEL_INFO, "Theme playback finished: %d", reason);
  playing = false;
  cancel_timer();
  speaker_set_finish_callback(NULL, NULL);
}

static void pump(void *context) {
  (void)context;
  timer = NULL;
  if (!playing) return;
  if (speaker_is_muted() || quiet_time_is_active() || ++pumps > MAX_PUMPS) {
    stop_owned_playback();
    return;
  }
  if (!draining) {
    uint32_t count = CLIP_BYTES - offset;
    if (count > sizeof(buffer)) count = sizeof(buffer);
    if (resource_load_byte_range(resource, offset, buffer, count) != count) {
      stop_owned_playback();
      return;
    }
    uint32_t written = speaker_stream_write(buffer, count);
    if (written > count || (!written && ++stalled >= MAX_STALLED_PUMPS)) {
      stop_owned_playback();
      return;
    }
    if (written) stalled = 0;
    offset += written;
    if (offset == CLIP_BYTES) {
      draining = true;
      speaker_stream_close();
    }
  }
  if (!playing) return; // Closing may invoke the finish callback immediately.
  timer = app_timer_register(PUMP_MS, pump, NULL);
  if (!timer) stop_owned_playback();
}

void theme_audio_trigger(void) {
  if (!enabled || playing) return;
  if (quiet_time_is_active() || speaker_is_muted() || speaker_get_status() != SpeakerStatusIdle) return;
  resource = resource_get_handle(RESOURCE_ID_THEME_AUDIO);
  if (!resource || resource_size(resource) != CLIP_BYTES) return;
  if (!speaker_stream_open(SpeakerPcmFormat_16kHz_8bit, 40)) return;
  playing = true;
  APP_LOG(APP_LOG_LEVEL_INFO, "Theme playback started");
  draining = false;
  offset = pumps = stalled = 0;
  speaker_set_finish_callback(finished, NULL);
  pump(NULL);
}

void theme_audio_set_enabled(bool requested) {
  if (enabled == requested) return;
  enabled = requested;
  if (!enabled) stop_owned_playback();
}

void theme_audio_deinit(void) {
  theme_audio_set_enabled(false);
}
#else
void theme_audio_trigger(void) {}
void theme_audio_set_enabled(bool enabled) { (void)enabled; }
void theme_audio_deinit(void) {}
#endif
