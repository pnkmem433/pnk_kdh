Import("env")

import os
import shutil
import sqlite3
import subprocess
import pathlib
import gzip
from colorama import Fore


def _get_config(key, default=""):
    try:
        return env.GetProjectConfig().get("common", key, default).strip()
    except:
        return default


def _get_version():
    # Tasmota는 override 값만 사용
    return _get_config("custom_firmware_version", "1")


def post_build_action(source, target, env):
    source_bin = str(target[0])

    if not source_bin.endswith(".bin"):
        source_bin = source_bin.replace(".elf", ".bin")

    if not os.path.exists(source_bin):
        print(Fore.RED + f"\n[Deploy] Error: Source bin not found: {source_bin}")
        return

    chip_type = _get_config("custom_server_chip_type", "esp8685").lower()
    family = _get_config("custom_server_firmware_family", "tasmota").lower()
    version = _get_version()
    project_id = int(_get_config("custom_server_project_id", "10"))

    # 실제 파일명 규칙
    base_name = f"{chip_type}_{family}_lite.bin"
    versioned_name = f"v{version}_{base_name}"
    versioned_gz = f"{versioned_name}.gz"

    print(Fore.GREEN + f"\n[Deploy] Starting Deployment for {versioned_name}")

    # ------------------------------------------------------------------
    # 1. gzip 생성
    # ------------------------------------------------------------------
    source_gz = source_bin + ".gz"
    with open(source_bin, "rb") as src, gzip.open(source_gz, "wb", compresslevel=9) as dst:
        shutil.copyfileobj(src, dst)

    # ------------------------------------------------------------------
    # 2. Google Drive 복사
    # ------------------------------------------------------------------
    gdrive_dir = _get_config("custom_gdrive_copy_dir")
    if gdrive_dir:
        os.makedirs(gdrive_dir, exist_ok=True)

        shutil.copy(source_bin, os.path.join(gdrive_dir, versioned_name))
        shutil.copy(source_gz, os.path.join(gdrive_dir, versioned_gz))

        print(Fore.GREEN + f"[Deploy] 1. Google Drive copied -> {versioned_name}")

    # ------------------------------------------------------------------
    # 3. Local server copy + SQLite
    # ------------------------------------------------------------------
    server_dir = _get_config("custom_server_upload_dir")
    if server_dir:
        os.makedirs(server_dir, exist_ok=True)

        dest_bin = os.path.join(server_dir, versioned_name)
        dest_gz = os.path.join(server_dir, versioned_gz)

        shutil.copy(source_bin, dest_bin)
        shutil.copy(source_gz, dest_gz)

        print(Fore.GREEN + f"[Deploy] 2. Local server copied -> {dest_bin}")

        db_path = pathlib.Path(server_dir).parent / "local-ota.sqlite"

        if db_path.exists():
            try:
                conn = sqlite3.connect(str(db_path))
                cur = conn.cursor()

                version_name_db = f"v{version}_{chip_type}_{family}_lite"

                existing = cur.execute(
                    """
                    SELECT id FROM project_version
                    WHERE project=? AND chipType=? AND firmwareFamily=? AND versionNumber=?
                    """,
                    (project_id, chip_type, family, int(version)),
                ).fetchone()

                if existing:
                    row_id = existing[0]
                    cur.execute(
                        """
                        UPDATE project_version
                        SET versionName=?, isActive=1, binFile=?
                        WHERE id=?
                        """,
                        (version_name_db, dest_bin, row_id),
                    )
                else:
                    cur.execute(
                        """
                        INSERT INTO project_version
                        (createdAt, versionNumber, versionName, chipType,
                         firmwareFamily, isActive, binFile, project)
                        VALUES
                        (datetime('now'), ?, ?, ?, ?, 1, ?, ?)
                        """,
                        (
                            int(version),
                            version_name_db,
                            chip_type,
                            family,
                            dest_bin,
                            project_id,
                        ),
                    )
                    row_id = cur.lastrowid

                cur.execute(
                    """
                    UPDATE project_version
                    SET isActive=0
                    WHERE project=? AND chipType=? AND firmwareFamily=? AND id<>?
                    """,
                    (project_id, chip_type, family, row_id),
                )

                conn.commit()
                conn.close()

                print(Fore.GREEN + f"[Deploy] 3. SQLite updated -> v{version}")

            except Exception as e:
                print(Fore.RED + f"[Deploy] SQLite Error: {e}")

    # ------------------------------------------------------------------
    # 4. Remote upload + latest symlink
    # ------------------------------------------------------------------
    host = _get_config("custom_remote_ota_host")
    if host:
        port = int(_get_config("custom_remote_ota_port", "22"))
        user = _get_config("custom_remote_ota_user")
        password = _get_config("custom_remote_ota_password")
        r_dir = _get_config("custom_remote_ota_dir")
        r_subdir = _get_config("custom_remote_ota_subdir")
        l_script = _get_config("custom_remote_ota_link_script")

        try:
            import paramiko
        except ImportError:
            import sys
            subprocess.check_call([sys.executable, "-m", "pip", "install", "paramiko"])
            import paramiko

        try:
            ssh = paramiko.SSHClient()
            ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            ssh.connect(host, port=port, username=user, password=password)

            remote_full_dir = f"{r_dir.rstrip('/')}/{r_subdir}"
            ssh.exec_command(f"mkdir -p {remote_full_dir}")

            sftp = ssh.open_sftp()
            sftp.put(source_bin, f"{remote_full_dir}/{versioned_name}")
            sftp.put(source_gz, f"{remote_full_dir}/{versioned_gz}")
            sftp.close()

            print(Fore.GREEN + f"[Deploy] 4. Remote uploaded -> {remote_full_dir}/{versioned_name}")

            if l_script:
                print(Fore.CYAN + "[Deploy] 5. Updating latest symlink...")

                targets = [
                    (base_name, f"v*_{base_name}"),
                    (f"{base_name}.gz", f"v*_{base_name}.gz"),
                ]

                for link_name, pattern in targets:
                    cmd = (
                        f"python3 {l_script} "
                        f"--root {r_dir} "
                        f"--subdir {r_subdir} "
                        f"--link-name {link_name} "
                        f"--pattern '{pattern}'"
                    )

                    stdin, stdout, stderr = ssh.exec_command(cmd)
                    err = stderr.read().decode().strip()

                    if err:
                        print(Fore.RED + err)

                print(Fore.GREEN + f"[Deploy] 5. Symlink updated -> {base_name}")

            ssh.close()

        except Exception as e:
            print(Fore.RED + f"[Deploy] Remote Error: {e}")

    print(Fore.GREEN + "[Deploy] All deployment steps completed!\n")


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", post_build_action)