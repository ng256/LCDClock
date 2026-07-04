/**
 * @file config.c
 * @brief Load from INI using new convenience functions.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#include "config.h"
#include "ini.h"

static char* ReadTextFile(const char* name, HANDLE heap) {
    HANDLE h = CreateFileA(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) return NULL;
    DWORD size = GetFileSize(h, NULL);
    if (size == INVALID_FILE_SIZE) { CloseHandle(h); return NULL; }
    char* buf = (char*)HeapAlloc(heap, 0, size + 1);
    if (!buf) { CloseHandle(h); return NULL; }
    DWORD read;
    if (!ReadFile(h, buf, size, &read, NULL)) { HeapFree(heap, 0, buf); CloseHandle(h); return NULL; }
    buf[read] = '\0';
    CloseHandle(h);
    return buf;
}

BOOL LoadConfig(AppConfig* config, const char* iniFileName) {
    HANDLE heap = GetProcessHeap();
    char* data = ReadTextFile(iniFileName, heap);
    IniFile* ini = NULL;

    // defaults
    config->baudRate = 9600;
    config->columns = 20;
    config->rows = 2;
    lstrcpyA(config->displayMode, "dt");

    if (data) {
        ini = IniParse(data, heap, INI_FLAG_NONE);
        HeapFree(heap, 0, data);
    }
    if (!ini) return TRUE;

    config->baudRate = IniGetInteger(ini, "settings", "speed", 9600);
    // validate baud rate
    if (config->baudRate != 9600 && config->baudRate != 19200 && config->baudRate != 115200)
        config->baudRate = 9600;

    config->columns = IniGetInteger(ini, "settings", "columns", 20);
    if (config->columns <= 0 || config->columns > 80) config->columns = 20;

    config->rows = IniGetInteger(ini, "settings", "rows", 2);
    if (config->rows != 1 && config->rows != 2) config->rows = 2;

    const char* mode = IniGetString(ini, "settings", "display", "dt");
    if (lstrcmpA(mode, "t") == 0 || lstrcmpA(mode, "d") == 0 || lstrcmpA(mode, "dt") == 0)
        lstrcpyA(config->displayMode, mode);
    else
        lstrcpyA(config->displayMode, "dt");

    IniFree(ini);
    return TRUE;
}