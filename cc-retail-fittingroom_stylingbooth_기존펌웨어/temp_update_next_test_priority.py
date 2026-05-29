from pathlib import Path


TARGET = Path(r"C:\WS\vs_kdh\pnk_kdh\report_html\styling_booth\260528_강동현_RFID테스트케이스 구조화.html")


OLD = """        <div class="mt-6 overflow-x-auto rounded-[1.25rem] border border-slate-200">
          <table class="min-w-full divide-y divide-slate-200 text-sm">
            <thead class="bg-slate-50 text-slate-800">
              <tr>
                <th class="px-4 py-3 text-left font-bold">영향 우선순위</th>
                <th class="px-4 py-3 text-left font-bold">핵심 변수</th>
                <th class="px-4 py-3 text-left font-bold">왜 먼저 봐야 하는가</th>
              </tr>
            </thead>
            <tbody class="divide-y divide-slate-100 bg-white text-slate-700">
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">1</td>
                <td class="px-4 py-3 font-semibold text-slate-900">안테나 위치</td>
                <td class="px-4 py-3">같은 실패 케이스도 안테나 위치만 바꾸면 0회에서 66~69회까지 회복된 사례가 있어 가장 큰 회복 변수로 확인됨.</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">2</td>
                <td class="px-4 py-3 font-semibold text-slate-900">태그 위치 / 배치 구조</td>
                <td class="px-4 py-3">벽 근접, 중앙 null point, 한 벽걸이 밀집, 모서리 배치에서 실패가 반복돼 공간 구조 자체의 음영 구간 영향이 큼.</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">3</td>
                <td class="px-4 py-3 font-semibold text-slate-900">태그 방향</td>
                <td class="px-4 py-3">후면, 세움, 뒤집힘 조건에서 급락이 반복됐고, 같은 위치에서도 방향이 바뀌면 0~수십 회까지 크게 차이남.</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">4</td>
                <td class="px-4 py-3 font-semibold text-slate-900">태그 밀집 / 중첩</td>
                <td class="px-4 py-3">한 벽걸이 밀집과 바닥 적층에서 일부 태그만 살아남고 약한 태그가 먼저 매몰되어 전체 편차가 크게 벌어짐.</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">5</td>
                <td class="px-4 py-3 font-semibold text-slate-900">벽 근접 훅 / 좌우 구조 차이</td>
                <td class="px-4 py-3">왼쪽, 중앙, 오른쪽 벽걸이이 동일 성능을 보이지 않았고, 벽 반사와 후면 조건이 겹칠 때 실패가 반복됐음.</td>
              </tr>
            </tbody>
          </table>
        </div>

        <div class="mt-6 overflow-x-auto rounded-[1.25rem] border border-slate-200">
          <table class="min-w-full divide-y divide-slate-200 text-sm">
            <thead class="bg-sky-50 text-slate-800">
              <tr>
                <th class="px-4 py-3 text-left font-bold">우선순위</th>
                <th class="px-4 py-3 text-left font-bold">테스트</th>
                <th class="px-4 py-3 text-left font-bold">검증 목적</th>
                <th class="px-4 py-3 text-left font-bold">수행 조건</th>
                <th class="px-4 py-3 text-left font-bold">고정 변수</th>
              </tr>
            </thead>
            <tbody class="divide-y divide-slate-100 bg-white text-slate-700 align-top">
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">1</td>
                <td class="px-4 py-3 font-semibold text-slate-900">안테나 오른쪽 축 우선 비교</td>
                <td class="px-4 py-3">기존 결과에서 오른쪽이 더 자주 유리했으므로, 가장 회복 가능성이 큰 축에서 먼저 최적점을 찾기 위함.</td>
                <td class="px-4 py-3">안테나 위치를 앞-오, 중-오, 뒤-오로 바꿔 비교하고 기존 위치와 성능 차이 확인</td>
                <td class="px-4 py-3">5벌, 벽걸이, 중앙훅, 태그 방향 혼합, 실제 사용 조건 유지</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">2</td>
                <td class="px-4 py-3 font-semibold text-slate-900">중앙축 상쇄 해소 확인</td>
                <td class="px-4 py-3">중앙이 원래 나빴던 것이 중앙축 전체 문제인지, 중-중 한 점의 상쇄 문제인지를 분리하기 위함.</td>
                <td class="px-4 py-3">안테나 위치를 앞-중, 중-중, 뒤-중으로 바꿔 비교</td>
                <td class="px-4 py-3">5벌, 벽걸이, 중앙훅, 태그 방향 혼합, 실제 사용 조건 유지</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">3</td>
                <td class="px-4 py-3 font-semibold text-slate-900">5벌 실패 대표 케이스 재검증</td>
                <td class="px-4 py-3">기존에 실제로 실패했던 5벌 조건이 안테나 위치 변경만으로 5회 이상 통과하는지 직접 확인.</td>
                <td class="px-4 py-3">후면, 벽 근접, 중앙훅 약한 조건, 한 벽걸이 밀집 등 대표 실패 케이스 2~3개를 후보 안테나 위치별로 반복 비교</td>
                <td class="px-4 py-3">5벌, 기존 실패 배치 유지, 실제 사용 조건 유지</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">4</td>
                <td class="px-4 py-3 font-semibold text-slate-900">5벌 분산 vs 밀집 비교</td>
                <td class="px-4 py-3">안테나 최적 후보에서도 밀집 자체가 근본 문제인지, 위치 변경만으로 완화되는지 확인.</td>
                <td class="px-4 py-3">한 벽걸이 밀집, 3-0-0-2-0-0-2 벽걸이, 연속 벽걸이 분산, 중앙 벽걸이 집중 배치를 비교</td>
                <td class="px-4 py-3">5벌, 안테나 위치는 1~2순위 최고 후보, 실제 사용 조건 유지</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">5</td>
                <td class="px-4 py-3 font-semibold text-slate-900">벽 근접 / 벽걸이 위치 재검증</td>
                <td class="px-4 py-3">좋은 안테나 위치를 써도 특정 훅이 계속 5회 미만이면 구조적 취약 구간으로 판단하기 위함.</td>
                <td class="px-4 py-3">왼쪽, 중앙, 오른쪽 벽걸이에서 정면/후면 대표 실패 방향을 비교</td>
                <td class="px-4 py-3">5벌, 안테나 위치는 최적 후보 고정, 실제 사용 조건 유지</td>
              </tr>
            </tbody>
          </table>
        </div>

        <div class="mt-6 rounded-[1.25rem] border border-sky-200 bg-sky-50 px-5 py-4 text-sm leading-7 text-slate-700">
          <span class="font-bold text-slate-900">정리:</span>
          지금 단계에서는 <span class="font-semibold text-sky-800">오른쪽 축 안테나 위치 → 중앙축 상쇄 확인 → 5벌 실패 케이스 회복 → 밀집 완화 확인 → 훅 구조 재검증</span> 순으로 좁혀가는 것이 가장 효율적입니다.
          즉 원리 확인보다 먼저, 기존에 안 됐던 실제 조건이 회복되는지를 우선 확인해야 합니다.
        </div>"""


NEW = """        <div class="mt-6 overflow-x-auto rounded-[1.25rem] border border-slate-200">
          <table class="min-w-full divide-y divide-slate-200 text-sm">
            <thead class="bg-slate-50 text-slate-800">
              <tr>
                <th class="px-4 py-3 text-left font-bold">영향 우선순위</th>
                <th class="px-4 py-3 text-left font-bold">핵심 변수</th>
                <th class="px-4 py-3 text-left font-bold">왜 먼저 봐야 하는가</th>
              </tr>
            </thead>
            <tbody class="divide-y divide-slate-100 bg-white text-slate-700">
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">1</td>
                <td class="px-4 py-3 font-semibold text-slate-900">안테나 위치</td>
                <td class="px-4 py-3">같은 실패 케이스도 안테나 위치만 바꾸면 0.00회에서 66.33~69.00회까지 회복된 사례가 있어, 현재 확인된 가장 큰 회복 변수입니다.</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">2</td>
                <td class="px-4 py-3 font-semibold text-slate-900">벽과 태그의 거리</td>
                <td class="px-4 py-3">1벌 테스트에서 같은 왼쪽 벽걸이 조건도 정면은 60.33~65.33회였지만 후면은 29.33회 또는 0.00회까지 떨어져, 벽 근접 영향이 매우 크게 나타났습니다.</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">3</td>
                <td class="px-4 py-3 font-semibold text-slate-900">벽걸이 위치</td>
                <td class="px-4 py-3">왼쪽, 중앙, 오른쪽 벽걸이가 같은 성능을 보이지 않았고, 중앙 취약 구간과 좌우 구조 차이가 반복적으로 관찰돼 위치 자체가 큰 변수로 확인됐습니다.</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">4</td>
                <td class="px-4 py-3 font-semibold text-slate-900">태그 밀집 / 분산</td>
                <td class="px-4 py-3">5벌 이상 조건에서 한 벽걸이 밀집은 평균 2.33회, 분산 배치는 20.00회, 연속 분산은 51.00회까지 올라가 배치만으로도 인식 횟수 차이가 크게 벌어졌습니다.</td>
              </tr>
            </tbody>
          </table>
        </div>

        <div class="mt-6 overflow-x-auto rounded-[1.25rem] border border-slate-200">
          <table class="min-w-full divide-y divide-slate-200 text-sm">
            <thead class="bg-sky-50 text-slate-800">
              <tr>
                <th class="px-4 py-3 text-left font-bold">우선순위</th>
                <th class="px-4 py-3 text-left font-bold">테스트</th>
                <th class="px-4 py-3 text-left font-bold">검증 목적</th>
                <th class="px-4 py-3 text-left font-bold">수행 조건</th>
                <th class="px-4 py-3 text-left font-bold">고정 변수</th>
              </tr>
            </thead>
            <tbody class="divide-y divide-slate-100 bg-white text-slate-700 align-top">
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">1</td>
                <td class="px-4 py-3 font-semibold text-slate-900">안테나 위치 3x2 정밀 스윕</td>
                <td class="px-4 py-3">현재 가장 큰 회복 변수인 안테나 위치에서 먼저 최적 구간을 찾기 위함입니다.</td>
                <td class="px-4 py-3">중앙축(앞-중, 중-중, 뒤-중)과 오른쪽축(앞-우, 중-우, 뒤-우)을 같은 5벌 기준 배치에서 비교</td>
                <td class="px-4 py-3">5벌, 벽걸이, 중앙 벽걸이 기준 배치, 실제 사용 조건 유지</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">2</td>
                <td class="px-4 py-3 font-semibold text-slate-900">벽 근접 거리 단계 테스트</td>
                <td class="px-4 py-3">벽과 태그의 거리만 바꿨을 때 인식 횟수가 얼마나 줄어드는지 정량화하기 위함입니다.</td>
                <td class="px-4 py-3">같은 벽걸이 위치에서 태그가 벽과 가까운 조건과 먼 조건을 나눠 반복 측정</td>
                <td class="px-4 py-3">안테나 위치는 1순위 최고 후보 고정, 5벌, 동일 의류 조합 유지</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">3</td>
                <td class="px-4 py-3 font-semibold text-slate-900">벽걸이 위치 구조 재검증</td>
                <td class="px-4 py-3">좋은 안테나 위치를 써도 특정 벽걸이만 계속 약하면 구조적 취약 구간으로 판단하기 위함입니다.</td>
                <td class="px-4 py-3">왼쪽, 중앙, 오른쪽 벽걸이에서 같은 5벌 배치와 같은 거리 조건을 맞춰 비교</td>
                <td class="px-4 py-3">안테나 위치 최적 후보, 태그 수량 동일, 실제 사용 조건 유지</td>
              </tr>
              <tr>
                <td class="px-4 py-3 font-bold text-slate-900">4</td>
                <td class="px-4 py-3 font-semibold text-slate-900">밀집 vs 분산 재검증</td>
                <td class="px-4 py-3">좋은 안테나 위치와 거리 조건을 써도 밀집 자체가 근본 문제인지 확인하기 위함입니다.</td>
                <td class="px-4 py-3">한 벽걸이 밀집, 3-0-0-2-0-0-2 배치, 3-1-2-1 분산, 중앙 집중 배치를 같은 안테나 위치에서 비교</td>
                <td class="px-4 py-3">5벌, 안테나 위치 최적 후보, 벽걸이 위치 비교 기준 고정</td>
              </tr>
            </tbody>
          </table>
        </div>

        <div class="mt-6 rounded-[1.25rem] border border-sky-200 bg-sky-50 px-5 py-4 text-sm leading-7 text-slate-700">
          <span class="font-bold text-slate-900">정리:</span>
          지금 단계에서는 <span class="font-semibold text-sky-800">안테나 위치 최적화 → 벽 근접 거리 정량화 → 벽걸이 위치 구조 확인 → 밀집/분산 재검증</span> 순으로 좁혀가는 것이 가장 효율적입니다.
          즉 먼저 가장 크게 회복되는 축을 찾고, 그다음 실제 감소폭이 큰 구조 변수들을 순서대로 분리해서 확인해야 합니다.
        </div>"""


def main() -> None:
    text = TARGET.read_text(encoding="utf-8")
    if OLD not in text:
        raise SystemExit("target block not found")
    TARGET.write_text(text.replace(OLD, NEW, 1), encoding="utf-8")
    print("updated nextTestPriority section")


if __name__ == "__main__":
    main()
