Import("env")


def _get_config(key, default=""):
    try:
        return env.GetProjectConfig().get("common", key, default).strip()
    except:
        return default


def _get_version():
    return _get_config("custom_firmware_version", "1")


chip_type = _get_config("custom_server_chip_type", "esp8685").lower()
family = _get_config("custom_server_firmware_family", "tasmota").lower()
version = _get_version()

image_name = f"v{version}_{chip_type}_{family}_lite"

env.Append(
    BUILD_FLAGS=[
        f"-DCODE_IMAGE_STR='\"{image_name}\"'"
    ]
)

print(f"\n[TASMOTA VERSION] Set CODE_IMAGE_STR = {image_name}\n")