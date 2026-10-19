# PebbleEye store package

Open [the store page](index.html) to review the full listing, icons, banner and
all eight palettes. This local package is ready to upload; it is not a published
Pebble store listing.

## Listing fields

- Name: **PebbleEye** (`title.txt`).
- Tagline: **Your time. Your mission.** (`tagline.txt`).
- Description: [description.txt](description.txt).
- Release notes: [release-notes.txt](release-notes.txt).
- App type: watchface. Supported models: **Time 2** and **Round 2** only.
- Bundle: `build/pebbleeye.pbw`, version **1.6.2**, code **19**.
- UUID: `ec7e38a9-3852-4459-9265-597ebcfb678f` (separate from Wordish).

## Artwork

| Field | File | Exact dimensions |
| --- | --- | --- |
| Small icon | [pebbleeye-icon-80.png](pebbleeye-icon-80.png) | 80×80 |
| Large icon | [pebbleeye-icon-144.png](pebbleeye-icon-144.png) | 144×144 |
| Banner | [pebbleeye-banner.png](pebbleeye-banner.png) | 720×320 |

The icons and banner are original promotional artwork, drawn directly at their
final sizes. They are not emulator screenshots. `artwork-manifest.json` records
the dimensions and checksums. Regenerate with `scripts/prepare_store_artwork.py`
using a Python environment with Pillow.

## Screenshot galleries

Sixteen PNGs contain unchanged native emulator pixels: eight palettes per watch,
spread across all three display modes. Upload `emery-*.png` to **Pebble Time 2**
(200×228) and `gabbro-*.png` to **Pebble Round 2** (260×260).

Gallery order: Classic, Amber, Ice, Orchid, Lagoon, Ember, Silver, Rose.
Colours apply to every display mode, not just the illustrated pairings.
`manifest.json` records image checksums and source bundle identity.

Run `python3 scripts/prepare_store_screenshots.py` after capturing both devices
to verify dimensions and source/bundle hashes before refreshing the screenshots.
Raw verification images stay in ignored `build/screenshots/`.

## Before publishing

Push the approved source and enable GitHub Pages from `main`, root folder. Verify
that the standalone settings URL in the phone bridge loads before uploading the
bundle. Store publication and the hosted phone settings still require a live
end-to-end check; the local package does not establish either.
