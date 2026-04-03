Import("env")

import pathlib
import re
import shutil
import subprocess
from colorama import Fore

import tasmotapiolib


# Local Google Drive folder used to archive every versioned OTA artifact.
GDRIVE_DIR = pathlib.Path(env.GetProjectOption("custom_gdrive_copy_dir"))

# Remote server connection details.
SSH_TARGET = "gha-runner@192.168.0.15"
SSH_REMOTE_DIR = "/ota/tasmota"
SSH_UPDATE_SCRIPT = "/ota/tasmota/update_latest.py"

# Base firmware name. The versioned artifact name becomes:
#   v<version>_tasmota-smartplug.bin.gz
BASE_FILENAME = "tasmota-smartplug.bin.gz"
VERSIONED_PATTERN = re.compile(rf"^v(\d+)_{re.escape(BASE_FILENAME)}$")

# Cache the chosen versioned artifact name so both post actions
# (Google Drive copy + server upload) use the exact same filename.
_cached_versioned_name = None


def _find_latest_version():
    """
    Scan the local archive folder and return the highest known version.

    Returns:
        tuple[int | None, str | None]:
            (latest_version_number, latest_filename)
    """
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
    """
    Ask the user for the next OTA version number once per build.

    The function refuses versions that are not numeric or are not strictly
    greater than the latest archived version. The chosen filename is cached
    so later steps reuse it without asking again.
    """
    global _cached_versioned_name
    if _cached_versioned_name:
        return _cached_versioned_name

    print(Fore.CYAN + "[0/3] OTA 버전 입력 단계 시작")
    latest_version, latest_name = _find_latest_version()
    if latest_version is None:
        print(Fore.CYAN + "서버의 최신버전 : 없음")
    else:
        print(Fore.CYAN + f"서버의 최신버전 : v{latest_version} ({latest_name})")

    print(Fore.CYAN + "업로드할 버전을 직접 입력해야 합니다.")
    version_number = input("업로드할 버전 : ").strip()
    if not version_number:
        raise RuntimeError("OTA 버전 번호를 입력해야 합니다")

    # Keep only digits so accidental spaces or letters do not leak
    # into the final filename.
    safe_version = "".join(ch for ch in version_number if ch.isdigit())
    if not safe_version:
        raise RuntimeError(f"잘못된 OTA 버전 번호입니다: {version_number}")

    version_value = int(safe_version)
    if (latest_version is not None) and (version_value <= latest_version):
        raise RuntimeError(
            f"업로드할 버전은 현재 최신 버전 v{latest_version}보다 커야 합니다"
        )

    _cached_versioned_name = f"v{version_value}_{BASE_FILENAME}"
    print(Fore.CYAN + f"업로드 파일 이름 : {_cached_versioned_name}")
    print(Fore.GREEN + "[0/3] OTA 버전 입력 단계 완료")
    return _cached_versioned_name


def _copy_gzip_firmware(source, target, env):
    """
    Copy the freshly built gzip firmware into the version archive folder.

    This keeps a local version history before anything is uploaded to the server.
    """
    source_bin = pathlib.Path(tasmotapiolib.get_final_bin_path(env))
    source_gz = source_bin.with_suffix(".bin.gz")
    print(Fore.CYAN + "[1/3] 구글드라이브 복사 단계 시작")
    if not source_gz.is_file():
        print(Fore.YELLOW + f"gzip 펌웨어를 찾지 못해 구글드라이브 복사를 건너뜁니다: {source_gz}")
        return

    destination_name = _get_versioned_name()
    GDRIVE_DIR.mkdir(parents=True, exist_ok=True)
    destination_file = GDRIVE_DIR / destination_name

    # Overwrite the same version file if it already exists.
    shutil.copyfile(source_gz, destination_file)
    print(Fore.GREEN + f"구글드라이브 복사 완료: {destination_file}")
    print(Fore.GREEN + "[1/3] 구글드라이브 복사 단계 완료")


def _upload_gzip_firmware(source, target, env):
    """
    Upload the same versioned gzip artifact to the OTA server and then ask
    the server-side helper script to refresh the latest symlink.
    """
    source_bin = pathlib.Path(tasmotapiolib.get_final_bin_path(env))
    source_gz = source_bin.with_suffix(".bin.gz")
    print(Fore.CYAN + "[2/3] 서버 업로드 단계 시작")
    if not source_gz.is_file():
        print(Fore.YELLOW + f"gzip 펌웨어를 찾지 못해 서버 업로드를 건너뜁니다: {source_gz}")
        return

    remote_name = _get_versioned_name()

    # Ensure the remote OTA folder exists before uploading.
    mkdir_cmd = [
        "ssh",
        "-o",
        "StrictHostKeyChecking=accept-new",
        SSH_TARGET,
        f"mkdir -p '{SSH_REMOTE_DIR}'",
    ]

    # Upload the exact versioned file to the server archive folder.
    copy_cmd = [
        "scp",
        "-o",
        "StrictHostKeyChecking=accept-new",
        str(source_gz),
        f"{SSH_TARGET}:{SSH_REMOTE_DIR}/{remote_name}",
    ]

    # After upload, let the server decide which version is the newest and
    # update the stable symlink name on the server side.
    update_cmd = [
        "ssh",
        "-o",
        "StrictHostKeyChecking=accept-new",
        SSH_TARGET,
        "python3",
        SSH_UPDATE_SCRIPT,
    ]

    print(Fore.CYAN + f"서버 업로드 대상: {SSH_TARGET}:{SSH_REMOTE_DIR}/{remote_name}")
    subprocess.run(mkdir_cmd, check=True)
    subprocess.run(copy_cmd, check=True)
    print(Fore.GREEN + f"서버 업로드 완료: {SSH_TARGET}:{SSH_REMOTE_DIR}/{remote_name}")
    print(Fore.GREEN + "[2/3] 서버 업로드 단계 완료")

    print(Fore.CYAN + "[3/3] 서버 최신 링크 갱신 단계 시작")
    print(Fore.CYAN + "서버 최신 링크 갱신 스크립트 호출중...")
    print(Fore.CYAN + f"서버 스크립트 실행: {SSH_UPDATE_SCRIPT}")
    subprocess.run(update_cmd, check=True)
    print(Fore.GREEN + "[3/3] 서버 최신 링크 갱신 단계 완료")
    print(Fore.GREEN + "서버 최신 심볼릭 링크 갱신 완료")


# Post action 1: archive the gzip artifact locally after firmware.bin is produced.
copy_action = env.Action(_copy_gzip_firmware)
copy_action.strfunction = lambda target, source, env: ""
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_action)

# Post action 2: upload the same versioned artifact to the server and refresh
# the server-side stable symlink.
upload_action = env.Action(_upload_gzip_firmware)
upload_action.strfunction = lambda target, source, env: ""
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", upload_action)
