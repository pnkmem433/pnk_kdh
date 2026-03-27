import os
import platform
import subprocess

def create_repo_folder():
    # 1. 테스트할 깃 주소 입력 (예: https://github.com/pnkmem433/pnk_kdh.git)
    repo_url = input("테스트할 GitHub 주소를 입력하세요: ").strip()
    
    if not repo_url.endswith(".git"):
        print("에러: .git으로 끝나는 주소를 입력해주세요.")
        return
    
    # 저장소 이름 추출
    repo_name = repo_url.split('/')[-1].replace('.git', '')
    
    # 2. OS별 경로 자동 설정
    current_os = platform.system()
    if current_os == "Windows":
        # 윈도우: 내 문서/www/html/저장소명
        base_dir = os.path.join(os.path.expanduser("~"), "Documents", "www", "html")
    else:
        # 리눅스: /var/www/html/저장소명
        base_dir = "/var/www/html"

    target_path = os.path.join(base_dir, repo_name)

    # 3. 폴더 생성 실행
    print(f"\n[테스트 시작]")
    print(f"- 현재 OS: {current_os}")
    print(f"- 생성 시도 경로: {target_path}")

    try:
        # exist_ok=True: 이미 폴더가 있어도 에러를 내지 않음
        os.makedirs(target_path, exist_ok=True)
        print(f"✅ 결과: 폴더 생성 성공!")

        # 4. 리눅스 환경일 때만 권한 변경 시도
        if current_os != "Windows":
            user = os.getenv("USER")
            # sudo를 사용하여 소유권 변경
            subprocess.run(['sudo', 'chown', '-R', f"{user}:{user}", target_path], check=True)
            print(f"✅ 결과: 리눅스 소유권(chown) 설정 완료")

        print(f"\n테스트가 완료되었습니다. 해당 경로에 폴더가 있는지 확인해보세요.")

    except Exception as e:
        print(f"❌ 실패: {e}")

if __name__ == "__main__":
    create_repo_folder()