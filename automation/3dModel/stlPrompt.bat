@echo off
:: 1. 한글 깨짐 방지 (UTF-8 모드)
chcp 65001 >nul

title Tripo 3D AI Intelligent Pipeline
:: 2. 지연된 확장 활성화 (특수문자 및 한글 에러 방지의 핵심)
setlocal enabledelayedexpansion

:: 작업 디렉토리 설정
cd /d "C:\WS\vs_kdh\pnk_kdh\automation"

:: 가상환경 체크
if exist "venv\Scripts\activate.bat" (
    call venv\Scripts\activate.bat
) else (
    echo [오류] venv를 찾을 수 없습니다.
    pause
    exit /b
)

:LOOP
cls
echo ==================================================
echo      Tripo 3D AI 지능형 자동화 시스템
echo ==================================================
echo.

:: 3. 변수 초기화 및 입력 받기
set "STL_PATH="
set "PROMPT_TEXT="

set /p STL_PATH="1. 원하는 stl 파일의 경로를 입력하세요: "
:: 따옴표 제거 후 다시 안전하게 감싸기
if defined STL_PATH set "STL_PATH=!STL_PATH:"=!"

set /p PROMPT_TEXT="2. AI에게 내릴 명령을 입력하세요: "

:: 4. 입력값 검증 (괄호 에러 방지를 위해 ! 사용)
if "!STL_PATH!"=="" (
    echo [오류] 경로가 비어있습니다.
    pause
    goto LOOP
)

echo.
echo --------------------------------------------------
echo >> 작업을 시작합니다...
echo --------------------------------------------------

:: 5. 파이썬 실행 (따옴표와 !를 사용하여 텍스트 통째로 전달)
python stlWithPrompt.py --file "!STL_PATH!" --prompt "!PROMPT_TEXT!"

echo.
echo ==================================================
echo 작업 완료!
echo ==================================================
echo [1] 다시 실행  [2] 종료
set /p MENU="선택: "

:: 비교 시에도 !변수! 형식을 써야 "을/를" 에러가 안 납니다.
if "!MENU!"=="1" goto LOOP

deactivate
exit