import csv
import os
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from statistics import mean
from typing import Dict, List, Optional, Tuple

try:
    MPL_DIR = Path(r"C:\WS\vs_kdh\pnk_kdh\espnow\.mplconfig")
    MPL_DIR.mkdir(parents=True, exist_ok=True)
    os.environ["MPLCONFIGDIR"] = str(MPL_DIR)
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError:
    plt = None


RAW_CSV_PATH = Path(r"C:\WS\vs_kdh\pnk_kdh\espnow\esp_now_test_data.csv")
OUTPUT_DIR = Path(r"C:\WS\vs_kdh\pnk_kdh\espnow\analysis_output")
HEADER = (
    "Test_ID,Phase,Environment,Pattern,Slave_ID,NFC_Index,UID,Event,Boot_ID,Packet_Seq,"
    "Poll_Seq,Retry_Count,MasterPollIntervalMs,RemoveTimeMs,NfcPollIntervalMs,"
    "T0_SlaveEvent,T1_Poll,T2_Recv,T3_MQTT,Latency_QueueWait,Latency_Radio,"
    "Latency_Wifi,Total_Latency,Status"
)
CSV_HEADER = HEADER.split(",")
SUMMARY_HEADER = [
    "label_ko",
    "master_poll_interval_ms",
    "nfc_remove_detect_ms",
    "total_events",
    "pickdown_count",
    "pickup_count",
    "hold_windows",
    "hold_success_20min_count",
    "avg_hold_ms",
    "max_hold_ms",
    "min_hold_ms",
    "avg_radio_wifi_ms",
    "nfc1_events",
    "nfc2_events",
    "interpretation",
]


@dataclass
class HoldWindow:
    test_id_down: int
    test_id_up: int
    slave_id: int
    nfc_index: int
    poll_ms: int
    remove_ms: int
    hold_ms: int
    down_recv_ms: int
    up_recv_ms: int


def load_rows(path: Path) -> List[Dict[str, str]]:
    text = path.read_text(encoding="utf-8-sig", errors="replace")
    header_index = text.find(HEADER)
    if header_index < 0:
        raise ValueError("CSV header not found in file")
    clean_text = text[header_index:]
    return list(csv.DictReader(clean_text.splitlines()))


def save_clean_csv(rows: List[Dict[str, str]], path: Path) -> None:
    with path.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=CSV_HEADER)
        writer.writeheader()
        writer.writerows(rows)


def as_int(row: Dict[str, str], key: str) -> int:
    return int(row[key])


def build_hold_windows(rows: List[Dict[str, str]]) -> List[HoldWindow]:
    rows_sorted = sorted(rows, key=lambda r: as_int(r, "Test_ID"))
    open_holds: Dict[Tuple[str, str, str, str, str], Dict[str, str]] = {}
    windows: List[HoldWindow] = []

    for row in rows_sorted:
        hold_key = (
            row["Slave_ID"],
            row["NFC_Index"],
            row["UID"],
            row["MasterPollIntervalMs"],
            row["RemoveTimeMs"],
        )

        if row["Event"] == "PICK-DOWN":
            open_holds[hold_key] = row
            continue

        if row["Event"] != "PICK-UP":
            continue

        down_row = open_holds.pop(hold_key, None)
        if down_row is None:
            continue

        down_ms = as_int(down_row, "T2_Recv")
        up_ms = as_int(row, "T2_Recv")
        windows.append(
            HoldWindow(
                test_id_down=as_int(down_row, "Test_ID"),
                test_id_up=as_int(row, "Test_ID"),
                slave_id=as_int(row, "Slave_ID"),
                nfc_index=as_int(row, "NFC_Index"),
                poll_ms=as_int(row, "MasterPollIntervalMs"),
                remove_ms=as_int(row, "RemoveTimeMs"),
                hold_ms=up_ms - down_ms,
                down_recv_ms=down_ms,
                up_recv_ms=up_ms,
            )
        )

    return windows


def make_label_ko(poll_ms: int, remove_ms: int) -> str:
    return f"마스터 폴링 주기 {poll_ms} / NFC 제거 인식 주기 {remove_ms}"


def interpret_summary(pickup_count: int, hold_success_20min_count: int, windows: List[HoldWindow]) -> str:
    if not windows:
        return "tag data is too short to treat as a pickdown hold test"
    if hold_success_20min_count > 0 and pickup_count == 0:
        return "looks like a stable pickdown hold run"
    if hold_success_20min_count == 0:
        return "looks like repeated manual tag/remove testing, not a true 20-minute pickdown hold run"
    return "contains mixed behavior; review manually"


def summarize_rows(rows: List[Dict[str, str]], windows: List[HoldWindow]) -> List[Dict[str, object]]:
    grouped_rows: Dict[Tuple[int, int], List[Dict[str, str]]] = defaultdict(list)
    grouped_windows: Dict[Tuple[int, int], List[HoldWindow]] = defaultdict(list)

    for row in rows:
        key = (as_int(row, "MasterPollIntervalMs"), as_int(row, "RemoveTimeMs"))
        grouped_rows[key].append(row)

    for window in windows:
        grouped_windows[(window.poll_ms, window.remove_ms)].append(window)

    summaries: List[Dict[str, object]] = []
    for poll_ms, remove_ms in sorted(grouped_rows.keys()):
        key_rows = grouped_rows[(poll_ms, remove_ms)]
        key_windows = grouped_windows.get((poll_ms, remove_ms), [])
        event_counts = Counter(row["Event"] for row in key_rows)
        nfc_counts = Counter(row["NFC_Index"] for row in key_rows)
        total_latency = [as_int(row, "Latency_Radio") + as_int(row, "Latency_Wifi") for row in key_rows]
        hold_ms_values = [window.hold_ms for window in key_windows]
        hold_success_20min_count = sum(1 for window in key_windows if window.hold_ms >= 20 * 60 * 1000)

        summaries.append(
            {
                "label_ko": make_label_ko(poll_ms, remove_ms),
                "master_poll_interval_ms": poll_ms,
                "nfc_remove_detect_ms": remove_ms,
                "total_events": len(key_rows),
                "pickdown_count": event_counts.get("PICK-DOWN", 0),
                "pickup_count": event_counts.get("PICK-UP", 0),
                "hold_windows": len(key_windows),
                "hold_success_20min_count": hold_success_20min_count,
                "avg_hold_ms": round(mean(hold_ms_values), 1) if hold_ms_values else "",
                "max_hold_ms": max(hold_ms_values) if hold_ms_values else "",
                "min_hold_ms": min(hold_ms_values) if hold_ms_values else "",
                "avg_radio_wifi_ms": round(mean(total_latency), 2) if total_latency else "",
                "nfc1_events": nfc_counts.get("1", 0),
                "nfc2_events": nfc_counts.get("2", 0),
                "interpretation": interpret_summary(event_counts.get("PICK-UP", 0), hold_success_20min_count, key_windows),
            }
        )

    return summaries


def save_summary_csv(summaries: List[Dict[str, object]], path: Path) -> None:
    if not summaries:
        return
    with path.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=SUMMARY_HEADER)
        writer.writeheader()
        writer.writerows(summaries)


def save_hold_windows_csv(windows: List[HoldWindow], path: Path) -> None:
    with path.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(
            f,
            fieldnames=[
                "test_id_down",
                "test_id_up",
                "slave_id",
                "nfc_index",
                "poll_ms",
                "remove_ms",
                "hold_ms",
                "down_recv_ms",
                "up_recv_ms",
            ],
        )
        writer.writeheader()
        for window in windows:
            writer.writerow(window.__dict__)


def save_summary_template(path: Path) -> None:
    if path.exists():
        return
    with path.open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=SUMMARY_HEADER)
        writer.writeheader()


def plot_single_run(windows: List[HoldWindow], out_path: Path) -> Optional[str]:
    if plt is None:
        return "matplotlib is not installed, so the graph was skipped."
    if not windows:
        return "No hold windows found to plot."

    grouped: Dict[Tuple[int, int], List[HoldWindow]] = defaultdict(list)
    for window in windows:
        grouped[(window.poll_ms, window.remove_ms)].append(window)

    fig, axes = plt.subplots(
        nrows=len(grouped),
        ncols=1,
        figsize=(12, max(4, 3.5 * len(grouped))),
        squeeze=False,
    )

    for ax, ((poll_ms, remove_ms), group_windows) in zip(axes.flatten(), sorted(grouped.items())):
        xs = list(range(1, len(group_windows) + 1))
        ys_sec = [window.hold_ms / 1000.0 for window in group_windows]
        colors = ["#2ca02c" if window.hold_ms >= 20 * 60 * 1000 else "#d62728" for window in group_windows]

        ax.bar(xs, ys_sec, color=colors)
        ax.axhline(20 * 60, color="#1f77b4", linestyle="--", linewidth=1.5, label="20 min target")
        ax.set_title(f"Master poll {poll_ms} ms / NFC remove detect {remove_ms} ms")
        ax.set_xlabel("Hold window index")
        ax.set_ylabel("Hold time (sec)")
        ax.legend()
        ax.grid(axis="y", linestyle=":", alpha=0.4)

    plt.tight_layout()
    fig.savefig(out_path, dpi=180)
    plt.close(fig)
    return None


def print_console_summary(rows: List[Dict[str, str]], summaries: List[Dict[str, object]], windows: List[HoldWindow]) -> None:
    ids = sorted(as_int(row, "Test_ID") for row in rows)
    missing_ids = [num for num in range(ids[0], ids[-1] + 1) if num not in ids]
    event_counts = Counter(row["Event"] for row in rows)
    hold_success_20min_count = sum(1 for window in windows if window.hold_ms >= 20 * 60 * 1000)

    print("=== CSV Inspection ===")
    print(f"raw file: {RAW_CSV_PATH}")
    print("top corruption cause: XLSX(zip) binary bytes are attached before the CSV header")
    print(f"clean row count: {len(rows)}")
    print(f"missing Test_ID: {missing_ids if missing_ids else 'none'}")
    print()

    print("=== Run Interpretation ===")
    print(f"PICK-DOWN count: {event_counts.get('PICK-DOWN', 0)}")
    print(f"PICK-UP count: {event_counts.get('PICK-UP', 0)}")
    print(f"hold windows: {len(windows)}")
    print(f"20-minute hold success windows: {hold_success_20min_count}")
    if windows:
        print(f"max hold time: {max(window.hold_ms for window in windows) / 1000:.1f} sec")
    print("current file looks like repeated manual tag/remove testing rather than a true 20-minute pickdown hold run")
    print()

    print("=== Notion Summary Rows ===")
    for summary in summaries:
        print(summary)


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    rows = load_rows(RAW_CSV_PATH)
    windows = build_hold_windows(rows)
    summaries = summarize_rows(rows, windows)

    clean_csv_path = OUTPUT_DIR / "esp_now_test_data_clean.csv"
    summary_csv_path = OUTPUT_DIR / "pickdown_summary_for_notion.csv"
    windows_csv_path = OUTPUT_DIR / "pickdown_hold_windows.csv"
    graph_path = OUTPUT_DIR / "pickdown_hold_graph.png"
    template_path = OUTPUT_DIR / "pickdown_summary_template.csv"

    save_clean_csv(rows, clean_csv_path)
    save_summary_csv(summaries, summary_csv_path)
    save_hold_windows_csv(windows, windows_csv_path)
    save_summary_template(template_path)
    plot_message = plot_single_run(windows, graph_path)

    print_console_summary(rows, summaries, windows)
    print()
    print("=== Generated Files ===")
    print(clean_csv_path)
    print(summary_csv_path)
    print(windows_csv_path)
    print(template_path)
    if plot_message:
        print(plot_message)
    else:
        print(graph_path)


if __name__ == "__main__":
    main()
