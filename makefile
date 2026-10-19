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
	node --test tests/configuration_test.js

lint: build
	node --check src/js/pebble-js-app.js
misc_checks: test
config-emery:
	pebble emu-app-config --emulator emery --file resources/configure-pebbleeye.html
config-gabbro:
	pebble emu-app-config --emulator gabbro --file resources/configure-pebbleeye.html
