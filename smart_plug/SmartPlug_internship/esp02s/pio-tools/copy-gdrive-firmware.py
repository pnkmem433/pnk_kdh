Import("env")

# 諛고룷 ?뚯씠?꾨씪??(鍮뚮뱶 ???먮룞):
#  1) Local: copy firmware to build_output/firmware with versioned name
#  2) Swagger: POST /versions/create ??smartScanner.py ? ?숈씪 API(濡쒓렇?맞톗ultipart쨌payload)
#  3) ?먭꺽 OTA: pscp 濡?bin/gz ?낅줈????update_latest_links.py 濡??щ낵由?留곹겕

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
def get_access_token():
    base_url = _get_option("custom_swagger_base_url", "http://gym907-0001.iptime.org:3315").rstrip("/")
    try:
        response = requests.post(
            f"{base_url}/auth/login",
            json={
                "id": _get_option("custom_swagger_user", "admin"),
                "password": _get_option("custom_swagger_password", "admin1234"),
            },
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
    base_url = _get_option("custom_swagger_base_url", "http://gym907-0001.iptime.org:3315").rstrip("/")
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
                f"{base_url}/versions/create",
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
        # ?꾩옱 鍮뚮뱶 以묒씤 [env:...] ?뱀뀡???ㅼ젙???곗꽑?곸쑝濡?媛?몄샃?덈떎.
        val = env.GetProjectOption(key)
        return str(val).strip()
    except Exception:
        # ?ㅽ뙣 ??.ini??[common] ?뱀뀡?대굹 湲곕낯媛??ъ슜
        try:
            return env.GetProjectConfig().get("common", key, default).strip()
        except:
            return default


# =========================
# Main deploy pipeline
# =========================
def post_build_action(source, target, env):
    # 0. 鍮뚮뱶???뚯씪 寃쎈줈 ?뺤쓽
    source_bin = str(target[0])
    if not os.path.exists(source_bin):
        return

    # 1. ?ㅼ젙 濡쒕뱶
    version = _get_option("custom_firmware_version", "1")
    base_name = "esp02s_custom.bin"
    r_subdir = _get_option("custom_remote_ota_subdir", "esp02s/custom")
    
    # 踰꾩쟾 ?대쫫 ?뺤쓽 (v7_esp02s_custom.bin / .bin.gz)
    versioned_name = f"v{version}_{base_name}"
    versioned_gz = f"{versioned_name}.gz"

    print(f"\n{Fore.GREEN}[Deploy] Master Deployment Start: {versioned_name}")
    print(f"{Fore.MAGENTA}[DEBUG] Targeting Subdir: {r_subdir}")

    # 2. Gzip ?뺤텞 ?뚯씪 ?앹꽦 (?먮낯 諛붿씠?덈━ -> .gz)
    source_gz = source_bin + ".gz"
    with open(source_bin, "rb") as src, gzip.open(source_gz, "wb", compresslevel=9) as dst:
        shutil.copyfileobj(src, dst)
    print(f"{Fore.CYAN}[Deploy] Gzip compression complete.")

    output_dir = pathlib.Path(env.subst("$PROJECT_DIR")) / "build_output" / "firmware"
    output_dir.mkdir(parents=True, exist_ok=True)
    local_bin = output_dir / versioned_name
    local_gz = output_dir / versioned_gz
    shutil.copyfile(source_bin, local_bin)
    shutil.copyfile(source_gz, local_gz)
    print(f"{Fore.CYAN}[Deploy] 1. Local Copied (BIN & GZ)")

    # 4. Swagger upload (湲곕낯 諛붿씠?덈━留??낅줈??
    print(f"{Fore.BLUE}[Deploy] 2. Uploading to Swagger...")
    upload_to_swagger(str(local_bin), version)

    # 5. ?먭꺽 ?쒕쾭 諛고룷 諛??щ낵由?留곹겕 媛깆떊
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
        
        # A. ?쒕쾭 ?대뜑 議댁옱 ?뺤씤 諛??앹꽦 (mkdir -p)
        subprocess.run([plink, "-batch", "-ssh", "-P", port, "-pw", pw, f"{user}@{host}", f"mkdir -p {shlex.quote(remote_full_path)}"], check=True)

        # B. ?뚯씪 ?꾩넚 (BIN & GZ)
        subprocess.run([pscp, "-batch", "-P", port, "-pw", pw, str(local_bin), remote_dest + versioned_name], check=True)
        subprocess.run([pscp, "-batch", "-P", port, "-pw", pw, str(local_gz), remote_dest + versioned_gz], check=True)

        # C. ?щ낵由?留곹겕 ?낅뜲?댄듃 (BIN / GZ 媛곴컖 ?섑뻾)
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
