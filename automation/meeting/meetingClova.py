"""
회의 자동화 시스템 v7.11
=======================================================
- 수정사항: 화자 판단 로직 고도화
  1. Gemini 프롬프트: 호칭 패턴 분석 추가
     - "건우야/건우씨" 라고 부른 직후 다른 화자가 응답하면 → 응답한 화자 = 건우
     - 자기 이름을 직접 언급하지 않는 패턴 반영
  2. 텍스트 검색 폴백: 호칭 → 다음 발화자 패턴으로 매핑
"""

import os
import re
import glob
import shutil
import time
import json
import requests
from datetime import datetime
from playwright.sync_api import sync_playwright
from google import genai
import config

client = genai.Client(api_key=config.GEMINI_API)

NOTION_TOKEN      = config.NOTION_TOKEN
CLOVA_EMAIL       = config.CLOVA_EMAIL
CLOVA_PASSWORD    = config.CLOVA_PASSWORD
DB_ID             = "31b136e19284813aaf71d9f6c16dff3f"

TARGET_DRIVE_PATH = r"G:\.shortcut-targets-by-id\18_ReZvonpmf6OyZv1wQqqC9zhDKFWbHG\강동현"
RECORDING_FOLDER  = r"C:\Users\pnks\Documents\소리 녹음"
TXT_FOLDER        = TARGET_DRIVE_PATH

MEETING_LIST      = ["주간팀회의", "인턴킥오프미팅", "온보딩미팅", "기타 (직접 입력)"]
ALL_MEMBERS       = ["성보경", "김건우", "강동현", "배윤선", "정세빈", "정재진"]

# 이름 호칭 변형 목록 (성/이름/별명 등)
# 검색 시 이 키워드가 발화에 등장하면 해당 멤버를 호칭한 것으로 판단
MEMBER_ALIASES = {
    "성보경": ["소장님","박사님","보경", "보경씨", "보경아", "보경아"],
    "김건우": ["건우", "건우씨", "건우야","건우님"],
    "강동현": ["동현", "동현씨", "동현아", "동현아","동현님"],
    "배윤선": ["윤선", "윤선씨", "윤선아", "윤선아","윤선님"],
    "정세빈": ["세빈", "세빈씨", "세빈아", "세빈이", "세빈님"],
    "정재진": ["재진", "재진씨", "재진아", "재진아","재진님"],
}

# ================================================================
# 1. Notion 관련 함수
# ================================================================

def create_notion_page(dt, meeting_name):
    print(f"\n[1] Notion 페이지 생성 중: {meeting_name}", flush=True)
    headers = {"Authorization": f"Bearer {NOTION_TOKEN}", "Content-Type": "application/json", "Notion-Version": "2022-06-28"}
    title = f"{dt.strftime('%y%m%d')}-{meeting_name}"
    payload = {
        "parent": {"database_id": DB_ID},
        "properties": {
            "이름": {"title": [{"text": {"content": title}}]},
            "미팅 일시": {"date": {"start": dt.strftime("%Y-%m-%dT%H:%M:%S+09:00")}}
        }
    }
    try:
        resp = requests.post("https://api.notion.com/v1/pages", headers=headers, json=payload)
        res_json = resp.json()
        if resp.status_code == 200: return res_json.get("id"), res_json.get("url")
    except: pass
    return None, None

def append_text_to_notion(page_id, text):
    print(f"     -> Notion 페이지 본문에 데이터 기입 중...", flush=True)
    headers = {"Authorization": f"Bearer {NOTION_TOKEN}", "Content-Type": "application/json", "Notion-Version": "2022-06-28"}
    chunks = [text[i:i+1500] for i in range(0, len(text), 1500)]
    for chunk in chunks:
        payload = {"children": [{"object": "block", "type": "paragraph", "paragraph": {"rich_text": [{"type": "text", "text": {"content": chunk}}]}}]}
        requests.patch(f"https://api.notion.com/v1/blocks/{page_id}/children", headers=headers, json=payload)
    print("     [OK] Notion 기입 완료", flush=True)

# ================================================================
# 2. Gemini AI 관련 함수
# ================================================================

def analyze_speakers_with_gemini(speaker_texts: dict, speaker_sequence: list) -> dict | None:
    """
    Gemini로 화자 동일인물 판단 + 호칭-응답 패턴 분석.
    speaker_sequence: [(화자, 발화내용), ...] 순서 보존 리스트
    성공 시: {"참석자1": "성보경", ...} 반환
    실패 시: None 반환
    """
    print(f"     [Gemini] 화자 분석 시도 중...", flush=True)

    members_str = ", ".join(ALL_MEMBERS)

    # 화자별 발화 샘플 (앞 400자)
    speaker_summary = ""
    for speaker, text in speaker_texts.items():
        speaker_summary += f"[{speaker}] 발화 샘플:\n{text[:400].strip()}\n\n"

    # 발화 순서 (앞 60개 발화만 사용)
    sequence_str = ""
    for i, (speaker, text) in enumerate(speaker_sequence[:60]):
        sequence_str += f"{i+1}. [{speaker}]: {text[:80].strip()}\n"

    # 호칭 변형 목록 문자열
    alias_str = ""
    for member, aliases in MEMBER_ALIASES.items():
        alias_str += f"  - {member}: {', '.join(aliases)}\n"

    prompt = f"""
너는 회의록 화자 식별 전문가야. 클로바노트가 자동으로 붙인 화자 레이블(참석자1, 참석자2 등)이 실제로 누구인지 판별해야 해.

## 실제 참석 가능한 멤버
{members_str}

## 이름 호칭 변형 목록
{alias_str}

## 핵심 판단 규칙
1. **호칭-응답 패턴**: 어떤 화자가 특정 이름을 호칭(예: "건우야", "동현씨")한 직후 다른 화자가 응답하면, 그 응답한 화자가 호칭된 사람이다. 호칭을 한 화자는 그 사람이 아니다.
2. **자기소개/직접언급**: 화자가 자신의 이름을 직접 언급하면 해당 화자가 그 사람이다.
3. **동일인물 판단**: 발화 패턴, 말투, 문맥이 유사하면 같은 사람으로 판단한다.
4. **불확실한 경우**: 판단 근거가 없으면 "알수없음"으로 표시한다.

## 화자별 발화 샘플
{speaker_summary}

## 발화 순서 (호칭-응답 패턴 분석용)
{sequence_str}

## 출력 형식
반드시 아래 JSON 형식으로만 응답해. 다른 텍스트 절대 포함 금지.
{{"참석자1": "성보경", "참석자2": "김건우", "참석자3": "성보경"}}
"""

    for attempt in range(3):
        try:
            response = client.models.generate_content(
                model="gemini-2.0-flash",
                contents=prompt
            )
            raw = response.text.strip()
            raw = re.sub(r"```json|```", "", raw).strip()
            mapping = json.loads(raw)
            print(f"     [OK] Gemini 분석 완료: {mapping}", flush=True)
            return mapping
        except Exception as e:
            print(f"     [Warning] Gemini 분석 실패 (시도 {attempt+1}/3): {e}", flush=True)
            if "429" in str(e):
                time.sleep(40)
            else:
                time.sleep(5)

    return None


def search_speaker_names_by_pattern(speaker_texts: dict, speaker_sequence: list) -> dict:
    """
    Gemini 실패 시 폴백:
    1. 호칭-응답 패턴: A가 "건우야" 호칭 → 다음 발화자 B = 건우
    2. 직접 언급: 발화 텍스트에 멤버 이름 직접 등장
    3. 둘 다 없으면 해당 화자는 변경 생략
    """
    print(f"     [폴백] 호칭-응답 패턴 + 텍스트 검색으로 화자 탐색 중...", flush=True)

    mapping = {}  # {화자: 이름}
    confidence = {}  # {화자: 신뢰도 점수} (높을수록 우선)

    # ── 1차: 호칭-응답 패턴 분석 ─────────────────────────────────
    # speaker_sequence: [(화자, 발화), ...] 순서대로
    for i, (speaker, text) in enumerate(speaker_sequence):
        for member, aliases in MEMBER_ALIASES.items():
            for alias in aliases:
                if alias in text:
                    # 이 화자가 member를 호칭함 → 다음 발화자가 member일 가능성
                    # 단, 이 화자 자신은 member가 아님
                    if i + 1 < len(speaker_sequence):
                        next_speaker, _ = speaker_sequence[i + 1]
                        if next_speaker != speaker:
                            # 다음 화자 → member로 매핑 (신뢰도 2)
                            if next_speaker not in confidence or confidence[next_speaker] < 2:
                                mapping[next_speaker] = member
                                confidence[next_speaker] = 2
                                print(f"     [패턴] [{speaker}]가 '{alias}' 호칭 → [{next_speaker}] = {member} 판단", flush=True)
                    # 호칭한 화자는 member가 아님을 명시적으로 기록
                    # (다른 근거로 이미 매핑됐다면 유지, 없으면 제외 표시)
                    if speaker not in mapping:
                        mapping[speaker] = f"NOT_{member}"  # 임시 제외 마크

    # ── 2차: 직접 텍스트 검색 (호칭 패턴으로 못 찾은 화자 대상) ──
    for speaker, text in speaker_texts.items():
        # 이미 확실히 매핑된 화자는 스킵
        if speaker in confidence and confidence[speaker] >= 2:
            continue

        name_counts = {}
        for member in ALL_MEMBERS:
            # 전체 이름 + 호칭 변형 모두 카운트
            count = text.count(member)
            for alias in MEMBER_ALIASES.get(member, []):
                count += text.count(alias)
            if count > 0:
                name_counts[member] = count

        if name_counts:
            best_name = max(name_counts, key=name_counts.get)
            # NOT_ 마크가 있는 경우 해당 이름이면 스킵
            excluded = mapping.get(speaker, "")
            if excluded == f"NOT_{best_name}":
                print(f"     [검색] [{speaker}]: '{best_name}' 발견되었으나 호칭한 사람이므로 제외", flush=True)
                del mapping[speaker]
                continue
            mapping[speaker] = best_name
            confidence[speaker] = 1
            print(f"     [검색] [{speaker}]: '{best_name}' {name_counts[best_name]}회 발견 → 매핑", flush=True)
        else:
            if speaker not in mapping or mapping[speaker].startswith("NOT_"):
                print(f"     [검색] [{speaker}]: 이름 발견 안 됨 → 변경 생략", flush=True)

    # ── NOT_ 임시 마크 제거 (실제 매핑으로 덮이지 않은 것) ─────────
    mapping = {k: v for k, v in mapping.items() if not v.startswith("NOT_")}

    # ── 동일 이름 충돌 처리: 신뢰도 높은 쪽 우선 ─────────────────
    name_to_speakers = {}
    for speaker, name in mapping.items():
        name_to_speakers.setdefault(name, []).append((speaker, confidence.get(speaker, 0)))

    final_mapping = {}
    for name, speakers_scores in name_to_speakers.items():
        if len(speakers_scores) == 1:
            final_mapping[speakers_scores[0][0]] = name
        else:
            speakers_scores.sort(key=lambda x: x[1], reverse=True)
            winner = speakers_scores[0][0]
            final_mapping[winner] = name
            losers = [s for s, _ in speakers_scores[1:]]
            print(f"     [충돌] '{name}' 중복 → {winner}(신뢰도 {speakers_scores[0][1]}) 채택, {losers} 생략", flush=True)

    print(f"     [폴백] 최종 매핑: {final_mapping}", flush=True)
    return final_mapping


def build_speaker_mapping(speaker_texts: dict, speaker_sequence: list) -> dict:
    """
    1차: Gemini (호칭-응답 패턴 포함 프롬프트)
    2차: 텍스트 검색 + 호칭-응답 패턴 폴백
    """
    print(f"\n[3] 화자 이름 매핑 시작...", flush=True)

    gemini_result = analyze_speakers_with_gemini(speaker_texts, speaker_sequence)

    if gemini_result is not None:
        mapping = {
            speaker: name
            for speaker, name in gemini_result.items()
            if name != "알수없음" and name != speaker
        }
        if mapping:
            reverse_map = {}
            for speaker, name in mapping.items():
                reverse_map.setdefault(name, []).append(speaker)
            for name, speakers in reverse_map.items():
                if len(speakers) > 1:
                    print(f"     [분석] {', '.join(speakers)} → 동일인물 '{name}' 판단됨", flush=True)
            return mapping
        else:
            print(f"     [Gemini] 변경할 화자 없음으로 판단됨.", flush=True)
            return {}

    print(f"     [Warning] Gemini 실패 → 호칭-응답 패턴 + 텍스트 검색 폴백으로 전환", flush=True)
    return search_speaker_names_by_pattern(speaker_texts, speaker_sequence)


def fix_text_with_gemini(file_path):
    print(f"\n[5] Gemini AI 문맥 교정 시작...", flush=True)
    with open(file_path, "r", encoding="utf-8") as f:
        original_text = f.read()

    max_retries = 2
    prompt = f"너는 전문 속기사야. 아래 회의록 내용을 문맥에 맞게 오타만 교정해줘. 형식을 바꾸지 마라:\n{original_text}"

    for attempt in range(max_retries + 1):
        try:
            response = client.models.generate_content(model="gemini-2.0-flash", contents=prompt)
            fixed_text = response.text
            with open(file_path, "w", encoding="utf-8") as f: f.write(fixed_text)
            print("     [OK] AI 교정 완료", flush=True)
            return fixed_text
        except Exception as e:
            if "429" in str(e) and attempt < max_retries:
                print(f"     [Wait] API 사용량 초과. 40초 후 재시도... ({attempt+1}/{max_retries})", flush=True)
                time.sleep(40)
            else:
                return original_text

# ================================================================
# 3. Clova Note 관련 함수
# ================================================================

def collect_speaker_data(page) -> tuple[dict, list]:
    """
    화자별 발화 텍스트(dict) + 발화 순서(list) 동시 수집.
    반환: (speaker_texts, speaker_sequence)
      - speaker_texts: {"참석자1": "전체 발화 합산", ...}
      - speaker_sequence: [("참석자1", "발화내용"), ("참석자2", "발화내용"), ...]
    """
    speaker_texts = {}
    speaker_sequence = []
    prev_height = -1

    for _ in range(20):  # 스크롤 최대 20회
        try:
            blocks = page.locator("[data-stt-item-id]").all()
            for block in blocks:
                name_btn = block.locator("button[class*='name_btn']").first
                speaker = name_btn.inner_text(timeout=500).strip()
                words = block.locator("[class*='SttEditor_word']").all()
                text = "".join(w.inner_text(timeout=300) for w in words).strip()

                # 발화 합산
                speaker_texts[speaker] = speaker_texts.get(speaker, "") + text + " "
                # 순서 보존 (중복 연속 발화 방지)
                if not speaker_sequence or speaker_sequence[-1][0] != speaker:
                    speaker_sequence.append((speaker, text))

            curr_h = page.evaluate("() => document.querySelector('.ReactVirtualized__Grid')?.scrollTop || 0")
            if curr_h == prev_height: break
            prev_height = curr_h
            page.evaluate("() => { const el = document.querySelector('.ReactVirtualized__Grid'); if(el) el.scrollTop += 2000; }")
            page.wait_for_timeout(500)
        except: continue

    return speaker_texts, speaker_sequence


def auto_rename_speakers(page):
    print("     [클로바] 화자 이름 변경 모드 진입...", flush=True)
    max_attempts = 3

    for attempt in range(1, max_attempts + 1):
        try:
            print(f"     [클로바] 이름 변경 시도 {attempt}/{max_attempts}...", flush=True)

            edit_btn = page.get_by_role("button", name="편집", exact=True)
            edit_btn.wait_for(state="visible", timeout=10000)
            edit_btn.click(force=True)
            page.wait_for_timeout(3000)

            print("     [클로바] 화자 데이터 수집 중...", flush=True)
            speaker_texts, speaker_sequence = collect_speaker_data(page)

            if not speaker_texts:
                print("     [Warning] 화자 데이터 수집 실패. 편집 종료.", flush=True)
                page.get_by_role("button", name="완료").click(force=True)
                page.wait_for_timeout(2000)
                return False

            print(f"     [OK] 수집된 화자: {list(speaker_texts.keys())} / 발화 순서 {len(speaker_sequence)}개", flush=True)

            mapping = build_speaker_mapping(speaker_texts, speaker_sequence)

            if not mapping:
                print("     [클로바] 변경할 화자 없음. 완료 처리.", flush=True)
            else:
                print(f"     [클로바] 적용할 매핑: {mapping}", flush=True)
                for target_speaker, new_name in mapping.items():
                    try:
                        print(f"     -> {target_speaker} → {new_name} 변경 중...", flush=True)
                        btn = page.get_by_role("button", name=target_speaker, exact=True).first
                        btn.wait_for(state="visible", timeout=5000)
                        btn.click(force=True)
                        page.wait_for_timeout(1000)
                        editor = page.locator(".ProseMirror-focused")
                        editor.wait_for(state="visible", timeout=3000)
                        editor.fill(new_name)
                        page.get_by_text("전체 구간", exact=True).click(force=True)
                        page.get_by_role("button", name="변경", exact=True).click(force=True)
                        page.wait_for_timeout(2000)
                        print(f"     [OK] {target_speaker} → {new_name} 변경 완료", flush=True)
                    except Exception as e:
                        print(f"     [Warning] {target_speaker} 변경 실패: {e}", flush=True)
                        page.keyboard.press("Escape")

            done_btn = page.get_by_role("button", name="완료")
            done_btn.wait_for(state="visible", timeout=5000)
            done_btn.click(force=True)
            page.wait_for_timeout(2000)

            print("     [클로바] 편집 모드 종료 확인 대기 중...", flush=True)
            page.get_by_role("button", name="편집", exact=True).wait_for(state="visible", timeout=15000)
            print("     [OK] 이름 변경 및 편집 모드 종료 확인 완료", flush=True)
            return True

        except Exception as e:
            print(f"     [Warning] 이름 변경 시도 {attempt} 실패: {e}", flush=True)
            try: page.keyboard.press("Escape")
            except: pass
            page.wait_for_timeout(2000)

    print("     [Warning] 이름 변경 최대 재시도 초과. 다음 단계로 진행합니다.", flush=True)
    return False


def run_clova_process(m4a_path):
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=False, slow_mo=200)
        context = browser.new_context(accept_downloads=True)
        page    = context.new_page()
        try:
            print("[2] 클로바 로그인 중...", flush=True)
            page.goto("https://clovanote.naver.com/")
            page.get_by_role("button", name="로그인").click()
            page.get_by_role("textbox", name="아이디 또는 전화번호").fill(config.CLOVA_EMAIL)
            page.get_by_role("textbox", name="비밀번호").fill(config.CLOVA_PASSWORD)
            page.get_by_role("button", name="로그인").click()
            page.wait_for_load_state("networkidle")
            for btn_name in ["등록안함", "다시 보지 않기"]:
                try: page.get_by_text(btn_name).click(timeout=3000)
                except: pass

            print("     [클로바] 새 노트 업로드 중...", flush=True)
            page.get_by_role("button", name="새 노트").click(force=True)
            with page.expect_file_chooser() as fc: page.get_by_role("button", name="파일 첨부").click(force=True)
            fc.value.set_files(m4a_path)
            page.wait_for_url("**/note-detail/**", timeout=60000)

            print("     [클로바] 전사 대기 시작 (성공할 때까지 무한 반복)...", flush=True)
            wait_count = 0
            while True:
                try:
                    page.keyboard.press("Escape")
                    is_ready = page.evaluate("() => [...document.querySelectorAll('button')].some(b => b.innerText.trim() === '다운로드')")
                    if is_ready:
                        print("     [OK] 전사 완료 확인됨.", flush=True)
                        break
                    wait_count += 1
                    if wait_count % 80 == 0:  # 15초 * 80 = 20분
                        print(f"     [System] {wait_count // 4}분 경과: 새로고침 시도...", flush=True)
                        try:
                            page.reload(timeout=60000)
                            page.wait_for_load_state("networkidle", timeout=10000)
                        except:
                            print("     [Warning] 새로고침 지연. 무시하고 계속 감시합니다.", flush=True)
                    time.sleep(15)
                except Exception:
                    time.sleep(15)
                    continue

            auto_rename_speakers(page)

            print("     [클로바] TXT 다운로드 및 저장 중...", flush=True)
            page.get_by_role("button", name="다운로드").first.click(force=True)
            page.get_by_text("음성 기록 다운로드").click(force=True)
            with page.expect_download(timeout=15000) as dl_info:
                page.get_by_role("button", name="다운로드", exact=True).last.click(force=True)

            save_path = os.path.join(TXT_FOLDER, dl_info.value.suggested_filename)
            dl_info.value.save_as(save_path)

            with open(save_path, "r", encoding="utf-8") as f: lines = f.readlines()
            if len(lines) >= 3:
                del lines[2]
                with open(save_path, "w", encoding="utf-8") as f: f.writelines(lines)

            print(f"     [OK] 파일 저장 완료: {os.path.basename(save_path)}", flush=True)
            return save_path

        except Exception as e:
            print(f"     [Error] 치명적 실패: {e}", flush=True)
            return None
        finally:
            browser.close()

# ================================================================
# 4. 메인 실행
# ================================================================

def main():
    print("=======================================================", flush=True)
    print("  회의 자동화 시스템 v7.11 (호칭-응답 패턴 분석)", flush=True)
    print("=======================================================", flush=True)

    m4a_files = glob.glob(os.path.join(RECORDING_FOLDER, "*.m4a"))
    if not m4a_files: return print("[Stop] 파일 없음", flush=True)
    latest_m4a = max(m4a_files, key=os.path.getmtime)
    rec_dt = datetime.fromtimestamp(os.path.getmtime(latest_m4a))

    print(f"\n[감지] 파일: {os.path.basename(latest_m4a)}", flush=True)
    for i, n in enumerate(MEETING_LIST, 1): print(f"    {i}. {n}", flush=True)
    choice = int(input("    미팅 번호: "))
    meeting_name = MEETING_LIST[choice-1] if choice <= 3 else input("    미팅명: ")

    new_path = os.path.join(TARGET_DRIVE_PATH, f"{rec_dt.strftime('%y%m%d')}-{meeting_name}.m4a")
    shutil.copy2(latest_m4a, new_path)
    print(f"     -> 파일 복사 완료 (G드라이브 동기화 시작)", flush=True)

    page_id, page_url = create_notion_page(rec_dt, meeting_name)
    txt_path = run_clova_process(new_path)

    if txt_path:
        fixed_text = fix_text_with_gemini(txt_path)
        if page_id: append_text_to_notion(page_id, fixed_text)
        print(f"\n[성공] 모든 작업 완료!\n🔗 Notion: {page_url}\n 저장: {txt_path}", flush=True)

if __name__ == "__main__":
    main()