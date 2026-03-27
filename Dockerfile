# ===========================================
# Dockerfile for static web deployment
#
# 목적:
# - nginx:alpine 이미지를 기반으로 아주 가벼운 정적 웹 서버를 만든다.
# - 현재 저장소의 index.html 을 컨테이너 기본 웹 루트에 배치한다.
#
# 참고:
# - 추가 정적 자산(css, js, image 등)이 생기면 COPY 대상을 확장하면 된다.
# - 커스텀 nginx 설정이 필요해지면 nginx.conf 복사 단계를 추가하면 된다.
# ===========================================

# 가볍고 널리 쓰이는 nginx Alpine 이미지를 사용한다.
FROM nginx:alpine

# 기본 nginx 정적 파일 경로에 index.html 을 덮어써서 배포 페이지를 제공한다.
COPY index.html /usr/share/nginx/html/index.html

# 현재 이미지는 nginx 기본 CMD 를 그대로 사용하므로 별도 CMD 선언은 생략한다.
