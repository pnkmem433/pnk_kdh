from __future__ import annotations

import json
import os
import pathlib
import re
import shutil
import subprocess
from typing import Any

CONFIG_PATH = pathlib.Path(__file__).resolve().parent / "config.json"
VERSION_RE = re.compile(rb"SMARTPLUG_FW_VERSION:([\d.]+)")
COMMON_SYNC_ROOT = CONFIG_PATH.parent / "shared" / "common"
TARGET_SYNC_ROOT = CONFIG_PATH.parent / "shared" / "targets"


def load_config() -> dict[str, Any]:
    return json.loads(CONFIG_PATH.read_text(encoding="utf-8"))


def log(step: str, message: str) -> None:
    print(f"[CUSTOM][{step}] {message}", flush=True)


def resolve_platformio_exe(raw_path: str) -> pathlib.Path:
    return pathlib.Path(os.path.expandvars(raw_path))


def target_exists(target: dict[str, Any]) -> bool:
    root = pathlib.Path(target["root"])
    check_file = target.get("project_check", "platformio.ini")
    return root.exists() and (root / check_file).exists()


def find_artifact(target: dict[str, Any]) -> pathlib.Path | None:
    root = pathlib.Path(target["root"])
    for hint in target.get("artifact_hints", []):
        artifact = root / hint
        if artifact.exists():
            return artifact
    return None


def copy_file(src: pathlib.Path, dst: pathlib.Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    content = src.read_bytes()
    if dst.exists() and dst.read_bytes() == content:
        return
    dst.write_bytes(content)
    log("???", f"{src} -> {dst}")


def sync_tree(src_root: pathlib.Path, dst_root: pathlib.Path) -> None:
    if not src_root.exists():
        return
    for src in src_root.rglob("*"):
        if not src.is_file():
            continue
        if src.name == ".gitkeep":
            continue
        rel = src.relative_to(src_root)
        copy_file(src, dst_root / rel)


def sync_overlays(config: dict[str, Any]) -> None:
    for target_name, target in config["targets"].items():
        target_root = pathlib.Path(target["root"])
        sync_tree(COMMON_SYNC_ROOT, target_root)
        sync_tree(TARGET_SYNC_ROOT / target_name, target_root)


def extract_version_info(bin_path: pathlib.Path) -> tuple[str, str]:
    content = bin_path.read_bytes()
    match = VERSION_RE.search(content)
    if not match:
        raise RuntimeError(f"?? ???? ?????? ?? ?????: {bin_path}")

    version_str = match.group(1).decode("utf-8")
    version_number = str(int(version_str.replace(".", "")))
    version_name = f"v{version_str}"
    return version_number, version_name


def copy_to_gdrive(
    bin_path: pathlib.Path,
    target_name: str,
    target: dict[str, Any],
    version_name: str,
) -> pathlib.Path:
    gdrive_dir = pathlib.Path(target["gdrive_dir"])
    gdrive_dir.mkdir(parents=True, exist_ok=True)
    destination_name = f"{version_name}_{target['artifact_name_suffix']}"
    destination_path = gdrive_dir / destination_name
    shutil.copyfile(bin_path, destination_path)
    log("??????", f"{target_name} ?? ??: {destination_path}")
    return destination_path


def build_target(target_name: str, target: dict[str, Any], platformio_exe: pathlib.Path) -> pathlib.Path | None:
    root = pathlib.Path(target["root"])
    if not target_exists(target):
        log("???", f"{target_name} ???? ??? ?? ?? ?????: {root}")
        return None

    cmd = [str(platformio_exe), "run", "-e", target["env"]]
    log("??", f"{target_name} ?? ??")
    log("??", " ".join(cmd))
    subprocess.run(cmd, cwd=root, check=True)

    artifact = find_artifact(target)
    if artifact is None:
        raise RuntimeError(f"{target_name} ?? ???? ?? ?????")

    log("??", f"??? ??: {artifact}")
    return artifact
