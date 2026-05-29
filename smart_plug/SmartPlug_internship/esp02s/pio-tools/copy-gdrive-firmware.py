Import("env")

# 배포 파이프라인 (빌드 후 자동):
#  1) GDrive: custom_gdrive_copy_dir 에 v{N}_esp02s_custom.bin(.gz)
#  2) Swagger: POST /versions/create — smartScanner.py 와 동일 API(로그인·multipart·payload)
#  3) 원격 OTA: pscp 로 bin/gz 업로드 후 update_latest_links.py 로 심볼릭 링크

import os
import shutil
import subprocess
import gzip
import shlex
import requests
import pathlib
from colorama import Fore


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


def upload_to_swagger(bin_path, version):
    token = get_access_token()
    if not token:
        print(f"{Fore.RED}[SWAGGER] Skip upload (token unavailable)")
        return

    headers = {"Authorization": f"Bearer {token}"}
    payload = {
        "projectId": "10",
        "versionNumber": str(version),
        "versionName": f"v{version}",
        "chipType": "esp02s",
        "firmwareFamily": "custom",
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
        # 현재 빌드 중인 [env:...] 섹션의 설정을 우선적으로 가져옵니다.
        val = env.GetProjectOption(key)
        return str(val).strip()
    except Exception:
        # 실패 시 .ini의 [common] 섹션이나 기본값 사용
        try:
            return env.GetProjectConfig().get("common", key, default).strip()
        except:
            return default


# =========================
# Main deploy pipeline
# =========================
def post_build_action(source, target, env):
    # 0. 빌드된 파일 경로 정의
    source_bin = str(target[0])
    if not os.path.exists(source_bin):
        return

    # 1. 설정 로드
    version = _get_option("custom_firmware_version", "1")
    base_name = "esp02s_custom.bin"
    r_subdir = _get_option("custom_remote_ota_subdir", "esp02s/custom")
    
    # 버전 이름 정의 (v7_esp02s_custom.bin / .bin.gz)
    versioned_name = f"v{version}_{base_name}"
    versioned_gz = f"{versioned_name}.gz"

    print(f"\n{Fore.GREEN}[Deploy] Master Deployment Start: {versioned_name}")
    print(f"{Fore.MAGENTA}[DEBUG] Targeting Subdir: {r_subdir}")

    # 2. Gzip 압축 파일 생성 (원본 바이너리 -> .gz)
    source_gz = source_bin + ".gz"
    with open(source_bin, "rb") as src, gzip.open(source_gz, "wb", compresslevel=9) as dst:
        shutil.copyfileobj(src, dst)
    print(f"{Fore.CYAN}[Deploy] Gzip compression complete.")

    # 3. GDrive 복사 (BIN & GZ 한 쌍)
    gdrive_dir = _get_option("custom_gdrive_copy_dir")
    if gdrive_dir:
        os.makedirs(gdrive_dir, exist_ok=True)
        shutil.copyfile(source_bin, os.path.join(gdrive_dir, versioned_name))
        shutil.copyfile(source_gz, os.path.join(gdrive_dir, versioned_gz))
        print(f"{Fore.CYAN}[Deploy] 1. GDrive Copied (BIN & GZ)")

    # 4. Swagger upload (기본 바이너리만 업로드)
    print(f"{Fore.BLUE}[Deploy] 2. Uploading to Swagger...")
    upload_to_swagger(source_bin, version)

    # 5. 원격 서버 배포 및 심볼릭 링크 갱신
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

        print(f"{Fore.YELLOW}[Deploy] 3. Uploading BIN & GZ to Server: {r_subdir}")
        
        # A. 서버 폴더 존재 확인 및 생성 (mkdir -p)
        subprocess.run([plink, "-batch", "-ssh", "-P", port, "-pw", pw, f"{user}@{host}", f"mkdir -p {shlex.quote(remote_full_path)}"], check=True)

        # B. 파일 전송 (BIN & GZ)
        subprocess.run([pscp, "-batch", "-P", port, "-pw", pw, source_bin, remote_dest + versioned_name], check=True)
        subprocess.run([pscp, "-batch", "-P", port, "-pw", pw, source_gz, remote_dest + versioned_gz], check=True)

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
