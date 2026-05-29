const PHASE2_ONE_CLOTH_GOOD = "https://drive.google.com/open?id=1Xe9gSTrmLtNsbHubPLz58weDG3_fNVlF&usp=drive_copy";
const PHASE2_ONE_CLOTH_BAD = "https://drive.google.com/open?id=1uGojWkj_4wrGKeFj2K5s2oua6vZfj-y8&usp=drive_copy";

const PHASE2_MULTI_HOOK_GOOD = "https://drive.google.com/open?id=1o8u69GZ_E84eb8JCEC1Te2-X14CFyL4k&usp=drive_copy";
const PHASE2_MULTI_HOOK_BAD = "https://drive.google.com/open?id=1qC4GbwiHpsMAam7muAYJLQjcn_T-pb1I&usp=drive_copy";

const PHASE2_MULTI_FLOOR_STACK = "https://drive.google.com/open?id=1YRlgpVF8TWD9KtEhfFLIemratazScxK3&usp=drive_copy";
const PHASE2_MULTI_FLOOR_RANDOM = "https://drive.google.com/open?id=1mIDcb3hdXz7xRAlHs7FUmHEtsi74Gii1&usp=drive_copy";

const CAUSE_WALL_IMAGE = "https://drive.google.com/open?id=1vrPW7bYWMu2DZkDTX6-W1tC084B2FzXj&usp=drive_copy";
const CAUSE_OVERLAP_IMAGE = "https://drive.google.com/open?id=1UTnrpIEtu5FHFjXaWDodexOaY49QHFBm&usp=drive_copy";
const CAUSE_STACK_IMAGE = "https://drive.google.com/open?id=1YRlgpVF8TWD9KtEhfFLIemratazScxK3&usp=drive_copy";
const CAUSE_SINGLE_HOOK_IMAGE = "https://drive.google.com/open?id=1qC4GbwiHpsMAam7muAYJLQjcn_T-pb1I&usp=drive_copy";

const TAG_FACE_TO_ANTENNA = "https://drive.google.com/open?id=1Xe9gSTrmLtNsbHubPLz58weDG3_fNVlF&usp=drive_copy";
const TAG_FACE_AWAY_FROM_ANTENNA = "https://drive.google.com/open?id=17av383q4V9ZV2o5xVKzRhIOpr8Aqrznq&usp=drive_copy";
const CENTER_QR_VISIBLE = "https://drive.google.com/open?id=1D7KF854iXbPgGKzirPwDK6qJG6c4NIo-&usp=drive_copy";
const CENTER_QR_HIDDEN = "https://drive.google.com/open?id=1MZ09_-CJ6eOu4ExWGBazHxVgD9yg8DhE&usp=drive_copy";
const RIGHT_QR_VISIBLE = "https://drive.google.com/open?id=1QLyhIJFlYfRH3bbnZcbYFjxzv-Mf-Soj&usp=drive_copy";
const RIGHT_QR_HIDDEN = "https://drive.google.com/open?id=1zQIIvjEv5_A6kCMysi2PJvFt1Te2VAKs&usp=drive_copy";

const ANTENNA_CASE_1_5 = "https://drive.google.com/open?id=1uGojWkj_4wrGKeFj2K5s2oua6vZfj-y8&usp=drive_copy";
const ANTENNA_CASE_2_5 = "https://drive.google.com/open?id=1vvL1lq80q-vmvmEpJFWiSDFVJGkZXRbv&usp=drive_copy";
const ANTENNA_CASE_3_2 = "https://drive.google.com/open?id=1zQIIvjEv5_A6kCMysi2PJvFt1Te2VAKs&usp=drive_copy";
const ANTENNA_CASE_3_6 = "https://drive.google.com/open?id=1cg94OtqyY1oLFgi3yy0raX_rufd9cWJb&usp=drive_copy";
const ANTENNA_POS_BASE = "https://drive.google.com/open?id=17zp03CAWdLdY_y9AOiwJfH5i3t9EPwkt&usp=drive_copy";
const ANTENNA_POS_LEFT = "https://drive.google.com/open?id=1pcoyvJxW4QSleC7Yhubp1dYGCVD5KgtT&usp=drive_copy";
const ANTENNA_POS_CENTER = "https://drive.google.com/open?id=1Nq7xv1ZQXhqTFCfPn31WUAIckDxk0y2q&usp=drive_copy";
const ANTENNA_POS_RIGHT = "https://drive.google.com/open?id=1bIvf7AwemEj2cv0Ywm7MOP_VVoQu7B-X&usp=drive_copy";

export const reportMeta = {
  badge: "RFID Styling Booth Report",
  title: "RFID 테스트 결과 요약",
  subtitle: "목표: 사용자가 옷 여러 벌을 들고 들어왔을 때 모든 태그가 안정적으로 읽히는가를 판단",
};

export const topIntro = [
  "핵심 질문 1. 언제 잘 읽히는가",
  "핵심 질문 2. 언제 안 읽히는가",
  "핵심 질문 3. 지금 무엇을 먼저 바꿔야 하는가",
];

export const causeFlow = {
  root: "RFID 인식 불량",
  groups: [
    { title: "벽 간섭", items: ["벽 근접 훅", "벽 반사 / 상쇄"] },
    { title: "태그 방향 / 안테나 각도", items: ["후면 배치", "세워진 태그 + 상부 안테나"] },
    { title: "태그 밀집 / 중첩", items: ["한 훅 밀집", "바닥 적층", "태그 간 결합 / 상쇄"] },
    { title: "사람 차폐", items: ["사람 서있기", "몸 가까이 태그"] },
    { title: "안테나 위치 불일치", items: ["태그를 비스듬히 봄", "중앙 상쇄 가능성"] },
  ],
};

export const topSummaryCards = [
  {
    label: "결론",
    value: "재질보다 배치",
    tone: "bad",
    text: "주요 변수: 벽, 방향, 밀집, 사람, 안테나 위치",
  },
  {
    label: "가장 큰 회복 변수",
    value: "안테나 위치",
    tone: "info",
    text: "최소 0.00회 ~ 최대 69.00회",
  },
  {
    label: "현재 우선 조치",
    value: "출력 세기 테스트",
    tone: "good",
    text: "구조 변경보다 SDK 기반 출력 세기 조절 우선",
  },
];

export const impactRows = [
  {
    category: "옷 재질",
    condition: "재질 추가",
    delta: "약 0~10% 감소, 일부 증가",
    impact: "⚪ 영향 거의 없음",
    tone: "neutral",
    reason: "옷감보다 태그 배치·방향 영향이 큼",
  },
  {
    category: "훅 위치/배치",
    condition: "밀집/분산 배치",
    delta: "각 태그 평균 인식 횟수 기준, 최소 0.00회 ~ 최대 54.67회",
    impact: "🔴 영향 큼",
    tone: "bad",
    reason: "벽 간섭 + 방향 불일치 -> 약한 태그 소멸",
  },
  {
    category: "태그 중첩",
    condition: "적층 / 랜덤 배치",
    delta: "각 태그 평균 인식 횟수 기준, 최소 0.00회 ~ 최대 72.67회",
    impact: "🔴 영향 큼",
    tone: "bad",
    reason: "같은 방향 보강 -> 반대 방향 태그 매몰",
  },
  {
    category: "사람 존재",
    condition: "사람 서있음",
    delta: "약 5.33~22.33회 감소",
    impact: "🔴 영향 보통",
    tone: "warn",
    reason: "인체 차폐 -> 원래 약한 태그부터 하락",
  },
  {
    category: "사람-태그 근접",
    condition: "몸 가까이 댐",
    delta: "약 0.00~27.67회 감소, 일부 소폭 회복",
    impact: "⚪ 영향 작음",
    tone: "neutral",
    reason: "가까워지면 나빠질 수는 있지만 영향이 아주 크지는 않음",
  },
  {
    category: "안테나 위치",
    condition: "왼쪽/중앙/오른쪽 재배치",
    delta: "각 테스트 태그 평균 인식 횟수 기준, 최소 0.00회 ~ 최대 69.00회",
    impact: "🔴 영향 큼",
    tone: "bad",
    reason: "위치가 맞으면 크게 회복, 안 맞으면 0회 유지",
  },
];

export const issueCauseCards = [
  {
    step: "1",
    title: "벽과 너무 가까워 벽 간섭이 커지는 경우",
    summary: "요약: 훅이 벽에 붙고 태그 뒤에 벽이 있으면 반사와 상쇄가 같이 생긴다.",
    detail:
      "대표 실패: 왼쪽훅 후면 8.59 -> 0.00회. 벽 가까운 훅일수록 정면/후면 차이가 커지고, 약한 태그부터 먼저 사라진다.",
    images: [
      {
        label: "벽 근접 훅",
        summary: "벽면과 태그가 가까운 구조",
        src: CAUSE_WALL_IMAGE,
      },
    ],
  },
  {
    step: "2",
    title: "사람이 부스 안에 들어와 전파를 가로막는 경우",
    summary: "요약: 인체가 전파를 일부 흡수·차단해 전체 인식 여유를 줄인다.",
    detail:
      "사람이 있으면 태그 평균이 약 5.33~22.33회 줄었다. 원래 약한 태그가 먼저 기준 이하로 떨어진다. 이 조건은 부스 내부에서 거의 항상 발생하므로, 사진보다 감소폭 해석이 더 중요하다.",
    images: [],
  },
  {
    step: "3",
    title: "RFID 리더기와 태그 방향 / 안테나 각도가 맞지 않는 경우",
    summary: "요약: 훅 태그는 세워져 있고 안테나는 위에서 아래로 보기 때문에 바닥보다 각도 민감도가 크다.",
    detail:
      "훅에서는 QR 방향만으로 결과가 고정되지 않는다. QR 방향 단독보다 태그 위치, 벽과 거리, 정면/후면, 상부 안테나 각도가 함께 결과를 만든다.",
    qrMatrixCaption: "정면 기준 QR 방향 비교",
    qrMatrix: [
      { hook: "왼쪽 훅", hidden: 60.33, visible: 65.33, note: "두 방향 모두 강함" },
      { hook: "중앙 훅", hidden: 12.33, visible: 9.33, note: "두 방향 모두 약함" },
      { hook: "오른쪽 훅", hidden: 0.67, visible: 11.0, note: "방향 민감도 큼" },
    ],
    imageColumns: [
      {
        title: "왼쪽 훅",
        items: [
          {
            label: "QR 안 보이게",
            summary: "태그 면이 안테나 쪽",
            src: TAG_FACE_TO_ANTENNA,
          },
          {
            label: "QR 보이게",
            summary: "태그 면이 안테나 반대쪽",
            src: TAG_FACE_AWAY_FROM_ANTENNA,
          },
        ],
      },
      {
        title: "중앙 훅",
        items: [
          {
            label: "QR 안 보이게",
            summary: "태그 면이 안테나 쪽",
            src: CENTER_QR_HIDDEN,
          },
          {
            label: "QR 보이게",
            summary: "태그 면이 안테나 반대쪽",
            src: CENTER_QR_VISIBLE,
          },
        ],
      },
      {
        title: "오른쪽 훅",
        items: [
          {
            label: "QR 안 보이게",
            summary: "태그 면이 안테나 쪽",
            src: RIGHT_QR_HIDDEN,
          },
          {
            label: "QR 보이게",
            summary: "태그 면이 안테나 반대쪽",
            src: RIGHT_QR_VISIBLE,
          },
        ],
      },
    ],
  },
  {
    step: "4",
    title: "여러 태그가 가까이 겹쳐 서로 간섭하는 경우",
    summary: "요약: 한 훅 밀집, 바닥 적층, 랜덤 겹침이 들어가면 태그별 편차가 크게 벌어진다.",
    detail:
      "같은 방향 태그끼리는 보강되지만, 그 사이에 낀 반대 방향 태그는 쉽게 매몰된다. 특정 태그만 살아남고 나머지는 급격히 줄어드는 패턴이 반복된다.",
    images: [
      {
        label: "적층 / 중첩 구조",
        summary: "여러 태그가 겹치고 몰린 상태",
        src: CAUSE_OVERLAP_IMAGE,
      },
      {
        label: "적층 사진",
        summary: "바닥 적층으로 태그가 겹친 상태",
        src: CAUSE_STACK_IMAGE,
      },
      {
        label: "한 훅 밀집",
        summary: "한 훅에 여러 태그가 몰린 상태",
        src: CAUSE_SINGLE_HOOK_IMAGE,
      },
    ],
  },
  {
    step: "5",
    title: "안테나 위치",
    summary: "요약: 안테나는 항상 중앙이 좋은 것이 아니라 태그 위치마다 최적점이 따로 있다.",
    detail:
      "중앙은 좌우 반사가 겹쳐 상쇄가 생길 수 있어 일관성이 낮다. 같은 실패 케이스도 좌우 위치로 옮기면 더 크게 회복되는 경우가 반복됐다.",
    images: [],
  },
];

export const keyConclusion =
  "결론: 옷 재질 영향은 작았고, 실제 인식 성능은 주로 벽 간섭, 태그 방향, 태그 밀집, 사람 존재, 안테나 위치에 의해 좌우됐다. 특히 안테나는 항상 중앙이 좋은 것이 아니고, 현재 공간에서는 오른쪽이 더 자주 유리했다. 결국 태그 면을 얼마나 정면에 가깝게 보느냐가 핵심이었다.";

export const conditionCompareRows = [
  {
    category: "훅 위치/배치",
    good: "벽 간섭이 덜하고 태그가 안테나 쪽을 비교적 잘 보는 배치",
    bad: "벽 가까운 훅, 후면 배치, 한 훅 밀집",
    reason: "벽 반사 + 방향 불일치 -> 약한 태그부터 매몰",
  },
  {
    category: "바닥 적층 방향",
    good: "여러 태그 방향이 비슷해 전체가 같이 응답",
    bad: "일부만 역방향이거나 하단 태그 방향이 다름",
    reason: "같은 방향 보강 -> 반대 방향 태그 약화",
  },
  {
    category: "바닥 랜덤 배치",
    good: "위치가 좋고 열린 태그가 우연히 유리한 자세",
    bad: "각도·겹침이 불리한 태그가 눌리거나 가려짐",
    reason: "특정 태그 독점은 줄어도 편차는 유지",
  },
  {
    category: "사람 존재",
    good: "사람이 없어 경로가 열림",
    bad: "사람이 안테나와 태그 사이를 가로막음",
    reason: "인체 흡수·차단 -> 약한 태그부터 하락",
  },
  {
    category: "사람-태그 근접",
    good: "사람과 태그가 떨어져 있어 자세 변화가 적음",
    bad: "몸 쪽으로 붙거나 태그 각도가 틀어짐",
    reason: "가까워짐 자체보다 자세 변화가 더 큼",
  },
  {
    category: "안테나 위치",
    good: "태그를 더 정면에 가깝게 보는 위치",
    bad: "태그를 비스듬히 보거나 중앙 상쇄가 겹치는 위치",
    reason: "위치마다 최적점이 다름",
  },
];

export const quickConditionBlocks = {
  good: [
    "태그 면이 안테나 쪽",
    "벽과 거리가 있음",
    "같은 방향 태그끼리 보강",
    "사람이 없어 경로가 열림",
    "안테나가 태그를 더 정면에 가깝게 봄",
  ],
  bad: [
    "후면 또는 반대 방향",
    "벽 가까이 배치",
    "보강된 태그 사이 반대 방향 태그",
    "사람이 경로를 가로막음",
    "안테나가 태그를 비스듬히 봄",
  ],
};

export const exampleSections = [
  {
    key: "hook",
    title: "훅 배치",
    subtitle: "정면은 안정, 후면은 급락",
    blocks: [
      {
        title: "옷 1벌 비교",
        caption: "대표 조건 비교",
        goodItems: [
          {
            label: "잘 됨",
            phase: "Phase2(옷1벌)",
            testId: "1-2",
            grade: "🟢 매우 좋음",
            summary: "정면 + 옷밖",
            meanLabel: "평균 인식 횟수",
            meanRead: 60.33,
            compareRead: "8.59 -> 60.33 / 7.02배 증가",
            src: PHASE2_ONE_CLOTH_GOOD,
          },
        ],
        badItems: [
          {
            label: "잘 안 됨",
            phase: "Phase2(옷1벌)",
            testId: "1-5",
            grade: "🔴 실패",
            summary: "후면 + 취약 방향",
            meanLabel: "평균 인식 횟수",
            meanRead: 0.0,
            compareRead: "8.59 -> 0.00 / -100%",
            src: PHASE2_ONE_CLOTH_BAD,
          },
        ],
      },
      {
        title: "옷 여러벌 대표 예시",
        caption: "대표 배치 기준 비교",
        note:
          "여기서 잘 됨은 대표 예시일 뿐이다. 모든 태그가 다 잘 읽힌다는 뜻은 아니며, 훅과 태그의 상대 위치·겹침·방향에 따라 어떤 태그는 살아남고 어떤 태그는 매몰된다.",
        meanNote:
          "평균 인식 횟수 = 파란셔츠(AA000010) 5회 평균. 전체 옷 평균이 아니라 대표 태그 기준이다.",
        goodItems: [
          {
            label: "잘 됨 예시",
            phase: "Phase2(옷여러벌)",
            testId: "1-3",
            grade: "🟢 매우 좋음",
            summary: "3,1,2,1 분산 배치",
            meanLabel: "평균 인식 횟수",
            meanRead: 51.0,
            compareRead: "1-1 대비 +48.67회",
            src: PHASE2_MULTI_HOOK_GOOD,
            tagReads: [
              ["AA000010", 51.0],
              ["AA000011", 60.0],
              ["AA000019", 3.67],
              ["AA000002", 54.67],
              ["AA000014", 1.33],
              ["AA000001", 66.33],
              ["AA000015", 0.0],
            ],
          },
        ],
        badItems: [
          {
            label: "잘 안 됨 예시",
            phase: "Phase2(옷여러벌)",
            testId: "1-1",
            grade: "🔴 실패",
            summary: "한 훅 7벌 밀집",
            meanLabel: "평균 인식 횟수",
            meanRead: 2.33,
            compareRead: "1-3 대비 -48.67회",
            src: PHASE2_MULTI_HOOK_BAD,
            tagReads: [
              ["AA000010", 2.33],
              ["AA000011", 61.67],
              ["AA000019", 51.67],
              ["AA000002", 18.33],
              ["AA000014", 25.67],
              ["AA000001", 60.0],
              ["AA000015", 3.33],
            ],
          },
        ],
      },
    ],
  },
  {
    key: "floor",
    title: "바닥 적층 / 랜덤",
    subtitle: "적층 방향과 랜덤 편차가 핵심",
    visuals: [
      { label: "적층 구조 사진", summary: "2-1,2,3,5,6,7 공통 배치", src: PHASE2_MULTI_FLOOR_STACK },
      { label: "랜덤 배치 사진", summary: "2-4 비적층 랜덤 배치", src: PHASE2_MULTI_FLOOR_RANDOM },
    ],
    blocks: [
      {
        title: "옷 여러벌 대표 예시",
        caption: "대표 배치 기준 비교",
        note:
          "여기서 잘 됨은 대표 예시일 뿐이다. 적층도 모든 태그가 같이 좋아지는 것이 아니라, 어떤 방향으로 쌓였는지에 따라 살아남는 태그와 매몰되는 태그가 갈린다.",
        meanNote:
          "평균 인식 횟수 = 파란셔츠(AA000010) 5회 평균. 적층 사진 기준 최상단 태그는 AA000010, 최하단 태그는 AA000019다.",
        goodItems: [
          {
            label: "잘 됨 예시",
            phase: "Phase2(옷여러벌)",
            testId: "2-5",
            grade: "🟢 매우 좋음",
            summary: "전 태그 정방향 적층 / 최상단 AA000010, 최하단 AA000019",
            meanLabel: "평균 인식 횟수",
            meanRead: 58.67,
            compareRead: "2-3 대비 +58.67회",
            reason: "정방향으로 놓인 태그들은 함께 보강돼 강하게 읽혔다. 다만 모든 태그가 같이 좋아진 것은 아니고, AA000002와 AA000014는 겹침과 축 불일치로 끝까지 매몰됐다.",
            src: PHASE2_MULTI_FLOOR_STACK,
            tagReads: [
              ["AA000010", 58.67],
              ["AA000011", 51.0],
              ["AA000002", 0.0],
              ["AA000014", 0.0],
              ["AA000001", 46.67],
              ["AA000015", 52.67],
              ["AA000019", 65.33],
            ],
          },
        ],
        badItems: [
          {
            label: "잘 안 됨 예시",
            phase: "Phase2(옷여러벌)",
            testId: "2-3",
            grade: "🔴 실패",
            summary: "최하단 AA000019만 정방향, 나머지 역방향",
            meanLabel: "평균 인식 횟수",
            meanRead: 0.0,
            compareRead: "2-5 대비 -58.67회",
            reason: "최하단 19만 정방향이고 나머지 태그가 역방향이라, 일부 강한 태그만 살아남고 파란셔츠를 포함한 여러 태그는 바닥 적층 안에서 에너지를 거의 못 받아 매몰됐다.",
            src: PHASE2_MULTI_FLOOR_STACK,
            tagReads: [
              ["AA000010", 0.0],
              ["AA000011", 54.0],
              ["AA000002", 0.0],
              ["AA000014", 0.0],
              ["AA000001", 72.67],
              ["AA000015", 29.67],
              ["AA000019", 65.33],
            ],
          },
        ],
        neutralItems: [
          {
            label: "랜덤 / 편차",
            phase: "Phase2(옷여러벌)",
            testId: "2-4",
            grade: "🟡 좋음",
            summary: "비적층 랜덤 배치",
            meanLabel: "평균 인식 횟수",
            meanRead: 39.0,
            compareRead: "2-5 대비 -19.67회",
            reason: "적층 독점은 줄었지만 태그마다 각도와 겹침이 제각각이라 편차가 커졌다. AA000019처럼 열린 태그는 계속 유리했고, 약한 태그들은 여전히 0회로 남았다.",
            src: PHASE2_MULTI_FLOOR_RANDOM,
            tagReads: [
              ["AA000010", 39.0],
              ["AA000011", 0.0],
              ["AA000002", 0.0],
              ["AA000014", 0.0],
              ["AA000001", 0.0],
              ["AA000015", 27.33],
              ["AA000019", 55.0],
            ],
          },
        ],
      },
    ],
  },
];

export const peopleSection = {
  title: "사람 영향",
  subtitle: "사람은 반드시 존재하는 조건이므로, 사진보다 감소폭 해석이 중요",
  cards: [
    {
      label: "사람 없음 기준",
      summary: "경로가 열려 있을 때",
      metric: "기준점",
      detail: "가장 안정적인 상태",
      tone: "good",
    },
    {
      label: "사람 존재",
      summary: "사람이 전파 경로를 가로막을 때",
      metric: "약 5.33~22.33회 감소",
      detail: "원래 약한 태그부터 먼저 하락",
      tone: "bad",
    },
    {
      label: "사람-태그 근접",
      summary: "몸이 태그에 가까워질 때",
      metric: "약 0.00~27.67회 감소",
      detail: "가까워지면 나빠질 수는 있지만 영향이 아주 크지는 않음",
      tone: "neutral",
    },
  ],
};

export const antennaSection = {
  title: "안테나 위치에 따른 인식 변화",
  subtitle: "x축은 안테나 위치, 색은 테스트 케이스",
  summaryCards: [
    { label: "기존", value: "0 / 4", text: "기존 위치는 실패 케이스를 거의 살리지 못함", tone: "bad", src: ANTENNA_POS_BASE },
    { label: "왼쪽", value: "3 / 4", text: "왼쪽 벽 반사가 강해 구석·후면은 불리하지만, 일부 태그는 정면 각도가 맞아 크게 회복", tone: "warn", src: ANTENNA_POS_LEFT },
    { label: "중앙", value: "2 / 4", text: "중앙은 좌우 반사가 겹쳐 상쇄가 생길 수 있어 회복 폭이 들쭉날쭉함", tone: "neutral", src: ANTENNA_POS_CENTER },
    { label: "오른쪽", value: "3 / 4", text: "좌우 대칭이 깨지며 상쇄가 줄고, 대각선 조사로 옷 틈새까지 전파가 더 잘 들어감", tone: "good", src: ANTENNA_POS_RIGHT },
  ],
  cases: [
    {
      caseId: "1-5",
      condition: "후면 왼쪽훅 옷밖 QR 안보이게",
      base: 0.0,
      left: 7.0,
      center: 0.0,
      right: 26.67,
      best: "오른쪽",
      summary: "오른쪽 이동에서 가장 크게 회복",
      src: ANTENNA_CASE_1_5,
    },
    {
      caseId: "2-5",
      condition: "후면 중앙훅 옷밖 QR 안보이게",
      base: 0.0,
      left: 9.33,
      center: 0.0,
      right: 4.67,
      best: "왼쪽",
      summary: "왼쪽만 기준 통과",
      src: ANTENNA_CASE_2_5,
    },
    {
      caseId: "3-2",
      condition: "정면 오른쪽훅 옷밖 QR 안보이게",
      base: 0.67,
      left: 66.33,
      center: 25.0,
      right: 69.0,
      best: "오른쪽",
      summary: "모든 위치 개선, 오른쪽 최고",
      src: ANTENNA_CASE_3_2,
    },
    {
      caseId: "3-6",
      condition: "후면 오른쪽훅 옷밖 QR 보이게",
      base: 0.0,
      left: 0.0,
      center: 14.0,
      right: 12.33,
      best: "중앙",
      summary: "중앙이 최고, 오른쪽도 회복",
      src: ANTENNA_CASE_3_6,
    },
  ],
};

export const externalEvidence = {
  title: "외부 자료에서 확인된 내용",
  intro:
    "외부 자료들도 공통적으로 옷감 자체보다 배치 환경, 안테나 위치, 공간 구조, 태그 방향, 밀집도가 인식 성능에 더 큰 영향을 준다는 흐름을 보였다.",
  rows: [
    {
      source: "RFIDTag - Why Use RFID Tags on Clothes",
      href: "https://rfidtag.com/why-use-rfid-tags-on-clothes/",
      finding: "RFID는 옷을 직접 보지 않아도 읽을 수 있고, 여러 겹 의류도 판독 가능하다고 설명",
      implication: "재질 자체보다 배치, 방향, 환경 영향이 더 큼",
    },
    {
      source: "Urovo - Applications and Benefits of RFID Technology in Fashion Retail",
      href: "https://urovo.jp/blog/RFID-handheld-readers/398.html",
      finding: "공간 제약이 큰 리테일 환경과 금속 간섭이 있는 복잡한 환경을 전제로 설명하고, SIC 간섭 억제 기능도 언급",
      implication: "실제 리테일 현장도 간섭을 전형적 문제로 보고 장비와 알고리즘으로 보완함",
    },
    {
      source: "XMINNOV - RFID Clothing Store Anti-theft Access Control",
      href: "https://www.rfidtagworld.com/news/rfid-clothing-store-anti-theft-access-control.html",
      finding: "fitting room 같은 사각지대를 언급하고, 출입구·출구 중심 안테나 커버를 강조",
      implication: "내부 전체 상시 읽기보다 통과 구간 중심 설계가 많음",
    },
    {
      source: "RFID News - Zara RFID case study",
      href: "https://www.rfidjournal.com/news/zara-uses-rfid-to-speed-up-store-operations/73817/",
      finding: "밀집 진열, 구조물, 안테나 위치, 태그 방향을 핵심 변수로 설명",
      implication: "의류 리테일에서도 밀집, 구조, 방향, 안테나 위치가 핵심 문제",
    },
    {
      source: "XIUCHENG RFID - Zara case study",
      href: "https://www.xc-rfid.com/news/how-does-zara-use-rfid-tags.html",
      finding: "fabric type, packaging density, store environment, reader infrastructure, pilot 테스트를 강조",
      implication: "실제 구축도 파일럿 테스트 기반으로 환경 적합성을 먼저 검증함",
    },
    {
      source: "하이태그 RFID 의류관리 시스템 소개",
      href: "https://www.hitag.co.kr/",
      finding: "게이트형, 아치형, 포탈형, 폴대형 등 구조물 기반 설치를 소개",
      implication: "좁은 공간도 리더기 1개보다 구조물과 안테나 배치로 해결하는 방향이 일반적",
    },
  ],
};

export const solutionSection = {
  title: "현재 판단",
  intro:
    "외부 사례와 자체 테스트를 종합하면, RFID 인식 불량은 재질보다 배치 환경, 태그 방향, 밀집도, 사람, 안테나 위치 영향이 크다. 이를 완전히 해결하려면 구조/배치 재설계와 반복적인 검증이 필요하다.",
  priorityCards: [
    {
      label: "HW 우선 후보",
      value: "안테나 개수 추가",
      tone: "info",
      text: "효과는 가장 클 가능성이 높지만 설치·배선·재테스트 부담이 커 현재는 보류",
    },
    {
      label: "SW 우선 후보",
      value: "태그 인식 시간 늘리기",
      tone: "good",
      text: "정지 상태 효과는 작지만 실제 사용처럼 움직임이 있으면 10~15초 여유에서 일부 추가 인식 가능성",
    },
    {
      label: "현재 실제 우선",
      value: "출력 세기 조절 테스트",
      tone: "bad",
      text: "지금 바로 시도 가능한 조치이며 반복 검증도 가장 단순함",
    },
  ],
  methods: [
    {
      name: "안테나 신호 파워 올리기",
      how: "리더 출력값 조정",
      needs: "SDK 또는 명령 프로토콜",
      limit: "세기만 올려도 모든 조건이 해결되지는 않음. 특정 태그 쏠림 가능",
      tone: "neutral",
    },
    {
      name: "안테나 개수 늘리기",
      how: "상부 1개 외에 측면/벽면 안테나 추가",
      needs: "추가 안테나, 설치 공간, 배선, 재테스트",
      limit: "위치를 잘못 잡으면 개선 없이 시간만 소요",
      tone: "info",
      badge: "HW 추천",
    },
    {
      name: "입구형 / 외부형 구조",
      how: "출입구 중심으로 읽기",
      needs: "구조 변경",
      limit: "부스 내부 최종 상태까지는 완전 보장 어려움",
      tone: "neutral",
    },
    {
      name: "태그 충돌 완화 로직",
      how: "덜 읽힌 태그 우선 읽기",
      needs: "SDK의 Q / Session 제어",
      limit: "현재는 Q-value만 확인, Session 2는 미확인",
      tone: "neutral",
    },
    {
      name: "태그 인식 횟수 기준 완화",
      how: "코드에서 최소 인식 횟수 숫자 조정",
      needs: "없음",
      limit: "아예 0회인 태그는 기준을 낮춰도 그대로 0회라 효과가 거의 없음",
      tone: "neutral",
    },
    {
      name: "태그 인식 시간 늘리기",
      how: "코드에서 스캔 시간 숫자 조정",
      needs: "없음",
      limit: "정지 상태에서는 변화가 작았음. 다만 실제 사용처럼 움직임이 있으면 문 닫힘부터 재개방까지 약 10~15초 여유에서 일부 추가 인식 가능성은 있음",
      tone: "good",
      badge: "SW 추천",
    },
  ],
  limits: [
    "전파 세기, dead zone, 반사 분포를 직접 보는 측정 도구가 없음",
    "출력·위치·구조를 바꿀 때마다 동일 조건 반복 측정이 필요",
    "구조를 바꿔도 특정 위치나 특정 배치의 누락이 완전히 사라진다는 보장이 없음",
  ],
  judgment: [
    "결론: 현 단계에서는 출력 세기 조절 테스트만 우선 진행",
    "안테나 개수 추가는 효과가 가장 클 가능성이 높지만 설치 부담 때문에 일단 보류",
    "인식 시간 증가는 정지 상태에서는 효과가 작고, 실제 동작 상황에서만 제한적 보완 가능성이 있음",
    "구조 변경 기반 해결과 안테나 추가 최적화는 보류",
  ],
  finalLine:
    "최종 결론: 지금 단계에서 실제로 바로 시도할 조치는 SDK 기반 출력 세기 조절 테스트다. 안테나 개수 추가는 가장 유력한 HW 방안으로 보이지만 설치 부담이 크고, 인식 시간 증가는 실제 움직임 상황에서만 제한적 보완 가능성이 있다.",
};

export const supportNotes = {
  title: "해석 기준",
  intro:
    "인식 횟수 기준은 지금까지 실험에서 나온 최소~최대 인식 범위를 바탕으로 4단계로 나눈 값이다. 초기 인식 속도 기준은 우리 기준상 성공(10초 내 5회 이상)과 실패 케이스의 최초 인식 시간을 비교해 잡았다.",
  readCountTitle: "RFID 인식 판정 기준 (우선순위 1)",
  readCountNote:
    "실험 전체에서 나온 인식 횟수 범위를 기준으로, 0회부터 최대값 구간까지를 4단계로 나눠 판정했다.",
  readCountRows: [
    { range: "40회 이상", grade: "🟢 매우 좋음", tone: "good" },
    { range: "20~39회", grade: "🟡 좋음", tone: "warn" },
    { range: "5~19회", grade: "🟠 인식은 되나 약함", tone: "info" },
    { range: "0~4회", grade: "🔴 실패", tone: "bad" },
  ],
  firstReadTitle: "초기 인식 속도 기준 (이론적 반영, 우선순위 2)",
  firstReadNote:
    "성공 케이스와 실패 케이스의 실제 최초 인식 시간을 비교했을 때, 10초 내 5회 조건을 넘기기 시작하는 지점을 기준으로 구간을 나눴다.",
  firstReadRows: [
    {
      range: "0~2000ms",
      grade: "🟢 매우 빠름",
      meaning: "10초 내 5회 인식 성공 가능성이 높음 -> 종합 판정 유지",
      tone: "good",
    },
    {
      range: "2001~3000ms",
      grade: "🟡 빠름",
      meaning: "통과는 가능하지만 경계선에 가까움 -> 종합 판정 유지",
      tone: "warn",
    },
    {
      range: "3001~4000ms",
      grade: "🟠 느림",
      meaning: "실패 확률이 급격히 높아짐 -> 종합 판정 1단계 하향",
      tone: "info",
    },
    {
      range: "4001ms 초과",
      grade: "🔴 매우 느림",
      meaning: "사실상 실패 구간 -> 종합 판정 2단계 하향",
      tone: "bad",
    },
    {
      range: "미인식",
      grade: "🔴 실패",
      meaning: "10초 내 5회 조건 충족 불가 -> 종합 판정 실패",
      tone: "bad",
    },
  ],
};

export const testScopeSection = {
  title: "전체 테스트 범위",
  subtitle:
    "지금 한 테스트와 아직 남은 경우의 수를 같이 봐야, 무엇을 더 하고 무엇을 덜어낼지 바로 판단할 수 있다.",
  headline:
    "실사용 조건은 위치, 옷 수량, 의류 종류, 배치 상태, 태그 방향, 사람, 안테나 위치가 동시에 겹치기 때문에 경우의 수가 빠르게 커진다.",
  axes: [
    {
      name: "위치",
      options: ["벽걸이 7개", "바닥", "손에 들고 있음"],
      note: "현재는 일부 벽걸이/바닥 중심으로 실험",
    },
    {
      name: "옷 수량",
      options: ["1벌", "2벌", "3벌", "4벌", "5벌", "가혹 7벌"],
      note: "실사용은 1~5벌, 현재는 1벌과 7벌 중심",
    },
    {
      name: "의류 종류",
      options: ["상의", "바지", "아우터", "패딩", "속옷/기타"],
      note: "현재는 일부 상의/바지/바람막이 위주",
    },
    {
      name: "배치 상태",
      options: ["벽걸이", "바닥 적층", "바닥 랜덤", "손에 들고 있음"],
      note: "손에 들고 있음과 이동 중 상태는 부족",
    },
    {
      name: "태그 방향",
      options: ["정면+QR 보임", "정면+QR 안 보임", "후면+QR 보임", "후면+QR 안 보임"],
      note: "1벌 기준은 비교적 있음, 여러벌 기준은 일부 조합만 있음",
    },
    {
      name: "추가 변수",
      options: ["사람 차폐", "안테나 위치", "악조건 랜덤", "외부 유출 반경"],
      note: "좌/중/우 안테나는 했지만 앞/뒤/대각은 미실시",
    },
  ],
  covered: [
    {
      title: "이미 한 테스트",
      items: [
        "Phase 1: 위치/방향 기준선",
        "Phase 2(옷 1벌): 좌/중/우 벽걸이 + 바닥 조건 비교",
        "Phase 2(옷 여러벌): 한 벽걸이 밀집 / 분산 배치",
        "Phase 2(옷 여러벌): 바닥 적층 / 랜덤",
        "Phase 2(옷 여러벌): 사람 유무 / 사람 접근",
        "안테나 위치: 좌 / 중앙 / 우 재배치",
      ],
    },
  ],
  gaps: [
    {
      title: "아직 비어 있는 테스트",
      items: [
        "벽걸이 7개 전 위치를 여러벌 기준으로 모두 비교",
        "실사용 수량 1~5벌을 체계적으로 분리 비교",
        "의류 종류/재질을 더 넓게 확장",
        "손에 들고 이동하는 상태",
        "문 밖 인식 반경과 외부 유출 범위",
        "안테나 앞/뒤 이동, 대각선 각도 조절",
        "악조건 랜덤 Worst-Case 혼합 테스트",
      ],
    },
  ],
  whyThisMatters: [
    "경우의 수를 먼저 펴놓아야 무엇을 스킵했고 왜 스킵했는지 설명할 수 있음",
    "전체 경우의 수 중 무엇을 줄이고 무엇을 집중할지 사전 합의가 쉬워짐",
    "현재 결과가 전체 공간을 대표하는지, 일부 조합만 본 것인지 구분 가능",
  ],
};

export const antennaReviewSection = {
  title: "피팅룸 목적 기준 RFID 안테나 비교",
  badge: "RFID Antenna Review",
  summary:
    "판단 기준은 두 가지다. 첫째, 피팅룸 내부의 태그는 빠짐없이 읽어야 한다. 둘째, 피팅룸 외부의 태그는 최대한 읽지 않아야 한다. 아래 평가는 현재 의류 테스트 문맥에 맞춰 UHF 의류 태그 + 고정형 리더를 전제로 정리했다.",
  winner: "최적 후보: 패치 안테나",
  rows: [
    {
      name: "Dipole",
      type: "선형 편파 중심의 범용형",
      strength: "저비용, 자유공간 성능 양호",
      fit: "조건부",
      fitTone: "warn",
      note: "옷 방향이 일정하면 가능하지만, 피팅룸처럼 태그 방향이 계속 바뀌는 환경에서는 미검출 위험이 있다.",
    },
    {
      name: "Monopole",
      type: "단일 극 구조, 소형 리더 지향",
      strength: "작고 단순함",
      fit: "낮음",
      fitTone: "bad",
      note: "휴대형/장거리 성격이 더 강해서 내부만 정밀하게 읽고 외부는 억제해야 하는 목적과는 거리가 있다.",
    },
    {
      name: "Patch",
      type: "평면형, 원형 편파 사용 빈도 높음",
      strength: "고정형 설치 용이, 태그 방향 변화 대응 우수",
      fit: "가장 적합",
      fitTone: "good",
      note: "피팅룸 벽면/천장 배치가 쉽고, 원형 편파로 다양한 옷 방향을 안정적으로 읽으면서 조사 방향도 비교적 제어하기 좋다.",
    },
    {
      name: "Yagi",
      type: "강한 지향성, 고이득, 장거리",
      strength: "먼 거리까지 집중 조사 가능",
      fit: "낮음",
      fitTone: "bad",
      note: "내부 태그는 잘 읽더라도 외부까지 멀리 읽을 가능성이 커서 피팅룸 경계 제어 목적에는 불리하다.",
    },
    {
      name: "Loop",
      type: "근거리장 중심, LF/HF 친화적",
      strength: "영역 제한에 유리",
      fit: "주파수 조건부",
      fitTone: "warn",
      note: "영역 제한은 매력적이지만, 의류 UHF 태그 기준이라면 기본 주파수 대역이 달라 이번 실험 주력 안테나로 보기 어렵다.",
    },
    {
      name: "Fractal",
      type: "소형, 광대역, 다주파수",
      strength: "복잡한 환경 대응력",
      fit: "보조 후보",
      fitTone: "warn",
      note: "유연성은 좋지만 좁은 공간 안에서 안정적인 전수 인식과 경계 제어라는 목적에는 패치보다 판단 근거가 약하다.",
    },
    {
      name: "Slot",
      type: "금속면 통합형, 특정 대역 설계 가능",
      strength: "금속 구조물 연동에 강함",
      fit: "구조 조건부",
      fitTone: "warn",
      note: "금속 프레임과 일체형으로 설계할 때는 후보가 될 수 있지만, 일반 피팅룸 기본 안테나로는 설계 난도가 높다.",
    },
    {
      name: "PIFA",
      type: "저프로파일, 소형 장치 친화형",
      strength: "모바일 기기 내장에 유리",
      fit: "낮음",
      fitTone: "bad",
      note: "모바일 리더에는 적합하지만 피팅룸 전체를 커버하는 고정형 주 안테나로는 우선순위가 낮다.",
    },
  ],
  conclusion: [
    "이번 실험 목적에는 원형 편파 기반 UHF 패치 안테나가 가장 잘 맞는다.",
    "옷의 방향이나 접힘 상태가 달라져도 읽기 안정성이 높고, 벽면 또는 천장 방향으로 조사 영역을 비교적 제어하기 쉬워 피팅룸 내부 전수 인식과 외부 태그 미인식 목표를 함께 맞추기 유리하다.",
    "다만 실제 성능은 안테나 종류만으로 결정되지 않고, 출력 세기, 설치 각도, 안테나 개수, 흡수재/차폐재 적용까지 함께 최적화해야 한다.",
  ],
  sources: [
    {
      label: "Dipole 안테나",
      text: "선형 편파 기반의 범용 안테나이며 자유공간 성능과 비용 효율이 장점이다.",
      href: "https://ko.rfidtagworld.com/news/rfid-antenna-types.html#:~:text=Dipole%20%EC%95%88%ED%85%8C%EB%82%98",
      tone: "neutral",
    },
    {
      label: "Monopole 안테나",
      text: "단일 극 구조이며 소형 리더와 장거리 응용에 적합하다고 소개된다.",
      href: "https://ko.rfidtagworld.com/news/rfid-antenna-types.html#:~:text=Monopole%20%EC%95%88%ED%85%8C%EB%82%98",
      tone: "neutral",
    },
    {
      label: "Patch 안테나",
      text: "평면형 microstrip 구조이며 원형 편파를 제공하는 경우가 많고, 고정형 리더와 도어 포털에 적합하다.",
      href: "https://ko.rfidtagworld.com/news/rfid-antenna-types.html#:~:text=%ED%8C%A8%EC%B9%98%20%EC%95%88%ED%85%8C%EB%82%98",
      tone: "good",
    },
    {
      label: "Yagi 안테나",
      text: "고이득 지향성 안테나로 장거리 RFID, 차량 추적, 접근 제어에 쓰인다.",
      href: "https://ko.rfidtagworld.com/news/rfid-antenna-types.html#:~:text=yagi%20%EC%95%88%ED%85%8C%EB%82%98",
      tone: "neutral",
    },
    {
      label: "Loop 안테나",
      text: "근거리장 중심이며 LF/HF 시스템과 아이템 레벨 태깅에 적합하다고 정리된다.",
      href: "https://ko.rfidtagworld.com/news/rfid-antenna-types.html#:~:text=%EB%B0%98%EB%B3%B5%20%EC%95%88%ED%85%8C%EB%82%98",
      tone: "neutral",
    },
    {
      label: "Fractal 안테나",
      text: "소형이면서 광대역, 다주파수 특성을 제공해 복잡한 환경에 대응한다.",
      href: "https://ko.rfidtagworld.com/news/rfid-antenna-types.html#:~:text=Fractal%20%EC%95%88%ED%85%8C%EB%82%98",
      tone: "neutral",
    },
    {
      label: "Slot 안테나",
      text: "금속면 슬롯을 활용하는 구조로 산업 환경과 금속 구조 통합에 강점이 있다.",
      href: "https://ko.rfidtagworld.com/news/rfid-antenna-types.html#:~:text=%EA%B5%AC%EB%A9%8D%20%EC%95%88%ED%85%8C%EB%82%98",
      tone: "neutral",
    },
    {
      label: "PIFA 안테나",
      text: "저프로파일 소형 안테나로 모바일 RFID 리더와 소형 RFID 장치에 적합하다.",
      href: "https://ko.rfidtagworld.com/news/rfid-antenna-types.html#:~:text=PIFA",
      tone: "neutral",
    },
  ],
  sourceNote:
    "출처: XMINNOV RFID Tag World, RFID 안테나의 다른 유형, 게시일 2024-05-27.",
  sourceHref: "https://ko.rfidtagworld.com/news/rfid-antenna-types.html",
};
