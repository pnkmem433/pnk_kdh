from pathlib import Path


TARGET = Path(r"C:\WS\vs_kdh\pnk_kdh\report_html\styling_booth\260528_강동현_RFID테스트케이스 구조화.html")


def main() -> None:
    text = TARGET.read_text(encoding="utf-8")

    text = text.replace(
        '<span class="rounded-full bg-rose-100 px-3 py-1 text-xs font-semibold text-rose-700">1-8 ~ 1-11</span>',
        '<span class="rounded-full bg-sky-100 px-3 py-1 text-xs font-semibold text-sky-700">1-8 ~ 1-11</span>',
    )

    text = text.replace(
        '"안테나 RF 전력 세기"',
        '"전력 세기(-2~25 dBm)"',
    )
    text = text.replace(
        '"전력 세기(-2~25 dBm)": Array.from({ length: 28 }, (_, index) => `${index - 2} dBm`)',
        '"전력 세기(-2~25 dBm)": ["조절 안함", "조절함"]',
    )
    text = text.replace(
        '"전력 세기(-2~25 dBm)": ""',
        '"전력 세기(-2~25 dBm)": "조절 안함"',
    )

    text = text.replace(
        'previewTestCode.textContent = `미리보기: ${hoveredTestId}`;',
        'previewTestCode.textContent = hoveredTestId;',
    )

    TARGET.write_text(text, encoding="utf-8")
    print("updated preview label, materials badge, and power factor")


if __name__ == "__main__":
    main()
