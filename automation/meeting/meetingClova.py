"""
회의 자동화 스크립트 v4.0
==========================
실행 흐름:
  1. Notion DB 확인 (최초 1회 생성)
  2. Music 폴더에서 최신 .m4a 파일 탐색
  3. 미팅 종류 선택
  4. 파일명 변경  →  YYMMDD-미팅명.m4a
  5. Notion DB에 새 페이지 생성
  6. 클로바노트 로그인 → m4a 업로드 → 변환 완료 대기 → txt 다운로드
  7. ChatGPT 프로젝트에 txt 업로드 → 요약 답변 수신
  8. Notion 페이지에 전사내용 + GPT요약 블록 자동 삽입

사전 준비:
  pip install requests playwright
  playwright install chromium
"""

import os
import sys
import glob
import shutil
import time
import json
import re
from datetime import datetime
from pathlib import Path
import requests
import threading

# ================================================================
#  ✅ 설정값
# ================================================================

NOTION_TOKEN        = "..."
WORKSPACE_PAGE_ID   = "..."
DB_ID_FILE          = os.path.join(os.path.dirname(os.path.abspath(__file__)), ".meeting_db_id")

RECORDING_FOLDER    = r"C:\Users\pnks\Documents\소리 녹음"
TXT_FOLDER          = r"C:\WS\미팅txt"

CHROME_USER_DATA    = r"C:\Users\pnks\AppData\Local\Google\Chrome\User Data"
CHROME_PROFILE      = "Default"

CHATGPT_PROJECT_URL = "https://chatgpt.com/g/g-p-69af853d94bc81919807a1d57a670757/project"

CLOVA_EMAIL         = "..."       # 네이버 아이디
CLOVA_PASSWORD      = "..."  # 네이버 비밀번호

MEETING_LIST = [
    "주간팀회의",
    "인턴킥오프미팅",
    "온보딩미팅",
    "기타 (직접 입력)",
]

NOTION_HEADERS = {
    "Authorization": f"Bearer {NOTION_TOKEN}",
    "Content-Type": "application/json",
    "Notion-Version": "2022-06-28",
}

# ================================================================
#  Notion DB
# ================================================================

def load_db_id():
    if os.path.exists(DB_ID_FILE):
        with open(DB_ID_FILE, "r") as f:
            return f.read().strip()
    return None

def save_db_id(db_id: str):
    with open(DB_ID_FILE, "w") as f:
        f.write(db_id)

def create_meeting_db():
    payload = {
        "parent": {"type": "page_id", "page_id": WORKSPACE_PAGE_ID},
        "icon": {"type": "emoji", "emoji": "👋"},
        "title": [{"type": "text", "text": {"content": "2-3. 미팅"}}],
        "properties": {
            "이름":      {"title": {}},
            "참가자":    {"people": {}},
            "미팅 일시": {"date": {}},
            "상태":      {"status": {}},
        },
    }
    resp = requests.post(
        "https://api.notion.com/v1/databases",
        headers=NOTION_HEADERS,
        data=json.dumps(payload),
    )
    if resp.status_code == 200:
        db_id = resp.json()["id"]
        save_db_id(db_id)
        print(f"     ✅ DB 생성 완료 (ID: {db_id})")
        return db_id
    else:
        print(f"\n[오류] DB 생성 실패 ({resp.status_code})\n{resp.text}")
        return None

def get_or_create_db():
    db_id = load_db_id()
    if db_id:
        print(f"     기존 DB 사용 (ID: {db_id})")
        return db_id
    print("     처음 실행 - DB 생성 중...")
    return create_meeting_db()

# ================================================================
#  녹음 파일
# ================================================================

def find_latest_m4a(folder: str):
    files = glob.glob(os.path.join(folder, "*.m4a"))
    if not files:
        return None
    return max(files, key=os.path.getmtime)

def get_file_datetime(filepath: str) -> datetime:
    return datetime.fromtimestamp(os.path.getmtime(filepath))

def select_meeting_name() -> str:
    print("\n[3] 미팅 종류를 선택하세요:")
    for i, name in enumerate(MEETING_LIST, 1):
        print(f"     {i}. {name}")
    while True:
        choice = input("     번호 입력: ").strip()
        if not choice.isdigit():
            print("     숫자를 입력해주세요.")
            continue
        idx = int(choice) - 1
        if idx < 0 or idx >= len(MEETING_LIST):
            print(f"     1 ~ {len(MEETING_LIST)} 사이 숫자를 입력해주세요.")
            continue
        selected = MEETING_LIST[idx]
        if "직접 입력" in selected:
            name = input("     미팅명 직접 입력: ").strip()
            if not name:
                print("     미팅명을 입력해야 합니다.")
                continue
            return name
        return selected

def rename_file(filepath: str, dt: datetime, meeting_name: str) -> str:
    folder    = os.path.dirname(filepath)
    date_str  = dt.strftime("%y%m%d")
    new_name  = f"{date_str}-{meeting_name}.m4a"
    new_path  = os.path.join(folder, new_name)
    if os.path.exists(new_path):
        print(f"\n[경고] 동일한 파일명이 이미 존재합니다: {new_name}")
        if input("         덮어씌울까요? (y/n): ").strip().lower() != "y":
            print("         이름 변경을 건너뜁니다.")
            return filepath
    shutil.move(filepath, new_path)
    return new_path

# ================================================================
#  Notion 페이지 생성
# ================================================================

def create_notion_page(db_id: str, dt: datetime, meeting_name: str):
    date_str     = dt.strftime("%y%m%d")
    title        = f"{date_str}-{meeting_name}"
    notion_date  = dt.strftime("%Y-%m-%dT%H:%M:%S+09:00")
    payload = {
        "parent": {"database_id": db_id},
        "properties": {
            "이름":      {"title": [{"text": {"content": title}}]},
            "미팅 일시": {"date":  {"start": notion_date}},
        },
    }
    resp = requests.post(
        "https://api.notion.com/v1/pages",
        headers=NOTION_HEADERS,
        data=json.dumps(payload),
    )
    if resp.status_code == 200:
        data = resp.json()
        return data.get("id", ""), data.get("url", "")
    else:
        print(f"\n[오류] 페이지 생성 실패 ({resp.status_code})\n{resp.text}")
        return None, None

# ================================================================
#  클로바노트 자동화
# ================================================================

def run_clova_upload_and_download(m4a_path: str, filename_base: str,
                                   ready_event: threading.Event = None) -> str | None:
    """
    클로바노트에 m4a 업로드 → 변환 완료 대기(JS) → txt 다운로드
    ready_event: 업로드 완료 후 병렬 작업에 신호를 보낼 Event (선택)
    반환값: 다운로드된 txt 파일 경로 (실패 시 None)
    """
    from playwright.sync_api import sync_playwright

    txt_save_path = os.path.join(TXT_FOLDER, f"{filename_base}.txt")
    os.makedirs(TXT_FOLDER, exist_ok=True)

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=False, slow_mo=500)
        context = browser.new_context(accept_downloads=True)
        page    = context.new_page()

        try:
            # ── 로그인 ──────────────────────────────────────────
            print("     [클로바] 로그인 중...")
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
                page.get_by_role("button", name="다시 보지 않기").click(timeout=5000)
            except Exception:
                pass

            # ── 파일 업로드 ─────────────────────────────────────
            print("     [클로바] 파일 업로드 중...")
            page.get_by_role("button", name="새 노트").click()
            page.wait_for_timeout(1000)

            with page.expect_file_chooser() as fc_info:
                page.get_by_role("button", name="파일 첨부").click()
            fc_info.value.set_files(m4a_path)
            print(f"     [클로바] ✅ 파일 선택 완료: {os.path.basename(m4a_path)}")

            # ── 노트 상세 페이지 진입 대기 ──────────────────────
            page.wait_for_url("**/note-detail/**", timeout=60_000)
            print("     [클로바] ✅ 노트 페이지 진입 - 전사 중...")

            # ── 업로드 완료 신호 → 병렬 작업 시작 ──────────────
            if ready_event:
                ready_event.set()
                print("     [클로바] 📢 병렬 작업 시작 신호 전송!")

            # ── JS로 다운로드 버튼 나타날 때까지 무한 대기 ───────
            print("     [클로바] 전사 완료 대기 중... (다운로드 버튼 감시)")
            page.wait_for_function(
                """() => {
                    for (const btn of document.querySelectorAll('button')) {
                        if (btn.innerText.trim() === '다운로드') return true;
                    }
                    return false;
                }""",
                timeout=0
            )
            print("     [클로바] ✅ 전사 완료 - 다운로드 버튼 감지!")

            # ── txt 다운로드 ─────────────────────────────────────
            print("     [클로바] txt 다운로드 중...")
            page.get_by_role("button", name="다운로드").click()
            page.wait_for_timeout(500)

            with page.expect_download(timeout=15000) as dl_info:
                page.get_by_role("button", name="음성 기록 다운로드").click(timeout=15000)
            download = dl_info.value
            download.save_as(txt_save_path)
            print(f"     [클로바] ✅ txt 저장 완료: {txt_save_path}")
            return txt_save_path

        except Exception as e:
            print(f"     [클로바] ❌ 오류: {e}")
            if ready_event and not ready_event.is_set():
                ready_event.set()  # 오류 시에도 블로킹 해제
            return None
        finally:
            context.close()
            browser.close()

# ================================================================
#  ChatGPT 자동화
# ================================================================

def run_chatgpt_summary(txt_path: str) -> str | None:
    """
    기존 Chrome 프로필로 ChatGPT 프로젝트 접속 →
    txt 파일 업로드 → 요약 답변 수신 → 텍스트 반환
    """
    from playwright.sync_api import sync_playwright

    kill_chrome()  # Chrome 실행 중이면 종료 후 프로필 열기

    with sync_playwright() as p:
        browser = p.chromium.launch_persistent_context(
            user_data_dir  = CHROME_USER_DATA,
            channel        = "chrome",
            headless       = False,
            slow_mo        = 400,
            args           = [f"--profile-directory={CHROME_PROFILE}"],
        )
        page = browser.new_page()

        try:
            print("     ChatGPT 프로젝트 접속 중...")
            page.goto(CHATGPT_PROJECT_URL, wait_until="networkidle", timeout=30000)
            page.wait_for_timeout(2000)

            # ── 파일 첨부 버튼 클릭 ──────────────────────────────
            print("     txt 파일 업로드 중...")
            attach_btn = page.locator(
                "button[aria-label='파일 첨부'], "
                "button[aria-label='Attach files'], "
                "button[data-testid='attach-button'], "
                "label[for='file-input']"
            ).first

            with page.expect_file_chooser() as fc_info:
                attach_btn.click()
            fc_info.value.set_files(txt_path)
            page.wait_for_timeout(1500)

            # ── 전송 ────────────────────────────────────────────
            send_btn = page.locator(
                "button[data-testid='send-button'], "
                "button[aria-label='Send message'], "
                "button[aria-label='메시지 보내기']"
            ).first
            send_btn.click()

            # ── 답변 완료 대기 ───────────────────────────────────
            print("     GPT 요약 대기 중... (최대 3분)")
            # 전송 버튼이 다시 활성화되면 완료
            page.wait_for_selector(
                "button[data-testid='send-button']:not([disabled]), "
                "button[aria-label='Send message']:not([disabled])",
                timeout=180_000
            )
            page.wait_for_timeout(1500)

            # ── 마지막 assistant 메시지 추출 ─────────────────────
            messages = page.locator(
                "[data-message-author-role='assistant'], "
                ".markdown.prose"
            ).all()
            if not messages:
                print("     ❌ 답변을 찾지 못했습니다.")
                return None

            gpt_text = messages[-1].inner_text()
            print("     ✅ GPT 요약 수신 완료")
            return gpt_text

        except Exception as e:
            print(f"     ❌ ChatGPT 오류: {e}")
            return None
        finally:
            browser.close()

# ================================================================
#  Notion 파일 첨부 자동화 (Playwright + 기존 Chrome 프로필)
# ================================================================

def kill_chrome():
    """실행 중인 Chrome 프로세스 종료 (프로필 잠금 해제)"""
    import subprocess
    try:
        subprocess.run(["taskkill", "/F", "/IM", "chrome.exe"], 
                      capture_output=True, timeout=5)
        time.sleep(2)  # 종료 대기
        print("     Chrome 프로세스 종료 완료")
    except Exception:
        pass


def upload_files_to_notion(page_url: str, m4a_path: str, txt_path: str):
    """
    기존 Chrome 프로필로 Notion 페이지 접속 →
    /오디오 입력 → 엔터 → m4a 업로드
    /파일 입력 → 엔터 → txt 업로드
    """
    from playwright.sync_api import sync_playwright

    kill_chrome()  # Chrome 실행 중이면 종료 후 프로필 열기

    with sync_playwright() as p:
        browser = p.chromium.launch_persistent_context(
            user_data_dir = CHROME_USER_DATA,
            channel       = "chrome",
            headless      = False,
            slow_mo       = 400,
            args          = [f"--profile-directory={CHROME_PROFILE}"],
        )
        page = browser.new_page()

        try:
            print("     Notion 페이지 접속 중...")
            page.goto(page_url, wait_until="networkidle", timeout=30000)
            page.wait_for_timeout(2000)

            # ── m4a 업로드: /오디오 블록 생성 후 파일 선택 ───────
            if m4a_path and os.path.exists(m4a_path):
                print("     m4a 업로드 중... (/오디오 블록 생성)")

                # 페이지 맨 아래 빈 줄 클릭 후 /오디오 입력
                page.locator("[contenteditable=true]").last.click()
                page.keyboard.press("End")
                page.keyboard.press("Enter")
                page.keyboard.type("/오디오")
                page.wait_for_timeout(800)
                page.keyboard.press("Enter")
                page.wait_for_timeout(1000)

                # "파일을 선택하세요" 버튼 클릭 → 파일 선택
                with page.expect_file_chooser() as fc_info:
                    page.get_by_role("button", name="파일을 선택하세요").last.click()
                fc_info.value.set_files(m4a_path)
                page.wait_for_timeout(4000)
                print("     ✅ m4a 업로드 완료")

            # ── txt 업로드: /파일 블록 생성 후 파일 선택 ─────────
            if txt_path and os.path.exists(txt_path):
                print("     txt 업로드 중... (/파일 블록 생성)")

                # 맨 아래 빈 줄 클릭 후 /파일 입력
                page.locator("[contenteditable=true]").last.click()
                page.keyboard.press("End")
                page.keyboard.press("Enter")
                page.keyboard.type("/파일")
                page.wait_for_timeout(800)
                page.keyboard.press("Enter")
                page.wait_for_timeout(1000)

                # "파일을 선택하세요" 버튼 클릭 → 파일 선택
                with page.expect_file_chooser() as fc_info:
                    page.get_by_role("button", name="파일을 선택하세요").last.click()
                fc_info.value.set_files(txt_path)
                page.wait_for_timeout(4000)
                print("     ✅ txt 업로드 완료")

        except Exception as e:
            print(f"     ❌ Notion 파일 업로드 오류: {e}")
        finally:
            browser.close()


# ================================================================
#  Notion 콘텐츠 블록 삽입
# ================================================================

def chunk_text(text: str, size: int = 1900):
    """Notion rich_text 2000자 제한 대응"""
    return [text[i:i+size] for i in range(0, len(text), size)]

def make_text_blocks(title: str, content: str) -> list:
    """섹션 제목 + 내용 단락 블록 생성"""
    blocks = [
        {
            "object": "block",
            "type": "heading_3",
            "heading_3": {
                "rich_text": [{"type": "text", "text": {"content": title}}]
            }
        }
    ]
    for chunk in chunk_text(content):
        blocks.append({
            "object": "block",
            "type": "paragraph",
            "paragraph": {
                "rich_text": [{"type": "text", "text": {"content": chunk}}]
            }
        })
    return blocks

def add_resource_skeleton(page_id: str, filename_base: str):
    """클로바노트 대기 중 Notion에 리소스 뼈대만 먼저 삽입"""
    m4a_name = f"{filename_base}.m4a"
    txt_name = f"{filename_base}.txt"
    blocks = [
        {"object": "block", "type": "divider", "divider": {}},
        {
            "object": "block", "type": "heading_3",
            "heading_3": {"rich_text": [{"type": "text", "text": {"content": "📁 회의 리소스"}}]}
        },
        {
            "object": "block", "type": "bulleted_list_item",
            "bulleted_list_item": {
                "rich_text": [{"type": "text", "text": {"content": "음성녹음"},
                               "annotations": {"bold": True}}]
            }
        },
        {
            "object": "block", "type": "paragraph",
            "paragraph": {"rich_text": [{"type": "text", "text": {"content": m4a_name},
                                         "annotations": {"code": True}}]}
        },
        {"object": "block", "type": "paragraph", "paragraph": {"rich_text": []}},
        {
            "object": "block", "type": "bulleted_list_item",
            "bulleted_list_item": {
                "rich_text": [{"type": "text", "text": {"content": "음성기록"},
                               "annotations": {"bold": True}}]
            }
        },
        {
            "object": "block", "type": "paragraph",
            "paragraph": {"rich_text": [{"type": "text", "text": {"content": txt_name},
                                         "annotations": {"code": True}}]}
        },
        {"object": "block", "type": "divider", "divider": {}},
        # 전사/요약 자리 표시자
        {
            "object": "block", "type": "heading_3",
            "heading_3": {"rich_text": [{"type": "text", "text": {"content": "📝 전사 내용 (화자 구분)"}}]}
        },
        {
            "object": "block", "type": "paragraph",
            "paragraph": {"rich_text": [{"type": "text", "text": {"content": "⏳ 클로바노트 변환 완료 후 자동 삽입됩니다..."},
                                         "annotations": {"color": "gray"}}]}
        },
        {"object": "block", "type": "divider", "divider": {}},
        {
            "object": "block", "type": "heading_3",
            "heading_3": {"rich_text": [{"type": "text", "text": {"content": "🤖 GPT 요약"}}]}
        },
        {
            "object": "block", "type": "paragraph",
            "paragraph": {"rich_text": [{"type": "text", "text": {"content": "⏳ GPT 요약 완료 후 자동 삽입됩니다..."},
                                         "annotations": {"color": "gray"}}]}
        },
    ]
    resp = requests.patch(
        f"https://api.notion.com/v1/blocks/{page_id}/children",
        headers=NOTION_HEADERS,
        data=json.dumps({"children": blocks}),
    )
    if resp.status_code == 200:
        print("     ✅ Notion 기본 형식 삽입 완료")
    else:
        print(f"     ❌ 기본 형식 삽입 실패: {resp.status_code}")


def add_full_content_blocks(page_id: str, filename_base: str,
                             transcript: str, gpt_summary: str):
    """
    Notion 페이지에 전체 콘텐츠 삽입:
    📁 회의 리소스 / 📝 전사 내용 / 🤖 GPT 요약
    """
    m4a_name = f"{filename_base}.m4a"
    txt_name = f"{filename_base}.txt"

    blocks = [
        # ── 리소스 섹션 ──────────────────────────────────────
        {"object": "block", "type": "divider", "divider": {}},
        {
            "object": "block", "type": "heading_3",
            "heading_3": {"rich_text": [{"type": "text", "text": {"content": "📁 회의 리소스"}}]}
        },
        {
            "object": "block", "type": "bulleted_list_item",
            "bulleted_list_item": {
                "rich_text": [{"type": "text", "text": {"content": "음성녹음"},
                               "annotations": {"bold": True}}]
            }
        },
        {
            "object": "block", "type": "paragraph",
            "paragraph": {"rich_text": [{"type": "text", "text": {"content": m4a_name},
                                         "annotations": {"code": True}}]}
        },
        {"object": "block", "type": "paragraph", "paragraph": {"rich_text": []}},
        {
            "object": "block", "type": "bulleted_list_item",
            "bulleted_list_item": {
                "rich_text": [{"type": "text", "text": {"content": "음성기록"},
                               "annotations": {"bold": True}}]
            }
        },
        {
            "object": "block", "type": "paragraph",
            "paragraph": {"rich_text": [{"type": "text", "text": {"content": txt_name},
                                         "annotations": {"code": True}}]}
        },
        {"object": "block", "type": "divider", "divider": {}},
    ]

    # ── 전사 내용 ──────────────────────────────────────────
    if transcript:
        blocks += make_text_blocks("📝 전사 내용 (화자 구분)", transcript)
        blocks.append({"object": "block", "type": "divider", "divider": {}})

    # ── GPT 요약 ───────────────────────────────────────────
    if gpt_summary:
        blocks += make_text_blocks("🤖 GPT 요약", gpt_summary)
        blocks.append({"object": "block", "type": "divider", "divider": {}})

    # Notion API는 한 번에 최대 100개 블록
    for i in range(0, len(blocks), 100):
        batch = blocks[i:i+100]
        resp  = requests.patch(
            f"https://api.notion.com/v1/blocks/{page_id}/children",
            headers=NOTION_HEADERS,
            data=json.dumps({"children": batch}),
        )
        if resp.status_code != 200:
            print(f"     ❌ 블록 삽입 실패 (배치 {i//100+1}): {resp.status_code}\n{resp.text}")
            return

    print("     ✅ Notion 콘텐츠 삽입 완료")

# ================================================================
#  메인
# ================================================================

def main():
    print("=" * 56)
    print("  회의 자동화 스크립트 v4.0")
    print("=" * 56)

    # 1. Notion DB
    print("\n[1] Notion DB 확인 중...")
    db_id = get_or_create_db()
    if not db_id:
        input("\n[Enter] 종료"); sys.exit(1)

    # 2. 녹음 파일 탐색
    print(f"\n[2] 녹음 파일 탐색 중...")
    print(f"     경로: {RECORDING_FOLDER}")
    if not os.path.exists(RECORDING_FOLDER):
        print(f"\n[오류] 폴더를 찾을 수 없습니다.")
        input("\n[Enter] 종료"); sys.exit(1)

    latest = find_latest_m4a(RECORDING_FOLDER)
    if not latest:
        print("\n[오류] .m4a 파일이 없습니다.")
        input("\n[Enter] 종료"); sys.exit(1)

    rec_dt = get_file_datetime(latest)
    print(f"     찾은 파일 : {os.path.basename(latest)}")
    print(f"     녹음 날짜 : {rec_dt.strftime('%Y년 %m월 %d일 %H:%M')}")

    # 3. 미팅명 선택
    meeting_name = select_meeting_name()

    # 4. 파일명 변경
    print("\n[4] 파일명 변경 중...")
    new_path      = rename_file(latest, rec_dt, meeting_name)
    new_filename  = os.path.basename(new_path)
    filename_base = os.path.splitext(new_filename)[0]
    print(f"     완료: {new_filename}")

    # 5. Notion 페이지 생성 + 기본 형식 삽입 (클로바노트 대기 전에 미리)
    print("\n[5] Notion 페이지 생성 중...")
    page_id, page_url = create_notion_page(db_id, rec_dt, meeting_name)
    if not page_url:
        print("     ❌ 실패")
        input("\n[Enter] 종료"); sys.exit(1)
    print(f"     ✅ 완료  URL: {page_url}")

    # 기본 리소스 형식 미리 삽입 (전사/요약 없이 뼈대만)
    print("     Notion 기본 형식 삽입 중...")
    add_resource_skeleton(page_id, filename_base)

    # 6. 클로바노트 업로드 → 전사 완료 → txt 다운로드
    #    (Chromium 사용, Chrome 프로필과 충돌 없음)
    print("\n[6] 클로바노트 업로드 및 전사 대기 중...")
    txt_path = run_clova_upload_and_download(new_path, filename_base)

    transcript = ""
    if txt_path and os.path.exists(txt_path):
        with open(txt_path, "r", encoding="utf-8") as f:
            transcript = f.read()
        print(f"     전사 내용 미리보기: {transcript[:80]}...")
    else:
        print("     ⚠️  클로바노트 실패 - 전사 내용 없이 계속합니다.")

    # 7. Notion에 m4a 파일 첨부 (Chrome 프로필)
    print("\n[7] Notion에 m4a 파일 첨부...")
    upload_files_to_notion(page_url, new_path, "")

    # 8. Notion에 txt 파일 첨부 (Chrome 프로필)
    if txt_path and os.path.exists(txt_path):
        print("\n[8] Notion에 txt 파일 첨부...")
        upload_files_to_notion(page_url, "", txt_path)

    # 9. ChatGPT 요약 (Chrome 프로필)
    gpt_summary = ""
    if txt_path and os.path.exists(txt_path):
        print("\n[9] ChatGPT 요약 중...")
        gpt_summary = run_chatgpt_summary(txt_path) or ""
        if gpt_summary:
            print(f"     요약 미리보기: {gpt_summary[:80]}...")
        else:
            print("     ⚠️  ChatGPT 요약 실패 - 요약 없이 계속합니다.")
    else:
        print("\n[9] txt 파일 없음 - ChatGPT 요약 건너뜁니다.")

    # 10. Notion에 전사내용 + GPT요약 텍스트 삽입 (API)
    if transcript or gpt_summary:
        print("\n[10] Notion 페이지에 전사내용 + GPT요약 삽입 중...")
        add_full_content_blocks(page_id, filename_base, transcript, gpt_summary)
    else:
        print("\n[10] 삽입할 전사/요약 내용 없음 - 건너뜁니다.")

    # 완료 요약
    print("\n" + "=" * 56)
    print("  ✅ 완료 요약")
    print("=" * 56)
    print(f"  파일명  : {new_filename}")
    print(f"  날짜    : {rec_dt.strftime('%Y년 %m월 %d일 %H:%M')}")
    print(f"  txt     : {txt_path or '(생성 실패)'}")
    print(f"  Notion  : {page_url}")
    print("=" * 56)

    input("\n[Enter] 키를 누르면 종료됩니다...")


if __name__ == "__main__":
    main()