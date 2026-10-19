#!/usr/bin/env python3
"""Bake three nested coverage masks using the installed Pebble SDK and FreeType.

Run with pebble-tool's Python. Outputs are native-size PBF fonts, not resized
images. All three masks share identical glyph metrics, including empty edges,
so drawing each at the same position preserves alignment and measured bounds.
"""
import argparse
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, default=Path.home() / 'Library/Application Support/Pebble SDK/SDKs/4.33.1')
    args = parser.parse_args()
    sys.path.insert(0, str(args.sdk / 'sdk-core/pebble/common/tools'))
    import freetype
    from font.fontgen import Font, MAX_GLYPHS

    class CoverageFont(Font):
        """Encode normal FreeType coverage as a standard Pebble one-bit mask."""
        threshold = 43

        def glyph_bits(self, codepoint, gindex):
            self.face.load_glyph(gindex, freetype.FT_LOAD_RENDER | freetype.FT_LOAD_TARGET_NORMAL)
            glyph = self.face.glyph
            bitmap = glyph.bitmap
            assert bitmap.pixel_mode == freetype.FT_PIXEL_MODE_GRAY
            width, height = bitmap.width, bitmap.rows
            assert (width * height + 7) // 8 <= self.max_glyph_size
            data = bytearray(((width * height + 31) // 32) * 4)
            coverage = bitmap.buffer
            for y in range(height):
                for x in range(width):
                    if coverage[y * bitmap.pitch + x] >= self.threshold:
                        bit = y * width + x
                        data[bit // 8] |= 1 << (bit % 8)
            header = struct.pack(self.glyph_header, width, height, glyph.bitmap_left,
                                 self.baseline - glyph.bitmap_top, glyph.advance.x // 64)
            return header + data

    output = ROOT / 'resources/fonts/smooth'
    output.mkdir(exist_ok=True)
    for size in (24, 28, 34, 42):
        for style in ('Bold',):
            for level, threshold in enumerate((43, 128, 213), 1):
                font = CoverageFont(str(ROOT / f'resources/fonts/Roboto-{style}.ttf'), size,
                                    MAX_GLYPHS, 256, False, baseline=size - 1)
                font.threshold = threshold
                font.set_regex_filter("[ 0-9:]")
                font.build_tables()
                path = output / f'Roboto-{style}-{size}-{level}.pbf'
                path.write_bytes(font.bitstring())
                print(path.relative_to(ROOT), path.stat().st_size)


if __name__ == '__main__':
    main()
