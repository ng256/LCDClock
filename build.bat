@echo off
setlocal

set TDM_GCC_PATH=C:\Program Files (x86)\Embarcadero\Dev-Cpp\TDM-GCC-64\bin
set PATH=%PATH%;%TDM_GCC_PATH%

set CC=gcc.exe
set STRIP=strip.exe
set UPX=upx.exe
set OBJDUMP=objdump.exe

set OUT=clock.exe
set SOURCES=nocrt.c main.c com.c display.c tray.c config.c ini.c service.c admin.c
set RESOURCE=resource.rc

echo ==============================
echo Building LCD Clock
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
    -lkernel32 -luser32 -lgdi32 -lshell32 -lsetupapi -ladvapi32 ^
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
echo.

echo ========================================
echo Dependencies of %OUT%:
echo ========================================
%OBJDUMP% -p %OUT% | find "DLL Name"

del resource.o 2>nul
pause
endlocal