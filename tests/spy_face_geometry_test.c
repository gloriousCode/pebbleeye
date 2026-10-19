#include "../src/spy_face_geometry.h"
#include <assert.h>
#include <stdio.h>

static void assert_rect_safe(SpyFaceRect rect, SpyFaceGeometry geometry)
{
  assert(rect.w > 0 && rect.h > 0);
  assert(rect.x >= 0 && rect.y >= 0);
  assert(rect.x + rect.w <= geometry.screen_width);
  assert(rect.y + rect.h <= geometry.screen_height);
  if (geometry.round) {
    // Check all four corners against the circular display, with a four-pixel inset.
    int radius = geometry.min_side / 2 - 4;
    for (int x = 0; x < 2; x++) {
      for (int y = 0; y < 2; y++) {
        int dx = rect.x + x * rect.w - geometry.centre_x;
        int dy = rect.y + y * rect.h - geometry.centre_y;
        assert(dx * dx + dy * dy <= radius * radius);
      }
    }
  }
}

static void test_geometry(void)
{
  const int sizes[][3] = { {200, 228, 0}, {260, 260, 1} };
  for (unsigned i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
    SpyFaceGeometry g = spy_face_geometry_for_bounds(sizes[i][0], sizes[i][1], sizes[i][2]);
    assert(g.outer_radius > g.inner_radius);
    assert(g.outer_radius - g.inner_radius == g.ring_width);
    assert(g.centre_x - g.outer_radius >= 4);
    assert(g.centre_y - g.outer_radius >= 4);
    assert(g.centre_x + g.outer_radius <= g.screen_width - 4);
    assert(g.centre_y + g.outer_radius <= g.screen_height - 4);
    assert_rect_safe(g.display, g);
    assert_rect_safe(g.date, g);
    assert_rect_safe(g.status, g);
    assert_rect_safe(g.combined_time, g);
    // Hybrid uses the full analogue dial: its lower inset leaves the hub visible.
    assert(g.combined_time.y >= g.centre_y + 8);
    int overlay_right = g.combined_time.x + g.combined_time.w - g.centre_x;
    int overlay_bottom = g.combined_time.y + g.combined_time.h - g.centre_y;
    int inset_radius = g.analogue_radius - 3;
    assert(overlay_right * overlay_right + overlay_bottom * overlay_bottom
        <= inset_radius * inset_radius);
    assert(g.combined_time.y + g.combined_time.h + 4 <= g.date.y);
    assert(g.centre_y + g.analogue_radius + 2 <= g.date.y);
    assert(g.status.y + g.status.h <= g.centre_y - g.analogue_radius);
    assert(g.analogue_radius < g.inner_radius);
    assert(g.date.w >= 110);
    assert(g.combined_time.w >= 76);
  }
}

static void test_scaled_value(void)
{
  assert(spy_face_scaled_value(260, 44) == 44);
  assert(spy_face_scaled_value(200, 44) == 34);
  assert(spy_face_scaled_value(200, 0) == 1);
}

int main(void)
{
  test_geometry();
  test_scaled_value();
  puts("PebbleEye: both displays have safe circular bounds and an inset hybrid readout");
  return 0;
}
