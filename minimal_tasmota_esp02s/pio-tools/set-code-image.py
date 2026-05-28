Import("env")
import pathlib

# 현재 활성화된 [env:...] 섹션의 옵션을 직접 읽어오는 함수
def _get_option(key, default=""):
    try:
        # GetProjectOption은 .ini 파일에 적힌 값을 가장 우선적으로 가져옵니다.
        return env.GetProjectOption(key)
    except:
        return default

def _get_version():
    ver = _get_option("custom_firmware_version")
    return str(ver) if ver else "1"

# 설정값 로드
chip_type = _get_option("custom_server_chip_type", "esp02s").lower()
family = _get_option("custom_server_firmware_family", "tasmota").lower()
version = _get_version()

# 이미지 이름 구성 (v5_esp02s_tasmota_lite)
image_name = f"v{version}_{chip_type}_{family}_lite"

# 빌드 플래그에 주입
env.Append(BUILD_FLAGS=[f"-DCODE_IMAGE_STR='\"{image_name}\"'"])

print(f"\n\033[94m[VERSION] Successfully set CODE_IMAGE_STR = {image_name}\033[0m\n")
