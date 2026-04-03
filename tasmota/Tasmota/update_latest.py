#!/usr/bin/env python3
"""
Deploy this file to: /ota/tasmota/update_latest.py

Behavior:
- scans /ota/tasmota for v*_tasmota-smartplug.bin.gz
- finds the highest numeric version
- updates /ota/tasmota/tasmota-smartplug.bin.gz symlink
  to point at that latest versioned file
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path


BASE_DIR = Path("/ota/tasmota")
BASE_NAME = "tasmota-smartplug.bin.gz"
VERSION_RE = re.compile(r"^v(\d+)_tasmota-smartplug\.bin\.gz$")


def find_latest_versioned_file(base_dir: Path) -> Path:
    latest_path: Path | None = None
    latest_version: int | None = None

    for path in base_dir.iterdir():
        if not path.is_file():
            continue
        match = VERSION_RE.match(path.name)
        if not match:
            continue
        version = int(match.group(1))
        if latest_version is None or version > latest_version:
            latest_version = version
            latest_path = path

    if latest_path is None:
        raise FileNotFoundError("No versioned smartplug firmware files were found.")

    return latest_path


def update_symlink(base_dir: Path, latest_file: Path) -> Path:
    link_path = base_dir / BASE_NAME
    if link_path.exists() or link_path.is_symlink():
        link_path.unlink()

    # Use a relative symlink so the folder can be moved without breaking the link.
    os.symlink(latest_file.name, link_path)
    return link_path


def main() -> int:
    print("[서버] update_latest.py 실행 시작")
    print(f"[서버] 작업 폴더: {BASE_DIR}")

    if not BASE_DIR.is_dir():
        print(f"ERROR: OTA directory not found: {BASE_DIR}", file=sys.stderr)
        return 1

    try:
        print("[서버] 최신 버전 파일 검색중...")
        latest = find_latest_versioned_file(BASE_DIR)
        print(f"[서버] 최신 버전 파일 확인: {latest.name}")
        print("[서버] 심볼릭 링크 갱신중...")
        link_path = update_symlink(BASE_DIR, latest)
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1

    print(f"[서버] 최신 버전 파일: {latest.name}")
    print(f"[서버] 심볼릭 링크 갱신 완료: {link_path} -> {latest.name}")
    print("[서버] update_latest.py 실행 완료")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
