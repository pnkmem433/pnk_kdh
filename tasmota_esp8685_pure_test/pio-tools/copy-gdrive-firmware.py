Import("env")

import os
import pathlib
import re
import shutil
import sys

from colorama import Fore

import tasmotapiolib


TARGET_ENVIRONMENTS = {"tasmota32c3-work", "tasmota32c3-work-dev"}
OUTPUT_BASENAME = "esp8685_tasmota.bin"

_cached_version_text = None


def _should_run() -> bool:
    return env["PIOENV"] in TARGET_ENVIRONMENTS


def _get_gdrive_dir() -> pathlib.Path:
    project_config = env.GetProjectConfig()
    gdrive_dir = project_config.get("common", "custom_gdrive_copy_dir", "")
    if not gdrive_dir:
        raise RuntimeError("Missing [common] custom_gdrive_copy_dir in platformio.ini")
    return pathlib.Path(gdrive_dir)


def _sanitize_version(version_text: str) -> str:
    sanitized = "".join(ch for ch in version_text.strip() if ch.isdigit())
    if not sanitized:
        raise RuntimeError("버전 번호는 숫자로 입력해야 합니다. 예: 5")
    return sanitized


def _find_current_gdrive_version(gdrive_dir: pathlib.Path) -> str | None:
    version_pattern = re.compile(r"^v(\d+)_esp8685_tasmota\.bin$", re.IGNORECASE)
    latest_version = None

    if not gdrive_dir.exists():
        return None

    for entry in gdrive_dir.iterdir():
        if not entry.is_file():
            continue
        match = version_pattern.match(entry.name)
        if not match:
            continue
        version_number = int(match.group(1))
        if latest_version is None or version_number > latest_version:
            latest_version = version_number

    if latest_version is None:
        return None
    return str(latest_version)


def _validate_newer_version(version_text: str, current_version: str | None) -> str:
    sanitized = _sanitize_version(version_text)
    if current_version is not None and int(sanitized) <= int(current_version):
        raise RuntimeError(
            f"업로드할 파일 버전은 현재 구글드라이브 버전(v{current_version})보다 큰 숫자여야 합니다."
        )
    return sanitized


def _prompt_version_text(current_version: str | None) -> str:
    if current_version:
        print(Fore.CYAN + f"[GDRIVE] 현재 구글드라이브 버전 : v{current_version}", flush=True)
    else:
        print(Fore.CYAN + "[GDRIVE] 현재 구글드라이브 버전 : 없음", flush=True)

    print(Fore.CYAN + "[GDRIVE] 업로드할 파일 버전을 입력하세요 :", flush=True)
    return input().strip()


def _get_version_text() -> str:
    global _cached_version_text
    if _cached_version_text:
        return _cached_version_text

    env_version = os.environ.get("GDRIVE_VERSION", "").strip()
    if env_version:
        _cached_version_text = _sanitize_version(env_version)
        print(Fore.CYAN + f"[GDRIVE] 환경변수 버전 사용 : v{_cached_version_text}")
        return _cached_version_text

    gdrive_dir = _get_gdrive_dir()
    current_version = _find_current_gdrive_version(gdrive_dir)

    print(Fore.CYAN + "[GDRIVE] OTA artifact copy step", flush=True)
    version_text = _prompt_version_text(current_version)
    _cached_version_text = _validate_newer_version(version_text, current_version)
    print(Fore.CYAN + f"[GDRIVE] 업로드 버전 : v{_cached_version_text}")
    return _cached_version_text


def _copy_versioned_firmware(source, target, env):
    if not _should_run():
        return

    source_bin = pathlib.Path(tasmotapiolib.get_final_bin_path(env))
    if not source_bin.is_file():
        print(Fore.YELLOW + f"[GDRIVE] Skip copy, firmware not found: {source_bin}")
        return

    version_text = _get_version_text()
    destination_name = f"v{version_text}_{OUTPUT_BASENAME}"
    gdrive_dir = _get_gdrive_dir()
    destination_path = gdrive_dir / destination_name

    gdrive_dir.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source_bin, destination_path)

    print(Fore.GREEN + f"[GDRIVE] Copied: {destination_path}")


copy_action = env.Action(_copy_versioned_firmware)
copy_action.strfunction = lambda target, source, env: ""
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_action)
