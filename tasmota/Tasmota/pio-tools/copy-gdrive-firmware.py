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
SERVER_HOST = "gha-runner@192.168.0.15"
SERVER_TARGET_DIR = "/ota/tasmota/esp02s"
SERVER_UPDATE_SCRIPT = "/ota/tasmota/update_latest.py"
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
        raise RuntimeError("[ESP02S][설정] platformio 설정에 custom_gdrive_copy_dir 값이 없습니다")
    return pathlib.Path(gdrive_dir)


def _sanitize_version(version_text: str) -> str:
    sanitized = "".join(ch for ch in version_text.strip() if ch.isdigit())
    if not sanitized:
        raise RuntimeError("[ESP02S][버전] 버전 번호는 숫자로 입력해야 합니다. 예: 24")
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
            f"[ESP02S][버전] 새 버전은 현재 구글드라이브 버전 v{current_version}보다 커야 합니다"
        )
    return sanitized


def _prompt_version_text(current_version: str | None) -> str | None:
    if not sys.stdin or not sys.stdin.isatty():
        _log("버전", "대화형 입력을 사용할 수 없어 복사 단계를 건너뜁니다", Fore.YELLOW)
        return None

    if current_version:
        _log("버전", f"현재 구글드라이브 버전: v{current_version}")
    else:
        _log("버전", "현재 구글드라이브 버전: 없음")

    print("업로드할 파일 버전을 입력하세요 :", flush=True)
    return sys.stdin.readline().strip()


def _get_version_text() -> str:
    global _cached_version_text
    if _cached_version_text:
        _log("버전", f"앞에서 선택한 버전을 재사용합니다: v{_cached_version_text}")
        return _cached_version_text

    env_version = os.environ.get("GDRIVE_VERSION", "").strip()
    if env_version:
        _cached_version_text = _sanitize_version(env_version)
        _log("버전", f"환경변수에서 선택된 버전을 사용합니다: v{_cached_version_text}")
        return _cached_version_text

    gdrive_dir = _get_gdrive_dir()
    current_version = _find_current_gdrive_version(gdrive_dir)

    _log("시작", "빌드 후 배포 파이프라인을 시작합니다")
    version_text = _prompt_version_text(current_version)
    if not version_text:
        return ""

    _cached_version_text = _validate_newer_version(version_text, current_version)
    _log("버전", f"선택한 버전: v{_cached_version_text}")
    return _cached_version_text


def _run_command(command: list[str], start_message: str, success_message: str) -> None:
    _log("CMD", start_message)
    _log("CMD", " ".join(command))
    try:
        subprocess.run(command, check=True)
        _log("CMD", success_message, Fore.GREEN)
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(f"[ESP02S][명령] 명령 실행에 실패했습니다: {' '.join(command)}") from exc


def _upload_to_server(firmware_path: pathlib.Path) -> None:
    _run_command(
        ["scp", str(firmware_path), f"{SERVER_HOST}:{SERVER_TARGET_DIR}/"],
        f"서버에 펌웨어 업로드 시작: {firmware_path.name}",
        f"서버 업로드 완료: {SERVER_TARGET_DIR}/{firmware_path.name}",
    )


def _upload_minimal_if_present() -> None:
    if not MINIMAL_SOURCE_PATH.is_file():
        _log("미니멀", f"minimal 파일이 없어 건너뜁니다: {MINIMAL_SOURCE_PATH}", Fore.YELLOW)
        return

    _run_command(
        ["scp", str(MINIMAL_SOURCE_PATH), f"{SERVER_HOST}:{SERVER_TARGET_DIR}/{MINIMAL_TARGET_NAME}"],
        f"minimal 파일 업로드 시작: {MINIMAL_SOURCE_PATH.name}",
        f"minimal 업로드 완료: {SERVER_TARGET_DIR}/{MINIMAL_TARGET_NAME}",
    )


def _refresh_server_symlink() -> None:
    _run_command(
        ["ssh", SERVER_HOST, f"python3 {SERVER_UPDATE_SCRIPT}"],
        "서버 심볼릭 링크 갱신 스크립트 실행",
        "서버 심볼릭 링크 갱신 완료",
    )


def _copy_versioned_firmware(source, target, env):
    if not _should_run():
        return

    source_bin = pathlib.Path(tasmotapiolib.get_final_bin_path(env)).with_suffix(".bin.gz")
    if not source_bin.is_file():
        _log("빌드", f"gzip 산출물을 찾지 못해 건너뜁니다: {source_bin}", Fore.YELLOW)
        return

    _log("빌드", f"빌드 산출물 확인: {source_bin}")
    version_text = _get_version_text()
    if not version_text:
        return

    destination_name = f"v{version_text}_{OUTPUT_BASENAME}"
    gdrive_dir = _get_gdrive_dir()
    destination_path = gdrive_dir / destination_name

    if not gdrive_dir.exists():
        raise RuntimeError(f"[ESP02S][구글드라이브] 폴더를 찾지 못했습니다: {gdrive_dir}")

    _log("구글드라이브", f"구글드라이브로 펌웨어 복사: {destination_path}")
    shutil.copyfile(source_bin, destination_path)
    _log("구글드라이브", f"복사 완료: {destination_path}", Fore.GREEN)

    _upload_to_server(destination_path)
    _upload_minimal_if_present()
    _refresh_server_symlink()
    _log("완료", "빌드 후 배포 파이프라인이 끝났습니다", Fore.GREEN)


copy_action = env.Action(_copy_versioned_firmware)
copy_action.strfunction = lambda target, source, env: ""
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_action)
