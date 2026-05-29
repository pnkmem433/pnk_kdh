from __future__ import annotations

import pathlib
import subprocess
from typing import Any

from deploy_common import log


def run_command(command: list[str], start_message: str, success_message: str) -> None:
    log("??", start_message)
    log("??", " ".join(command))
    try:
        subprocess.run(command, check=True)
        log("??", success_message)
    except subprocess.CalledProcessError as exc:
        raise RuntimeError(f"?? ??? ??????: {' '.join(command)}") from exc


def upload_to_lattepanda(config: dict[str, Any], target_name: str, artifact_path: pathlib.Path) -> None:
    server = config["lattepanda_server"]
    target_cfg = server["targets"][target_name]
    remote_dir = target_cfg["remote_dir"]
    host = server["host"]
    port = str(server.get("port", 22))

    run_command(
        ["scp", "-P", port, str(artifact_path), f"{host}:{remote_dir}/"],
        f"???? ??? ???? ?????: {artifact_path.name}",
        f"???? ?? ???? ???????: {remote_dir}/{artifact_path.name}",
    )


def refresh_lattepanda_symlink(config: dict[str, Any]) -> None:
    server = config["lattepanda_server"]
    host = server["host"]
    script_path = server["update_script"]
    port = str(server.get("port", 22))

    run_command(
        ["ssh", "-p", port, host, f"python3 {script_path}"],
        "???? ??? ?? ??? ?? ?? ????? ?????.",
        "???? ??? ?? ??? ?? ??? ???????.",
    )
