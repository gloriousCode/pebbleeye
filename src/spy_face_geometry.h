#pragma once

#include <stdbool.h>

typedef struct {
  int x;
  int y;
  int w;
  int h;
} SpyFaceRect;

typedef struct {
  int screen_width;
  int screen_height;
  bool round;
  int centre_x;
  int centre_y;
  int min_side;
  int outer_radius;
  int inner_radius;
  int ring_width;
  SpyFaceRect display;
  SpyFaceRect date;
  SpyFaceRect status;
  SpyFaceRect combined_time;
  int analogue_radius;
} SpyFaceGeometry;

SpyFaceGeometry spy_face_geometry_for_bounds(int screen_width, int screen_height, bool round);
int spy_face_scaled_value(int min_side, int value_at_gabbro);
