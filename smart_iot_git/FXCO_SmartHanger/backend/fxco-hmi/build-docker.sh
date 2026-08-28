#!/bin/bash

# 도커 이미지 빌드 스크립트

echo "=== FXCO HMI 도커 이미지 빌드 시작 ==="

# 이미지 이름과 태그 설정
IMAGE_NAME="fxco-hmi"
IMAGE_TAG="latest"

# 도커 이미지 빌드
docker build -t ${IMAGE_NAME}:${IMAGE_TAG} .

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ 이미지 빌드 성공!"
    echo "이미지 이름: ${IMAGE_NAME}:${IMAGE_TAG}"
    echo ""
    echo "다음 명령어로 이미지를 실행할 수 있습니다:"
    echo "  docker run -p 3000:3000 --env-file .env ${IMAGE_NAME}:${IMAGE_TAG}"
    echo ""
    echo "또는 docker-compose 사용:"
    echo "  docker-compose up -d"
else
    echo ""
    echo "❌ 이미지 빌드 실패!"
    exit 1
fi




