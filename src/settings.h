#pragma once
#include <stdbool.h>

typedef struct {
  int date_format;
  int display_mode;
  int theme;
  bool analogue_seconds;
  bool show_status;
} Settings;

Settings settings_defaults(void);
bool settings_apply(Settings *settings, int key, int value);
bool settings_want_seconds(const Settings *settings, bool backlight_on);
