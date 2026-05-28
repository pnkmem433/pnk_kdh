import argparse
import csv
import datetime as dt
import sys
from pathlib import Path

try:
    import serial
    from serial.tools import list_ports
except ImportError as exc:
    print("pyserial is required. Install with: py -m pip install pyserial", file=sys.stderr)
    raise SystemExit(1) from exc


CSV_HEADER = [
    "record_time_ms",
    "experiment_label",
    "run_number",
    "test_step",
    "location",
    "pattern",
    "expected_tag_count",
    "session_id",
    "configured_scan_time_sec",
    "row_type",
    "actual_scan_time_ms",
    "first_read_time_all_ms",
    "first_valid_read_time_ms",
    "total_unique_tags",
    "valid_tag_count",
    "recognition_rate_pct",
    "ghost_read_count",
    "raw_tag_id",
    "server_tag_id",
    "tag_read_count",
    "tag_first_read_time_ms",
    "passed_minimum_rule",
    "server_send_state",
    "memo",
]


def parse_args():
    parser = argparse.ArgumentParser(
        description="Capture firmware serial logs and save CSV scan rows automatically."
    )
    parser.add_argument("--port", help="Serial port such as COM30. Auto-detect if omitted.")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--label", default="default")
    parser.add_argument("--run", type=int, default=1)
    parser.add_argument("--step", default="phase1")
    parser.add_argument("--location", default="unknown")
    parser.add_argument("--pattern", default="unknown")
    parser.add_argument("--expected", type=int, default=0)
    parser.add_argument("--memo", default="")
    parser.add_argument("--output-dir", default="captures")
    return parser.parse_args()


def auto_detect_port():
    ports = list(list_ports.comports())
    if not ports:
        raise SystemExit("No serial ports found. Connect the board or pass --port.")
    if len(ports) == 1:
        return ports[0].device

    preferred = [p.device for p in ports if "USB" in (p.description or "").upper()]
    if len(preferred) == 1:
        return preferred[0]

    port_list = ", ".join(p.device for p in ports)
    raise SystemExit(f"Multiple serial ports found. Use --port. Available: {port_list}")


def send_meta(ser, key, value):
    command = f"META {key} {value}\n"
    ser.write(command.encode("utf-8"))


def main():
    args = parse_args()
    port = args.port or auto_detect_port()

    timestamp = dt.datetime.now().strftime("%Y%m%d_%H%M%S")
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    raw_log_path = output_dir / f"serial_log_{timestamp}.txt"
    csv_path = output_dir / f"rfid_capture_{timestamp}.csv"

    with serial.Serial(port, args.baud, timeout=1) as ser, raw_log_path.open(
        "w", encoding="utf-8", newline=""
    ) as raw_log, csv_path.open("w", encoding="utf-8-sig", newline="") as csv_file:
        csv_writer = csv.writer(csv_file)
        csv_writer.writerow(CSV_HEADER)
        csv_file.flush()

        print(f"Connected to {port} @ {args.baud}")
        print(f"Raw log : {raw_log_path}")
        print(f"CSV log : {csv_path}")

        meta_pairs = [
            ("LABEL", args.label),
            ("RUN", str(args.run)),
            ("STEP", args.step),
            ("LOCATION", args.location),
            ("PATTERN", args.pattern),
            ("EXPECTED", str(args.expected)),
            ("MEMO", args.memo),
            ("SHOW", ""),
        ]

        for key, value in meta_pairs:
            send_meta(ser, key, value)

        try:
            while True:
                line = ser.readline()
                if not line:
                    continue

                decoded = line.decode("utf-8", errors="replace").rstrip("\r\n")
                print(decoded)
                raw_log.write(decoded + "\n")
                raw_log.flush()

                if not decoded.startswith("CSV,"):
                    continue

                row = next(csv.reader([decoded[4:]]))
                if len(row) != len(CSV_HEADER):
                    print(
                        f"WARN ignored malformed CSV row with {len(row)} columns",
                        file=sys.stderr,
                    )
                    continue

                csv_writer.writerow(row)
                csv_file.flush()
        except KeyboardInterrupt:
            print("\nCapture stopped by user.")


if __name__ == "__main__":
    main()
