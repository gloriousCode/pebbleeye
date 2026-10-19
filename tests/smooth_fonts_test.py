"""Verify real generated font coverage, metrics and platform isolation."""
from pathlib import Path
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]


def read_glyphs(path):
    data = path.read_bytes()
    version, height, count, wildcard, buckets, cp_bytes, header_size, features = struct.unpack_from('<BBHHBBBB', data)
    assert version == 3 and cp_bytes == 2 and features in (0, 1)
    offsets = header_size + buckets * 4
    entry_size = 4 if features & 1 else 6
    glyph_base = offsets + count * entry_size
    glyphs = {}
    for index in range(count):
        cp, offset = struct.unpack_from('<HH' if features & 1 else '<HI', data, offsets + index * entry_size)
        start = glyph_base + offset
        metrics = struct.unpack_from('<BBbbb', data, start)
        width, rows = metrics[:2]
        length = ((width * rows + 31) // 32) * 4
        glyphs[cp] = (metrics, data[start + 5:start + 5 + length])
    return glyphs


class SmoothFontTests(unittest.TestCase):
    def test_nested_masks_have_identical_layout_metrics(self):
        for size in (24, 28, 34, 42):
            for style in ('Bold',):
                with self.subTest(size=size, style=style):
                    masks = [read_glyphs(ROOT / f'resources/fonts/smooth/Roboto-{style}-{size}-{level}.pbf') for level in (1, 2, 3)]
                    self.assertTrue(set(map(ord, "0123456789: ")).issubset(masks[0]))
                    self.assertEqual(set(masks[0]), set(map(ord, "0123456789: ")) | {8230, 9647})
                    for cp, (metrics, outer) in masks[0].items():
                        middle_metrics, middle = masks[1][cp]
                        inner_metrics, inner = masks[2][cp]
                        self.assertEqual(metrics, middle_metrics)
                        self.assertEqual(metrics, inner_metrics)
                        self.assertEqual(len(outer), len(inner))
                        self.assertTrue(all((c & ~b) == 0 and (b & ~a) == 0 for a, b, c in zip(outer, middle, inner)))
                        width, rows, left, top, advance = metrics
                        self.assertLessEqual(top + rows, size)
                        self.assertGreaterEqual(left, 0)
                        self.assertLessEqual(left + width, advance)
                    # Curves really contain intermediate shades, not three identical masks.
                    self.assertNotEqual(masks[0][ord('8')][1], masks[1][ord('8')][1])
                    self.assertNotEqual(masks[1][ord('8')][1], masks[2][ord('8')][1])

    def test_every_time_fits_its_native_frame(self):
        # Insets used by the watch renderer: hybrid first, then full digital.
        for size, available_width, available_height in ((24, 72, 30), (28, 96, 34), (34, 128, 54), (42, 166, 54)):
            glyphs = read_glyphs(ROOT / f'resources/fonts/smooth/Roboto-Bold-{size}-1.pbf')
            for hour in range(24):
                for minute in range(60):
                    text = f'{hour:02}:{minute:02}'
                    self.assertLessEqual(sum(glyphs[ord(c)][0][4] for c in text), available_width)
                    self.assertLessEqual(max(glyphs[ord(c)][0][3] + glyphs[ord(c)][0][1] for c in text), available_height)


if __name__ == '__main__':
    unittest.main()
