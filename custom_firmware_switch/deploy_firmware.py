from __future__ import annotations

import argparse

from deploy_common import (
    build_target,
    copy_to_gdrive,
    extract_version_info,
    load_config,
    log,
    resolve_platformio_exe,
    sync_overlays,
)
from deploy_custom_ota import upload_to_ota_dashboard
from deploy_lattepanda import refresh_lattepanda_symlink, upload_to_lattepanda


def main() -> int:
    parser = argparse.ArgumentParser(description="커스텀 펌웨어 ESP8685/ESP-02S 빌드 및 배포")
    parser.add_argument("--target", choices=["esp8685", "esp02s", "all"], default="all")
    parser.add_argument(
        "--deploy-destination",
        choices=["gdrive-only", "custom-ota", "tasmota-host"],
        default="gdrive-only",
        help="기본은 구글드라이브까지만 복사하고, 필요할 때만 추가 배포를 수행합니다.",
    )
    parser.add_argument("--sync-only", action="store_true", help="공통/타깃 오버레이만 동기화하고 종료합니다.")
    args = parser.parse_args()

    config = load_config()
    sync_overlays(config)
    if args.sync_only:
        log("동기화", "공통/타깃 파일 동기화만 수행했습니다.")
        return 0

    platformio_exe = resolve_platformio_exe(config["platformio_exe"])
    if not platformio_exe.exists():
        raise FileNotFoundError(f"PlatformIO 실행 파일을 찾을 수 없습니다: {platformio_exe}")

    build_order = ["esp8685", "esp02s"] if args.target == "all" else [args.target]
    uploaded_any = False
    copied_any = False

    for target_name in build_order:
        target = config["targets"][target_name]
        artifact = build_target(target_name, target, platformio_exe)
        if artifact is None:
            continue

        version_number, version_name = extract_version_info(artifact)
        log("버전", f"{target_name} 버전 추출: number={version_number}, name={version_name}")

        copied_artifact = copy_to_gdrive(artifact, target_name, target, version_name)
        copied_any = True

        if args.deploy_destination == "custom-ota":
            upload_to_ota_dashboard(config, target_name, copied_artifact, version_number, version_name)
        elif args.deploy_destination == "tasmota-host":
            upload_to_lattepanda(config, target_name, copied_artifact)
            uploaded_any = True

    if args.deploy_destination == "tasmota-host" and uploaded_any:
        refresh_lattepanda_symlink(config)

    if args.deploy_destination == "gdrive-only":
        if copied_any:
            log("완료", "구글드라이브 복사까지만 완료했습니다. 서버 업로드는 수행하지 않았습니다.")
        else:
            log("완료", "생성된 산출물이 없어 서버 업로드 없이 종료했습니다.")
    elif args.deploy_destination == "custom-ota":
        log("완료", "커스텀 OTA 대시보드 등록까지 모두 완료되었습니다.")
    else:
        log("완료", "타스모타 OTA 서버 custom 경로 업로드까지 모두 완료되었습니다.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except KeyboardInterrupt:
        log("중단", "사용자가 작업을 중단했습니다.")
        raise SystemExit(130)
