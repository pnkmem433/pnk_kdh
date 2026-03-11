@echo off
set SCRIPT_PATH=C:\WS\vs_kdh\pnk_kdh\automation\meeting
set VENV_PYTHON=%SCRIPT_PATH%\venv\Scripts\python.exe

echo [System] 회의 자동화 시스템을 시작합니다.
cd /d "%SCRIPT_PATH%"

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