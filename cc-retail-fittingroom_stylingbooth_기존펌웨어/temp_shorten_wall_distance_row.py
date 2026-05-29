from pathlib import Path


TARGET = Path(r"C:\WS\vs_kdh\pnk_kdh\report_html\styling_booth\260528_강동현_RFID테스트케이스 구조화.html")


def main() -> None:
    text = TARGET.read_text(encoding="utf-8")
    old = """              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">2</td>
                <td class="px-4 py-3 font-semibold text-slate-900">벽 근접 거리 단계 테스트</td>
                <td class="px-4 py-3">벽과 태그의 거리만 바꿨을 때 인식 횟수가 얼마나 줄어드는지 정량화하기 위함입니다. 여기서는 5벌 전체의 모든 조합을 다 뒤집는 것이 아니라, 기존 결과에서 약했던 옷이나 벽 쪽에 붙은 옷부터 1벌씩 바꿔 영향이 어디서 커지는지 확인합니다.</td>
                <td class="px-4 py-3">같은 벽걸이 위치와 같은 5벌 조합을 유지한 채, 기준 배치에서 문제 의심 옷 1벌만 뒤집어 벽 근접 상태를 만들고, 필요하면 2벌까지 늘려 비교합니다. 즉 2^5 전체 경우의 수를 도는 실험이 아니라, 문제 옷 중심으로 단계적으로 바꿔 벽 근접 영향만 분리합니다.</td>
                <td class="px-4 py-3">안테나 위치는 1번에서 찾은 최고 후보로 고정, 5벌 수량 유지, 동일 의류 조합 유지, 벽걸이 위치는 동일하게 유지</td>
              </tr>"""
    new = """              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">2</td>
                <td class="px-4 py-3 font-semibold text-slate-900">벽 근접 거리 단계 테스트</td>
                <td class="px-4 py-3">벽과 태그의 거리만 바꿨을 때 인식 횟수가 얼마나 줄어드는지 확인하기 위함입니다.</td>
                <td class="px-4 py-3">같은 5벌 조합과 같은 벽걸이 위치를 유지한 채, 문제 의심 옷 1벌을 먼저 뒤집고 필요하면 2벌까지 늘려 비교합니다.</td>
                <td class="px-4 py-3">안테나 위치는 1번 최고 후보 고정, 5벌 유지, 동일 의류 조합 유지, 벽걸이 위치 동일</td>
              </tr>"""
    if old not in text:
        raise SystemExit("target row not found")
    TARGET.write_text(text.replace(old, new, 1), encoding="utf-8")
    print("shortened wall distance row")


if __name__ == "__main__":
    main()
