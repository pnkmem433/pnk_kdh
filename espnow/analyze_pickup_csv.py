import csv
from collections import defaultdict
from pathlib import Path
from statistics import mean, median, pstdev
from typing import Dict, List


RAW_CSV_PATH = Path(r"C:\WS\vs_kdh\pnk_kdh\espnow\esp_now_pickup_test_data.csv")
OUTPUT_DIR = Path(r"C:\WS\vs_kdh\pnk_kdh\espnow\analysis_output")
SUMMARY_PATH = OUTPUT_DIR / "pickup_summary.csv"
HEADER = (
    "Test_ID,Phase,Environment,Pattern,Slave_ID,NFC_Index,UID,Event,Boot_ID,Packet_Seq,"
    "Poll_Seq,Retry_Count,MasterPollIntervalMs,RemoveTimeMs,NfcPollIntervalMs,"
    "T0_SlaveEvent,T1_Poll,T2_Recv,T3_MQTT,Latency_QueueWait,Latency_Radio,"
    "Latency_Wifi,Total_Latency,Status"
)
CSV_HEADER = HEADER.split(",")
SUMMARY_HEADER = [
    "phase",
    "environment",
    "pattern",
    "slave_id",
    "nfc_index",
    "event",
    "master_poll_interval_ms",
    "remove_time_ms",
    "nfc_poll_interval_ms",
    "rows",
    "success_rows",
    "fail_rows",
    "success_rate_pct",
    "avg_total_latency_ms",
    "median_total_latency_ms",
    "max_total_latency_ms",
    "stddev_total_latency_ms",
    "avg_queue_wait_ms",
    "avg_radio_ms",
    "avg_wifi_ms",
    "avg_retry_count",
    "max_retry_count",
]


def load_rows(path: Path) -> List[Dict[str, str]]:
    text = path.read_text(encoding="utf-8-sig", errors="replace")
    header_index = text.find(HEADER)
    if header_index < 0:
      raise ValueError("CSV header not found in file")
    clean_text = text[header_index:]
    rows = list(csv.DictReader(clean_text.splitlines()))
    return [row for row in rows if row.get("Phase", "").startswith("PICKUP_")]


def as_int(row: Dict[str, str], key: str) -> int:
    return int(row[key])


def summarize(rows: List[Dict[str, str]]) -> List[Dict[str, object]]:
    grouped: Dict[tuple, List[Dict[str, str]]] = defaultdict(list)
    for row in rows:
        key = (
            row["Phase"],
            row["Environment"],
            row["Pattern"],
            row["Slave_ID"],
            row["NFC_Index"],
            row["Event"],
            row["MasterPollIntervalMs"],
            row["RemoveTimeMs"],
            row["NfcPollIntervalMs"],
        )
        grouped[key].append(row)

    summary_rows: List[Dict[str, object]] = []
    for key in sorted(grouped.keys()):
        group = grouped[key]
        success_rows = [row for row in group if row["Status"] == "SUCCESS"]
        total_latencies = [as_int(row, "Total_Latency") for row in group]
        queue_latencies = [as_int(row, "Latency_QueueWait") for row in group]
        radio_latencies = [as_int(row, "Latency_Radio") for row in group]
        wifi_latencies = [as_int(row, "Latency_Wifi") for row in group]
        retries = [as_int(row, "Retry_Count") for row in group]

        summary_rows.append(
            {
                "phase": key[0],
                "environment": key[1],
                "pattern": key[2],
                "slave_id": key[3],
                "nfc_index": key[4],
                "event": key[5],
                "master_poll_interval_ms": key[6],
                "remove_time_ms": key[7],
                "nfc_poll_interval_ms": key[8],
                "rows": len(group),
                "success_rows": len(success_rows),
                "fail_rows": len(group) - len(success_rows),
                "success_rate_pct": round((len(success_rows) / len(group)) * 100, 2) if group else 0,
                "avg_total_latency_ms": round(mean(total_latencies), 2) if total_latencies else "",
                "median_total_latency_ms": round(median(total_latencies), 2) if total_latencies else "",
                "max_total_latency_ms": max(total_latencies) if total_latencies else "",
                "stddev_total_latency_ms": round(pstdev(total_latencies), 2) if len(total_latencies) > 1 else 0,
                "avg_queue_wait_ms": round(mean(queue_latencies), 2) if queue_latencies else "",
                "avg_radio_ms": round(mean(radio_latencies), 2) if radio_latencies else "",
                "avg_wifi_ms": round(mean(wifi_latencies), 2) if wifi_latencies else "",
                "avg_retry_count": round(mean(retries), 2) if retries else "",
                "max_retry_count": max(retries) if retries else "",
            }
        )
    return summary_rows


def save_summary(rows: List[Dict[str, object]], path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=SUMMARY_HEADER)
        writer.writeheader()
        writer.writerows(rows)


def print_summary(rows: List[Dict[str, object]]) -> None:
    if not rows:
        print("No PICKUP_* rows found in the CSV.")
        return

    print("Pickup summary")
    print("-" * 72)
    for row in rows:
        print(
            f"{row['phase']} / {row['environment']} / slave={row['slave_id']} / nfc={row['nfc_index']} / {row['event']} | "
            f"poll={row['master_poll_interval_ms']} remove={row['remove_time_ms']} "
            f"rows={row['rows']} success={row['success_rate_pct']}% "
            f"avg_total={row['avg_total_latency_ms']}ms median={row['median_total_latency_ms']}ms "
            f"max_total={row['max_total_latency_ms']}ms stddev={row['stddev_total_latency_ms']}ms "
            f"avg_retry={row['avg_retry_count']}"
        )


def main() -> None:
    rows = load_rows(RAW_CSV_PATH)
    summary_rows = summarize(rows)
    save_summary(summary_rows, SUMMARY_PATH)
    print_summary(summary_rows)
    print(f"\nSaved: {SUMMARY_PATH}")


if __name__ == "__main__":
    main()
