#!/usr/bin/env python3
"""
Deploy this file to: /ota/tasmota/update_latest.py

Purpose:
- scan the esp32 and esp02s OTA folders
- find the newest versioned firmware by modified time
- refresh the stable symlink for each target
- print detailed server-side logs for every step
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path


BASE_DIR = Path("/ota/tasmota")

TARGETS = {
    "esp32": {
        "search_dir": BASE_DIR / "esp32",
        "pattern": re.compile(r"^v\d+_esp8685_tasmota\.bin$"),
        "link_path": BASE_DIR / "esp32" / "esp8685_tasmota.bin",
    },
    "esp02s": {
        "search_dir": BASE_DIR / "esp02s",
        "pattern": re.compile(r"^v\d+_esp02s_tasmota\.bin\.gz$"),
        "link_path": BASE_DIR / "esp02s" / "esp02s_tasmota.bin.gz",
    },
}


def log(step: str, message: str) -> None:
    print(f"[SERVER][{step}] {message}", flush=True)


def find_latest_by_mtime(search_dir: Path, pattern: re.Pattern[str]) -> Path:
    candidates: list[Path] = []
    for path in search_dir.iterdir():
        if not path.is_file():
            continue
        if pattern.match(path.name):
            candidates.append(path)

    if not candidates:
        raise FileNotFoundError(f"No matching versioned files found in {search_dir}")

    candidates.sort(key=lambda path: path.stat().st_mtime, reverse=True)
    return candidates[0]


def refresh_symlink(link_path: Path, latest: Path) -> None:
    if link_path.exists() or link_path.is_symlink():
        log("링크", f"기존 링크 또는 파일 삭제: {link_path}")
        link_path.unlink()

    os.symlink(latest.name, link_path)
    log("링크", f"심볼릭 링크 생성: {link_path} -> {latest.name}")


def process_target(name: str, config: dict[str, object]) -> None:
    search_dir = config["search_dir"]
    pattern = config["pattern"]
    link_path = config["link_path"]

    log("대상", f"처리 시작: {name}")
    log("대상", f"검색 폴더: {search_dir}")
    log("대상", f"고정 링크 경로: {link_path}")

    if not isinstance(search_dir, Path) or not isinstance(link_path, Path):
        raise TypeError(f"대상 {name}의 경로 설정이 올바르지 않습니다")
    if not isinstance(pattern, re.Pattern):
        raise TypeError(f"대상 {name}의 정규식 설정이 올바르지 않습니다")

    if not search_dir.is_dir():
        raise FileNotFoundError(f"대상 폴더를 찾지 못했습니다: {search_dir}")

    latest = find_latest_by_mtime(search_dir, pattern)
    log("대상", f"최신 파일 선택: {latest.name}")
    refresh_symlink(link_path, latest)
    log("대상", f"처리 완료: {name}")


def main() -> int:
    log("시작", "update_latest.py 실행 시작")
    log("시작", f"기준 폴더: {BASE_DIR}")

    if not BASE_DIR.is_dir():
        print(f"오류: OTA 기준 폴더를 찾지 못했습니다: {BASE_DIR}", file=sys.stderr)
        return 1

    try:
        for name, config in TARGETS.items():
            process_target(name, config)
    except Exception as exc:
        print(f"오류: {exc}", file=sys.stderr)
        return 1

    log("완료", "update_latest.py 실행 완료")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
