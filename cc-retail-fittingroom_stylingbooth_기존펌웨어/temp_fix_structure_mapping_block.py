from pathlib import Path
import re


TARGET = Path(r"C:\WS\vs_kdh\pnk_kdh\report_html\styling_booth\260528_강동현_RFID테스트케이스 구조화.html")


REPLACEMENT = r'''
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
      "안테나 위치": ["좌", "중", "우", "앞", "뒤", "앞-좌", "앞-중", "앞-우", "중-좌", "중-중", "중-우", "뒤-좌", "뒤-중", "뒤-우"],
      "천장 차폐": ["차폐 없음", "차폐 있음"],
      "안테나 RF 전력 세기": Array.from({ length: 28 }, (_, index) => `${index - 2} dBm`)
    };

    const normalizedCases = {
      "1-1": {
        buttonLabel: "7벌을 한 벽걸이에 모두 몰아 건 기준 배치",
        summary: "7벌을 한 벽걸이에 모두 몰아 걸어 태그 밀집이 가장 큰 상태를 만든 기준 실패 테스트입니다.",
        factors: {
          "옷 수량": "가혹 7벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["1"],
          "바닥 위치": [],
          "벽과의 거리": "벽과 가까움",
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "1-2": {
        buttonLabel: "7벌을 1,4,7 위치로 나눠 분산 배치",
        summary: "7벌을 3-0-0-2-0-0-2로 나눠 걸어, 한 위치 밀집이 완화되면 얼마나 회복되는지 본 분산 테스트입니다.",
        factors: {
          "옷 수량": "가혹 7벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["1", "4", "7"],
          "바닥 위치": [],
          "벽과의 거리": ["벽과 멂", "벽과 가까움"],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "1-3": {
        buttonLabel: "7벌을 여러 벽걸이에 이어 분산 배치",
        summary: "7벌을 3-1-2-1로 이어서 분산해 밀집과 벽걸이 위치 효과를 함께 본 테스트입니다.",
        factors: {
          "옷 수량": "가혹 7벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["1", "3", "5", "7"],
          "바닥 위치": [],
          "벽과의 거리": ["벽과 멂", "벽과 가까움"],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "1-9": {
        buttonLabel: "소재 추가 없는 5벌 기준 배치",
        summary: "추가 의류 없이 5벌 기준 배치를 먼저 측정한 기준 테스트입니다.",
        factors: {
          "옷 수량": "5벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["4"],
          "바닥 위치": [],
          "벽과의 거리": "벽과 멂",
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "1-8": {
        buttonLabel: "기준 배치에서 바람막이만 추가",
        summary: "기준 5벌 배치에 바람막이만 추가해 가벼운 소재 차폐가 인식에 주는 변화를 본 테스트입니다.",
        factors: {
          "옷 수량": "5벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["1", "5", "6"],
          "바닥 위치": [],
          "벽과의 거리": ["벽과 멂", "벽과 가까움"],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "1-10": {
        buttonLabel: "기준 배치에서 청바지만 추가",
        summary: "기준 5벌 배치에 청바지만 추가해 두꺼운 소재가 만드는 차폐 변화를 본 테스트입니다.",
        factors: {
          "옷 수량": "5벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["1", "3", "6"],
          "바닥 위치": [],
          "벽과의 거리": ["벽과 멂", "벽과 가까움"],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "1-11": {
        buttonLabel: "기준 배치에서 청바지와 바람막이 추가",
        summary: "기준 5벌 배치에 청바지와 바람막이를 함께 추가해 가장 강한 소재 차폐 조합을 본 테스트입니다.",
        factors: {
          "옷 수량": "5벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["1", "3", "5", "6"],
          "바닥 위치": [],
          "벽과의 거리": ["벽과 멂", "벽과 가까움"],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "1-6": {
        buttonLabel: "추가 의류 없는 4벌 기준 배치",
        summary: "4벌 기준 배치를 만든 뒤 추가 의류 없이 시작 성능을 확인한 기준 테스트입니다.",
        factors: {
          "옷 수량": "4벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["1", "6"],
          "바닥 위치": [],
          "벽과의 거리": ["벽과 멂", "벽과 가까움"],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "1-5": {
        buttonLabel: "4벌 기준에서 청바지만 추가",
        summary: "4벌 기준 배치에서 청바지만 추가해 기존 위치는 유지한 채 소재만 바뀌는 경우를 비교한 테스트입니다.",
        factors: {
          "옷 수량": "5벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["1", "3", "6"],
          "바닥 위치": [],
          "벽과의 거리": ["벽과 멂", "벽과 가까움"],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "1-7": {
        buttonLabel: "4벌 기준에서 바람막이만 추가",
        summary: "4벌 기준 배치에서 바람막이만 추가해 위치는 그대로 두고 차폐 소재만 바뀌는 경우를 본 테스트입니다.",
        factors: {
          "옷 수량": "5벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["1", "5", "6"],
          "바닥 위치": [],
          "벽과의 거리": ["벽과 멂", "벽과 가까움"],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "1-4": {
        buttonLabel: "4벌 기준에서 청바지와 바람막이 추가",
        summary: "4벌 기준 배치에 청바지와 바람막이를 함께 추가한 가장 가혹한 추가 의류 조합 테스트입니다.",
        factors: {
          "옷 수량": "6벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["1", "3", "5", "6"],
          "바닥 위치": [],
          "벽과의 거리": ["벽과 멂", "벽과 가까움"],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "1-12": {
        buttonLabel: "추가 의류 없이 위치 조합만 변경",
        summary: "추가 의류 없이 벽걸이 위치 조합만 바꿔 배치 구조 자체가 인식에 미치는 영향을 본 특수 배치 테스트입니다.",
        factors: {
          "옷 수량": "5벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["1", "2", "3", "6"],
          "바닥 위치": [],
          "벽과의 거리": ["벽과 멂", "벽과 가까움"],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "2-1": {
        buttonLabel: "최하단 19번만 안테나와 마주보는 적층",
        summary: "바닥 적층에서 최하단 19번 태그만 안테나와 마주보게 두고 나머지는 혼합으로 둔 기준 테스트입니다.",
        factors: {
          "옷 수량": "가혹 7벌",
          "배치 방식": "바닥 적층",
          "벽걸이 위치": [],
          "바닥 위치": "중",
          "벽과의 거리": [],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "2-2": {
        buttonLabel: "최하단 19번만 안테나 반대 방향 적층",
        summary: "바닥 적층에서 최하단 19번 태그만 반대 방향으로 둬 방향 민감도를 본 테스트입니다.",
        factors: {
          "옷 수량": "가혹 7벌",
          "배치 방식": "바닥 적층",
          "벽걸이 위치": [],
          "바닥 위치": "중",
          "벽과의 거리": [],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "2-3": {
        buttonLabel: "19번 제외 나머지만 안테나와 마주보는 적층",
        summary: "19번을 제외한 나머지 태그만 안테나와 마주보게 둬 특정 하단 태그가 전체 적층에 미치는 영향을 본 테스트입니다.",
        factors: {
          "옷 수량": "가혹 7벌",
          "배치 방식": "바닥 적층",
          "벽걸이 위치": [],
          "바닥 위치": "중",
          "벽과의 거리": [],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "2-5": {
        buttonLabel: "전체 태그를 안테나 반대 방향으로 둔 적층",
        summary: "적층된 전체 태그를 모두 안테나 반대 방향으로 통일해 바닥 적층 가혹 조건을 만든 테스트입니다.",
        factors: {
          "옷 수량": "가혹 7벌",
          "배치 방식": "바닥 적층",
          "벽걸이 위치": [],
          "바닥 위치": "중",
          "벽과의 거리": [],
          "태그 방향": "안테나 반대 방향",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "2-6": {
        buttonLabel: "최상단 10번만 안테나와 마주보는 적층",
        summary: "최상단 10번 태그만 안테나와 마주보게 두고 나머지는 혼합으로 둬 상단 열린 태그 효과를 본 테스트입니다.",
        factors: {
          "옷 수량": "가혹 7벌",
          "배치 방식": "바닥 적층",
          "벽걸이 위치": [],
          "바닥 위치": "중",
          "벽과의 거리": [],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "2-7": {
        buttonLabel: "19번만 QR 보이게 둔 적층",
        summary: "19번 태그만 QR이 보이게 두고 나머지를 가려, 하단 한 장만 열린 경우를 확인한 테스트입니다.",
        factors: {
          "옷 수량": "가혹 7벌",
          "배치 방식": "바닥 적층",
          "벽걸이 위치": [],
          "바닥 위치": "중",
          "벽과의 거리": [],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "2-4": {
        buttonLabel: "7벌을 바닥에 랜덤 분산 배치",
        summary: "7벌을 바닥에 랜덤으로 분산시켜 적층이 아닌 실사용 혼합 상태를 본 테스트입니다.",
        factors: {
          "옷 수량": "가혹 7벌",
          "배치 방식": "바닥 분산",
          "벽걸이 위치": [],
          "바닥 위치": ["좌", "중", "우"],
          "벽과의 거리": [],
          "태그 방향": "혼합",
          "안테나 위치": "중-중",
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "1-5 new": {
        buttonLabel: "왼쪽 벽걸이 후면에서 안테나 위치만 비교",
        summary: "왼쪽 벽걸이 후면 조건을 유지한 채 안테나 위치만 바꿔 회복 폭을 비교한 테스트입니다.",
        factors: {
          "옷 수량": "1벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["1"],
          "바닥 위치": [],
          "벽과의 거리": "벽과 가까움",
          "태그 방향": "안테나와 마주봄",
          "안테나 위치": ["중-좌", "중-중", "중-우"],
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "2-5 new": {
        buttonLabel: "중앙 벽걸이 후면에서 안테나 위치만 비교",
        summary: "중앙 벽걸이 후면 조건을 유지한 채 안테나 위치만 바꿔 중앙축 상쇄 완화 여부를 본 테스트입니다.",
        factors: {
          "옷 수량": "1벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["4"],
          "바닥 위치": [],
          "벽과의 거리": "벽과 가까움",
          "태그 방향": "안테나와 마주봄",
          "안테나 위치": ["중-좌", "중-중", "중-우"],
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "3-2 new": {
        buttonLabel: "오른쪽 벽걸이 정면에서 안테나 위치만 비교",
        summary: "오른쪽 벽걸이 정면 조건을 유지한 채 안테나 위치만 바꿔 회복 폭을 비교한 테스트입니다.",
        factors: {
          "옷 수량": "1벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["7"],
          "바닥 위치": [],
          "벽과의 거리": "벽과 멂",
          "태그 방향": "안테나와 마주봄",
          "안테나 위치": ["중-좌", "중-중", "중-우"],
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      },
      "3-6 new": {
        buttonLabel: "오른쪽 벽걸이 후면에서 안테나 위치만 비교",
        summary: "오른쪽 벽걸이 후면 조건을 유지한 채 안테나 위치만 바꿔 회복 한계를 비교한 테스트입니다.",
        factors: {
          "옷 수량": "1벌",
          "배치 방식": "벽걸이",
          "벽걸이 위치": ["7"],
          "바닥 위치": [],
          "벽과의 거리": "벽과 가까움",
          "태그 방향": "안테나 반대 방향",
          "안테나 위치": ["중-좌", "중-중", "중-우"],
          "천장 차폐": "차폐 없음",
          "안테나 RF 전력 세기": ""
        }
      }
    };

    Object.entries(normalizedCases).forEach(([testId, patch]) => {
      if (!testCaseData[testId]) return;
      testCaseData[testId].summary = patch.summary;
      testCaseData[testId].factors = patch.factors;
      testCaseData[testId].buttonLabel = patch.buttonLabel;
    });

'''


def main():
    text = TARGET.read_text(encoding="utf-8")

    pattern = re.compile(
        r"\n\s*const factorOrder = \[\n.*?\n\s*const selectedTestCode = document\.getElementById\(\"selectedTestCode\"\);",
        re.S,
    )

    replacement = "\n" + REPLACEMENT + "\n    const selectedTestCode = document.getElementById(\"selectedTestCode\");"
    new_text, count = pattern.subn(replacement, text, count=1)
    if count != 1:
        raise RuntimeError("Failed to replace factor mapping block")

    old_conditions_pattern = re.compile(
        r"""      selectedTestConditions\.innerHTML = "";\n      selectedTest\.conditions\.forEach\(\(condition\) => \{\n        const chip = document\.createElement\("span"\);\n        chip\.className = "condition-chip";\n        chip\.textContent = condition;\n        selectedTestConditions\.appendChild\(chip\);\n      \}\);\n\n      if \(previewTest\) \{\n        previewTest\.conditions\.forEach\(\(condition\) => \{\n          const chip = document\.createElement\("span"\);\n          chip\.className = "condition-chip is-preview";\n          chip\.textContent = `미리보기: \$\{condition\}`;\n          selectedTestConditions\.appendChild\(chip\);\n        \}\);\n      \}""",
        re.S,
    )

    new_conditions = '''      selectedTestConditions.innerHTML = "";
      factorOrder.forEach((label) => {
        const selectedValues = normalizeToList(selectedTest.factors[label]);
        if (!selectedValues.length) return;

        const chip = document.createElement("span");
        chip.className = "condition-chip";
        chip.textContent = `${label}: ${selectedValues.join(", ")}`;
        selectedTestConditions.appendChild(chip);
      });

      if (previewTest) {
        factorOrder.forEach((label) => {
          const previewValues = normalizeToList(previewTest.factors[label]);
          if (!previewValues.length) return;

          const chip = document.createElement("span");
          chip.className = "condition-chip is-preview";
          chip.textContent = `미리보기 ${label}: ${previewValues.join(", ")}`;
          selectedTestConditions.appendChild(chip);
        });
      }'''

    new_text, count = old_conditions_pattern.subn(new_conditions, new_text, count=1)
    if count not in (0, 1):
        raise RuntimeError("Unexpected condition rendering replacement count")

    new_text = new_text.replace('?대떦 ?놁쓬', '해당 없음')

    TARGET.write_text(new_text, encoding="utf-8")


if __name__ == "__main__":
    main()
