import os
import time
import argparse
import requests
import trimesh
import boto3
import certifi

# SSL 인증서 설정
os.environ['SSL_CERT_FILE'] = certifi.where()
os.environ['REQUESTS_CA_BUNDLE'] = certifi.where()

# ==========================================
# 🔑 발급받으신 Tripo API Key
# ==========================================
TRIPO_API_KEY = "tsk_G_IRoBEBeR3_vAiBNstb4gjF48roFQ1mS_Eb2lkxbfK"

BASE_URL = "https://api.tripo3d.ai/v2/openapi"
HEADERS = {"Authorization": f"Bearer {TRIPO_API_KEY}"}

def upload_3d_model_via_sts(file_path):
    print(f"\n[1/5] S3 보안 채널을 통해 3D 모델 업로드 중... ({os.path.basename(file_path)})")
    sts_url = f"{BASE_URL}/upload/sts/token"
    res = requests.post(sts_url, headers=HEADERS, json={"format": "stl"})
    if res.status_code != 200: return None
    sts_data = res.json().get("data", {})
    try:
        s3 = boto3.client('s3', aws_access_key_id=sts_data['sts_ak'], 
                          aws_secret_access_key=sts_data['sts_sk'], 
                          aws_session_token=sts_data['session_token'])
        s3.upload_file(file_path, sts_data['resource_bucket'], sts_data['resource_uri'])
        return {"object": {"bucket": sts_data['resource_bucket'], "key": sts_data['resource_uri']}}
    except Exception: return None

def submit_and_wait(payload, step_name):
    print(f"\n[작업] {step_name} 요청 중...")
    task_url = f"{BASE_URL}/task"
    res = requests.post(task_url, headers=HEADERS, json=payload)
    if res.status_code != 200: return None
    task_id = res.json().get("data", {}).get("task_id")
    while True:
        poll_res = requests.get(f"{BASE_URL}/task/{task_id}", headers=HEADERS).json()
        status = poll_res.get("data", {}).get("status")
        if status == "success": return poll_res.get("data")
        elif status in ["failed", "error"]: return None
        time.sleep(5)

def main(file_path, user_prompt):
    print("\n" + "="*50)
    print(" Tripo 3D 지능형 하이브리드 자동화 파이프라인")
    print("="*50)

    # 1. 업로드 및 등록
    file_obj = upload_3d_model_via_sts(file_path)
    if not file_obj: return
    import_res = submit_and_wait({"type": "import_model", "file": file_obj}, "모델 등록")
    if not import_res: return
    import_id = import_res.get("task_id")

    # 2. 프롬프트 분석 및 모드 결정
    # 질감 관련 키워드가 있으면 texture_model, 없으면 stylize_model 사용
    texture_keywords = ['색', '질감', '재질', '텍스처', 'color', 'texture', 'material', 'paint']
    is_texture_mode = any(keyword in user_prompt.lower() for keyword in texture_keywords)

    if is_texture_mode:
        print(f"\n💡 판단 결과: [질감/재질 변환 모드]로 실행합니다.")
        payload = {
            "type": "texture_model",
            "original_model_task_id": import_id,
            "texture_prompt": {"text": user_prompt},
            "texture_quality": "detailed"
        }
    else:
        print(f"\n💡 판단 결과: [모양/스타일 변환 모드]로 실행합니다.")
        payload = {
            "type": "stylize_model",
            "original_model_task_id": import_id,
            "prompt": user_prompt
        }

    # 3. 작업 실행 및 대기
    result = submit_and_wait(payload, "AI 변환 작업")
    if not result: return

    # 4. 결과물 처리 (STL 변환)
    model_url = result.get("output", {}).get("model")
    base_name = os.path.splitext(os.path.basename(file_path))[0]
    final_path = os.path.join(os.path.dirname(file_path), f"{base_name}_ai_result.stl")
    
    try:
        r = requests.get(model_url)
        with open("temp.glb", 'wb') as f: f.write(r.content)
        mesh = trimesh.load("temp.glb", force='mesh')
        mesh.export(final_path)
        print(f"   ✅ 최종 파일 생성: {final_path}")
        mesh.show(caption=f"AI Result: {user_prompt}")
    except Exception as e: print(f" 오류: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--file", required=True)
    parser.add_argument("--prompt", required=True)
    args = parser.parse_args()
    if os.path.exists(args.file): main(args.file, args.prompt)