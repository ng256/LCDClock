/**
 * @file display.h
 * @brief Display formatting functions.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <windows.h>

void PadCenter(char* out, const char* in, int width);
void BuildFrame(char* frame, const SYSTEMTIME* st, int width, const char* displayMode);
void ClearFrame(HANDLE hCom, int width, int rows);

#endif