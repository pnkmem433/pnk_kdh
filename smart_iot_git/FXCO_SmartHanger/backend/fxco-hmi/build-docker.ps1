# PowerShell 스크립트 - 도커 이미지 빌드

Write-Host "=== FXCO HMI 도커 이미지 빌드 시작 ===" -ForegroundColor Cyan

# 이미지 이름과 태그 설정
$IMAGE_NAME = "fxco-hmi"
$IMAGE_TAG = "latest"

# 도커 이미지 빌드
docker build -t "${IMAGE_NAME}:${IMAGE_TAG}" .

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "✅ 이미지 빌드 성공!" -ForegroundColor Green
    Write-Host "이미지 이름: ${IMAGE_NAME}:${IMAGE_TAG}"
    Write-Host ""
    Write-Host "다음 명령어로 이미지를 실행할 수 있습니다:" -ForegroundColor Yellow
    Write-Host "  docker run -p 3000:3000 --env-file .env ${IMAGE_NAME}:${IMAGE_TAG}"
    Write-Host ""
    Write-Host "또는 docker-compose 사용:" -ForegroundColor Yellow
    Write-Host "  docker-compose up -d"
} else {
    Write-Host ""
    Write-Host "❌ 이미지 빌드 실패!" -ForegroundColor Red
    exit 1
}




