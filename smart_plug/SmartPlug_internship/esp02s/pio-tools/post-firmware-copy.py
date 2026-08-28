Import("env")
import gzip, os, pathlib, re, shlex, shutil, subprocess, sys
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

# --- 설정값 읽기 (GetProjectOption 우선 사용) ---
def _get_option(key, default=""):
    try:
        # .ini 파일의 [env:esp02s] 설정을 가장 먼저 읽어옵니다.
        return str(env.GetProjectOption(key)).strip()
    except:
        # 실패 시 [common] 섹션이나 기본값 사용
        try:
            return env.GetProjectConfig().get("common", key, default).strip()
        except:
            return default

def _basename():
    return _get_option("custom_output_basename", "esp02s_custom.bin")


def _upload_swagger(bin_path: pathlib.Path, version: str) -> None:
    """공용 OTA 등록 API 서버(NestJS) /versions/create 에 펌웨어 등록."""
    base = _get_option("custom_swagger_base_url", "http://gym907-0001.iptime.org:3315").rstrip("/")
    login_url = f"{base}/auth/login"
    upload_url = f"{base}/versions/create"
    user = _get_option("custom_swagger_user", "admin")
    password = _get_option("custom_swagger_password", "admin1234")
    project_id = _get_option("custom_swagger_project_id", "10")
    chip = _get_option("custom_swagger_chip_type", "esp02s")
    family = _get_option("custom_swagger_firmware_family", "custom")

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
        print(f"{Fore.BLUE}[POST] Swagger 로그인 성공")

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
            print(f"{Fore.BLUE}[POST] Swagger 등록 성공: v{version} ({chip} / {family})")
            print(f"{Fore.GREEN}[Swagger] 등록 성공")
        else:
            raise RuntimeError(f"Swagger 등록 실패: HTTP {up.status_code} {up.text}")
    except Exception as exc:
        print(f"{Fore.RED}[Swagger] 등록 실패: {exc}")
        raise exc


# --- AppConfig.h에서 버전 읽기 ---
def _get_version():
    try:
        app_config = (pathlib.Path(env.subst("$PROJECT_DIR")) / "include" / "AppConfig.h").read_text(encoding="utf-8")
        major = int(re.search(rf"^#define\s+FW_VER_MAJOR\s+(\d+)", app_config, re.M).group(1))
        minor = int(re.search(rf"^#define\s+FW_VER_MINOR\s+(\d+)", app_config, re.M).group(1))
        patch = int(re.search(rf"^#define\s+FW_VER_PATCH\s+(\d+)", app_config, re.M).group(1))
        return str((major * 100) + (minor * 10) + patch)
    except:
        return "1"

# --- 메인 실행 함수 ---
def _run_post_firmware_copy(source, target, env):
    # 0. 빌드 파일 확인
    firmware_bin = pathlib.Path(env.subst("$BUILD_DIR")) / "firmware.bin"
    if not firmware_bin.is_file(): return

    # 1. 정보 정의
    version = _get_version()
    base_name = _basename()
    versioned_name = f"v{version}_{base_name}" # v300_esp02s_custom.bin

    print(f"\n{Fore.GREEN}[POST] Starting Custom Deployment: {versioned_name}")

    # 2. 로컬 준비 (BIN 복사 및 GZ 압축)
    output_dir = pathlib.Path(env.subst("$PROJECT_DIR")) / "build_output" / "firmware"
    output_dir.mkdir(parents=True, exist_ok=True)
    local_bin = output_dir / versioned_name
    local_gz = output_dir / f"{versioned_name}.gz"

    shutil.copyfile(firmware_bin, local_bin)
    with firmware_bin.open("rb") as f_in, gzip.open(local_gz, "wb", compresslevel=9) as f_out:
        shutil.copyfileobj(f_in, f_out)

    print(f"{Fore.CYAN}[POST] 1. Local Copied (BIN/GZ)")

    # 2. 로컬 Swagger(OTA 대시보드)에 버전 등록
    if _get_option("custom_swagger_enable", "1") not in ("0", "false", "False", "no", "NO"):
        print(f"{Fore.BLUE}[POST] 2. Registering on Swagger (local OTA server)...")
        try:
            _upload_swagger(local_bin, version)
        except Exception as exc:
            pass

    # 3. 원격 서버 업로드 및 심볼릭 링크 갱신
    host = _get_option("custom_remote_ota_host")
    if host:
        try:
            r_dir = _get_option("custom_remote_ota_dir")
            r_subdir = _get_option("custom_remote_ota_subdir", "custom")
            pw = _get_option("custom_remote_ota_password")
            user = _get_option("custom_remote_ota_user")
            port = _get_option("custom_remote_ota_port", "23")
            l_script = _get_option(
                "custom_remote_ota_link_script",
                "/home/pnkslabserver/tasmota-firmware/common/update_latest_links.py",
            )

            pscp = shutil.which("pscp") or r"C:\Program Files\PuTTY\pscp.exe"
            plink = shutil.which("plink") or r"C:\Program Files\PuTTY\plink.exe"

            remote_full_path = f"{r_dir.rstrip('/')}/{r_subdir}"
            remote_dest = f"{user}@{host}:{remote_full_path}/"

            # A. 서버 폴더 확인 및 생성 (mkdir -p)
            subprocess.run([plink, "-batch", "-ssh", "-P", port, "-pw", pw, f"{user}@{host}", f"mkdir -p {shlex.quote(remote_full_path)}"], check=True)

            # B. 파일 전송 (BIN & GZ)
            print(f"{Fore.YELLOW}[POST] 3. Uploading to Server: {r_subdir}")
            subprocess.run([pscp, "-batch", "-P", port, "-pw", pw, str(local_bin), remote_dest + versioned_name], check=True)
            subprocess.run([pscp, "-batch", "-P", port, "-pw", pw, str(local_gz), remote_dest + versioned_name + ".gz"], check=True)

            # C. 심볼릭 링크 업데이트 (BIN / GZ 각각 수행)
            print(f"{Fore.YELLOW}[POST] 4. Updating Symlinks...")
            for ext in ["", ".gz"]:
                link_name = base_name + ext
                cmd = f"python3 {shlex.quote(l_script)} --root {shlex.quote(r_dir)} --subdir {shlex.quote(r_subdir)} --link-name {shlex.quote(link_name)} --pattern {shlex.quote('v*_' + link_name)}"
                subprocess.run([plink, "-batch", "-ssh", "-P", port, "-pw", pw, f"{user}@{host}", cmd], check=True)
            print(f"{Fore.GREEN}[pnkslabserver] 업로드 성공")
        except Exception as e:
            print(f"{Fore.RED}[pnkslabserver] 업로드 실패: {e}")

    print(f"{Fore.GREEN}[POST] Deployment Complete for v{version}!\n")

# 빌드 완료 후 액션 등록
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _run_post_firmware_copy)
