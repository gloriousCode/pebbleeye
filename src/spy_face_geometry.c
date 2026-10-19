#include "spy_face_geometry.h"

int spy_face_scaled_value(int min_side, int value_at_gabbro)
{
  int scaled = (min_side * value_at_gabbro + 130) / 260;
  return scaled > 0 ? scaled : 1;
}

SpyFaceGeometry spy_face_geometry_for_bounds(int width, int height, bool round)
{
  int side = width < height ? width : height;
  int cx = width / 2;
  int cy = height / 2;
  int outer = side / 2 - spy_face_scaled_value(side, 8);
  int ring = spy_face_scaled_value(side, 24);
  int display_width = spy_face_scaled_value(side, 166);
  int display_height = spy_face_scaled_value(side, 120);
  int date_width = spy_face_scaled_value(side, 144);
  int overlay_width = side >= 220 ? 100 : 76;
  int status_width = side >= 220 ? 130 : spy_face_scaled_value(side, 144);
  return (SpyFaceGeometry) {
    .screen_width = width, .screen_height = height, .round = round,
    .centre_x = cx, .centre_y = cy, .min_side = side,
    .outer_radius = outer, .inner_radius = outer - ring, .ring_width = ring,
    .display = { cx - display_width / 2, cy - display_height / 2,
        display_width, display_height },
    .date = { cx - date_width / 2, cy + spy_face_scaled_value(side, 80),
        date_width, 22 },
    .status = { cx - status_width / 2, cy - outer + spy_face_scaled_value(side, 15),
        status_width, side >= 220 ? 20 : 16 },
    .combined_time = { cx - overlay_width / 2, cy + spy_face_scaled_value(side, 10),
        overlay_width, side >= 220 ? 34 : 30 },
    .analogue_radius = spy_face_scaled_value(side, 76),
  };
}
