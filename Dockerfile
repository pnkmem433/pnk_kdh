# ===========================================
# Dockerfile for deploy_root static web deployment
# ===========================================

FROM nginx:alpine

COPY index.html /usr/share/nginx/html/index.html
