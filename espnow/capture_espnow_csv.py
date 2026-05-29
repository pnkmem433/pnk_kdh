import csv
import os
import sys
import time
from datetime import datetime
from typing import Dict, List, Optional

try:
    import serial
    from serial import SerialException
except ImportError:
    print("pyserial is required. Install with: pip install pyserial")
    sys.exit(1)


CSV_PATH = r"C:\WS\vs_kdh\pnk_kdh\espnow\esp_now_pickdown_1234_test_data.csv"
SESSION_LOG_PATH = r"C:\WS\vs_kdh\pnk_kdh\espnow\esp_now_pickdown_1234_capture_sessions.csv"
CSV_HEADER = [
    "Test_ID",
    "Phase",
    "Environment",
    "Pattern",
    "Slave_ID",
    "NFC_Index",
    "UID",
    "Event",
    "Boot_ID",
    "Packet_Seq",
    "Poll_Seq",
    "Retry_Count",
    "MasterPollIntervalMs",
    "RemoveTimeMs",
    "NfcPollIntervalMs",
    "T0_SlaveEvent",
    "T1_Poll",
    "T2_Recv",
    "T3_MQTT",
    "Latency_QueueWait",
    "Latency_Radio",
    "Latency_Wifi",
    "Total_Latency",
    "Status",
]
SESSION_HEADER = [
    "session_started_at",
    "serial_port",
    "baudrate",
    "phase",
    "environment",
    "pattern",
    "master_poll_interval_ms",
    "remove_time_ms",
    "nfc_poll_interval_ms",
    "first_test_id",
]


def ensure_csv_file(path: str) -> None:
    directory = os.path.dirname(path)
    if directory and not os.path.isdir(directory):
        raise FileNotFoundError(f"Directory does not exist: {directory}")

    file_exists = os.path.exists(path) and os.path.getsize(path) > 0
    if not file_exists:
        with open(path, "w", newline="", encoding="utf-8-sig") as csv_file:
            writer = csv.writer(csv_file)
            writer.writerow(CSV_HEADER)
        print(f"[INFO] Created new CSV file: {path}")
    else:
        print(f"[INFO] Appending to existing CSV file: {path}")


def ensure_session_log_file(path: str) -> None:
    directory = os.path.dirname(path)
    if directory and not os.path.isdir(directory):
        raise FileNotFoundError(f"Directory does not exist: {directory}")

    file_exists = os.path.exists(path) and os.path.getsize(path) > 0
    if file_exists:
        return

    with open(path, "w", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(SESSION_HEADER)


def append_csv_row(path: str, row: List[str]) -> None:
    with open(path, "a", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.writer(csv_file)
        writer.writerow(row)


def append_session_row(path: str, row: Dict[str, str]) -> None:
    with open(path, "a", newline="", encoding="utf-8-sig") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=SESSION_HEADER)
        writer.writerow(row)


def parse_csv_line(raw_line: str) -> Optional[List[str]]:
    line = raw_line.strip()
    if not line.startswith("CSV,"):
        return None

    row = line.split(",")[1:]
    if len(row) != len(CSV_HEADER):
        print(f"[WARN] Ignored malformed CSV row: {line}")
        return None
    return row


def build_session_row(port: str, baudrate: int, row: List[str]) -> Dict[str, str]:
    return {
        "session_started_at": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "serial_port": port,
        "baudrate": str(baudrate),
        "phase": row[1],
        "environment": row[2],
        "pattern": row[3],
        "master_poll_interval_ms": row[12],
        "remove_time_ms": row[13],
        "nfc_poll_interval_ms": row[14],
        "first_test_id": row[0],
    }


def main() -> None:
    port = sys.argv[1] if len(sys.argv) > 1 else "COM19"
    baudrate = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

    try:
        ensure_csv_file(CSV_PATH)
        ensure_session_log_file(SESSION_LOG_PATH)
    except (FileNotFoundError, PermissionError, OSError) as exc:
        print(f"[ERROR] CSV file setup failed: {exc}")
        sys.exit(1)

    print(f"[INFO] Serial port: {port}")
    print(f"[INFO] CSV path: {CSV_PATH}")
    print(f"[INFO] Session log path: {SESSION_LOG_PATH}")

    try:
        while True:
            try:
                with serial.Serial(port, baudrate, timeout=1) as ser:
                    print("[INFO] Serial connected. Streaming serial output and capturing CSV rows...")
                    session_logged = False
                    while True:
                        try:
                            raw = ser.readline()
                        except SerialException as exc:
                            print(f"[ERROR] Serial read failed: {exc}")
                            break

                        if not raw:
                            continue

                        try:
                            line = raw.decode("utf-8", errors="replace")
                        except UnicodeDecodeError:
                            line = raw.decode("latin-1", errors="replace")

                        line = line.rstrip("\r\n")
                        if line:
                            print(f"[SERIAL] {line}")

                        row = parse_csv_line(line)
                        if row is None:
                            continue

                        try:
                            if not session_logged:
                                session_row = build_session_row(port, baudrate, row)
                                append_session_row(SESSION_LOG_PATH, session_row)
                                session_logged = True
                                print(
                                    "[SESSION] "
                                    f"{session_row['phase']} / {session_row['environment']} / {session_row['pattern']} / "
                                    f"master_poll={session_row['master_poll_interval_ms']} / "
                                    f"remove={session_row['remove_time_ms']} / "
                                    f"nfc_poll={session_row['nfc_poll_interval_ms']}"
                                )
                            append_csv_row(CSV_PATH, row)
                            print(f"[CSV-SAVED] {','.join(row)}")
                        except (PermissionError, OSError) as exc:
                            print(f"[ERROR] Failed to append CSV row: {exc}")

            except SerialException as exc:
                print(f"[ERROR] Unable to open serial port {port}: {exc}")
                print("[INFO] Retrying in 3 seconds...")
                time.sleep(3)
    except KeyboardInterrupt:
        print("\n[INFO] Capture stopped by user.")


if __name__ == "__main__":
    main()
