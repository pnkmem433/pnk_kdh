rom __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
TASMOTA_SWITCH = ROOT / "tasmota_firmware_switch" / "build_all.py"
CUSTOM_SWITCH = ROOT / "custom_firmware_switch" / "build_all.py"


def ask_choice(title: str, options: list[tuple[str, str]]) -> str:
    print("=" * 72, flush=True)
    print(title, flush=True)
    for key, label in options:
        print(f"  {key}. {label}", flush=True)

    while True:
        print("??? ????? :", flush=True)
        value = sys.stdin.readline().strip()
        for key, _ in options:
            if value == key:
                return value
        print("??? ??? ?? ??????.", flush=True)


def run_script(script_path: Path, extra_args: list[str] | None = None) -> int:
    extra_args = extra_args or []
    cmd = [sys.executable, str(script_path), *extra_args]
    print(f"[??] {' '.join(cmd)}", flush=True)
    result = subprocess.run(cmd, cwd=script_path.parent)
    return result.returncode


def main() -> int:
    build_family = ask_choice(
        "??? ??? ??? ?????.",
        [
            ("1", "???? ??? ?? ? ??"),
            ("2", "???? ??? ?? ? ??"),
        ],
    )

    if build_family == "1":
        print("[??] ???? ???? ???? OTA ??? tasmota ??? ?????.", flush=True)
        return run_script(TASMOTA_SWITCH)

    deploy_target = ask_choice(
        "??? ??? ???? ???? ?? ?? ??? ??? ??????",
        [
            ("1", "?? ??? ???? ??????. ???? OTA ????? ?????."),
            ("2", "?? ??? ???? ??????. ???? OTA ??? custom ??? ?????."),
        ],
    )

    if deploy_target == "1":
        return run_script(CUSTOM_SWITCH, ["--deploy-destination", "custom-ota"])

    return run_script(CUSTOM_SWITCH, ["--deploy-destination", "tasmota-host"])


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        print("\n[??] ???? ??? ??????.", flush=True)
        raise SystemExit(130)
