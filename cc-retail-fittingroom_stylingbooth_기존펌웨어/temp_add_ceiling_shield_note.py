from pathlib import Path


TARGET = Path(r"C:\WS\vs_kdh\pnk_kdh\report_html\styling_booth\260528_강동현_RFID테스트케이스 구조화.html")


def main() -> None:
    text = TARGET.read_text(encoding="utf-8")
    old = """        </div>

        <div class="mt-6 overflow-x-auto rounded-[1.25rem] border border-slate-200">
          <table class="min-w-full divide-y divide-slate-200 text-sm">"""
    new = """        </div>

        <div class="mt-4 rounded-[1.25rem] border border-amber-200 bg-amber-50 px-5 py-4 text-sm leading-7 text-slate-700">
          <span class="font-bold text-slate-900">+ 추가 검토 변수:</span>
          <span class="font-semibold text-amber-900">천장 차폐</span>는 아직 실제로 분리 측정한 항목이 아니므로 현재 우선순위 1~4에는 넣지 않았습니다.
          다만 상부 안테나를 쓰는 구조상 영향 가능성은 있어, <span class="font-semibold text-slate-900">안테나 위치, 벽 근접, 벽걸이 위치, 밀집/분산 테스트를 끝낸 뒤</span>
          후순위 추가 테스트로 천장 차폐 유무를 따로 확인하는 흐름이 적절합니다.
        </div>

        <div class="mt-6 overflow-x-auto rounded-[1.25rem] border border-slate-200">
          <table class="min-w-full divide-y divide-slate-200 text-sm">"""
    if old not in text:
        raise SystemExit("anchor block not found")
    TARGET.write_text(text.replace(old, new, 1), encoding="utf-8")
    print("added ceiling shield note")


if __name__ == "__main__":
    main()
