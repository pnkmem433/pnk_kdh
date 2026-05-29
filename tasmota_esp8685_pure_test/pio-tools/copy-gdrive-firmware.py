Import("env")

import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import base64

from colorama import Fore

import tasmotapiolib


OUTPUT_BASENAME = "esp8685_tasmota.bin"

_cached_version_text = None

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")
if hasattr(sys.stderr, "reconfigure"):
    sys.stderr.reconfigure(encoding="utf-8")


def _ps_literal(value: pathlib.Path | str) -> str:
    return str(value).replace("'", "''")


def _run_powershell(command: str) -> subprocess.CompletedProcess:
    encoded = base64.b64encode(
        ("[Console]::OutputEncoding = [System.Text.Encoding]::UTF8; " + command).encode("utf-16le")
    ).decode("ascii")
    return subprocess.run(
        [
            "C:\\WINDOWS\\System32\\WindowsPowerShell\\v1.0\\powershell.exe",
            "-EncodedCommand",
            encoded,
        ],
        capture_output=True,
        check=False,
    )


def _decode_output(data: bytes) -> str:
    for encoding in ("utf-8", "cp949", sys.getdefaultencoding()):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            continue
    return data.decode("utf-8", errors="ignore")


def _log(step: str, message: str, color: str = Fore.CYAN) -> None:
    print(color + f"[ESP32][{step}] {message}", flush=True)


def _get_gdrive_dir() -> pathlib.Path:
    project_config = env.GetProjectConfig()
    gdrive_dir = project_config.get("common", "custom_gdrive_copy_dir", "")
    if not gdrive_dir:
        raise RuntimeError("[ESP32][설정] platformio.ini에 custom_gdrive_copy_dir 설정이 없습니다.")
    return pathlib.Path(gdrive_dir)


def _sanitize_version(version_text: str) -> str:
    sanitized = "".join(ch for ch in version_text.strip() if ch.isdigit())
    if not sanitized:
        raise RuntimeError("[ESP32][버전] 버전 번호에서 숫자를 찾을 수 없습니다. 예: 24")
    return sanitized


def _find_current_gdrive_version(gdrive_dir: pathlib.Path) -> str | None:
    version_pattern = re.compile(r"^v(\d+)_esp8685_tasmota\.bin$", re.IGNORECASE)
    latest_version = None
    latest_mtime = None

    try:
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
    except PermissionError:
        pass

    escaped_dir = _ps_literal(gdrive_dir)
    result = _run_powershell(
        f"Get-ChildItem -LiteralPath '{escaped_dir}' -File | "
        "Select-Object Name, LastWriteTime | ConvertTo-Json -Compress"
    )
    stdout_text = _decode_output(result.stdout)
    if result.returncode != 0 or not stdout_text.strip():
        return None

    try:
        items = json.loads(stdout_text)
    except Exception:
        return None

    if isinstance(items, dict):
        items = [items]

    for item in items:
        match = version_pattern.match(item.get("Name", ""))
        if not match:
            continue
        entry_mtime = str(item.get("LastWriteTime", ""))
        if latest_mtime is None or entry_mtime > latest_mtime:
            latest_mtime = entry_mtime
            latest_version = match.group(1)

    return latest_version


def _copy_file(source_bin: pathlib.Path, destination_path: pathlib.Path) -> None:
    try:
        shutil.copyfile(source_bin, destination_path)
        return
    except PermissionError:
        pass

    escaped_destination = _ps_literal(destination_path)
    escaped_source = _ps_literal(source_bin)
    result = _run_powershell(
        f"$dst = '{escaped_destination}'; "
        "$dir = Split-Path -Parent $dst; "
        "New-Item -ItemType Directory -Force -Path $dir | Out-Null; "
        f"Copy-Item -LiteralPath '{escaped_source}' -Destination $dst -Force"
    )
    if result.returncode != 0:
        raise subprocess.CalledProcessError(result.returncode, "Copy-Item")


def _validate_newer_version(version_text: str, current_version: str | None) -> str:
    sanitized = _sanitize_version(version_text)
    if current_version is not None and int(sanitized) <= int(current_version):
        raise RuntimeError(
            f"[ESP32][버전] 새 버전은 현재 구글드라이브 버전(v{current_version})보다 커야 합니다."
        )
    return sanitized


def _prompt_version_text(current_version: str | None) -> str | None:
    if not sys.stdin or not sys.stdin.isatty():
        _log("버전", "대화형 입력을 받을 수 없는 환경이라 버전 입력을 건너뜁니다.", Fore.YELLOW)
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
        _log("버전", f"이미 선택한 버전을 재사용합니다: v{_cached_version_text}")
        return _cached_version_text

    env_version = os.environ.get("GDRIVE_VERSION", "").strip()
    if env_version:
        _cached_version_text = _sanitize_version(env_version)
        _log("버전", f"환경변수에서 받은 버전을 사용합니다: v{_cached_version_text}")
        return _cached_version_text

    gdrive_dir = _get_gdrive_dir()
    current_version = _find_current_gdrive_version(gdrive_dir)

    _log("버전", "현재 구글드라이브 최신 버전을 확인합니다")
    version_text = _prompt_version_text(current_version)
    if not version_text:
        return ""

    _cached_version_text = _validate_newer_version(version_text, current_version)
    _log("버전", f"선택한 버전: v{_cached_version_text}")
    return _cached_version_text


def _copy_versioned_firmware(source, target, env):
    source_bin = pathlib.Path(tasmotapiolib.get_final_bin_path(env))
    if not source_bin.is_file():
        _log("산출물", f"최종 바이너리 파일을 찾지 못했습니다: {source_bin}", Fore.YELLOW)
        return

    _log("산출물", f"최종 바이너리 경로: {source_bin}")
    version_text = _get_version_text()
    if not version_text:
        return

    destination_name = f"v{version_text}_{OUTPUT_BASENAME}"
    gdrive_dir = _get_gdrive_dir()
    destination_path = gdrive_dir / destination_name

    if not gdrive_dir.exists():
        _log("구글드라이브", f"복사 폴더가 없어 새로 만듭니다: {gdrive_dir}", Fore.YELLOW)
        gdrive_dir.mkdir(parents=True, exist_ok=True)

    _log("구글드라이브", f"복사 대상 경로: {destination_path}")
    _copy_file(source_bin, destination_path)
    _log("구글드라이브", f"복사 완료: {destination_path}", Fore.GREEN)
    _log("배포", "서버 업로드는 생략하고 구글드라이브 복사까지만 수행했습니다.", Fore.GREEN)


copy_action = env.Action(_copy_versioned_firmware)
copy_action.strfunction = lambda target, source, env: ""
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_action)
