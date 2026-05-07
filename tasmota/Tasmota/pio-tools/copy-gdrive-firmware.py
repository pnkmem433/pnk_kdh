Import("env")

import os
import pathlib
import re
import shutil
import subprocess
import sys

from colorama import Fore

import tasmotapiolib


TARGET_ENVIRONMENTS = {"tasmota-smartplug"}
OUTPUT_BASENAME = "esp02s_tasmota.bin.gz"
SERVER_HOST = "dockeruser@gym907-0001.iptime.org"
SERVER_PORT = "23"
SERVER_TARGET_DIR = "/ota/esp02s/tasmota"
SERVER_UPDATE_SCRIPT = "/ota/update_latest.py"
MINIMAL_SOURCE_PATH = pathlib.Path(r"C:\Users\pnks\Downloads\tasmota-minimal.bin.gz")
MINIMAL_TARGET_NAME = "esp02s_tasmota-minimal.bin.gz"

_cached_version_text = None


def _log(step: str, message: str, color: str = Fore.CYAN) -> None:
    print(color + f"[ESP02S][{step}] {message}", flush=True)


def _should_run() -> bool:
    return env["PIOENV"] in TARGET_ENVIRONMENTS


def _get_gdrive_dir() -> pathlib.Path:
    project_config = env.GetProjectConfig()
    gdrive_dir = project_config.get("env:tasmota-smartplug", "custom_gdrive_copy_dir", "")
    if not gdrive_dir:
        raise RuntimeError("[ESP02S][??] platformio ??? custom_gdrive_copy_dir ?? ????")
    return pathlib.Path(gdrive_dir)


def _sanitize_version(version_text: str) -> str:
    sanitized = "".join(ch for ch in version_text.strip() if ch.isdigit())
    if not sanitized:
        raise RuntimeError("[ESP02S][??] ?? ??? ??? ???? ???. ?: 24")
    return sanitized


def _find_current_gdrive_version(gdrive_dir: pathlib.Path) -> str | None:
    version_pattern = re.compile(r"^v(\d+)_esp02s_tasmota\.bin\.gz$", re.IGNORECASE)
    latest_version = None
    latest_mtime = None

    if not gdrive_dir.exists():
        return None

    for entry in gdrive_dir.iterdir():
        if not entry.is_file():
            continue
        match = version_pattern.match(entry.name)
        if not match:
            continue
        entry_mtime = entry.stat().st_mtime
        if latest_mtime is None or entry_mtime > latest_mtime:
            latest_mtime = entry_mtime
            latest_version = match.group(1)

    return latest_version


def _validate_newer_version(version_text: str, current_version: str | None) -> str:
    sanitized = _sanitize_version(version_text)
    if current_version is not None and int(sanitized) <= int(current_version):
        raise RuntimeError(
            f"[ESP02S][??] ? ??? ?? ?????? ?? v{current_version}?? ?? ???"
        )
    return sanitized


def _prompt_version_text(current_version: str | None) -> str | None:
    if not sys.stdin or not sys.stdin.isatty():
        _log("??", "??? ??? ??? ? ?? ?? ??? ?????.", Fore.YELLOW)
        return None

    if current_version:
        _log("??", f"?? ?????? ??: v{current_version}")
    else:
        _log("??", "?? ?????? ??: ??")

    print("???? ?? ??? ????? :", flush=True)
    return sys.stdin.readline().strip()


def _get_version_text() -> str:
    global _cached_version_text
    if _cached_version_text:
        _log("??", f"??? ??? ??? ??????: v{_cached_version_text}")
        return _cached_version_text

    env_version = os.environ.get("GDRIVE_VERSION", "").strip()
    if env_version:
        _cached_version_text = _sanitize_version(env_version)
        _log("??", f"?????? ??? ??? ?????: v{_cached_version_text}")
        return _cached_version_text

    gdrive_dir = _get_gdrive_dir()
    current_version = _find_current_gdrive_version(gdrive_dir)

    _log("??", "?? ? ?? ?????? ?????")
    version_text = _prompt_version_text(current_version)
    if not version_text:
        return ""

    _cached_version_text = _validate_newer_version(version_text, current_version)
    _log("??", f"??? ??: v{_cached_version_text}")
    return _cached_version_text


def _run_command(command: list[str], start_message: str, success_message: str) -> None:
    _log("??", start_message)
    _log("??", " ".join(command))
    try:
        subprocess.run(command, check=True)
        _log("??", success_message, Fore.GREEN)
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(f"[ESP02S][??] ?? ??? ??????: {' '.join(command)}") from exc


def _upload_to_server(firmware_path: pathlib.Path) -> None:
    _run_command(
        ["scp", "-P", SERVER_PORT, str(firmware_path), f"{SERVER_HOST}:{SERVER_TARGET_DIR}/"],
        f"?? ???? ?????: {firmware_path.name}",
        f"?? ???? ???????: {SERVER_TARGET_DIR}/{firmware_path.name}",
    )


def _upload_minimal_if_present() -> None:
    if not MINIMAL_SOURCE_PATH.is_file():
        _log("???", f"minimal ??? ?? ?????: {MINIMAL_SOURCE_PATH}", Fore.YELLOW)
        return

    _run_command(
        ["scp", "-P", SERVER_PORT, str(MINIMAL_SOURCE_PATH), f"{SERVER_HOST}:{SERVER_TARGET_DIR}/{MINIMAL_TARGET_NAME}"],
        f"minimal ?? ???? ?????: {MINIMAL_SOURCE_PATH.name}",
        f"minimal ???? ???????: {SERVER_TARGET_DIR}/{MINIMAL_TARGET_NAME}",
    )


def _refresh_server_symlink() -> None:
    _run_command(
        ["ssh", "-p", SERVER_PORT, SERVER_HOST, f"python3 {SERVER_UPDATE_SCRIPT}"],
        "?? ?? ??? ?? ?? ????? ?????",
        "?? ?? ??? ?? ??? ???????",
    )


def _copy_versioned_firmware(source, target, env):
    if not _should_run():
        return

    source_bin = pathlib.Path(tasmotapiolib.get_final_bin_path(env)).with_suffix(".bin.gz")
    if not source_bin.is_file():
        _log("??", f"gzip ???? ?? ?? ?????: {source_bin}", Fore.YELLOW)
        return

    _log("??", f"?? ??? ??: {source_bin}")
    version_text = _get_version_text()
    if not version_text:
        return

    destination_name = f"v{version_text}_{OUTPUT_BASENAME}"
    gdrive_dir = _get_gdrive_dir()
    destination_path = gdrive_dir / destination_name

    if not gdrive_dir.exists():
        _log("구글드라이브", f"복사 폴더가 없어 새로 만듭니다: {gdrive_dir}", Fore.YELLOW)
        gdrive_dir.mkdir(parents=True, exist_ok=True)

    _log("??????", f"??????? ???? ?????: {destination_path}")
    shutil.copyfile(source_bin, destination_path)
    _log("??????", f"??? ???????: {destination_path}", Fore.GREEN)

    _upload_to_server(destination_path)
    _upload_minimal_if_present()
    _refresh_server_symlink()
    _log("??", "?? ? ?? ?????? ???????", Fore.GREEN)


copy_action = env.Action(_copy_versioned_firmware)
copy_action.strfunction = lambda target, source, env: ""
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_action)
