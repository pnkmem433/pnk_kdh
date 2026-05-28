Import("env")

import os
import pathlib
import re

_VERSION_CACHE = None


def _project_dir() -> pathlib.Path:
    return pathlib.Path(env.subst("$PROJECT_DIR"))


def _app_config_path() -> pathlib.Path:
    return _project_dir() / "include" / "AppConfig.h"


def _read_macro_int(text: str, name: str) -> int:
    match = re.search(rf"^#define\s+{name}\s+(\d+)\s*$", text, re.MULTILINE)
    if not match:
        raise RuntimeError(f"[VERSION] {name} not found in AppConfig.h")
    return int(match.group(1))


def _version_code() -> str:
    global _VERSION_CACHE
    if _VERSION_CACHE is not None:
        return _VERSION_CACHE

    app_config = _app_config_path().read_text(encoding="utf-8")
    major = _read_macro_int(app_config, "FW_VER_MAJOR")
    minor = _read_macro_int(app_config, "FW_VER_MINOR")
    patch = _read_macro_int(app_config, "FW_VER_PATCH")
    _VERSION_CACHE = str((major * 100) + (minor * 10) + patch)
    os.environ["GDRIVE_VERSION"] = _VERSION_CACHE
    print(f"[VERSION] Derived from AppConfig.h => v{_VERSION_CACHE}", flush=True)
    return _VERSION_CACHE


def _image_name() -> str:
    return f"v{_version_code()}_esp02s_custom"


image_name = _image_name()
env.Append(BUILD_FLAGS=[f'-DCODE_IMAGE_STR=\"{image_name}\"'])
print(f"[CODE_IMAGE] Using CODE_IMAGE_STR={image_name}")