@echo off
title Control LCD Clock Service
set SERVICE_NAME=LCDClockService

:menu
cls
echo ========================================
echo   LCD Clock Service Control
echo ========================================
sc query %SERVICE_NAME% >nul 2>&1
if %errorlevel% neq 0 (
    echo Service %SERVICE_NAME% is not installed.
    echo Please run install_service.bat first.
    pause
    exit /b 1
)

for /f "tokens=2 delims=: " %%a in ('sc query %SERVICE_NAME% ^| find "STATE"') do set STATUS=%%a
if "%STATUS%"=="RUNNING" (echo Status: RUNNING) else (echo Status: STOPPED)
echo.
echo 1. Start
echo 2. Stop
echo 3. Restart
echo 4. Show detailed status
echo 5. Exit
set /p CHOICE="Choice (1-5): "

if "%CHOICE%"=="1" (net start %SERVICE_NAME% & pause & goto menu)
if "%CHOICE%"=="2" (net stop %SERVICE_NAME% & pause & goto menu)
if "%CHOICE%"=="3" (net stop %SERVICE_NAME% >nul 2>&1 & net start %SERVICE_NAME% & pause & goto menu)
if "%CHOICE%"=="4" (sc query %SERVICE_NAME% & pause & goto menu)
if "%CHOICE%"=="5" exit /b
goto menu