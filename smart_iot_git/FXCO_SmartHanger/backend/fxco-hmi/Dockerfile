# Node.js 18 LTS 이미지 사용
FROM node:18-alpine AS builder

# 작업 디렉토리 설정
WORKDIR /app

# package.json과 package-lock.json 복사
COPY package*.json ./

# 모든 의존성 설치 (빌드를 위해 devDependencies 필요)
RUN npm ci

# 소스 코드 복사
COPY . .

# TypeScript 빌드
RUN npm run build

# 프로덕션 이미지
FROM node:18-alpine

WORKDIR /app

# package.json과 package-lock.json 복사
COPY package*.json ./

# 프로덕션 의존성만 설치
RUN npm ci --only=production && npm cache clean --force

# 빌드된 파일 복사
COPY --from=builder /app/dist ./dist

# 포트 노출
EXPOSE 3000

# 환경 변수는 런타임에 주입됨 (이미지에 포함되지 않음)
# .env 파일은 이미지에 포함되지 않습니다!

# 애플리케이션 실행
CMD ["node", "dist/main"]

