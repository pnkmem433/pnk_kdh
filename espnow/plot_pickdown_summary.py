import csv
import os
from pathlib import Path
from typing import List, Dict

try:
    MPL_DIR = Path(r"C:\WS\vs_kdh\pnk_kdh\espnow\.mplconfig")
    MPL_DIR.mkdir(parents=True, exist_ok=True)
    os.environ["MPLCONFIGDIR"] = str(MPL_DIR)
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
except ImportError:
    plt = None


SUMMARY_PATH = Path(r"C:\WS\vs_kdh\pnk_kdh\espnow\analysis_output\pickdown_summary_for_notion.csv")
OUT_PATH = Path(r"C:\WS\vs_kdh\pnk_kdh\espnow\analysis_output\pickdown_summary_overview.png")


def load_rows(path: Path) -> List[Dict[str, str]]:
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def main() -> None:
    if plt is None:
        print("matplotlib is not installed. Install it in .venv first.")
        return

    if not SUMMARY_PATH.exists():
        print(f"summary file not found: {SUMMARY_PATH}")
        return

    rows = load_rows(SUMMARY_PATH)
    if not rows:
        print("summary file is empty")
        return

    labels = [row["label_ko"] for row in rows]
    hold_success = [int(row["hold_success_20min_count"] or 0) for row in rows]
    pickup_count = [int(row["pickup_count"] or 0) for row in rows]
    avg_hold_sec = [float(row["avg_hold_ms"] or 0) / 1000.0 for row in rows]

    fig, axes = plt.subplots(3, 1, figsize=(14, 12), squeeze=False)
    axes = axes.flatten()

    axes[0].bar(labels, hold_success, color="#2ca02c")
    axes[0].set_title("20-minute pickdown success count by parameter")
    axes[0].set_ylabel("Success windows")
    axes[0].grid(axis="y", linestyle=":", alpha=0.4)

    axes[1].bar(labels, pickup_count, color="#d62728")
    axes[1].set_title("Unexpected PICK-UP count by parameter")
    axes[1].set_ylabel("PICK-UP count")
    axes[1].grid(axis="y", linestyle=":", alpha=0.4)

    axes[2].bar(labels, avg_hold_sec, color="#1f77b4")
    axes[2].set_title("Average hold time by parameter")
    axes[2].set_ylabel("Average hold time (sec)")
    axes[2].grid(axis="y", linestyle=":", alpha=0.4)

    for ax in axes:
        ax.tick_params(axis="x", rotation=20)

    plt.tight_layout()
    fig.savefig(OUT_PATH, dpi=180)
    plt.close(fig)
    print(OUT_PATH)


if __name__ == "__main__":
    main()
