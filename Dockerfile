# ===========================================
# Docker 레시피 (Dockerfile)
# 목적: nginx 웹 서버를 담은 도커 컨테이너 생성
# 특징: 아주 가벼운 alpine 리눅스 기반
# ===========================================

# 1. 베이스 이미지 선택: 가벼운 nginx 웹 서버
FROM nginx:alpine

# 2. 내가 만든 index.html을 도커 컨테이너의 웹 경로로 복사
# 호스트의 index.html → 컨테이너의 /usr/share/nginx/html/index.html
COPY index.html /usr/share/nginx/html/index.html

# 3. (선택) 추가 설정: nginx 설정 파일 복사 가능
# COPY nginx.conf /etc/nginx/nginx.conf

# ===========================================
# 실행 시 자동으로 nginx가 시작됨 (CMD는 nginx 이미지에 이미 설정됨)
# ===========================================
