Import("env")

import gzip
import os
import pathlib
import shutil


def _project_output_dir() -> pathlib.Path:
    return pathlib.Path(env.subst("$PROJECT_DIR")) / "build_output" / "firmware"


def _get_output_basename() -> str:
    project_config = env.GetProjectConfig()
    return project_config.get("common", "custom_output_basename", "esp02s_custom.bin")


def _get_version_text() -> str:
    version_text = os.environ.get("GDRIVE_VERSION", "").strip()
    if not version_text:
        raise RuntimeError("[GZIP] GDRIVE_VERSION is missing. Run the build through PlatformIO so set-code-image.py can prompt first.")
    return "".join(ch for ch in version_text if ch.isdigit())


def _gzip_firmware(source, target, env):
    bin_file = pathlib.Path(env.subst("$BUILD_DIR")) / f"{env.subst('$PROGNAME')}.bin"
    if not bin_file.is_file():
        print(f"[GZIP] Firmware not found: {bin_file}")
        return

    output_dir = _project_output_dir()
    output_dir.mkdir(parents=True, exist_ok=True)

    version = _get_version_text()
    basename = _get_output_basename()
    versioned_name = f"v{version}_{basename}"
    named_bin = output_dir / versioned_name
    named_gz = output_dir / f"{versioned_name}.gz"

    shutil.copyfile(bin_file, named_bin)
    with bin_file.open("rb") as src, gzip.open(named_gz, "wb", compresslevel=9) as dst:
        shutil.copyfileobj(src, dst)

    print(f"[GZIP] BIN: {named_bin}")
    print(f"[GZIP] GZ : {named_gz}")


gzip_action = env.Action(_gzip_firmware)
gzip_action.strfunction = lambda target, source, env: ""
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", gzip_action)