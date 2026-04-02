Import("env")

import pathlib
import shutil
from colorama import Fore

import tasmotapiolib


def _copy_gzip_firmware(source, target, env):
    destination_dir_raw = env.GetProjectOption("custom_gdrive_copy_dir", default="")
    destination_name = env.GetProjectOption("custom_gdrive_copy_name", default="")

    if not destination_dir_raw:
        return

    source_bin = pathlib.Path(tasmotapiolib.get_final_bin_path(env))
    source_gz = source_bin.with_suffix(".bin.gz")
    if not source_gz.is_file():
        print(Fore.YELLOW + f"Skipping Google Drive copy because gzip firmware was not found: {source_gz}")
        return

    destination_dir = pathlib.Path(destination_dir_raw)
    destination_dir.mkdir(parents=True, exist_ok=True)

    if not destination_name:
        destination_name = source_gz.name

    destination_file = destination_dir / destination_name
    shutil.copy2(source_gz, destination_file)
    print(Fore.GREEN + f"Copied OTA gzip firmware to {destination_file}")


silent_action = env.Action(_copy_gzip_firmware)
silent_action.strfunction = lambda target, source, env: ""
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", silent_action)
