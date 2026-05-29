import re
import os

def extract_and_format_version(bin_path):
    # 1. 파일 확인
    if not os.path.exists(bin_path):
        print(f"❌ 에러: 파일을 찾을 수 없습니다 -> {bin_path}")
        return None, None

    try:
        with open(bin_path, "rb") as f:
            content = f.read()
            
            # 2. 정규표현식으로 마커 탐색 (SMARTPLUG_FW_VERSION:0.0.8 형식)
            match = re.search(rb"SMARTPLUG_FW_VERSION:([\d.]+)", content)
            
            if match:
                # 3. 바이너리에서 추출된 문자열 (예: "0.0.8")
                version_str = match.group(1).decode('utf-8')
                
                # 4. 버전 번호(Integer) 추출 로직
                # "0.0.8" -> "008" -> int() -> 8
                # "1.0.8" -> "108" -> int() -> 108
                version_number = int(version_str.replace(".", ""))
                
                # 5. 버전 이름(String) 형식 생성
                # "0.0.8" -> "v0.0.8"
                version_name = f"v{version_str}"
                
                return version_number, version_name
            else:
                print("❌ 에러: 바이너리 내에서 버전 마커를 찾지 못했습니다.")
                return None, None

    except Exception as e:
        print(f"❌ 오류 발생: {e}")
        return None, None

# ==========================================
# 실행 및 결과 확인
# ==========================================
# 사용자님의 실제 경로
target_path = r"C:\Users\pnks\Documents\PlatformIO\Projects\SmartPlug\.pio\build\esp32-c3-devkitm-1\firmware.bin"

ver_num, ver_name = extract_and_format_version(target_path)

if ver_num is not None:
    print("-" * 30)
    print(f"✅ 추출 성공!")
    print(f"🔢 버전 번호 (int)   : {ver_num}")
    print(f"🏷️ 버전 이름 (string): {ver_name}")
    print("-" * 30)
    
    # 이제 이 변수들을 사용하여 서버 업로드 API에 데이터를 던지면 됩니다.
    # 예: payload = {'version': ver_num, 'name': ver_name}
else:
    print("⚠️ 버전을 가져오는 데 실패했습니다.")