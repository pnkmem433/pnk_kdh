Import("env")

import os
import pathlib
import re
import sys

from colorama import Fore


TARGET_ENVIRONMENTS = {"tasmota-smartplug"}
_VERSION_CACHE = None


def _sanitize_version(value: str) -> str:
    sanitized = "".join(ch for ch in value if ch.isdigit())
    if not sanitized:
        raise RuntimeError("[VERSION] 버전 번호는 숫자로 입력해야 합니다. 예: 10")
    return sanitized


def _get_gdrive_dir() -> pathlib.Path:
    project_config = env.GetProjectConfig()
    gdrive_dir = project_config.get("env:tasmota-smartplug", "custom_gdrive_copy_dir", "")
    if not gdrive_dir:
        raise RuntimeError("[VERSION] platformio 설정에 custom_gdrive_copy_dir 값이 없습니다.")
    return pathlib.Path(gdrive_dir)


def _find_current_gdrive_version(gdrive_dir: pathlib.Path) -> str | None:
    version_pattern = re.compile(r"^v(\d+)_esp02s_tasmota\.bin\.gz$", re.IGNORECASE)
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
            f"[VERSION] 새 버전은 현재 구글드라이브 버전(v{current_version})보다 커야 합니다."
        )
    return sanitized


def _prompt_version_once() -> str:
    global _VERSION_CACHE
    if _VERSION_CACHE:
        print(Fore.CYAN + f"[VERSION] 앞에서 선택한 버전 사용 : v{_VERSION_CACHE}", flush=True)
        return _VERSION_CACHE

    env_version = os.environ.get("GDRIVE_VERSION", "").strip()
    if env_version:
        _VERSION_CACHE = _sanitize_version(env_version)
        print(Fore.CYAN + f"[VERSION] 앞에서 선택한 버전 사용 : v{_VERSION_CACHE}", flush=True)
        return _VERSION_CACHE

    gdrive_dir = _get_gdrive_dir()
    current_version = _find_current_gdrive_version(gdrive_dir)

    if not sys.stdin or not sys.stdin.isatty():
        raise RuntimeError(
            "[VERSION] 터미널 입력을 받을 수 없습니다. VS Code Task 또는 PowerShell에서 직접 실행하거나 "
            "GDRIVE_VERSION 환경변수를 먼저 지정하세요."
        )

    if current_version:
        print(Fore.CYAN + f"[VERSION] 현재 구글드라이브 버전 : v{current_version}", flush=True)
    else:
        print(Fore.CYAN + "[VERSION] 현재 구글드라이브 버전 : 없음", flush=True)

    print("업로드할 파일 버전을 입력하세요 :", flush=True)
    version_text = sys.stdin.readline().strip()
    _VERSION_CACHE = _validate_newer_version(version_text, current_version)
    os.environ["GDRIVE_VERSION"] = _VERSION_CACHE

    print(Fore.CYAN + f"[VERSION] 선택한 버전 : v{_VERSION_CACHE}", flush=True)
    return _VERSION_CACHE


def _image_name() -> str:
    version = _prompt_version_once()
    return f"v{version}_esp02s"


if env["PIOENV"] in TARGET_ENVIRONMENTS:
    image_name = _image_name()
    env.Append(BUILD_FLAGS=[f"-DCODE_IMAGE_STR='\"{image_name}\"'"])
    print(f"[CODE_IMAGE] Using CODE_IMAGE_STR={image_name}")
