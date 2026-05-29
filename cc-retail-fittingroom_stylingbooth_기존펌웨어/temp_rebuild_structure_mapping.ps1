$suffix = ([string][char]0xAD6C) + ([string][char]0xC870) + ([string][char]0xD654) + '.html'
$target = Get-ChildItem -LiteralPath 'C:\WS\vs_kdh\pnk_kdh\report_html\styling_booth' |
  Where-Object { $_.Name.EndsWith($suffix) } |
  Select-Object -First 1
if (-not $target) {
  throw 'target html not found'
}

$path = $target.FullName
$text = Get-Content -LiteralPath $path -Raw -Encoding UTF8

$factorBlockPattern = '(?s)    const factorOrder = \[.*?const selectedTestCode = document\.getElementById\("selectedTestCode"\);'
$factorBlockReplacement = @'
    const factorOrder = [
      "옷 수량",
      "배치 방식",
      "벽걸이 위치",
      "바닥 위치",
      "벽과의 거리",
      "태그 방향",
      "안테나 위치",
      "천장 차폐",
      "안테나 RF 전력 세기"
    ];

    const factorOptions = {
      "옷 수량": ["1벌", "2벌", "3벌", "4벌", "5벌", "6벌", "가혹 7벌"],
      "배치 방식": ["벽걸이", "바닥 적층", "바닥 분산"],
      "벽걸이 위치": ["1", "2", "3", "4", "5", "6", "7"],
      "바닥 위치": ["좌", "중", "우"],
      "벽과의 거리": ["벽과 멂", "벽과 가까움"],
      "태그 방향": ["안테나와 마주봄", "안테나 반대 방향", "혼합"],
      "안테나 위치": ["앞-좌", "앞-중", "앞-우", "중-좌", "중-중", "중-우", "뒤-좌", "뒤-중", "뒤-우"],
      "천장 차폐": ["차폐 없음", "차폐 있음"],
      "안테나 RF 전력 세기": Array.from({ length: 28 }, (_, index) => `${index - 2} dBm`)
    };

    const normalizedCases = {
      "1-1": {
        buttonLabel: "7벌을 한 위치에 몰아 건 기준 밀집 테스트",
        summary: "7벌을 한 위치에 몰아 걸어 밀집이 가장 큰 기준 실패 상태를 만든 테스트입니다.",
        factors: { "옷 수량": "가혹 7벌", "배치 방식": "벽걸이", "벽걸이 위치": ["1"], "바닥 위치": [], "벽과의 거리": ["벽과 멂", "벽과 가까움"], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "1-2": {
        buttonLabel: "7벌을 1·4·7 위치로 분산 배치",
        summary: "7벌을 3-0-0-2-0-0-2로 나눠 걸어, 한 위치 밀집을 줄이면 회복되는지 본 분산 테스트입니다.",
        factors: { "옷 수량": "가혹 7벌", "배치 방식": "벽걸이", "벽걸이 위치": ["1", "4", "7"], "바닥 위치": [], "벽과의 거리": ["벽과 멂", "벽과 가까움"], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "1-3": {
        buttonLabel: "7벌을 여러 위치로 이어 분산 배치",
        summary: "7벌을 3-1-2-1 형태로 나눠 걸어, 벽걸이 위치 분산과 밀집 완화가 함께 작동하는지 본 테스트입니다.",
        factors: { "옷 수량": "가혹 7벌", "배치 방식": "벽걸이", "벽걸이 위치": ["1", "3", "5", "7"], "바닥 위치": [], "벽과의 거리": ["벽과 멂", "벽과 가까움"], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "1-9": {
        buttonLabel: "소재 추가 없는 5벌 기준 배치",
        summary: "5벌 기준 배치에서 추가 의류 없이 먼저 측정한 기준 테스트입니다.",
        factors: { "옷 수량": "5벌", "배치 방식": "벽걸이", "벽걸이 위치": ["4"], "바닥 위치": [], "벽과의 거리": ["벽과 멂", "벽과 가까움"], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "1-8": {
        buttonLabel: "기준 배치에서 바람막이만 추가",
        summary: "기존 기준 배치에 바람막이만 추가해, 가벼운 차폐 소재가 인식에 주는 변화를 본 테스트입니다.",
        factors: { "옷 수량": "5벌", "배치 방식": "벽걸이", "벽걸이 위치": ["1", "5", "6"], "바닥 위치": [], "벽과의 거리": ["벽과 멂", "벽과 가까움"], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "1-10": {
        buttonLabel: "기준 배치에서 청바지만 추가",
        summary: "기존 기준 배치에 청바지만 추가해, 두꺼운 의류가 밀집과 차폐에 주는 변화를 본 테스트입니다.",
        factors: { "옷 수량": "5벌", "배치 방식": "벽걸이", "벽걸이 위치": ["1", "3", "6"], "바닥 위치": [], "벽과의 거리": ["벽과 멂", "벽과 가까움"], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "1-11": {
        buttonLabel: "기준 배치에서 청바지와 바람막이 추가",
        summary: "기존 기준 배치에 청바지와 바람막이를 함께 추가해 가장 강한 차폐 조합을 본 테스트입니다.",
        factors: { "옷 수량": "5벌", "배치 방식": "벽걸이", "벽걸이 위치": ["1", "3", "5", "6"], "바닥 위치": [], "벽과의 거리": ["벽과 멂", "벽과 가까움"], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "1-6": {
        buttonLabel: "추가 의류 없는 4벌 기준 배치",
        summary: "4벌 기준 배치를 먼저 만들고, 추가 의류 없이 시작 성능을 확인한 기준 테스트입니다.",
        factors: { "옷 수량": "4벌", "배치 방식": "벽걸이", "벽걸이 위치": ["1", "6"], "바닥 위치": [], "벽과의 거리": ["벽과 멂", "벽과 가까움"], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "1-5": {
        buttonLabel: "4벌 기준에서 청바지만 추가",
        summary: "4벌 기준 배치에 청바지만 추가해, 위치는 유지한 채 의류만 바뀌는 경우를 비교한 테스트입니다.",
        factors: { "옷 수량": "5벌", "배치 방식": "벽걸이", "벽걸이 위치": ["1", "3", "6"], "바닥 위치": [], "벽과의 거리": ["벽과 멂", "벽과 가까움"], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "1-7": {
        buttonLabel: "4벌 기준에서 바람막이만 추가",
        summary: "4벌 기준 배치에 바람막이만 추가해, 위치는 유지한 채 차폐 소재만 바뀌는 경우를 본 테스트입니다.",
        factors: { "옷 수량": "5벌", "배치 방식": "벽걸이", "벽걸이 위치": ["1", "5", "6"], "바닥 위치": [], "벽과의 거리": ["벽과 멂", "벽과 가까움"], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "1-4": {
        buttonLabel: "4벌 기준에서 청바지와 바람막이 추가",
        summary: "4벌 기준 배치에 청바지와 바람막이를 함께 추가해 가장 가혹한 추가 의류 조합을 본 테스트입니다.",
        factors: { "옷 수량": "6벌", "배치 방식": "벽걸이", "벽걸이 위치": ["1", "3", "5", "6"], "바닥 위치": [], "벽과의 거리": ["벽과 멂", "벽과 가까움"], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "1-12": {
        buttonLabel: "추가 의류 없이 위치 조합만 변경",
        summary: "추가 의류 없이 벽걸이 위치 조합만 바꿔, 배치 구조 자체가 인식에 미치는 영향을 본 특수 배치 테스트입니다.",
        factors: { "옷 수량": "5벌", "배치 방식": "벽걸이", "벽걸이 위치": ["1", "2", "3", "6"], "바닥 위치": [], "벽과의 거리": ["벽과 멂", "벽과 가까움"], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "2-1": {
        buttonLabel: "최하단 19번만 정방향으로 둔 기준 적층",
        summary: "바닥 적층에서 최하단 19번 태그만 안테나와 마주보게 두고 나머지는 혼합으로 둔 기준 테스트입니다.",
        factors: { "옷 수량": "가혹 7벌", "배치 방식": "바닥 적층", "벽걸이 위치": [], "바닥 위치": ["중"], "벽과의 거리": [], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "2-2": {
        buttonLabel: "최하단 19번만 역방향으로 둔 적층",
        summary: "바닥 적층에서 최하단 19번 태그만 반대 방향으로 돌려 방향 민감도를 본 테스트입니다.",
        factors: { "옷 수량": "가혹 7벌", "배치 방식": "바닥 적층", "벽걸이 위치": [], "바닥 위치": ["중"], "벽과의 거리": [], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "2-3": {
        buttonLabel: "19번 제외 나머지만 정방향으로 둔 적층",
        summary: "19번을 제외한 나머지 태그만 안테나와 마주보게 두어, 특정 하단 태그가 전체 적층에 미치는 영향을 본 테스트입니다.",
        factors: { "옷 수량": "가혹 7벌", "배치 방식": "바닥 적층", "벽걸이 위치": [], "바닥 위치": ["중"], "벽과의 거리": [], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "2-5": {
        buttonLabel: "전체 태그를 모두 역방향으로 둔 적층",
        summary: "적층된 전체 태그를 모두 안테나 반대 방향으로 통일해 만든 가혹 적층 테스트입니다.",
        factors: { "옷 수량": "가혹 7벌", "배치 방식": "바닥 적층", "벽걸이 위치": [], "바닥 위치": ["중"], "벽과의 거리": [], "태그 방향": "안테나 반대 방향", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "2-6": {
        buttonLabel: "최상단 10번만 정방향으로 둔 적층",
        summary: "최상단 10번 태그만 안테나와 마주보게 두고 나머지는 혼합으로 둬 상단 열린 태그 효과를 본 테스트입니다.",
        factors: { "옷 수량": "가혹 7벌", "배치 방식": "바닥 적층", "벽걸이 위치": [], "바닥 위치": ["중"], "벽과의 거리": [], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "2-7": {
        buttonLabel: "19번만 QR이 보이게 둔 적층",
        summary: "19번 태그만 QR이 보이게 두고 나머지를 가려, 하단 한 장만 열린 경우를 확인한 테스트입니다.",
        factors: { "옷 수량": "가혹 7벌", "배치 방식": "바닥 적층", "벽걸이 위치": [], "바닥 위치": ["중"], "벽과의 거리": [], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "2-4": {
        buttonLabel: "7벌을 바닥에 랜덤 분산 배치",
        summary: "7벌을 바닥에 랜덤으로 펼쳐, 적층이 아닌 실제 혼합 상태에서 어떻게 읽히는지 본 테스트입니다.",
        factors: { "옷 수량": "가혹 7벌", "배치 방식": "바닥 분산", "벽걸이 위치": [], "바닥 위치": ["좌", "중", "우"], "벽과의 거리": [], "태그 방향": "혼합", "안테나 위치": "중-중", "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "1-5 new": {
        buttonLabel: "1번 후면 조건에서 안테나 위치만 비교",
        summary: "왼쪽 1번 위치의 후면 실패 조건을 유지한 채, 안테나 위치만 중-좌, 중-중, 중-우로 바꿔 본 비교 테스트입니다.",
        factors: { "옷 수량": "1벌", "배치 방식": "벽걸이", "벽걸이 위치": ["1"], "바닥 위치": [], "벽과의 거리": "벽과 가까움", "태그 방향": "안테나와 마주봄", "안테나 위치": ["중-좌", "중-중", "중-우"], "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "2-5 new": {
        buttonLabel: "4번 후면 조건에서 안테나 위치만 비교",
        summary: "중앙 4번 위치의 후면 실패 조건을 유지한 채, 안테나 위치만 중-좌, 중-중, 중-우로 바꿔 본 비교 테스트입니다.",
        factors: { "옷 수량": "1벌", "배치 방식": "벽걸이", "벽걸이 위치": ["4"], "바닥 위치": [], "벽과의 거리": "벽과 가까움", "태그 방향": "안테나와 마주봄", "안테나 위치": ["중-좌", "중-중", "중-우"], "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "3-2 new": {
        buttonLabel: "7번 정면 조건에서 안테나 위치만 비교",
        summary: "오른쪽 7번 위치의 정면 회복 조건을 유지한 채, 안테나 위치만 중-좌, 중-중, 중-우로 바꿔 본 비교 테스트입니다.",
        factors: { "옷 수량": "1벌", "배치 방식": "벽걸이", "벽걸이 위치": ["7"], "바닥 위치": [], "벽과의 거리": "벽과 멂", "태그 방향": "안테나와 마주봄", "안테나 위치": ["중-좌", "중-중", "중-우"], "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      },
      "3-6 new": {
        buttonLabel: "7번 후면 조건에서 안테나 위치만 비교",
        summary: "오른쪽 7번 위치의 후면 실패 조건을 유지한 채, 안테나 위치만 중-좌, 중-중, 중-우로 바꿔 본 비교 테스트입니다.",
        factors: { "옷 수량": "1벌", "배치 방식": "벽걸이", "벽걸이 위치": ["7"], "바닥 위치": [], "벽과의 거리": "벽과 가까움", "태그 방향": "안테나 반대 방향", "안테나 위치": ["중-좌", "중-중", "중-우"], "천장 차폐": "차폐 없음", "안테나 RF 전력 세기": "" }
      }
    };

    Object.entries(normalizedCases).forEach(([testId, patch]) => {
      if (!testCaseData[testId]) return;
      testCaseData[testId].summary = patch.summary;
      testCaseData[testId].factors = patch.factors;
      testCaseData[testId].buttonLabel = patch.buttonLabel;
    });

    const selectedTestCode = document.getElementById("selectedTestCode");
'@
$text = [regex]::Replace($text, $factorBlockPattern, $factorBlockReplacement)

$createPattern = '(?s)    function createFactorCard\(label, selectedValue, previewValue\) \{.*?      return card;\s*    \}'
$createReplacement = @'
    function normalizeToList(value) {
      if (Array.isArray(value)) return value.map(String);
      if (value === null || value === undefined || value === "") return [];
      return [String(value)];
    }

    function createFactorCard(label, selectedValue, previewValue) {
      const card = document.createElement("div");
      card.className = "factor-card";

      const title = document.createElement("div");
      title.className = "text-xs font-semibold uppercase tracking-[0.14em] text-slate-500";
      title.textContent = label;

      const chipWrap = document.createElement("div");
      chipWrap.className = "mt-3 flex flex-wrap gap-2";

      const options = factorOptions[label] || [];
      const selectedList = new Set(normalizeToList(selectedValue));
      const previewList = new Set(normalizeToList(previewValue));

      if (!options.length) {
        const fallback = document.createElement("div");
        fallback.className = "factor-value is-selected";
        fallback.textContent = normalizeToList(selectedValue).join(", ") || "해당 없음";
        chipWrap.appendChild(fallback);
      } else {
        options.forEach((option) => {
          const isSelected = selectedList.has(option);
          const isPreview = previewList.has(option);
          const chip = document.createElement("div");
          let chipClass = "factor-value";

          if (isSelected) chipClass += " is-selected button-mapped-highlight";
          if (isPreview) chipClass += " is-preview";
          if (isSelected && isPreview) chipClass += " is-match";

          chip.className = chipClass;
          chip.textContent = option;
          chipWrap.appendChild(chip);
        });
      }

      card.appendChild(title);
      card.appendChild(chipWrap);
      return card;
    }
'@
$text = [regex]::Replace($text, $createPattern, $createReplacement)

$buttonsAnchor = '    const buttons = Array.from(document.querySelectorAll(".test-case-button"));'
$buttonsPrefix = @'
    document.querySelectorAll(".test-case-button").forEach((button) => {
      const label = button.querySelector("p");
      const code = button.dataset.testId;
      if (label && testCaseData[code]?.buttonLabel) {
        label.textContent = testCaseData[code].buttonLabel;
      }
    });

    const buttons = Array.from(document.querySelectorAll(".test-case-button"));
'@
$text = $text.Replace($buttonsAnchor, $buttonsPrefix)

Set-Content -LiteralPath $path -Value $text -Encoding UTF8
