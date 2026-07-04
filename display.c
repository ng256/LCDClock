/**
 * @file display.c
 * @brief Implementation of display formatting.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#include "display.h"
#include "com.h"

static size_t StrLen(const char* s) {
    const char* p = s;
    while (*p) ++p;
    return (size_t)(p - s);
}

static void StrCpy(char* dst, const char* src) {
    while ((*dst++ = *src++));
}

void PadCenter(char* out, const char* in, int width) {
    int len = (int)StrLen(in);
    if (len > width) len = width;
    int left = (width - len) / 2;
    int right = width - len - left;
    int i = 0;
    for (int j = 0; j < left; ++j) out[i++] = ' ';
    for (int j = 0; j < len; ++j) out[i++] = in[j];
    for (int j = 0; j < right; ++j) out[i++] = ' ';
    out[i] = '\0';
}

void BuildFrame(char* frame, const SYSTEMTIME* st, int width, const char* displayMode) {
    char timeStr[16], dateStr[16], line1[32], line2[32];
    wsprintfA(timeStr, "%02d:%02d:%02d", st->wHour, st->wMinute, st->wSecond);
    wsprintfA(dateStr, "%02d.%02d.%04d", st->wDay, st->wMonth, st->wYear);

    if (lstrcmpA(displayMode, "t") == 0) {
        PadCenter(line1, timeStr, width);
        StrCpy(frame, "\x0C");
        StrCpy(frame + 1, line1);
    } else if (lstrcmpA(displayMode, "d") == 0) {
        PadCenter(line1, dateStr, width);
        StrCpy(frame, "\x0C");
        StrCpy(frame + 1, line1);
    } else {
        PadCenter(line1, timeStr, width);
        PadCenter(line2, dateStr, width);
        wsprintfA(frame, "\x0C%s%s", line1, line2);
    }
}

void ClearFrame(HANDLE hCom, int width, int rows) {
    if (!hCom || hCom == INVALID_HANDLE_VALUE) return;
    /* Build a string of spaces: width * rows characters */
    int total = width * rows;
    char* spaces = (char*)HeapAlloc(GetProcessHeap(), 0, total + 2); /* +2 for \x0C and \0 */
    if (!spaces) return;
    spaces[0] = '\x0C';
    for (int i = 0; i < total; ++i) spaces[1 + i] = ' ';
    spaces[total + 1] = '\0';
    SendText(hCom, spaces);
    HeapFree(GetProcessHeap(), 0, spaces);
}