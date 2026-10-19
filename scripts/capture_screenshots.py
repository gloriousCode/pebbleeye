#!/usr/bin/env python3
"""Capture native PebbleEye pixels on dedicated Time 2 / Round 2 emulators.

Uses pebble-tool's Python and installed SDK. Generated evidence stays in build/.
The emulator is deliberately left running for inspection.
"""
import argparse
import bz2
import hashlib
import json
import os
from pathlib import Path
import queue
import shutil
import socket
import subprocess
import time
import uuid
import zipfile
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[1]
SDK = Path.home() / "Library/Application Support/Pebble SDK/SDKs/4.33.1"
PORTS = {"emery": 12544, "gabbro": 12545}
BOARDS = {"emery": ("pebble-emery", "cortex-m33", "mtd"),
          "gabbro": ("pebble-gabbro", "cortex-m33", "mtd")}
APP_UUID = "ec7e38a9-3852-4459-9265-597ebcfb678f"

def repo_path(value):
    """Reject output or input paths outside the current checkout."""
    path = (ROOT / value).resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError(f"Path must stay inside this checkout: {path}")
    return path


def source_fingerprint():
    """Record source hashes without copying source into a scratch directory."""
    paths = [ROOT / "appinfo.json", ROOT / "wscript"]
    paths.extend(sorted((ROOT / "src").rglob("*")))
    paths.extend(sorted((ROOT / "resources").rglob("*")))
    return {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in paths if path.is_file()}


def ensure_emulator(platform, sdk):
    """Start or reuse only this helper's recorded, dedicated emulator."""
    state_dir = repo_path(f"build/emulator/{platform}")
    record_path = state_dir / "capture-session.json"
    port = PORTS[platform]
    if record_path.exists():
        record = json.loads(record_path.read_text())
        result = subprocess.run(["ps", "-p", str(record["pid"]), "-o", "command="],
                                capture_output=True, text=True, check=False)
        if result.returncode == 0:
            command = result.stdout.strip()
            if str(state_dir / "flash.bin") not in command or f":{port}," not in command:
                raise RuntimeError("Recorded PID belongs to a different process; refusing reuse")
            return record
    # A bind check detects unrelated listeners before any emulator is started.
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", port))
    firmware = sdk / f"sdk-core/pebble/{platform}/qemu"
    qemu = sdk / "toolchain/bin/qemu-pebble"
    kernel = firmware / "qemu_micro_flash.bin"
    seed = firmware / "qemu_spi_flash.bin.bz2"
    for path in (qemu, kernel, seed):
        if not path.is_file():
            raise FileNotFoundError(path)
    state_dir.mkdir(parents=True, exist_ok=True)
    flash = state_dir / "flash.bin"
    if not flash.exists():
        with bz2.open(seed, "rb") as source, flash.open("xb") as destination:
            shutil.copyfileobj(source, destination)
    machine, cpu, flash_interface = BOARDS[platform]
    flash_options = (f"if=none,id=spi-flash,file={flash},format=raw"
                     if flash_interface == "spi" else f"if=mtd,format=raw,file={flash}")
    command = [str(qemu), "-rtc", "base=localtime", "-serial", "null",
               "-serial", f"tcp:127.0.0.1:{port},server=on,wait=off",
               "-serial", f"file:{state_dir / 'serial.log'}", "-kernel", str(kernel),
               "-machine", machine, "-cpu", cpu,
               "-drive", flash_options, "-display", "none"]
    if platform in ("emery", "flint"):
        command.extend(["-audio", "driver=none,id=audio0"])
    environment = os.environ.copy()
    environment["DYLD_LIBRARY_PATH"] = str(sdk / "toolchain/lib")
    with (state_dir / "qemu.log").open("ab") as log:
        process = subprocess.Popen(command, cwd=ROOT, env=environment,
                                   stdin=subprocess.DEVNULL, stdout=log, stderr=log,
                                   start_new_session=True)
    record = {"pid": process.pid, "platform": platform, "port": port,
              "command": command, "started_utc": datetime.now(timezone.utc).isoformat()}
    record_path.write_text(json.dumps(record, indent=2) + "\n")
    time.sleep(5)
    if process.poll() is not None:
        raise RuntimeError(f"Emulator exited; inspect {state_dir / 'qemu.log'}")
    return record


def capture(platform):
    import png
    from libpebble2.communication import PebbleConnection
    from libpebble2.communication.transports.qemu import QemuTransport, MessageTargetQemu
    from libpebble2.communication.transports.qemu.protocol import QemuBattery, QemuBluetoothConnection, QemuTimeFormat
    from libpebble2.services.appmessage import AppMessageService, Int32
    from libpebble2.services.install import AppInstaller
    from libpebble2.services.screenshot import Screenshot
    from libpebble2.protocol.system import TimeMessage, SetUTC

    pbw = ROOT / "build/pebbleeye.pbw"
    with zipfile.ZipFile(pbw) as bundle:
        metadata = json.loads(bundle.read("appinfo.json"))
    assert metadata["uuid"] == APP_UUID and metadata["targetPlatforms"] == ["emery", "gabbro"]
    session = ensure_emulator(platform, SDK)
    pebble = PebbleConnection(QemuTransport("127.0.0.1", PORTS[platform]))
    pebble.connect()
    pebble.transport.send_packet(QemuBluetoothConnection(connected=True), MessageTargetQemu())
    pebble.run_async()
    assert pebble.watch_platform == platform
    AppInstaller(pebble, str(pbw)).install()
    time.sleep(2)
    service = AppMessageService(pebble)
    acknowledgements = queue.Queue()
    service.register_handler("ack", lambda transaction, app: acknowledgements.put((transaction, True)))
    service.register_handler("nack", lambda transaction, app: acknowledgements.put((transaction, False)))
    output = ROOT / "build/screenshots"
    output.mkdir(exist_ok=True)
    manifest = {"bundle": "build/pebbleeye.pbw", "bundle_metadata": metadata,
        "bundle_sha256": hashlib.sha256(pbw.read_bytes()).hexdigest(),
        "source_sha256": source_fingerprint(), "emulator": session, "captures": []}
    scenes = [(mode + "-" + theme, {7:2, 14:0, 15:m, 16:t, 17:1}, 10, 8, True, True)
        for m, mode in enumerate(["digital", "analogue", "combined"])
        for t, theme in enumerate(["classic", "amber", "ice"])]
    scenes += [("date-" + str(d), {7:d, 14:0, 15:0, 16:0, 17:1}, 23, 59, True, True) for d in range(5)]
    scenes += [("midnight-12h", {7:2, 14:0, 15:2, 16:0, 17:0}, 0, 0, False, True),
               ("noon-12h", {7:2, 14:0, 15:2, 16:0, 17:0}, 12, 0, False, True),
               ("disconnected", {7:2, 14:0, 15:2, 16:0, 17:1}, 10, 8, True, False)]
    for name, settings, hour, minute, use_24h, connected in scenes:
        stamp = datetime(2026, 10, 19, hour, minute, 10, tzinfo=timezone.utc)
        pebble.send_packet(TimeMessage(message=SetUTC(unix_time=int(stamp.timestamp()), utc_offset=0, tz_name="UTC")))
        pebble.transport.send_packet(QemuTimeFormat(is_24_hour=use_24h), MessageTargetQemu())
        pebble.transport.send_packet(QemuBattery(percent=76 if connected else 15, charging=False), MessageTargetQemu())
        transaction = service.send_message(uuid.UUID(APP_UUID), {key:Int32(value) for key,value in settings.items()})
        received, accepted = acknowledgements.get(timeout=10)
        assert received == transaction and accepted, name
        pebble.transport.send_packet(QemuBluetoothConnection(connected=connected), MessageTargetQemu())
        time.sleep(1 if connected else 27)
        pixels = Screenshot(pebble).grab_image()
        path = output / f"{platform}-{name}.png"
        png.from_array(pixels, mode="RGB;8").save(str(path))
        manifest["captures"].append({"file":str(path.relative_to(ROOT)), "settings":settings,
            "time_utc":stamp.isoformat(), "24_hour":use_24h, "connected":connected,
            "sha256":hashlib.sha256(path.read_bytes()).hexdigest()})
        print(path, flush=True)
    (output / f"{platform}-manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    service.shutdown()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", choices=PORTS, required=True)
    args = parser.parse_args()
    capture(args.platform)
