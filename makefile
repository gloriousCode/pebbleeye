.PHONY: build test lint misc_checks config-emery config-gabbro
build:
	pebble build

test:
	mkdir -p build
	$(CC) -std=c99 -Wall -Wextra -Werror -Isrc tests/spy_face_geometry_test.c src/spy_face_geometry.c -o build/spy_face_geometry_test
	./build/spy_face_geometry_test
	$(CC) -std=c99 -Wall -Wextra -Werror -Isrc tests/settings_test.c src/settings.c -o build/settings_test
	./build/settings_test
	$(CC) -std=c99 -Wall -Wextra -Werror -Itests/stubs -Isrc tests/runtime_test.c src/settings.c -o build/runtime_test
	./build/runtime_test
	$(CC) -std=c99 -Wall -Wextra -Werror -DPBL_PLATFORM_EMERY -Itests/stubs -Isrc tests/theme_audio_test.c -o build/theme_audio_test
	./build/theme_audio_test
	$(CC) -std=c99 -Wall -Wextra -Werror -DPBL_PLATFORM_GABBRO -Itests/stubs -Isrc tests/theme_audio_test.c -o build/theme_audio_gabbro_test
	./build/theme_audio_gabbro_test
	$(CC) -std=c99 -Wall -Wextra -Werror -DPBL_PLATFORM_GABBRO -Itests/stubs -Isrc tests/theme_vibes_test.c -o build/theme_vibes_test
	./build/theme_vibes_test
	$(CC) -std=c99 -Wall -Wextra -Werror -DPBL_PLATFORM_EMERY -Itests/stubs -Isrc tests/theme_vibes_test.c -o build/theme_vibes_emery_test
	./build/theme_vibes_emery_test
	node --test tests/configuration_test.js
	$(CC) -std=c99 -Wall -Wextra -Werror -Itests/smooth_font_stubs -Isrc tests/smooth_font_renderer_test.c -o build/smooth_font_renderer_test
	./build/smooth_font_renderer_test
	python3 tests/smooth_fonts_test.py

lint: build
	node --check src/js/pebble-js-app.js
misc_checks: test
config-emery:
	pebble emu-app-config --emulator emery --file resources/configure-pebbleeye.html
config-gabbro:
	pebble emu-app-config --emulator gabbro --file resources/configure-pebbleeye.html
