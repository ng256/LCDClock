/**
 * @file admin.c
 * @brief Implementation of administrator check.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#include "admin.h"

BOOL IsNtAdmin(void) {
    HANDLE hToken = NULL;
    TOKEN_ELEVATION elevation;
    DWORD size = sizeof(TOKEN_ELEVATION);
    BOOL result = FALSE;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        return FALSE;

    if (GetTokenInformation(hToken, TokenElevation, &elevation, size, &size))
        result = (elevation.TokenIsElevated != 0);

    CloseHandle(hToken);
    return result;
}