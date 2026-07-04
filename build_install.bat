@echo off
setlocal

set TDM_GCC_PATH=C:\TDM-GCC-64\bin
set PATH=%PATH%;%TDM_GCC_PATH%

set CC=gcc.exe
set STRIP=strip.exe
set UPX=upx.exe

set OUT=install.exe
set SOURCES=install.c
set RESOURCE=resource_install.rc

echo ==============================
echo Building Installer
echo ==============================

windres %RESOURCE% -o resource.o
if %errorlevel% neq 0 (
    echo Failed to compile resources.
    pause
    exit /b 1
)

%CC% %SOURCES% resource.o -o %OUT% ^
    -nostdlib -nodefaultlibs ^
    -lgcc -lmingw32 ^
    -lkernel32 -luser32 -lshell32 -ladvapi32 -lole32 -lshlwapi -luuid ^
    -mwindows ^
    -e WinMainCRTStartup ^
    -Wall -Wextra -O2

if %errorlevel% neq 0 (
    echo BUILD FAILED
    pause
    exit /b 1
)

echo Stripping...
%STRIP% %OUT% 2>nul

echo Compressing with UPX...
%UPX% --best --lzma %OUT% 2>nul

echo.
echo BUILD SUCCESS
echo Output: %OUT%

del resource.o 2>nul
pause
endlocal
