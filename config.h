/**
 * @file config.h
 * @brief Configuration.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>

typedef struct {
    DWORD baudRate;
    int columns;
    int rows;
    char displayMode[8];
} AppConfig;

BOOL LoadConfig(AppConfig* config, const char* iniFileName);

#endif