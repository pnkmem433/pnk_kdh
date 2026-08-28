Import("env")

import os
import shutil
import subprocess
import pathlib
import gzip
import sys
from colorama import Fore

# requests 라이브러리 자동 설치 로직
try:
    import requests
except ImportError:
    print("[Deploy] requests module not found. Installing via pip...")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "requests"])
        import requests
    except Exception as e:
        raise RuntimeError(f"[Deploy] Failed to install requests: {e}")


def _get_config(key, default=""):
    try:
        return env.GetProjectConfig().get("common", key, default).strip()
    except:
        return default


def _get_version():
    # Tasmota는 override 값만 사용
    return _get_config("custom_firmware_version", "1")


def post_build_action(source, target, env):
    source_bin = str(target[0])

    if not source_bin.endswith(".bin"):
        source_bin = source_bin.replace(".elf", ".bin")

    if not os.path.exists(source_bin):
        print(Fore.RED + f"\n[Deploy] Error: Source bin not found: {source_bin}")
        return

    chip_type = _get_config("custom_server_chip_type", "esp8685").lower()
    family = _get_config("custom_server_firmware_family", "tasmota").lower()
    version = _get_version()
    project_id = int(_get_config("custom_server_project_id", "10"))

    # 실제 파일명 규칙
    base_name = f"{chip_type}_{family}_lite.bin"
    versioned_name = f"v{version}_{base_name}"
    versioned_gz = f"{versioned_name}.gz"

    print(Fore.GREEN + f"\n[Deploy] Starting Deployment for {versioned_name}")

    # ------------------------------------------------------------------
    # 1. gzip 생성
    # ------------------------------------------------------------------
    source_gz = source_bin + ".gz"
    with open(source_bin, "rb") as src, gzip.open(source_gz, "wb", compresslevel=9) as dst:
        shutil.copyfileobj(src, dst)

    # ------------------------------------------------------------------
    # 2. 로컬 복사
    # ------------------------------------------------------------------
    output_dir = pathlib.Path(env.subst("$PROJECT_DIR")) / "build_output" / "firmware"
    output_dir.mkdir(parents=True, exist_ok=True)
    local_bin = output_dir / versioned_name
    local_gz = output_dir / versioned_gz

    shutil.copyfile(source_bin, local_bin)
    shutil.copyfile(source_gz, local_gz)
    print(Fore.GREEN + f"[Deploy] 1. Local copied -> {local_bin}")

    # ------------------------------------------------------------------
    # 3. Swagger 등록 (Try/Except)
    # ------------------------------------------------------------------
    enable_swagger = _get_config("custom_swagger_enable", "1")
    if enable_swagger not in ("0", "false", "False", "no", "NO"):
        print(Fore.BLUE + f"[Deploy] 2. Uploading to Swagger...")
        try:
            base = _get_config("custom_swagger_base_url", "http://gym907-0001.iptime.org:3315").rstrip("/")
            login_url = f"{base}/auth/login"
            upload_url = f"{base}/versions/create"
            user = _get_config("custom_swagger_user", "admin")
            password = _get_config("custom_swagger_password", "admin1234")

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
            print(f"[Deploy] Swagger 로그인 성공", flush=True)

            headers = {"Authorization": f"Bearer {token}"}
            payload = {
                "projectId": str(project_id),
                "versionNumber": str(version),
                "versionName": f"v{version}",
                "chipType": chip_type,
                "firmwareFamily": family,
            }
            with open(local_bin, "rb") as handle:
                files = {
                    "binFile": (local_bin.name, handle, "application/octet-stream"),
                }
                up = requests.post(
                    upload_url,
                    data=payload,
                    files=files,
                    headers=headers,
                    timeout=120,
                )
            if up.status_code == 201:
                print(f"[Deploy] Swagger 등록 성공: v{version} ({chip_type} / {family})", flush=True)
                print(Fore.GREEN + "[Swagger] 등록 성공")
            else:
                raise RuntimeError(f"Swagger 등록 실패: HTTP {up.status_code} {up.text}")
        except Exception as exc:
            print(Fore.RED + f"[Swagger] 등록 실패: {exc}")

    # ------------------------------------------------------------------
    # 4. Remote upload + latest symlink (Unchanged pnkslabserver block)
    # ------------------------------------------------------------------
    host = _get_config("custom_remote_ota_host")
    if host:
        try:
            port = int(_get_config("custom_remote_ota_port", "22"))
            user = _get_config("custom_remote_ota_user")
            password = _get_config("custom_remote_ota_password")
            r_dir = _get_config("custom_remote_ota_dir")
            r_subdir = _get_config("custom_remote_ota_subdir")
            l_script = _get_config("custom_remote_ota_link_script")

            try:
                import paramiko
            except ImportError:
                import sys
                subprocess.check_call([sys.executable, "-m", "pip", "install", "paramiko"])
                import paramiko

            ssh = paramiko.SSHClient()
            ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            ssh.connect(host, port=port, username=user, password=password)

            remote_full_dir = f"{r_dir.rstrip('/')}/{r_subdir}"
            ssh.exec_command(f"mkdir -p {remote_full_dir}")

            sftp = ssh.open_sftp()
            sftp.put(str(local_bin), f"{remote_full_dir}/{versioned_name}")
            sftp.put(str(local_gz), f"{remote_full_dir}/{versioned_gz}")
            sftp.close()

            print(Fore.GREEN + f"[Deploy] 4. Remote uploaded -> {remote_full_dir}/{versioned_name}")

            if l_script:
                print(Fore.CYAN + "[Deploy] 5. Updating latest symlink...")

                targets = [
                    (base_name, f"v*_{base_name}"),
                    (f"{base_name}.gz", f"v*_{base_name}.gz"),
                ]

                for link_name, pattern in targets:
                    cmd = (
                        f"python3 {l_script} "
                        f"--root {r_dir} "
                        f"--subdir {r_subdir} "
                        f"--link-name {link_name} "
                        f"--pattern '{pattern}'"
                    )

                    stdin, stdout, stderr = ssh.exec_command(cmd)
                    err = stderr.read().decode().strip()

                    if err:
                        print(Fore.RED + err)

                print(Fore.GREEN + f"[Deploy] 5. Symlink updated -> {base_name}")

            ssh.close()
            print(Fore.GREEN + "[pnkslabserver] 업로드 성공")

        except Exception as e:
            print(Fore.RED + f"[pnkslabserver] 업로드 실패: {e}")

    print(Fore.GREEN + "[Deploy] All deployment steps completed!\n")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", post_build_action)
