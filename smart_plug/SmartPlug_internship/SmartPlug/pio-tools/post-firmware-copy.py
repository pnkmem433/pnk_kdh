Import("env")

import os
import pathlib
import re
import shlex
import shutil
import subprocess
import sys
from colorama import Fore

# requests 라이브러리 자동 설치 로직
try:
    import requests
except ImportError:
    print("[POST] requests module not found. Installing via pip...")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "requests"])
        import requests
    except Exception as e:
        raise RuntimeError(f"[POST] Failed to install requests: {e}")

_cached_version = None


def _project_dir() -> pathlib.Path:
    return pathlib.Path(env.subst("$PROJECT_DIR"))


def _build_dir() -> pathlib.Path:
    return pathlib.Path(env.subst("$BUILD_DIR"))


def _output_dir() -> pathlib.Path:
    return _project_dir() / "build_output" / "firmware"


def _app_config_path() -> pathlib.Path:
    return _project_dir() / "include" / "AppConfig.h"


def _board_config_path() -> pathlib.Path:
    return _project_dir() / "include" / "BoardConfig.h"


def _read_macro_int(text: str, name: str) -> int:
    match = re.search(rf"^#define\s+{name}\s+(\d+)\s*$", text, re.MULTILINE)
    if not match:
        raise RuntimeError(f"[POST] {name} not found")
    return int(match.group(1))


def _read_macro_string(text: str, name: str) -> str:
    match = re.search(rf'^#define\s+{name}\s+"([^"]+)"\s*$', text, re.MULTILINE)
    if not match:
        raise RuntimeError(f"[POST] {name} not found")
    return match.group(1)


def _app_config_text() -> str:
    return _app_config_path().read_text(encoding="utf-8")


def _board_config_text() -> str:
    return _board_config_path().read_text(encoding="utf-8")


def _project_config():
    return env.GetProjectConfig()


def _config_value(name: str, default: str = "") -> str:
    try:
        return _project_config().get("common", name, default).strip()
    except Exception:
        return default


def _require_config_value(name: str) -> str:
    value = _config_value(name)
    if not value:
        raise RuntimeError(f"[POST] {name} is missing in platformio.ini")
    return value


def _basename() -> str:
    return _config_value("custom_output_basename", "esp8685_custom.bin")


def _remote_config() -> dict[str, str]:
    return {
        "host": _require_config_value("custom_remote_ota_host"),
        "port": _config_value("custom_remote_ota_port", "23") or "23",
        "user": _require_config_value("custom_remote_ota_user"),
        "password": _require_config_value("custom_remote_ota_password"),
        "directory": _require_config_value("custom_remote_ota_dir"),
        "subdir": _require_config_value("custom_remote_ota_subdir"),
        "link_script": _require_config_value("custom_remote_ota_link_script"),
    }


def _find_executable(*names: str) -> str:
    lower_names = {name.lower() for name in names}
    for name in names:
        found = shutil.which(name)
        if found:
            return found

    fallback_candidates = [
        pathlib.Path(r"C:\Program Files\PuTTY\pscp.exe"),
        pathlib.Path(r"C:\Program Files\PuTTY\plink.exe"),
    ]
    for candidate in fallback_candidates:
        if candidate.is_file() and candidate.name.lower() in lower_names:
            return str(candidate)

    raise RuntimeError(f"[POST] Executable not found: {', '.join(names)}")


def _run_process(command: list[str], label: str) -> None:
    print(f"[POST] {label} CMD -> {' '.join(command)}", flush=True)
    result = subprocess.run(
        command,
        check=False,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    if result.stdout.strip():
        print(f"[POST] {label} OUT -> {result.stdout.strip()}", flush=True)
    if result.returncode != 0:
        stderr = result.stderr.strip()
        if stderr:
            print(f"[POST] {label} ERR -> {stderr}", flush=True)
        raise RuntimeError(f"[POST] {label} failed with exit code {result.returncode}")


def _ensure_remote_directory(remote: dict[str, str]) -> None:
    plink_path = _find_executable("plink.exe", "plink")
    remote_subdir = f"{remote['directory'].rstrip('/')}/{remote['subdir']}"
    _run_process(
        [
            plink_path,
            "-batch",
            "-ssh",
            "-P",
            remote["port"],
            "-pw",
            remote["password"],
            f"{remote['user']}@{remote['host']}",
            f"mkdir -p {shlex.quote(remote_subdir)}",
        ],
        "REMOTE-MKDIR",
    )


def _upload_remote_versioned_file(versioned_bin: pathlib.Path) -> None:
    remote = _remote_config()
    _ensure_remote_directory(remote)
    pscp_path = _find_executable("pscp.exe", "pscp")
    remote_target = (
        f"{remote['user']}@{remote['host']}:"
        f"{remote['directory'].rstrip('/')}/{remote['subdir']}/{versioned_bin.name}"
    )
    _run_process(
        [
            pscp_path,
            "-batch",
            "-P",
            remote["port"],
            "-pw",
            remote["password"],
            str(versioned_bin),
            remote_target,
        ],
        "REMOTE-UPLOAD",
    )
    print(f"[POST] Remote OTA BIN -> {remote_target}", flush=True)


def _update_remote_latest_symlink() -> None:
    remote = _remote_config()
    plink_path = _find_executable("plink.exe", "plink")
    basename = _basename()
    remote_command = (
        f"python3 {shlex.quote(remote['link_script'])} "
        f"--root {shlex.quote(remote['directory'])} "
        f"--subdir {shlex.quote(remote['subdir'])} "
        f"--link-name {shlex.quote(basename)} "
        f"--pattern {shlex.quote(f'v*_{basename}')}"
    )
    _run_process(
        [
            plink_path,
            "-batch",
            "-ssh",
            "-P",
            remote["port"],
            "-pw",
            remote["password"],
            f"{remote['user']}@{remote['host']}",
            remote_command,
        ],
        "REMOTE-LINK",
    )
    print(
        f"[POST] Remote OTA symlink updated -> http://{remote['host']}/ota/tasmota/{remote['subdir']}/{basename}",
        flush=True,
    )


def _find_current_version(output_dir: pathlib.Path, basename: str) -> str | None:
    pattern = re.compile(rf"^v(\d+)_{re.escape(basename)}$", re.IGNORECASE)
    latest_version = None
    latest_mtime = None
    if not output_dir.exists():
        return None

    for entry in output_dir.iterdir():
        if not entry.is_file() or entry.suffix.lower() != ".bin":
            continue
        match = pattern.match(entry.name)
        if not match:
            continue
        mtime = entry.stat().st_mtime
        if latest_mtime is None or mtime > latest_mtime:
            latest_mtime = mtime
            latest_version = match.group(1)
    return latest_version


def _get_version() -> str:
    global _cached_version
    if _cached_version:
        return _cached_version

    app_config = _app_config_text()
    major = _read_macro_int(app_config, "FW_VER_MAJOR")
    minor = _read_macro_int(app_config, "FW_VER_MINOR")
    patch = _read_macro_int(app_config, "FW_VER_PATCH")
    _cached_version = str((major * 100) + (minor * 10) + patch)

    current_version = _find_current_version(_output_dir(), _basename())
    if current_version is not None and int(_cached_version) < int(current_version):
        raise RuntimeError(
            f"[POST] AppConfig version v{_cached_version} is older than current local version v{current_version}."
        )

    print(f"[POST] Derived from AppConfig.h => v{_cached_version}", flush=True)
    return _cached_version


def _project_id() -> int:
    return _read_macro_int(_app_config_text(), "OTA_PROJECT_ID")


def _chip_type() -> str:
    return _read_macro_string(_board_config_text(), "CHIP_TYPE").strip().lower()


def _firmware_family() -> str:
    return _read_macro_string(_app_config_text(), "CURRENT_FIRMWARE_FAMILY").strip().lower()


def _upload_swagger(bin_path: pathlib.Path, version: str) -> None:
    """공용 OTA 등록 API 서버(NestJS) /versions/create 에 펌웨어 등록."""
    base = _config_value("custom_swagger_base_url", "http://gym907-0001.iptime.org:3315").rstrip("/")
    login_url = f"{base}/auth/login"
    upload_url = f"{base}/versions/create"
    user = _config_value("custom_swagger_user", "admin")
    password = _config_value("custom_swagger_password", "admin1234")
    project_id = _project_id()
    chip = _chip_type()
    family = _firmware_family()

    try:
        login = requests.post(
            login_url,
            json={"id": user, "password": password},
            timeout=15,
        )
        if login.status_code not in (200, 201):
            raise RuntimeError(f"Swagger 로그인 실패: HTTP {login.status_code} {login.text}")
        token = login.json().get("token", {}).get("access_token")
        if not token:
            raise RuntimeError("응답에 access_token 이 없습니다.")
        print(f"[POST] Swagger 로그인 성공", flush=True)

        headers = {"Authorization": f"Bearer {token}"}
        payload = {
            "projectId": str(project_id),
            "versionNumber": str(version),
            "versionName": f"v{version}",
            "chipType": chip,
            "firmwareFamily": family,
        }
        with bin_path.open("rb") as handle:
            files = {
                "binFile": (bin_path.name, handle, "application/octet-stream"),
            }
            up = requests.post(
                upload_url,
                data=payload,
                files=files,
                headers=headers,
                timeout=120,
            )
        if up.status_code == 201:
            print(f"[POST] Swagger 등록 성공: v{version} ({chip} / {family})", flush=True)
            print(f"[Swagger] 등록 성공", flush=True)
        else:
            raise RuntimeError(f"Swagger 등록 실패: HTTP {up.status_code} {up.text}")
    except Exception as exc:
        print(f"[Swagger] 등록 실패: {exc}", flush=True)
        raise exc


def _run_post_firmware_copy(source, target, env):
    firmware_bin = _build_dir() / "firmware.bin"
    if not firmware_bin.is_file():
        print(f"[POST] firmware.bin not found: {firmware_bin}", flush=True)
        return

    version_text = _get_version()
    version = int(version_text)
    basename = _basename()
    versioned_name = f"v{version_text}_{basename}"

    output_dir = _output_dir()
    output_dir.mkdir(parents=True, exist_ok=True)
    local_bin = output_dir / versioned_name

    # 1. 로컬 복사
    shutil.copyfile(firmware_bin, local_bin)
    print(f"[POST] BIN: {local_bin}", flush=True)
    print(f"[POST] 1. Local Copied (BIN)", flush=True)

    # 2. Swagger 업로드
    enable_swagger = _config_value("custom_swagger_enable", "1")
    if enable_swagger not in ("0", "false", "False", "no", "NO"):
        print(f"[POST] 2. Registering on Swagger (local OTA server)...", flush=True)
        try:
            _upload_swagger(local_bin, version)
        except Exception as exc:
            pass

    # 3. pnkslabserver 업로드 & 심링크
    try:
        _upload_remote_versioned_file(local_bin)
    except Exception as exc:
        print(f"[pnkslabserver] 업로드 실패: {exc}", flush=True)

    try:
        _update_remote_latest_symlink()
        print(f"[pnkslabserver] 업로드 성공", flush=True)
    except Exception as exc:
        print(f"[pnkslabserver] 업로드 실패: {exc}", flush=True)

    print(f"[POST] Deployment Complete for v{version}!\n", flush=True)


post_action = env.Action(_run_post_firmware_copy)
post_action.strfunction = lambda target, source, env: ""
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", post_action)
