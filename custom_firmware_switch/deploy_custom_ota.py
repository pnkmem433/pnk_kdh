from __future__ import annotations

import pathlib
from typing import Any

try:
    import requests
except ImportError:
    requests = None

from deploy_common import log


def get_access_token(config: dict[str, Any]) -> str:
    if requests is None:
        raise RuntimeError("requests 모듈이 없어 OTA 대시보드 업로드를 진행할 수 없습니다.")

    ota = config["ota_server"]
    login_url = ota["base_url"] + ota["login_endpoint"]
    log("인증", f"로그인 시도: {login_url}")
    response = requests.post(login_url, json=ota["credentials"], timeout=20)
    if response.status_code not in (200, 201):
      raise RuntimeError(f"로그인 실패: HTTP {response.status_code} / {response.text}")

    token = response.json().get("token", {}).get("access_token")
    if not token:
      raise RuntimeError("로그인 응답에서 access_token을 찾지 못했습니다.")

    log("인증", "JWT 토큰 발급 완료")
    return token


def upload_to_ota_dashboard(
    config: dict[str, Any],
    target_name: str,
    artifact_path: pathlib.Path,
    version_number: str,
    version_name: str,
) -> None:
    if requests is None:
        raise RuntimeError("requests 모듈이 없어 OTA 대시보드 업로드를 진행할 수 없습니다.")

    ota = config["ota_server"]
    upload_url = ota["base_url"] + ota["upload_endpoint"]
    token = get_access_token(config)

    target = config["targets"][target_name]
    chip_type = target["chip_type"]
    firmware_family = config.get("firmware_family", "custom")

    log("업로드", f"OTA 대시보드 업로드 시작: {artifact_path.name}")
    with artifact_path.open("rb") as handle:
        files = {
            "binFile": (artifact_path.name, handle, "application/octet-stream")
        }
        data = {
            "projectId": str(ota["project_id_fixed"]),
            "versionNumber": str(version_number),
            "versionName": str(version_name),
            "chipType": chip_type,
            "firmwareFamily": firmware_family,
        }
        headers = {"Authorization": f"Bearer {token}"}
        response = requests.post(upload_url, data=data, files=files, headers=headers, timeout=60)

    if response.status_code != 201:
        raise RuntimeError(f"OTA 대시보드 업로드 실패: HTTP {response.status_code} / {response.text}")

    log("업로드", f"OTA 대시보드 등록 완료: {version_name} ({chip_type}/{firmware_family})")
