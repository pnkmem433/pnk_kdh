# ===========================================
# Dockerfile for static web deployment
#
# 목적:
# - nginx:alpine 이미지를 기반으로 정적 웹 페이지를 서비스하는 컨테이너를 만든다.
# - 현재 저장소 루트의 index.html 을 nginx 기본 웹 루트로 복사한다.
#
# 현재 구조:
# - Dockerfile, docker-compose.yml, index.html 이 모두 저장소 루트에 있다.
# - docker-compose.yml 의 build: . 은 이 파일이 있는 현재 루트를 빌드 컨텍스트로 사용한다.
# ===========================================

# 가볍고 널리 쓰이는 nginx Alpine 이미지를 사용한다.
FROM nginx:alpine

# 루트의 index.html 을 nginx 기본 정적 파일 경로에 복사한다.
COPY index.html /usr/share/nginx/html/index.html
