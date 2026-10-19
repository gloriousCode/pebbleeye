# Store listing concept

The [visual concept](index.html) presents the proposed listing; it is not a live
Pebble store entry and does not claim downloads or ratings.

Use `title.txt`, `tagline.txt` and `description.txt` for the listing. The sixteen PNGs
are unaltered native emulator captures: eight palettes per device, spread across all three modes. Time 2 images are 200×228; Round 2 images are 260×260. Upload each
to the corresponding hardware gallery. `manifest.json` records their checksums
and the source bundle identity. The installable file is `build/pebbleeye.pbw`.

Order each gallery: Classic, Amber, Ice, Orchid, Lagoon, Ember, Silver, Rose.
The colour controls apply to all three modes, not just the illustrated pairings.
Run `python3 scripts/prepare_store_screenshots.py` after capturing both devices
to validate image dimensions and source/bundle hashes before updating this set.
Raw verification images are generated under ignored `build/screenshots/`.
