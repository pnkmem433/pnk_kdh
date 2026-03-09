import os
import re
import requests
import json

# =============================================================
# [1] 서버 접속 및 프로젝트 설정
# =============================================================
BASE_URL = "http://gym907-0001.iptime.org:3004"
LOGIN_ENDPOINT = f"{BASE_URL}/auth/login"
UPLOAD_ENDPOINT = f"{BASE_URL}/versions/create"

# serverCheck.py를 통해 확인된 진짜 ID 10번을 적용합니다.
PROJECT_ID_FIXED = 10

USER_CREDENTIALS = {
    "id": "pnkmem433",
    "password": "12345678"
}

# PlatformIO 환경 설정 확인
try:
    from SCons.Script import Import
    Import("env")
    RUNNING_IN_PIO = True
except ImportError:
    RUNNING_IN_PIO = False
    env = None

# =============================================================
# [함수 1] 서버 로그인 및 실시간 토큰 확보
# =============================================================
def get_access_token():
    print("-" * 75)
    print(f"[인증] 서버 로그인 시도: {LOGIN_ENDPOINT}")
    try:
        # 로그인 시마다 항상 새로운 유효 토큰을 발급받습니다.
        response = requests.post(LOGIN_ENDPOINT, json=USER_CREDENTIALS)
        if response.status_code in [200, 201]:
            token = response.json().get("token", {}).get("access_token")
            if token:
                print("[인증] 성공: 최신 JWT 토큰을 확보했습니다.")
                return token
        print(f"[인증] 실패: 상태 코드 {response.status_code}")
    except Exception as e:
        print(f"[인증] 오류: {e}")
    return None

# =============================================================
# [함수 2] 버전 정보 정제 (0.1.2 -> 12 및 v0.1.2)
# =============================================================
def extract_version_info(bin_path):
    if not os.path.exists(bin_path): return None, None
    try:
        with open(bin_path, "rb") as f:
            content = f.read()
            match = re.search(rb"SMARTPLUG_FW_VERSION:([\d.]+)", content)
            if match:
                version_str = match.group(1).decode('utf-8')
                # 0.1.2에서 점을 제거하고 앞의 0을 없애기 위해 정수 변환 후 문자열화
                version_number = str(int(version_str.replace(".", "")))
                version_name = f"v{version_str}"
                return version_number, version_name
    except Exception as e:
        print(f"[추출] 오류: {e}")
    return None, None

# =============================================================
# [함수 3] 서버 업로드 (MIME 타입 지정 및 멀티파트 전송)
# =============================================================
def upload_firmware(target_path, token, ver_num, ver_name):
    print("-" * 75)
    print("[업로드] 서버 전송 프로세스 시작")
    print(f" -> 대상 파일: {os.path.basename(target_path)}")
    print(f" -> 전송 데이터: ID={PROJECT_ID_FIXED}, Num={ver_num}, Name={ver_name}")
    
    try:
        # 파일 데이터 구성 (MIME 타입을 명시하여 curl -F 형식을 재현)
        files = {
            'binFile': (
                os.path.basename(target_path), 
                open(target_path, 'rb'), 
                'application/octet-stream'
            )
        }
        
        # 프로젝트 ID와 버전 정보를 모두 문자열로 전송
        data_payload = {
            'projectId': str(PROJECT_ID_FIXED),
            'versionNumber': str(ver_num),
            'versionName': str(ver_name)
        }

        headers = { 'Authorization': f'Bearer {token}' }

        # POST 요청 실행
        response = requests.post(UPLOAD_ENDPOINT, data=data_payload, files=files, headers=headers)

        print("-" * 75)
        if response.status_code == 201:
            print("[결과] 성공: 서버 데이터베이스에 버전 등록이 완료되었습니다.")
            print(f" -> 서버 응답: {response.text}")
        else:
            print(f"[결과] 실패: 상태 코드 {response.status_code}")
            print(f" -> 에러 내용: {response.text}")

    except Exception as e:
        print(f"[업로드] 예외 발생: {e}")
    print("-" * 75)

# =============================================================
# [메인] 실행 제어
# =============================================================
def run_automation(target_path):
    print("\n" + "=" * 75)
    print(" [FIRMWARE DEPLOY SYSTEM]")
    print("=" * 75)

    v_num, v_name = extract_version_info(target_path)
    if v_num is None:
        print("[오류] 버전 정보를 추출하지 못했습니다.")
        return

    token = get_access_token()
    if not token: return

    upload_firmware(target_path, token, v_num, v_name)
    print("=" * 75 + "\n")

# PlatformIO Post-Action 등록
def post_build_action(target, source, env):
    run_automation(str(target[0]))

if RUNNING_IN_PIO and env:
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", post_build_action)

# 터미널 직접 실행 지원
if __name__ == "__main__" and not RUNNING_IN_PIO:
    test_path = r".pio\build\esp32-c3-devkitm-1\firmware.bin"
    if os.path.exists(test_path):
        run_automation(test_path)
    else:
        print(f"파일을 찾을 수 없습니다: {test_path}")