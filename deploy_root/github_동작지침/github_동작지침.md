# 깃허브 동작 지침 (GitHub Action Guide)

이 파일은 AI(GitHub Copilot / Codex / Claude)가 이 저장소에서
git 관련 작업을 수행할 때 따라야 할 규칙과 절차를 정의합니다.
작업을 요청할 때 이 파일을 먼저 읽고 지침에 맞게 행동하세요.

---

## 1. 저장소 기본 정보

```
저장소 주소 : https://github.com/PNKsolution-CODE/SmartPlug_internship.git
기본 브랜치 : main
작업 폴더  : C:\Users\pnks\Documents\PlatformIO\Projects
언어/프레임워크 : C++ (PlatformIO / ESP32)
```

> 이 파일의 저장소 주소와 작업 폴더를 기준으로 모든 git 명령을 수행하세요.
> 다른 저장소 주소나 폴더로 명령을 실행하지 마세요.

---

## 2. 브랜치 규칙

- `main` 브랜치에 직접 커밋하거나 push하지 않습니다.
- 모든 작업은 아래 형식의 브랜치를 만들어서 진행합니다.

```
feature/기능명        새 기능 개발
fix/버그명            버그 수정
docs/문서명           문서 수정
refactor/대상명       리팩토링
hotfix/긴급수정명     긴급 수정
```

- 새 브랜치는 반드시 `main`에서 출발합니다.
  ```bash
  git checkout main
  git pull origin main
  git checkout -b feature/기능명
  ```
- 작업 전에 반드시 `git branch`로 현재 브랜치를 확인합니다.

---

## 3. 커밋 규칙

### 커밋 단위
- 논리적으로 하나의 작업이 완료될 때마다 커밋합니다.
- 여러 기능을 한 커밋에 묶지 않습니다.
- 작업 중인 상태(미완성 코드)는 커밋하지 않습니다.

### 커밋 메시지 형식
```
타입: 한줄 설명 (50자 이내, 명령형)
```

| 타입 | 사용 시점 |
|------|-----------|
| `feat` | 새 기능 추가 |
| `fix` | 버그 수정 |
| `docs` | 문서 수정 |
| `style` | 코드 스타일 변경 (기능 무관) |
| `refactor` | 리팩토링 |
| `test` | 테스트 추가·수정 |
| `chore` | 빌드 설정, 패키지 등 기타 |

### 커밋 메시지 예시
```
feat: MQTT 연결 재시도 로직 추가
fix: WiFi 연결 실패 시 null 포인터 오류 수정
docs: README 빌드 방법 추가
chore: platformio.ini 라이브러리 버전 업데이트
```

### 이슈 연결
- GitHub에 이슈가 등록되어 있으면 커밋 메시지 끝에 이슈 번호를 붙입니다.
  ```
  fix: 센서 초기화 오류 수정 #5
  ```

---

## 4. 작업 요청 시 AI 수행 절차

사용자가 작업을 요청하면 아래 순서로 수행합니다.

### 4-1. 새 기능 추가 요청
```
1. git checkout main
2. git pull origin main
3. git checkout -b feature/요청기능명
4. 요청한 코드 작성 또는 수정
5. git add .
6. git commit -m "feat: 요청 내용 요약"
7. git push origin feature/요청기능명
8. GitHub에서 PR 생성 안내 메시지 출력
```

### 4-2. 버그 수정 요청
```
1. git checkout main
2. git pull origin main
3. git checkout -b fix/버그명
4. 버그 수정
5. git add .
6. git commit -m "fix: 버그 내용 요약"
7. git push origin fix/버그명
8. GitHub에서 PR 생성 안내 메시지 출력
```

### 4-3. 코드 수정 후 커밋만 요청
```
1. git status  (현재 브랜치·변경 파일 확인)
2. 현재 브랜치가 main이면 → 브랜치 생성 후 진행
3. git add .
4. git commit -m "타입: 수정 내용 요약"
```

### 4-4. PR 생성 안내
push 완료 후 아래 메시지를 출력합니다:
```
✅ push 완료!
GitHub에서 PR을 생성하세요:
https://github.com/PNKsolution-CODE/SmartPlug_internship/pulls
→ Compare & pull request 버튼 클릭
→ 제목: [브랜치명에서 작업한 내용 요약]
→ 설명: 변경 사항 + Closes #이슈번호 (있으면)
```

---

## 5. 하면 안 되는 것

- `git push origin main` — main에 직접 push 금지
- `git push --force` — 강제 push 금지 (팀 협의 없이)
- `git commit -m "수정"` / `git commit -m "작업중"` — 의미 없는 메시지 금지
- 여러 기능을 한 커밋에 묶는 것 금지
- push 후 `git commit --amend` — push 후 커밋 수정 금지

---

## 6. 자주 발생하는 오류와 해결법

### 중첩 .git 오류
```
error: 'SmartPlug/' does not have a commit checked out
```
**원인**: 하위 폴더 안에 .git이 있는 경우 (git 안에 git)
**해결** (Windows PowerShell):
```powershell
Remove-Item -Recurse -Force 폴더명\.git
git add .
```
**해결** (Mac / Linux):
```bash
rm -rf 폴더명/.git
git add .
```

### push 거부 오류
```
! [rejected] main -> main (fetch first)
```
**원인**: GitHub 저장소에 로컬에 없는 커밋이 있음 (README 등)
**해결**:
```bash
git pull origin main --allow-unrelated-histories
# vim이 열리면 :wq 입력 후 엔터
git push origin main
```

### 브랜치 이름이 master로 생성된 경우
```bash
git branch -M main
git push -u origin main
```

---

## 7. 태그 (배포 버전 관리)

배포 가능한 버전이 완성됐을 때 태그를 붙입니다.
```bash
git tag -a v1.0.0 -m "버전 설명"
git push origin v1.0.0
```

버전 규칙:
- `v메이저.마이너.패치`
- 메이저: 하위 호환 안 되는 큰 변경
- 마이너: 새 기능 추가
- 패치: 버그 수정

---

## 8. 작업 요청 예시

아래처럼 요청하면 이 지침에 따라 수행합니다.

```
"MQTT 재연결 기능 추가해줘"
→ feature/mqtt-reconnect 브랜치 생성 후 코드 작성 + 커밋 + push 안내

"WiFi 연결 안 되는 버그 고쳐줘"
→ fix/wifi-connection 브랜치 생성 후 수정 + 커밋 + push 안내

"지금 변경사항 커밋해줘"
→ 현재 브랜치 확인 후 적절한 타입으로 커밋 메시지 작성

"v1.0.0 태그 붙여줘"
→ git tag -a v1.0.0 + push
```