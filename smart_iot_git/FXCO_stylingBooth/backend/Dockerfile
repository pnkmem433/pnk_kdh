# --- 빌드 스테이지 ---
    FROM node:20.11.1 AS builder
    WORKDIR /var/app
    ENV TZ=Asia/Seoul
    
    # 1. 의존성 설치
    COPY package*.json ./
    RUN npm install
    RUN npm rebuild bcrypt --build-from-source
    
    # 2. 소스 복사 및 빌드
    COPY . .
    RUN npm run build
    
    # --- 실행 스테이지 ---
    FROM node:20.11.1 AS runner
    WORKDIR /var/app
    ENV TZ=Asia/Seoul
    
    # 3. dist와 public 폴더 복사
    COPY --from=builder /var/app/dist ./dist
    COPY --from=builder /var/app/public ./public
    COPY package*.json ./
    
    # 4. 실행에 필요한 의존성만 설치
    RUN npm install --omit=dev
    
    EXPOSE 3000
    CMD ["node", "dist/main.js"]
    