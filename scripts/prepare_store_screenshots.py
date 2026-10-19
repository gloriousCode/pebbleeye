#!/usr/bin/env python3
"""Verify native capture provenance and select one screenshot per palette/device."""
import hashlib
import json
from pathlib import Path
import shutil
import struct

ROOT = Path(__file__).resolve().parents[1]
SELECTION = {"classic": "digital", "amber": "analogue", "ice": "combined",
             "orchid": "digital", "lagoon": "combined", "ember": "analogue",
             "silver": "digital", "rose": "combined"}


def main():
    bundle = ROOT / "build/pebbleeye.pbw"
    digest = hashlib.sha256(bundle.read_bytes()).hexdigest()
    target = ROOT / "store"
    result = {"bundle_sha256": digest, "screenshots": []}
    for platform, dimensions in (("emery", (200, 228)), ("gabbro", (260, 260))):
        evidence = json.loads((ROOT / f"build/screenshots/{platform}-manifest.json").read_text())
        assert evidence["bundle_sha256"] == digest, "Rebuild requires fresh captures"
        for name, expected in evidence["source_sha256"].items():
            assert hashlib.sha256((ROOT / name).read_bytes()).hexdigest() == expected, name
        metadata = evidence["bundle_metadata"]
        result.update(version=metadata["versionLabel"], version_code=metadata["versionCode"], uuid=metadata["uuid"])
        for palette, mode in SELECTION.items():
            name = f"{platform}-{mode}-{palette}.png"
            shot = next(item for item in evidence["captures"] if item["file"] == f"build/screenshots/{name}")
            source = ROOT / shot["file"]
            pixels = source.read_bytes()
            assert pixels[:8] == b"\x89PNG\r\n\x1a\n"
            assert struct.unpack(">II", pixels[16:24]) == dimensions
            assert hashlib.sha256(pixels).hexdigest() == shot["sha256"]
            shutil.copyfile(source, target / name)
            result["screenshots"].append(dict(shot, file=name, palette=palette,
                width=dimensions[0], height=dimensions[1]))
    (target / "manifest.json").write_text(json.dumps(result, indent=2) + "\n")
    print("Verified and copied 16 native screenshots covering all eight palettes on both watches")


if __name__ == "__main__":
    main()
