@echo off
title Uninstall LCD Clock Service
echo Uninstalling LCD Clock service...
"%~dp0clock.exe" -u
echo.
echo If you see "Success", the service is uninstalled.
pause