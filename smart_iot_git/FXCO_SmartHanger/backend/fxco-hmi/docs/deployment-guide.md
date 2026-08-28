# 배포 가이드 (Deployment Guide)

## 다른 서버에서 실행하는 방법

### 1. 이미지 이름 확인

현재 로컬에 있는 이미지:
- `fxco-backend:latest` (실제 서버에서 사용 중인 것으로 추정)
- `fxco-backend-local:latest`
- `fxco-backend-old:latest`
- `fxco-hmi-fxco-hmi:latest`

### 2. 이미지 빌드 방법

#### 방법 1: 프로젝트 이름으로 빌드 (권장)
```bash
docker build -t fxco-backend:latest .
```

#### 방법 2: 빌드 스크립트 사용
```bash
# PowerShell (Windows)
.\build-docker.ps1

# Bash (Linux/Mac)
./build-docker.sh
```

### 3. 서버에서 실행하는 방법

#### 방법 A: Docker Compose 사용 (개발 환경)
```bash
# 1. 프로젝트 디렉토리로 이동
cd /path/to/fxco-hmi

# 2. .env 파일 생성 및 설정
cp env.example .env
# .env 파일 편집하여 데이터베이스 정보 입력

# 3. Docker Compose로 실행
docker-compose up -d

# 4. 로그 확인
docker-compose logs -f

# 5. 중지
docker-compose down
```

#### 방법 B: Docker Compose 사용 (프로덕션)
```bash
# 1. 환경 변수 설정 (서버 환경 변수 또는 .env 파일)
export DB_HOST=your_db_host
export DB_PORT=3306
export DB_USERNAME=your_username
export DB_PASSWORD=your_password
export DB_DATABASE=your_database

# 2. 프로덕션 모드로 실행
docker-compose -f docker-compose.prod.yml up -d

# 3. 로그 확인
docker-compose -f docker-compose.prod.yml logs -f
```

#### 방법 C: Docker run 직접 사용
```bash
# 이미지가 이미 빌드되어 있는 경우
docker run -d \
  --name fxco-backend \
  -p 3000:3000 \
  --env-file .env \
  --restart unless-stopped \
  fxco-backend:latest

# 또는 환경 변수를 직접 지정
docker run -d \
  --name fxco-backend \
  -p 3000:3000 \
  -e DB_HOST=your_db_host \
  -e DB_PORT=3306 \
  -e DB_USERNAME=your_username \
  -e DB_PASSWORD=your_password \
  -e DB_DATABASE=your_database \
  -e PORT=3000 \
  -e NODE_ENV=production \
  --restart unless-stopped \
  fxco-backend:latest
```

### 4. 기존 이미지로 실행 (이미 빌드된 경우)

```bash
# 이미지 확인
docker images | grep fxco-backend

# 컨테이너 실행
docker run -d \
  --name fxco-backend \
  -p 3000:3000 \
  --env-file .env \
  --restart unless-stopped \
  fxco-backend:latest
```

### 5. 컨테이너 관리 명령어

```bash
# 실행 중인 컨테이너 확인
docker ps | grep fxco

# 모든 컨테이너 확인 (중지된 것 포함)
docker ps -a | grep fxco

# 컨테이너 로그 확인
docker logs fxco-backend
docker logs -f fxco-backend  # 실시간 로그

# 컨테이너 중지
docker stop fxco-backend

# 컨테이너 시작
docker start fxco-backend

# 컨테이너 재시작
docker restart fxco-backend

# 컨테이너 삭제
docker rm fxco-backend

# 컨테이너 중지 및 삭제
docker rm -f fxco-backend
```

### 6. 데이터베이스 마이그레이션 실행

서버에서 실행하기 전에 데이터베이스 마이그레이션을 실행해야 합니다:

```bash
# MySQL에 접속하여 마이그레이션 스크립트 실행
mysql -u [username] -p [database_name] < docs/migration-add-rack-table.sql

# 또는 Docker 컨테이너 내부에서 실행
docker exec -i fxco-backend mysql -u [username] -p [database_name] < docs/migration-add-rack-table.sql
```

### 7. 환경 변수 설정

`.env` 파일 예시:
```env
DB_HOST=192.168.1.67
DB_PORT=3306
DB_USERNAME=root
DB_PASSWORD=your_password
DB_DATABASE=cc_nanaland_mvp
PORT=3000
NODE_ENV=production
```

### 8. 네트워크 설정

Docker Compose를 사용하는 경우 네트워크가 자동으로 생성됩니다.
직접 실행하는 경우:

```bash
# 네트워크 생성 (필요한 경우)
docker network create fxco-network

# 네트워크에 연결하여 실행
docker run -d \
  --name fxco-backend \
  --network fxco-network \
  -p 3000:3000 \
  --env-file .env \
  fxco-backend:latest
```

### 9. 포트 확인

```bash
# 포트 사용 확인
netstat -tulpn | grep 3000
# 또는
lsof -i :3000

# Docker 포트 매핑 확인
docker port fxco-backend
```

### 10. 문제 해결

#### 컨테이너가 시작되지 않는 경우
```bash
# 로그 확인
docker logs fxco-backend

# 컨테이너 상태 확인
docker inspect fxco-backend
```

#### 데이터베이스 연결 오류
- `.env` 파일의 데이터베이스 정보 확인
- 데이터베이스 서버가 실행 중인지 확인
- 방화벽 설정 확인
- 네트워크 연결 확인

#### 포트 충돌
```bash
# 다른 프로세스가 3000 포트를 사용하는 경우
# docker-compose.yml에서 포트 변경:
# ports:
#   - "3001:3000"  # 호스트 포트를 3001로 변경
```

---

## 추정되는 실제 서버 실행 명령어

이미지 이름이 `fxco-backend:latest`인 것을 보면, 실제 서버에서는 다음과 같이 실행했을 가능성이 높습니다:

```bash
# 1. 이미지 빌드
docker build -t fxco-backend:latest .

# 2. 실행
docker run -d \
  --name fxco-backend \
  -p 3000:3000 \
  --env-file .env \
  --restart unless-stopped \
  fxco-backend:latest

# 또는 Docker Compose 사용
docker-compose up -d
```

---

## 참고사항

- 이미지 이름은 프로젝트에 따라 다를 수 있습니다 (`fxco-hmi`, `fxco-backend` 등)
- 실제 서버의 환경 변수는 `.env` 파일 또는 서버 환경 변수로 관리됩니다
- 프로덕션 환경에서는 `docker-compose.prod.yml`을 사용하는 것이 좋습니다
- 데이터베이스 마이그레이션은 컨테이너 실행 전에 완료되어야 합니다
