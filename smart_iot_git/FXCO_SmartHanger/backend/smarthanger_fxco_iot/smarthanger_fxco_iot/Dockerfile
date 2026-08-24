# 1. Node 이미지
FROM node:20-alpine

# 2. 작업 디렉토리
WORKDIR /app

# 3. 패키지 파일 복사
COPY package*.json ./

# 4. 의존성 설치
RUN npm install

# 5. 소스 복사
COPY . .

# 6. 빌드
RUN npm run build

# 7. 실행
CMD ["node", "dist/main.js"]
