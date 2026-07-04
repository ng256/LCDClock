/**
 * @file ini.h
 * @brief Simple INI parser (ANSI).
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#ifndef INI_H
#define INI_H

#include <windows.h>

#define INI_FLAG_NONE 0
typedef struct IniFile IniFile;

IniFile* IniParse(const char* data, HANDLE heap, DWORD flags);
void IniFree(IniFile* ini);
const char* IniGetValue(IniFile* ini, const char* section, const char* key);
const char* IniGetString(IniFile* ini, const char* section, const char* key, const char* defaultValue);
int IniGetInteger(IniFile* ini, const char* section, const char* key, int defaultValue);
#endif