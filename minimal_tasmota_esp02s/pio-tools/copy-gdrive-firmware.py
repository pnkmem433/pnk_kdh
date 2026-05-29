Import("env")

import os
import shutil
import subprocess
import shlex
import time
import requests
import pathlib
from colorama import Fore

import tasmotapiolib


# =========================
# Swagger(Local OTA Server)
# =========================
BASE_URL = "http://192.168.0.84:3004"
LOGIN_ENDPOINT = f"{BASE_URL}/auth/login"
UPLOAD_ENDPOINT = f"{BASE_URL}/versions/create"

USER_CREDENTIALS = {
    "id": "admin",
    "password": "admin1234",
}


def get_access_token():
    try:
        response = requests.post(
            LOGIN_ENDPOINT,
            json=USER_CREDENTIALS,
            timeout=10
        )
        if response.status_code in (200, 201):
            token = response.json().get("token", {}).get("access_token")
            if token:
                print(f"{Fore.BLUE}[SWAGGER] Login success")
                return token
        print(f"{Fore.RED}[SWAGGER] Login failed: {response.status_code}")
        return None
    except Exception as e:
        print(f"{Fore.RED}[SWAGGER] Login exception: {e}")
        return None


def upload_to_swagger(bin_path, version, chip_type, project_id):
    token = get_access_token()
    if not token:
        print(f"{Fore.RED}[SWAGGER] Skip upload (token unavailable)")
        return

    headers = {"Authorization": f"Bearer {token}"}
    payload = {
        "projectId": str(project_id),
        "versionNumber": str(version),
        "versionName": f"v{version}",
        "chipType": chip_type,
        "firmwareFamily": "tasmota",
    }

    try:
        with open(bin_path, "rb") as f:
            files = {
                "binFile": (os.path.basename(bin_path), f, "application/octet-stream")
            }
            response = requests.post(
                UPLOAD_ENDPOINT,
                data=payload,
                files=files,
                headers=headers,
                timeout=30
            )
        if response.status_code == 201:
            print(f"{Fore.BLUE}[SWAGGER] Upload success: v{version}")
        else:
            print(f"{Fore.RED}[SWAGGER] Upload failed: {response.status_code} {response.text}")
    except Exception as e:
        print(f"{Fore.RED}[SWAGGER] Upload exception: {e}")


# =========================
# PlatformIO config helper
# =========================
def _get_option(key, default=""):
    try:
        # 현재 활성화된 [env:tasmota-smartplug] 섹션의 설정을 우선적으로 가져옵니다.
        return env.GetProjectOption(key)
    except Exception:
        return default


def _resolve_bin_and_gz_paths(target, env):
    """
    gzip-firmware.py는 tasmotapiolib.get_final_bin_path 기준으로 <env>.bin.gz 를 씁니다
    (예: build_output/firmware/tasmota-smartplug.bin.gz).
    .pio/build/.../firmware.bin.gz 는 생성되지 않을 수 있으므로 동일 규칙으로 찾고,
    없으면 공식 스크립트와 같은 방식으로 압축해 둡니다.
    """
    pio_bin = pathlib.Path(str(target[0]))
    final_bin = tasmotapiolib.get_final_bin_path(env)
    final_gz = final_bin.with_suffix(".bin.gz")
    pio_gz = pio_bin.with_suffix(".bin.gz")

    bin_path = final_bin if final_bin.is_file() else pio_bin
    if not bin_path.is_file():
        return None, None

    gz_path = None
    if final_gz.is_file():
        gz_path = final_gz
    elif pio_gz.is_file():
        gz_path = pio_gz
    else:
        gzip_level = int(env["ENV"].get("GZIP_LEVEL", 10))
        final_gz.parent.mkdir(parents=True, exist_ok=True)
        t0 = time.time()
        raw = bin_path.read_bytes()
        final_gz.write_bytes(tasmotapiolib.compress(raw, gzip_level))
        print(
            f"{Fore.CYAN}[Deploy] Built missing .bin.gz (same path as gzip-firmware): "
            f"{final_gz} ({len(raw)} -> {final_gz.stat().st_size} bytes, {time.time() - t0:.2f}s)"
        )
        gz_path = final_gz

    return str(bin_path), str(gz_path) if gz_path is not None else None


# =========================
# Main deploy pipeline
# =========================
def post_build_action(source, target, env):
    source_bin = str(target[0])
    if not os.path.exists(source_bin):
        return

    deploy_bin, deploy_gz = _resolve_bin_and_gz_paths(target, env)
    if not deploy_bin:
        return

    # 1. 설정 로드
    version = _get_option("custom_firmware_version", "1")
    base_name = _get_option("custom_output_basename", "esp02s_tasmota_lite.bin")
    chip_type = _get_option("custom_server_chip_type", "esp02s")
    project_id = _get_option("custom_server_project_id", "10")
    
    # 버전 이름 정의 (v8_esp02s_tasmota_lite.bin / .bin.gz)
    versioned_name = f"v{version}_{base_name}"
    versioned_gz = f"{versioned_name}.gz"
    
    r_subdir = _get_option("custom_remote_ota_subdir", "esp02s/tasmota_lite")

    print(f"\n{Fore.GREEN}[Deploy] Master Deployment Start: {versioned_name}")
    print(f"{Fore.MAGENTA}[DEBUG] Targeting Subdir: {r_subdir}")

    # 2. GDrive 복사 (BIN & GZ 한 쌍)
    gdrive_dir = _get_option("custom_gdrive_copy_dir")
    if gdrive_dir:
        os.makedirs(gdrive_dir, exist_ok=True)
        shutil.copyfile(deploy_bin, os.path.join(gdrive_dir, versioned_name))
        if deploy_gz and os.path.exists(deploy_gz):
            shutil.copyfile(deploy_gz, os.path.join(gdrive_dir, versioned_gz))
            print(f"{Fore.CYAN}[Deploy] 1. GDrive Copied (BIN & GZ)")
        else:
            print(f"{Fore.YELLOW}[Deploy] 1. GDrive Warning: .bin.gz not available after resolve/build")

    # 3. Swagger upload (기본 바이너리만 업로드)
    print(f"{Fore.BLUE}[Deploy] 2. Uploading to Swagger...")
    upload_to_swagger(deploy_bin, version, chip_type, project_id)

    # 4. 원격 서버 배포 및 심볼릭 링크 갱신
    host = _get_option("custom_remote_ota_host")
    if host:
        r_dir = _get_option("custom_remote_ota_dir")
        pw = _get_option("custom_remote_ota_password")
        user = _get_option("custom_remote_ota_user")
        port = _get_option("custom_remote_ota_port", "23")
        l_script = _get_option("custom_remote_ota_link_script", "/home/pnkslabserver/tasmota-firmware/common/update_latest_links.py")

        pscp = shutil.which("pscp") or r"C:\Program Files\PuTTY\pscp.exe"
        plink = shutil.which("plink") or r"C:\Program Files\PuTTY\plink.exe"

        remote_full_path = f"{r_dir.rstrip('/')}/{r_subdir}"
        remote_dest = f"{user}@{host}:{remote_full_path}/"

        print(f"{Fore.YELLOW}[Deploy] 3. Uploading to Server: {r_subdir}")
        
        # A. 서버 폴더 존재 확인 및 생성
        subprocess.run([plink, "-batch", "-ssh", "-P", port, "-pw", pw, f"{user}@{host}", f"mkdir -p {shlex.quote(remote_full_path)}"], check=True)

        # B. 파일 전송 (BIN & GZ)
        subprocess.run([pscp, "-batch", "-P", port, "-pw", pw, deploy_bin, remote_dest + versioned_name], check=True)
        if deploy_gz and os.path.exists(deploy_gz):
            subprocess.run([pscp, "-batch", "-P", port, "-pw", pw, deploy_gz, remote_dest + versioned_gz], check=True)

        # C. 심볼릭 링크 업데이트 (BIN / GZ 각각 수행)
        if plink:
            print(f"{Fore.YELLOW}[Deploy] 4. Running Remote Link Update...")
            for ext in ["", ".gz"]:
                link_name = base_name + ext
                pattern = f"v*_{link_name}"
                cmd = (
                    f"python3 {shlex.quote(l_script)} "
                    f"--root {shlex.quote(r_dir)} "
                    f"--subdir {shlex.quote(r_subdir)} "
                    f"--link-name {shlex.quote(link_name)} "
                    f"--pattern {shlex.quote(pattern)}"
                )
                subprocess.run([plink, "-batch", "-ssh", "-P", port, "-pw", pw, f"{user}@{host}", cmd], check=True)

    print(f"{Fore.GREEN}[Deploy] All Steps Completed Successfully for v{version}!\n")


env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.bin",
    post_build_action
)
