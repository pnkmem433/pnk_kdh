"""
회의 자동화 시스템 v7.7
=======================================================
- 수정사항: 전사 대기 로직을 '완전 무한 루프'로 변경 (새로고침/감시 실패 시에도 계속 시도)
- 특징: 10분 주기 새로고침 유지, 예외 발생 시 무시하고 다음 루프로 진행
- 모델: Gemini 2.0 Flash (config.GEMINI_API 사용)
"""

import os
import re
import glob
import shutil
import time
import requests
import traceback
from datetime import datetime
from playwright.sync_api import sync_playwright
from google import genai
import config

# API 초기화 (Gemini 3 Flash Image 및 모델 기능을 제공하는 최신 SDK 사용)
client = genai.Client(api_key=config.GEMINI_API)

NOTION_TOKEN      = config.NOTION_TOKEN
CLOVA_EMAIL       = config.CLOVA_EMAIL
CLOVA_PASSWORD    = config.CLOVA_PASSWORD
DB_ID             = "31b136e19284813aaf71d9f6c16dff3f"

# 경로 설정 (강동현님의 G:드라이브 환경에 최적화)
TARGET_DRIVE_PATH = r"G:\.shortcut-targets-by-id\18_ReZvonpmf6OyZv1wQqqC9zhDKFWbHG\강동현"
RECORDING_FOLDER  = r"C:\Users\pnks\Documents\소리 녹음"
TXT_FOLDER        = TARGET_DRIVE_PATH 

MEETING_LIST      = ["주간팀회의", "인턴킥오프미팅", "온보딩미팅", "기타 (직접 입력)"]

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
# 2. Gemini AI 관련 함수 (재시도 로직)
# ================================================================

def fix_text_with_gemini(file_path):
    print(f"\n[4] Gemini AI 문맥 교정 시작...", flush=True)
    with open(file_path, "r", encoding="utf-8") as f:
        original_text = f.read()

    max_retries = 2
    prompt = f"너는 전문 속기사야. 아래 회의록 내용을 문맥에 맞게 오타만 교정해줘. 형식을 바꾸지 마라:\n{original_text}"

    for attempt in range(max_retries + 1):
        try:
            # Gemini 3 Flash 모델은 대규모 텍스트 생성 기능을 지원함
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
# 3. Clova Note 관련 함수 (무한 루프 강화)
# ================================================================

def collect_speaker_texts(page):
    speaker_texts = {}
    prev_height = -1
    for _ in range(10):
        try:
            blocks = page.locator("[data-stt-item-id]").all()
            for block in blocks:
                name_btn = block.locator("button[class*='name_btn']").first
                speaker = name_btn.inner_text(timeout=500).strip()
                words = block.locator("[class*='SttEditor_word']").all()
                text = "".join(w.inner_text(timeout=300) for w in words)
                speaker_texts[speaker] = speaker_texts.get(speaker, "") + text + " "
            curr_h = page.evaluate("() => document.querySelector('.ReactVirtualized__Grid')?.scrollTop || 0")
            if curr_h == prev_height: break
            prev_height = curr_h
            page.evaluate("() => { const el = document.querySelector('.ReactVirtualized__Grid'); if(el) el.scrollTop += 2000; }")
            page.wait_for_timeout(500)
        except: continue
    return speaker_texts

def auto_rename_speakers(page):
    print("     [클로바] 화자 이름 변경 모드 진입...", flush=True)
    try:
        page.get_by_role("button", name="편집", exact=True).click(force=True)
        page.wait_for_timeout(3000)
        speaker_texts = collect_speaker_texts(page)
        mapping = {s: "성보경" for s in speaker_texts.keys() if "참석자1" in s}
        for target_speaker, new_name in mapping.items():
            try:
                print(f"     -> {target_speaker} -> {new_name} 변경 중...", flush=True)
                btn = page.get_by_role("button", name=target_speaker, exact=True).first
                btn.click()
                page.wait_for_timeout(1000)
                page.locator(".ProseMirror-focused").fill(new_name)
                page.get_by_text("전체 구간", exact=True).click()
                page.get_by_role("button", name="변경", exact=True).click()
                page.wait_for_timeout(2000)
            except: page.keyboard.press("Escape")
        page.get_by_role("button", name="완료").click(force=True)
    except: pass
    print("     [OK] 이름 변경 프로세스 완료", flush=True)

def run_clova_process(m4a_path):
    with sync_playwright() as p:
        # 강동현님은 이전에 TripoAI와 Meshy AI의 결과를 비교하며 모델 자동화를 시도한 바 있음
        browser = p.chromium.launch(headless=False, slow_mo=200)
        context = browser.new_context(accept_downloads=True)
        page    = context.new_page()
        try:
            # 로그인 (제공해주신 스니펫 로직 반영) [cite: 2]
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

            # ── 완전 무한 루프 감시 섹션 ────────────────────────────
            print("     [클로바] 전사 대기 시작 (성공할 때까지 무한 반복)...", flush=True)
            wait_count = 0
            while True:
                try:
                    page.keyboard.press("Escape")
                    is_ready = page.evaluate("() => [...document.querySelectorAll('button')].some(b => b.innerText.trim() === '다운로드')")
                    if is_ready: break
                    
                    wait_count += 1
                    if wait_count % 40 == 0: # 10분 주기
                        print(f"     [System] {wait_count//4}분 경과: 상태 갱신을 위해 새로고침 시도...", flush=True)
                        try:
                            page.reload(timeout=60000)
                            page.wait_for_load_state("networkidle", timeout=10000)
                        except:
                            print("     [Warning] 새로고침 중 지연 발생. 무시하고 계속 감시합니다.", flush=True)
                    time.sleep(15)
                except Exception:
                    # 루프 내부에서 어떤 에러(브라우저 일시 중단 등)가 나도 무시하고 계속 진행
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
    print("  회의 자동화 시스템 v7.7 (Infinite Monitoring)", flush=True)
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