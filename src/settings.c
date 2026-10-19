#include "settings.h"

Settings settings_defaults(void) {
  return (Settings){.date_format = 0, .display_mode = 0, .theme = 0,
                    .analogue_seconds = false, .show_status = true};
}

bool settings_apply(Settings *settings, int key, int value) {
  if (!settings) return false;
  switch (key) {
    case 7:
      if (value < 0 || value > 4) return false;
      settings->date_format = value;
      return true;
    case 14:
      if (value != 0 && value != 1) return false;
      settings->analogue_seconds = value != 0;
      return true;
    case 15:
      if (value < 0 || value > 2) return false;
      settings->display_mode = value;
      return true;
    case 16:
      if (value < 0 || value > 2) return false;
      settings->theme = value;
      return true;
    case 17:
      if (value != 0 && value != 1) return false;
      settings->show_status = value != 0;
      return true;
    default: return false;
  }
}

bool settings_want_seconds(const Settings *settings, bool backlight_on) {
  return settings && settings->analogue_seconds && backlight_on
      && (settings->display_mode == 1 || settings->display_mode == 2);
}
