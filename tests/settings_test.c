#include "settings.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int main(void) {
  Settings s = settings_defaults();
  assert(s.date_format == 0 && s.display_mode == 0 && s.theme == 0);
  assert(!s.analogue_seconds && s.show_status);
  const int keys[] = {7, 14, 15, 16, 17};
  const int maxima[] = {4, 1, 2, 2, 1};
  for (unsigned i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
    assert(!settings_apply(&s, keys[i], -1));
    assert(!settings_apply(&s, keys[i], maxima[i] + 1));
    for (int v = 0; v <= maxima[i]; ++v) assert(settings_apply(&s, keys[i], v));
  }
  assert(!settings_apply(&s, 0, 1));
  assert(!settings_apply(NULL, 7, 0));
  assert(!settings_want_seconds(NULL, true));
  for (int mode = 0; mode < 3; ++mode) {
    for (int enabled = 0; enabled < 2; ++enabled) {
      for (int light = 0; light < 2; ++light) {
        s.display_mode = mode; s.analogue_seconds = enabled;
        assert(settings_want_seconds(&s, light) == (mode != 0 && enabled && light));
      }
    }
  }
  puts("Settings validation and backlight tick policy pass");
}
