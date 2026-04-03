Import("env")

import pathlib
import re
import shutil
import subprocess
from colorama import Fore

import tasmotapiolib


GDRIVE_DIR = pathlib.Path(env.GetProjectOption("custom_gdrive_copy_dir"))
SSH_TARGET = "gha-runner@192.168.0.15"
SSH_REMOTE_DIR = "/ota/tasmota"
BASE_FILENAME = "tasmota-smartplug.bin.gz"
VERSIONED_PATTERN = re.compile(rf"^v(\d+)_{re.escape(BASE_FILENAME)}$")

_cached_versioned_name = None


def _find_latest_version():
    if not GDRIVE_DIR.exists():
        return None, None

    latest_version = None
    latest_name = None
    for path in GDRIVE_DIR.iterdir():
        if not path.is_file():
            continue
        match = VERSIONED_PATTERN.match(path.name)
        if not match:
            continue
        version = int(match.group(1))
        if (latest_version is None) or (version > latest_version):
            latest_version = version
            latest_name = path.name

    return latest_version, latest_name


def _get_versioned_name():
    global _cached_versioned_name
    if _cached_versioned_name:
        return _cached_versioned_name

    latest_version, latest_name = _find_latest_version()
    if latest_version is None:
        print(Fore.CYAN + "Latest OTA version: none")
    else:
        print(Fore.CYAN + f"Latest OTA version: v{latest_version} ({latest_name})")

    version_number = input("Enter OTA version number (example: 52): ").strip()
    if not version_number:
        raise RuntimeError("OTA version number is required")

    safe_version = "".join(ch for ch in version_number if ch.isdigit())
    if not safe_version:
        raise RuntimeError(f"Invalid OTA version number: {version_number}")

    version_value = int(safe_version)
    if (latest_version is not None) and (version_value <= latest_version):
        raise RuntimeError(
            f"OTA version must be greater than the latest existing version v{latest_version}"
        )

    _cached_versioned_name = f"v{version_value}_{BASE_FILENAME}"
    print(Fore.CYAN + f"Using OTA artifact name: {_cached_versioned_name}")
    return _cached_versioned_name


def _copy_gzip_firmware(source, target, env):
    source_bin = pathlib.Path(tasmotapiolib.get_final_bin_path(env))
    source_gz = source_bin.with_suffix(".bin.gz")
    if not source_gz.is_file():
        print(Fore.YELLOW + f"Skipping Google Drive copy because gzip firmware was not found: {source_gz}")
        return

    destination_name = _get_versioned_name()
    GDRIVE_DIR.mkdir(parents=True, exist_ok=True)
    destination_file = GDRIVE_DIR / destination_name
    shutil.copyfile(source_gz, destination_file)
    print(Fore.GREEN + f"Copied OTA gzip firmware to {destination_file}")


def _upload_gzip_firmware(source, target, env):
    source_bin = pathlib.Path(tasmotapiolib.get_final_bin_path(env))
    source_gz = source_bin.with_suffix(".bin.gz")
    if not source_gz.is_file():
        print(Fore.YELLOW + f"Skipping SSH upload because gzip firmware was not found: {source_gz}")
        return

    remote_name = _get_versioned_name()
    mkdir_cmd = ["ssh", "-o", "StrictHostKeyChecking=accept-new", SSH_TARGET, f"mkdir -p '{SSH_REMOTE_DIR}'"]
    copy_cmd = [
        "scp",
        "-o",
        "StrictHostKeyChecking=accept-new",
        str(source_gz),
        f"{SSH_TARGET}:{SSH_REMOTE_DIR}/{remote_name}",
    ]

    subprocess.run(mkdir_cmd, check=True)
    subprocess.run(copy_cmd, check=True)
    print(Fore.GREEN + f"Uploaded OTA gzip firmware to {SSH_TARGET}:{SSH_REMOTE_DIR}/{remote_name}")


copy_action = env.Action(_copy_gzip_firmware)
copy_action.strfunction = lambda target, source, env: ""
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_action)

upload_action = env.Action(_upload_gzip_firmware)
upload_action.strfunction = lambda target, source, env: ""
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", upload_action)
