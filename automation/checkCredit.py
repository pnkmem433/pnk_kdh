import requests

# 사용자님의 API Key
API_KEY = "tsk_G_IRoBEBeR3_vAiBNstb4gjF48roFQ1mS_Eb2lkxbfK"
url = "https://api.tripo3d.ai/v2/openapi/user/balance"
headers = {
    "Content-Type": "application/json",
    "Authorization": f"Bearer {API_KEY}"
}

response = requests.get(url, headers=headers)

if response.status_code == 200:
    data = response.json().get("data", {})
    balance = data.get("balance")
    frozen = data.get("frozen")
    print(f"\n" + "="*30)
    print(f"💰 현재 잔액(Balance): {balance}")
    print(f"❄️ 작업 중인 금액(Frozen): {frozen}")
    print("="*30)
    
    if balance < 30: # 스타일 변환에는 보통 30 이상이 필요합니다.
        print("⚠️ 주의: 잔액이 부족하여 새로운 작업을 시작할 수 없습니다.")
else:
    print(f"❌ 조회 실패: {response.text}")