/**
 * @file nocrt.c
 * @brief CRT replacement: custom entry point and compiler stubs to avoid msvcrt.dll.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#include <windows.h>

/* --------------------------------------------------------------------- */
/* Memory functions required by compiler                                 */
/* --------------------------------------------------------------------- */
void* memcpy(void* dest, const void* src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

void* memset(void* s, int c, size_t n) {
    char* p = (char*)s;
    while (n--) *p++ = (char)c;
    return s;
}

/* --------------------------------------------------------------------- */
/* Compiler stack checking and initialization                           */
/* --------------------------------------------------------------------- */
void __chkstk(void) {
    /* Required by GCC for large stack frames. No action needed. */
}

void __main(void) {
    /* GCC initialization routine. Does nothing in our environment. */
}

/* Optional: floating point support (not used, but may be referenced) */
int _fltused = 0;

/* --------------------------------------------------------------------- */
/* Custom entry point (replaces CRT startup)                            */
/* --------------------------------------------------------------------- */
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

void WinMainCRTStartup(void) {
    HINSTANCE hInstance = GetModuleHandleA(NULL);
    LPSTR cmdLine = GetCommandLineA();
    int result = WinMain(hInstance, NULL, cmdLine, SW_SHOWDEFAULT);
    ExitProcess(result);
}