# PebbleEye

A spy-watch display for **Pebble Time 2** (Emery, 200×228) and **Pebble Round 2**
(Gabbro, 260×260). Segmented red/gold and blue rings frame a digital, analogue or
hybrid clock. Classic green, warm amber and cool ice palettes are available in
all three modes.

## Preview

| | Digital · Classic | Analogue · Amber | Hybrid · Ice |
| --- | --- | --- | --- |
| Time 2 | ![Time 2 digital](store/emery-digital-classic.png) | ![Time 2 analogue](store/emery-analogue-amber.png) | ![Time 2 hybrid](store/emery-combined-ice.png) |
| Round 2 | ![Round 2 digital](store/gabbro-digital-classic.png) | ![Round 2 analogue](store/gabbro-analogue-amber.png) | ![Round 2 hybrid](store/gabbro-combined-ice.png) |

[Store-page concept](store/index.html) · [Listing copy](store/description.txt) ·
[Configuration page](resources/configure-pebbleeye.html)

## Features

- Full-size analogue hands, clear digital time, or digital time over the analogue dial.
- Three coordinated colour palettes and five date formats.
- Optional battery, phone connection and Quiet Time status (`Q`).
- Optional seconds hand while the backlight is on. Off by default; otherwise the
  face updates on minute ticks and settings/status events.
- Watch system 12-hour or 24-hour preference.
- Mobile settings with illustrative preview, Undo, Save and Cancel.
- Validated settings, preserved preferences and bounded delivery retries.

This is a separate app from Wordish. Its existing experimental UUID
`ec7e38a9-3852-4459-9265-597ebcfb678f` is preserved, along with the
`gabbroeye-hud-options-v1` phone storage key. The standalone release is **1.6.2**,
version code **19**, advancing the branch's 1.5.1. Older Pebble models are not supported.

## Build and verify

Requires Pebble SDK 4.33.1 with Emery/Gabbro support, the Pebble CLI, a host C
compiler, Python and Node.js.

```sh
make lint
make misc_checks
```

The installable bundle is `build/pebbleeye.pbw`. Checks cover two native builds,
circular layout bounds, settings validation, runtime lifecycle/backlight
subscriptions, and configuration delivery and form round-trips.

For native emulator captures, use pebble-tool's Python:

```sh
python scripts/capture_screenshots.py --platform emery
python scripts/capture_screenshots.py --platform gabbro
```

The helper owns dedicated ports 12544/12545 and leaves its emulators running.
It checks all three modes and palettes, five dates, midnight/noon in 12-hour
mode, and disconnected status. Raw images and source/bundle checksums stay in
ignored `build/screenshots/`. Only six current promotional images are tracked.
Physical watch contrast, battery endurance and the Pebble phone webview remain
separate device checks; emulator and host tests do not establish those results.

## Configuration hosting

Enable GitHub Pages for the repository's `main` branch, root folder. The bridge
opens `https://gloriouscode.github.io/pebbleeye/resources/configure-pebbleeye.html`
with a version query and saved settings in the fragment. The store concept is
available at `/pebbleeye/store/`. No separate web framework or build is needed.

## Origins

Split from [Wordish](https://github.com/gloriousCode/wordish), branch
`codex/gabbro-pebbleeye-hud`, commit `1d6028bb3785fc85e7fd8de4b9d2eefb797aa316`.
The source branch remains untouched. The experimental ring renderer originated
there; the standalone app removes its underlying fuzzy-text layers and resources.
The original word-clock project was [Fuzzy Text International](https://github.com/hallettj/Fuzzy-Text-International).

PebbleEye is an independent, unofficial homage to classic spy-game watch displays.
