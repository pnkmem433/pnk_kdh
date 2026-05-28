@echo off
chcp 65001 > nul
cd /d C:\WS\vs_kdh\pnk_kdh
echo report_html 1-time deploy start
powershell -ExecutionPolicy Bypass -File ".\report_html\deploy_report_html.ps1"
