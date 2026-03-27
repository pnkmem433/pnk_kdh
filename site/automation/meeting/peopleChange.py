"""
클로바노트 화자 이름 변경 스크립트 (최종 통합본)
====================================
- 전체 노트에서 가장 최신 노트 접속
- 튜토리얼 팝업 등 오버레이 방해 요소 강제 돌파 (force=True)
- JS로 전체 DOM 스캔 (가상스크롤 대응)
- 화자별 발화 텍스트 수집 -> 이름 자동 매핑
- exact=True 매칭을 통한 안전한 화자 이름 일괄 변경 (토스트 메시지 중복 방지)
- 완료 -> 클로바노트 원본 파일명으로 다운로드 -> 3번째 줄 삭제 후처리
"""

import os
import re
from playwright.sync_api import sync_playwright
import config  # 분리한 설정 파일 불러오기

# ================================================================
#  설정값
# ================================================================

# 민감한 정보는 config에서 가져옵니다.
CLOVA_EMAIL    = config.CLOVA_EMAIL
CLOVA_PASSWORD = config.CLOVA_PASSWORD

# 일반 설정값은 코드 내에서 직접 관리합니다.
TXT_FOLDER     = r"G:\.shortcut-targets-by-id\18_ReZvonpmf6OyZv1wQqqC9zhDKFWbHG\강동현"
ALL_MEMBERS    = ["성보경", "김건우", "강동현", "배윤선", "정세빈", "정재진"]

# ================================================================
#  JS로 전체 발화 블록 수집 (가상 스크롤 대응: 스크롤하며 수집)
# ================================================================

def collect_speaker_texts(page) -> dict:
    """
    페이지를 끝까지 스크롤하며 렌더링된 블록을 순차 수집
    반환: {"참석자1": "발화전체...", "참석자2": "발화전체...", ...}
    """
    speaker_texts = {}
    seen_ids = set()

    scroll_container = page.locator(".ReactVirtualized__Grid").first

    prev_height = -1
    while True:
        # 현재 렌더된 블록 수집
        blocks = page.locator("[data-stt-item-id]").all()
        for block in blocks:
            try:
                item_id = block.get_attribute("data-stt-item-id", timeout=500)
                if item_id in seen_ids:
                    continue
                seen_ids.add(item_id)

                # 화자 이름
                name_btn = block.locator("button[class*='name_btn']").first
                speaker = name_btn.inner_text(timeout=500).strip()

                # 발화 텍스트
                words = block.locator("[class*='SttEditor_word']").all()
                text  = "".join(w.inner_text(timeout=300) for w in words)

                speaker_texts[speaker] = speaker_texts.get(speaker, "") + text + " "
            except Exception:
                continue

        # 스크롤 내리기
        current_height = page.evaluate(
            "() => document.querySelector('.ReactVirtualized__Grid') "
            "? document.querySelector('.ReactVirtualized__Grid').scrollTop : 0"
        )
        if current_height == prev_height:
            break
        prev_height = current_height

        page.evaluate(
            "() => { const el = document.querySelector('.ReactVirtualized__Grid'); "
            "if(el) el.scrollTop += 2000; }"
        )
        page.wait_for_timeout(400)

    # 스크롤을 다시 맨 위로
    page.evaluate(
        "() => { const el = document.querySelector('.ReactVirtualized__Grid'); "
        "if(el) el.scrollTop = 0; }"
    )
    page.wait_for_timeout(500)
    return speaker_texts


def get_speaker_count(speaker_texts: dict) -> int:
    """수집된 화자 키에서 최대 숫자 추출"""
    max_num = 0
    for key in speaker_texts:
        m = re.search(r'참석자\s*(\d+)', key)
        if m:
            max_num = max(max_num, int(m.group(1)))
    return max_num


# ================================================================
#  이름 자동 매핑
# ================================================================

def find_name(text: str, member: str) -> bool:
    if member in text:
        return True
    first = member[1:]
    if len(first) >= 2 and re.search(first + r'[\s씨야아요]', text):
        return True
    return False


def auto_map(speaker_texts: dict, speaker_count: int) -> dict:
    speakers = [f"참석자{i}" for i in range(1, speaker_count + 1)]
    mapping  = {s: None for s in speakers}

    if not speaker_texts:
        return mapping

    volumes = {s: len(speaker_texts.get(s, "")) for s in speakers}
    most    = max(volumes, key=volumes.get)
    mapping[most] = "성보경"

    assigned = {"성보경"}
    bogyeong = speaker_texts.get(most, "")
    remaining = [s for s in speakers if s != most]

    for member in ALL_MEMBERS:
        if member in assigned:
            continue
        if find_name(bogyeong, member):
            for s in remaining:
                if mapping[s] is None:
                    mapping[s] = member
                    assigned.add(member)
                    break

    return mapping


def confirm_or_fix(mapping: dict, speaker_count: int) -> dict:
    speakers = [f"참석자{i}" for i in range(1, speaker_count + 1)]

    print("\n  [자동 매핑 결과]")
    for s in speakers:
        print(f"  - {s} -> {mapping.get(s) or '(미배정)'}")

    ans = input("\n  이대로 진행할까요? (y / n): ").strip().lower()
    if ans == "y":
        return mapping

    print("\n  수동으로 이름을 선택합니다.")
    for s in speakers:
        print(f"\n  [{s}] 이름 선택:")
        for j, name in enumerate(ALL_MEMBERS, 1):
            print(f"     {j}. {name}")
        print(f"     {len(ALL_MEMBERS)+1}. 직접 입력")
        while True:
            choice = input("     번호 입력: ").strip()
            if not choice.isdigit():
                continue
            idx = int(choice) - 1
            if idx == len(ALL_MEMBERS):
                name = input("     이름 직접 입력: ").strip()
                if name:
                    mapping[s] = name
                    break
            elif 0 <= idx < len(ALL_MEMBERS):
                mapping[s] = ALL_MEMBERS[idx]
                break
    return mapping


# ================================================================
#  메인
# ================================================================

def run():
    os.makedirs(TXT_FOLDER, exist_ok=True)

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=False, slow_mo=300)
        context = browser.new_context(accept_downloads=True)
        page    = context.new_page()

        try:
            # ── 로그인 ───────────────────────────────────────────
            print("[1] 로그인 중...")
            page.goto("https://clovanote.naver.com/")
            page.get_by_role("button", name="로그인").click()
            page.get_by_role("textbox", name="아이디 또는 전화번호").click()
            page.get_by_role("textbox", name="아이디 또는 전화번호").fill(CLOVA_EMAIL)
            page.get_by_role("textbox", name="비밀번호").click()
            page.get_by_role("textbox", name="비밀번호").fill(CLOVA_PASSWORD)
            page.get_by_role("button", name="로그인").click()
            page.wait_for_load_state("networkidle")
            
            try:
                page.get_by_role("link", name="등록안함").click(timeout=5000)
            except Exception:
                pass
            try:
                with page.expect_popup(timeout=3000) as p1_info:
                    page.locator(".MuiDialog-container").click(timeout=3000)
                p1_info.value.close()
            except Exception:
                pass
            try:
                page.get_by_role("button", name="다시 보지 않기").click(timeout=5000)
            except Exception:
                pass
            print("     [OK] 로그인 완료")

            # ── 전체 노트 -> 최신 노트 클릭 ──────────────────────
            print("[2] 최신 노트 접속 중...")
            page.get_by_role("tree", name="menu").get_by_label("전체 노트").click()
            page.wait_for_timeout(1500)
            page.locator("a[href*='note-detail']").first.click()
            page.wait_for_url("**/note-detail/**", timeout=30000)
            page.wait_for_timeout(2000)
            print("     [OK] 노트 접속 완료")

            # ── 편집 모드 진입 (튜토리얼 팝업 방해 요소 선제 제거 포함) ──
            print("[3] 편집 모드 진입...")
            
            # 1. 화면을 가리는 튜토리얼 '닫기' 버튼 제거
            try:
                page.locator(".introjs-tooltipbuttons").get_by_role("button", name="닫기").click(timeout=2000)
                page.wait_for_timeout(500)
            except Exception:
                pass

            # 2. ESC 키를 눌러 혹시 모를 오버레이 강제 종료
            try:
                page.keyboard.press("Escape")
                page.wait_for_timeout(500)
            except Exception:
                pass
            
            # 3. 편집 버튼 클릭 (force=True로 오버레이 무시)
            page.get_by_role("button", name="편집", exact=True).click(force=True)
            page.wait_for_timeout(1000)
            
            # 4. 편집 모드 진입 직후 뜨는 튜토리얼 제거
            try:
                page.locator("#tutorial_main_content").get_by_role("button", name="닫기").click(timeout=2000)
                page.wait_for_timeout(500)
            except Exception:
                pass

            # ── 화자별 발화 텍스트 수집 (스크롤하며) ────────────
            print("[4] 전체 발화 텍스트 수집 중... (스크롤)")
            speaker_texts = collect_speaker_texts(page)
            speaker_count = get_speaker_count(speaker_texts)

            print(f"     감지된 참석자 수: {speaker_count}명")
            for s, t in sorted(speaker_texts.items()):
                print(f"     {s}: {len(t)}자 | 미리보기: {t[:40].strip()}...")

            # ── 자동 매핑 + 확인 ─────────────────────────────────
            print("[5] 이름 자동 분석 중...")
            mapping = auto_map(speaker_texts, speaker_count)
            final   = confirm_or_fix(mapping, speaker_count)

            # ── 참석자 이름 일괄 변경 ─────────────────────────────────
            print("\n[6] 참석자 이름 변경 중...")
            for i in range(1, speaker_count + 1):
                target_speaker = f"참석자{i}"
                new_name = final.get(target_speaker, "")
                
                # 매핑된 이름이 없거나 이미 같은 이름이면 스킵
                if not new_name or target_speaker == new_name:
                    continue
                    
                print(f"     {target_speaker} -> {new_name}")
                
                try:
                    # '참석자N' 버튼 중 렌더링된 첫 번째 것을 클릭
                    page.get_by_role("button", name=target_speaker, exact=True).first.click()
                    page.wait_for_timeout(500)
                    
                    # 팝업의 입력창 선택 후 새 이름 채우기
                    page.locator(".ProseMirror-focused").fill(new_name)
                    page.wait_for_timeout(300)
                    
                    # '전체 구간' 선택 (exact=True로 하단 토스트 메시지와 겹침 방지)
                    page.get_by_text("전체 구간", exact=True).click()
                    page.wait_for_timeout(300)
                    
                    # '변경' 버튼 클릭
                    page.get_by_role("button", name="변경", exact=True).click()
                    
                    # UI에 변경 내역이 완전히 반영될 때까지 대기
                    page.wait_for_timeout(1000)
                    
                except Exception as e:
                    print(f"     [Warning] {target_speaker} 이름 변경 중 오류: {e}")
                    # 오류 시 떠있는 팝업이 있다면 닫기 시도
                    try:
                        page.locator("#tutorial_main_content").get_by_role("button", name="닫기").click(timeout=1000)
                    except:
                        pass

            # ── 완료 ─────────────────────────────────────────────
            print("[7] 완료 클릭...")
            page.get_by_role("button", name="완료").click()
            page.wait_for_timeout(1500)

            # ── 다운로드 ─────────────────────────────────────────
            print("[8] 다운로드 팝업 열기 및 파일 저장 중...")
            page.wait_for_timeout(2000)
            
            # 1. 메인 화면 우측 상단의 '다운로드' 아이콘 클릭
            page.get_by_role("button", name="다운로드").first.click()
            page.wait_for_timeout(1000)
            
            # 2. 드롭다운 메뉴에서 '음성 기록 다운로드' 클릭 (팝업 열기)
            page.get_by_text("음성 기록 다운로드").click()
            page.wait_for_timeout(1000)
            
            # 3. 팝업 창 하단의 최종 '다운로드' 버튼 클릭 및 다운로드 이벤트 대기
            with page.expect_download(timeout=15000) as dl_info:
                page.get_by_role("button", name="다운로드", exact=True).last.click()
                
            download = dl_info.value
            
            # 💡 핵심 변경: 클로바노트가 제공하는 원래 파일명(음성 파일명 기반)을 그대로 가져옵니다.
            original_filename = download.suggested_filename
            save_path = os.path.join(TXT_FOLDER, original_filename)
            
            download.save_as(save_path)
            print(f"\n[OK] 최종 다운로드 완료: {save_path}")

            # ── 텍스트 후처리 (3번째 줄 삭제) ──────────────────────
            print("[9] 텍스트 후처리 (참석자 목록 줄 삭제) 진행 중...")
            try:
                with open(save_path, "r", encoding="utf-8") as f:
                    lines = f.readlines()
                
                # 3번째 줄(인덱스 2)이 존재하는 경우 삭제
                if len(lines) >= 3:
                    del lines[2]
                    
                    with open(save_path, "w", encoding="utf-8") as f:
                        f.writelines(lines)
                    print("     [OK] 후처리 완료 (참석자 목록 삭제 및 덮어쓰기 성공)")
                else:
                    print("     [Warning] 파일이 3줄 미만이라 삭제를 건너뜁니다.")
            except Exception as e:
                print(f"     [Error] 텍스트 후처리 중 오류 발생: {e}")

        except Exception as e:
            print(f"\n[Error] 오류: {e}")
            import traceback; traceback.print_exc()
        finally:
            context.close()
            browser.close()

if __name__ == "__main__":
    run()