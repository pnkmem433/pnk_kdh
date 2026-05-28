import os
import re
import shutil

import requests

BASE_URL = "http://192.168.0.84:3004"
LOGIN_ENDPOINT = f"{BASE_URL}/auth/login"
VERSIONS_LIST_ENDPOINT = f"{BASE_URL}/versions/list"
UPLOAD_ENDPOINT = f"{BASE_URL}/versions/create"

PROJECT_ID_FIXED = 10
CHIP_TYPE = "esp8685"
FIRMWARE_FAMILY = "custom"
GDRIVE_COPY_DIR = (
    r"G:\.shortcut-targets-by-id\1eOl--DT4rr56gYI69AsP3L1Eyd-glIoh"
    r"\3D 프린터 사용방법 인수인계\촬영 사진 영상\강동현"
    r"\Smartplug 관련 파일\smartplug 펌웨어 버전 폴더\esp32\custom"
)

USER_CREDENTIALS = {
    "id": "admin",
    "password": "admin1234",
}

try:
    from SCons.Script import Import

    Import("env")
    RUNNING_IN_PIO = True
except ImportError:
    RUNNING_IN_PIO = False
    env = None


def log(step, message):
    print(f"[SMARTSCANNER][{step}] {message}")


def get_access_token():
    log("인증", f"서버 로그인 시도: {LOGIN_ENDPOINT}")
    try:
        response = requests.post(LOGIN_ENDPOINT, json=USER_CREDENTIALS, timeout=15)
        if response.status_code in (200, 201):
            token = response.json().get("token", {}).get("access_token")
            if token:
                log("인증", "JWT 토큰 발급 완료")
                return token
        log("인증", f"로그인 실패: HTTP {response.status_code} / {response.text}")
    except Exception as exc:
        log("인증", f"로그인 예외 발생: {exc}")
    return None


def extract_version_info(bin_path):
    if not os.path.exists(bin_path):
        return None, None, None

    try:
        with open(bin_path, "rb") as handle:
            content = handle.read()
        match = re.search(rb"SMARTPLUG_FW_VERSION:([\d.]+)", content)
        if not match:
            return None, None, None

        version_str = match.group(1).decode("utf-8")
        version_number = int(version_str.replace(".", ""))
        version_name = f"v{version_str}"
        return version_number, version_name, version_str
    except Exception as exc:
        log("버전", f"버전 추출 예외 발생: {exc}")
        return None, None, None


def get_latest_same_track_version(token):
    headers = {"Authorization": f"Bearer {token}"}
    payload = {"projectId": PROJECT_ID_FIXED}

    try:
        response = requests.post(
            VERSIONS_LIST_ENDPOINT,
            json=payload,
            headers=headers,
            timeout=15,
        )
        if response.status_code != 200:
            log("조회", f"버전 목록 조회 실패: HTTP {response.status_code} / {response.text}")
            return None

        items = response.json()
        same_track = [
            item
            for item in items
            if str(item.get("chipType", "")).strip() == CHIP_TYPE
            and str(item.get("firmwareFamily", "")).strip() == FIRMWARE_FAMILY
        ]

        if not same_track:
            log("조회", "같은 칩/계열의 등록 버전이 아직 없습니다.")
            return None

        latest = max(int(item.get("id", 0)) for item in same_track)
        log("조회", f"서버 최신 같은 트랙 버전: {latest}")
        return latest
    except Exception as exc:
        log("조회", f"버전 목록 조회 예외 발생: {exc}")
        return None


def ensure_newer_than_server(version_number, latest_version):
    if latest_version is None:
        return True

    if int(version_number) <= int(latest_version):
        log(
            "버전",
            f"업로드 중단: 코드 버전 {version_number} <= 서버 최신 버전 {latest_version}",
        )
        return False

    return True


def copy_to_gdrive(source_path, version_number):
    os.makedirs(GDRIVE_COPY_DIR, exist_ok=True)
    destination_name = f"v{version_number}_esp8685_custom.bin"
    destination_path = os.path.join(GDRIVE_COPY_DIR, destination_name)
    shutil.copyfile(source_path, destination_path)
    log("구글드라이브", f"복사 완료: {destination_path}")
    return destination_path


def upload_firmware(target_path, token, version_number, version_name):
    log("업로드", f"파일 업로드 시작: {os.path.basename(target_path)}")
    log(
        "업로드",
        (
            f"projectId={PROJECT_ID_FIXED}, versionNumber={version_number}, "
            f"versionName={version_name}, chipType={CHIP_TYPE}, "
            f"firmwareFamily={FIRMWARE_FAMILY}"
        ),
    )

    try:
        with open(target_path, "rb") as handle:
            files = {
                "binFile": (
                    os.path.basename(target_path),
                    handle,
                    "application/octet-stream",
                )
            }
            data_payload = {
                "projectId": str(PROJECT_ID_FIXED),
                "versionNumber": str(version_number),
                "versionName": version_name,
                "chipType": CHIP_TYPE,
                "firmwareFamily": FIRMWARE_FAMILY,
            }
            headers = {"Authorization": f"Bearer {token}"}
            response = requests.post(
                UPLOAD_ENDPOINT,
                data=data_payload,
                files=files,
                headers=headers,
                timeout=30,
            )

        if response.status_code == 201:
            log("업로드", f"서버 등록 성공: {response.text}")
            return True

        log("업로드", f"서버 등록 실패: HTTP {response.status_code} / {response.text}")
        return False
    except Exception as exc:
        log("업로드", f"업로드 예외 발생: {exc}")
        return False


def run_automation(target_path):
    print("\n" + "=" * 75)
    print("[FIRMWARE DEPLOY SYSTEM]")
    print("=" * 75)

    version_number, version_name, version_string = extract_version_info(target_path)
    if version_number is None:
        log("버전", "바이너리에서 버전 정보를 추출하지 못했습니다.")
        return

    log("버전", f"바이너리 문자열 버전: v{version_string}")
    log("버전", f"OTA 비교용 버전 번호: {version_number}")
    log("버전", f"서버 등록 이름: {version_name}")

    token = get_access_token()
    if not token:
        log("인증", "토큰을 받지 못해 서버 업로드는 건너뜁니다.")
        return

    latest_version = get_latest_same_track_version(token)
    if not ensure_newer_than_server(version_number, latest_version):
        print("=" * 75 + "\n")
        return

    copied_path = copy_to_gdrive(target_path, version_number)
    upload_firmware(copied_path, token, version_number, version_name)
    print("=" * 75 + "\n")


def post_build_action(target, source, env):
    run_automation(str(target[0]))


if RUNNING_IN_PIO and env:
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", post_build_action)


if __name__ == "__main__" and not RUNNING_IN_PIO:
    test_path = r".pio\build\esp32-c3-devkitm-1\firmware.bin"
    if os.path.exists(test_path):
        run_automation(test_path)
    else:
        log("테스트", f"파일을 찾을 수 없습니다: {test_path}")
