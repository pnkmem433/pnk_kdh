Import("env")

import os
import pathlib
import re
import shlex
import shutil
import sqlite3
import subprocess

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
    return _project_config().get("common", name, default).strip()


def _require_config_value(name: str) -> str:
    value = _config_value(name)
    if not value:
        raise RuntimeError(f"[POST] {name} is missing in platformio.ini")
    return value


def _gdrive_dir() -> pathlib.Path:
    return pathlib.Path(_require_config_value("custom_gdrive_copy_dir"))


def _server_upload_dir() -> pathlib.Path:
    return pathlib.Path(_require_config_value("custom_server_upload_dir"))


def _server_db_path() -> pathlib.Path:
    return _server_upload_dir().parent / "local-ota.sqlite"


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


def _find_current_version(gdrive_dir: pathlib.Path, basename: str) -> str | None:
    pattern = re.compile(rf"^v(\d+)_{re.escape(basename)}$", re.IGNORECASE)
    latest_version = None
    latest_mtime = None
    if not gdrive_dir.exists():
        return None

    for entry in gdrive_dir.iterdir():
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

    current_version = _find_current_version(_gdrive_dir(), _basename())
    if current_version is not None and int(_cached_version) < int(current_version):
        raise RuntimeError(
            f"[POST] AppConfig version v{_cached_version} is older than current GDrive version v{current_version}."
        )

    os.environ["GDRIVE_VERSION"] = _cached_version
    print(f"[POST] Derived from AppConfig.h => v{_cached_version}", flush=True)
    return _cached_version


def _project_id() -> int:
    return _read_macro_int(_app_config_text(), "OTA_PROJECT_ID")


def _chip_type() -> str:
    return _read_macro_string(_board_config_text(), "CHIP_TYPE").strip().lower()


def _firmware_family() -> str:
    return _read_macro_string(_app_config_text(), "CURRENT_FIRMWARE_FAMILY").strip().lower()


def _copy_file(
    bin_source: pathlib.Path,
    target_dir: pathlib.Path,
    base_name: str,
    label: str,
) -> pathlib.Path:
    target_dir.mkdir(parents=True, exist_ok=True)
    target_bin = target_dir / base_name
    shutil.copyfile(bin_source, target_bin)
    print(f"[POST] {label} BIN -> {target_bin}", flush=True)
    return target_bin


def _cleanup_legacy_server_latest_files() -> None:
    basename = _basename()
    upload_dir = _server_upload_dir()
    for path in (upload_dir / basename, upload_dir / f"{basename}.gz"):
        if path.exists():
            path.unlink()
            print(f"[POST] Removed legacy server file -> {path}", flush=True)


def _register_server_version(version: int, version_name: str, server_bin_path: pathlib.Path) -> None:
    db_path = _server_db_path()
    if not db_path.is_file():
        raise RuntimeError(f"[POST] OTA server DB not found: {db_path}")

    project_id = _project_id()
    chip_type = _chip_type()
    firmware_family = _firmware_family()

    conn = sqlite3.connect(str(db_path))
    try:
        cur = conn.cursor()
        project = cur.execute("SELECT id FROM project WHERE id = ?", (project_id,)).fetchone()
        if not project:
            raise RuntimeError(f"[POST] Project {project_id} not found in OTA server DB")

        existing = cur.execute(
            "SELECT id FROM project_version WHERE project = ? AND chipType = ? AND firmwareFamily = ? AND versionNumber = ? ORDER BY id DESC LIMIT 1",
            (project_id, chip_type, firmware_family, version),
        ).fetchone()

        if existing:
            row_id = int(existing[0])
            cur.execute(
                "UPDATE project_version SET versionName = ?, chipType = ?, firmwareFamily = ?, isActive = 1, binFile = ?, project = ? WHERE id = ?",
                (version_name, chip_type, firmware_family, str(server_bin_path), project_id, row_id),
            )
            print(f"[POST] Updated OTA DB row id={row_id} for {chip_type}/{firmware_family} v{version}", flush=True)
        else:
            cur.execute(
                "INSERT INTO project_version (createdAt, versionNumber, versionName, chipType, firmwareFamily, isActive, binFile, project) VALUES (datetime('now'), ?, ?, ?, ?, 1, ?, ?)",
                (version, version_name, chip_type, firmware_family, str(server_bin_path), project_id),
            )
            row_id = int(cur.lastrowid)
            print(f"[POST] Inserted OTA DB row id={row_id} for {chip_type}/{firmware_family} v{version}", flush=True)

        cur.execute(
            "UPDATE project_version SET isActive = 0 WHERE project = ? AND chipType = ? AND firmwareFamily = ? AND id <> ?",
            (project_id, chip_type, firmware_family, row_id),
        )
        conn.commit()
    finally:
        conn.close()


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

    shutil.copyfile(firmware_bin, local_bin)

    print(f"[POST] BIN: {local_bin}", flush=True)

    _copy_file(local_bin, _gdrive_dir(), local_bin.name, "GDrive")
    server_bin = _copy_file(local_bin, _server_upload_dir(), local_bin.name, "Server versioned")
    _cleanup_legacy_server_latest_files()
    _register_server_version(version, f"v{version_text}_{_chip_type()}_{_firmware_family()}", server_bin)
    _upload_remote_versioned_file(local_bin)
    _update_remote_latest_symlink()


post_action = env.Action(_run_post_firmware_copy)
post_action.strfunction = lambda target, source, env: ""
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", post_action)
