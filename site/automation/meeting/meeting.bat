@echo off
:: 한글 깨짐 방지
chcp 65001 >nul

:: 1. 가상환경 및 스크립트 경로 설정
set BASE_PATH=C:\WS\vs_kdh\pnk_kdh\automation
set SCRIPT_FOLDER=%BASE_PATH%\meeting
set VENV_PYTHON=%BASE_PATH%\venv\Scripts\python.exe

echo [System] 회의 자동화 시스템을 시작합니다. (v7.8)

:: 2. 가상환경 파이썬 존재 여부 확인
if not exist "%VENV_PYTHON%" (
    echo [Error] 가상환경을 찾을 수 없습니다. 
    echo 경로 확인: %VENV_PYTHON%
    pause
    exit /b
)

:: 3. 스크립트 폴더로 이동
cd /d "%SCRIPT_FOLDER%"

:: 4. 실행 (가상환경의 파이썬으로 스크립트 호출)
"%VENV_PYTHON%" "meetingClova.py"

echo.
echo -------------------------------------------------------
echo [System] 모든 작업이 성공적으로 완료되었습니다.
echo [System] 10초 후에 이 창은 자동으로 닫힙니다.
echo -------------------------------------------------------
echo.

:: 10초 대기 (아무 키나 누르면 즉시 종료)
timeout /t 10
exit