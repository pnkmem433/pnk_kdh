import requests

# 1. 최신 토큰 확보
login_url = "http://gym907-0001.iptime.org:3004/auth/login"
user_data = {"id": "pnkmem433", "password": "12345678"}
token = requests.post(login_url, json=user_data).json()['token']['access_token']

# 2. 내 프로젝트 목록 조회
headers = {"Authorization": f"Bearer {token}"}
list_url = "http://gym907-0001.iptime.org:3004/projects/list"
projects = requests.get(list_url, headers=headers).json()

print("-" * 50)
if not projects:
    print("현재 계정에 등록된 프로젝트가 하나도 없습니다.")
    print("먼저 Swagger에서 프로젝트를 생성(/projects/create)해야 합니다.")
else:
    for p in projects:
        print(f"프로젝트 이름: {p['name']}")
        print(f"진짜 ID (숫자): {p['id']}")  # <--- 이 숫자를 확인하세요!
        print(f"UUID: {p['uuid']}")
print("-" * 50)