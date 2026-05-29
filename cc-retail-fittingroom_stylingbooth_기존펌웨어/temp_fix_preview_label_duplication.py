from pathlib import Path


TARGET = Path(r"C:\WS\vs_kdh\pnk_kdh\report_html\styling_booth\260528_강동현_RFID테스트케이스 구조화.html")


def main() -> None:
    text = TARGET.read_text(encoding="utf-8")
    text = text.replace(
        '<span class="rounded-full bg-amber-100 px-3 py-1 text-[11px] font-semibold text-amber-800">&#48120;&#47532;&#48372;&#44592;</span>\n                    <span id="previewTestCode" class="font-black"></span>',
        '<span id="previewTestCode" class="font-black"></span>',
    )
    text = text.replace(
        'previewTestCode.textContent = hoveredTestId;',
        'previewTestCode.textContent = `미리 보기: ${hoveredTestId}`;',
    )
    TARGET.write_text(text, encoding="utf-8")
    print("fixed preview label duplication")


if __name__ == "__main__":
    main()
