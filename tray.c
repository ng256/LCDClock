/**
 * @file tray.c
 * @brief Implementation.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#include "tray.h"
#include <shellapi.h>

static NOTIFYICONDATAA g_nid = {0};
static BOOL g_iconAdded = FALSE;

BOOL CreateTrayIcon(HWND hwnd, UINT callbackMsg, HICON hIcon, const char* tooltip) {
    if (g_iconAdded) RemoveTrayIcon();
    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize = sizeof(NOTIFYICONDATAA);
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = callbackMsg;
    g_nid.hIcon = hIcon;
    lstrcpynA(g_nid.szTip, tooltip, sizeof(g_nid.szTip));
    g_iconAdded = Shell_NotifyIconA(NIM_ADD, &g_nid);
    return g_iconAdded;
}

void RemoveTrayIcon(void) {
    if (g_iconAdded) {
        Shell_NotifyIconA(NIM_DELETE, &g_nid);
        g_iconAdded = FALSE;
    }
}

void UpdateTrayMenu(TrayMenuData* data, HWND hwnd) {
    (void)data;
    (void)hwnd;
}

void HandleTrayMessage(WPARAM wParam, LPARAM lParam, HWND hwnd, BOOL* running, TrayMenuData* menuData) {
    if (lParam != WM_RBUTTONUP) return;
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMainMenu = CreatePopupMenu();

    HMENU hSubMenu = CreatePopupMenu();
    for (int i = 0; i < menuData->displayCount; ++i) {
        UINT flags = MF_STRING;
        if (i == menuData->currentDisplayIndex) flags |= MF_CHECKED;
        AppendMenuA(hSubMenu, flags, 2000 + i, menuData->displayNames[i]);
    }
    AppendMenuA(hMainMenu, MF_POPUP, (UINT_PTR)hSubMenu, "Select Display");
    AppendMenuA(hMainMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMainMenu, MF_STRING, 1001, "Exit");

    SetForegroundWindow(hwnd);
    int cmd = TrackPopupMenu(hMainMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hMainMenu);

    if (cmd >= 2000 && cmd < 2000 + menuData->displayCount) {
        menuData->currentDisplayIndex = cmd - 2000;
        PostMessageA(hwnd, WM_APP + 2, 0, 0);
    } else if (cmd == 1001) {
        *running = FALSE;
        PostQuitMessage(0);
    }
}