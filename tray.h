/**
 * @file tray.h
 * @brief Tray menu management.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#ifndef TRAY_H
#define TRAY_H

#include <windows.h>

typedef struct {
    HMENU hSubMenu;          // menu for display selection
    int currentDisplayIndex; // -1 if none
    int displayCount;
    char** displayNames;     // array of strings like "COM3 (DSBOS...)"
} TrayMenuData;

BOOL CreateTrayIcon(HWND hwnd, UINT callbackMsg, HICON hIcon, const char* tooltip);
void RemoveTrayIcon(void);
void UpdateTrayMenu(TrayMenuData* data, HWND hwnd);
void HandleTrayMessage(WPARAM wParam, LPARAM lParam, HWND hwnd, BOOL* running, TrayMenuData* menuData);

#endif