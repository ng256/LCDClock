@echo off
title Install LCD Clock Service
echo Installing LCD Clock service...
"%~dp0clock.exe" -is
echo.
echo If you see "Success", the service is installed.
pause