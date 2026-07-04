/**
 * @file main.c
 * @brief Main: GUI (tray) or service entry point.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "com.h"
#include "display.h"
#include "tray.h"
#include "config.h"
#include "service.h"
#include "admin.h"
#include "resource.h"

static HANDLE g_hCom = NULL;
static volatile BOOL g_running = TRUE;
static AppConfig g_config;
static HWND g_hwnd = NULL;
static HINSTANCE g_hInst;

static TrayMenuData g_menuData = { NULL, -1, 0, NULL };
static CRITICAL_SECTION g_cs;
static HANDLE g_hThread = NULL;

/* Именованное событие для graceful shutdown */
static HANDLE g_hShutdownEvent = NULL;
static HANDLE g_hShutdownThread = NULL;

/* --------------------------------------------------------------------- */
/* Simple command line parser (ANSI)                                     */
/* --------------------------------------------------------------------- */
static int ParseCommandLineA(char* cmdLine, char* argv[], int maxArgs) {
    int argc = 0;
    char* p = cmdLine;
    int inQuote = 0;
    while (*p && argc < maxArgs) {
        while (*p && (*p == ' ' || *p == '\t')) ++p;
        if (!*p) break;
        if (*p == '"') {
            inQuote = 1;
            ++p;
        } else {
            inQuote = 0;
        }
        argv[argc] = p;
        ++argc;
        while (*p && (inQuote ? (*p != '"') : (*p != ' ' && *p != '\t'))) ++p;
        if (*p) {
            if (inQuote) {
                *p = '\0';
                ++p;
                while (*p && (*p == ' ' || *p == '\t')) ++p;
            } else {
                *p = '\0';
                ++p;
            }
        }
    }
    return argc;
}

/* --------------------------------------------------------------------- */
/* Display thread                                                        */
/* --------------------------------------------------------------------- */
static DWORD WINAPI DisplayThread(LPVOID param) {
    (void)param;
    char frame[128];
    while (g_running) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        BuildFrame(frame, &st, g_config.columns, g_config.displayMode);

        EnterCriticalSection(&g_cs);
        HANDLE hCom = g_hCom;
        LeaveCriticalSection(&g_cs);
        if (hCom) {
            SendText(hCom, frame);
        }
        Sleep(1000);
    }
    return 0;
}

/* --------------------------------------------------------------------- */
/* Rescan devices and rebuild menu                                       */
/* --------------------------------------------------------------------- */
static void RescanAndUpdateMenu(HWND hwnd) {
    DisplayDevice devices[16];
    int count = EnumerateDisplays(devices, 16, "S142T16");

    EnterCriticalSection(&g_cs);
    for (int i = 0; i < g_menuData.displayCount; ++i) {
        if (g_menuData.displayNames[i]) HeapFree(GetProcessHeap(), 0, g_menuData.displayNames[i]);
    }
    if (g_menuData.displayNames) HeapFree(GetProcessHeap(), 0, g_menuData.displayNames);

    g_menuData.displayCount = count;
    g_menuData.displayNames = (char**)HeapAlloc(GetProcessHeap(), 0, count * sizeof(char*));
    for (int i = 0; i < count; ++i) {
        char name[256];
        if (devices[i].friendlyName[0] != '\0') {
            lstrcpynA(name, devices[i].friendlyName, sizeof(name));
        } else {
            wsprintfA(name, "%s (%s)", devices[i].portName, devices[i].instanceId);
        }
        size_t len = lstrlenA(name) + 1;
        g_menuData.displayNames[i] = (char*)HeapAlloc(GetProcessHeap(), 0, len);
        lstrcpyA(g_menuData.displayNames[i], name);
    }
    if (g_menuData.currentDisplayIndex >= count) g_menuData.currentDisplayIndex = -1;
    LeaveCriticalSection(&g_cs);

    UpdateTrayMenu(&g_menuData, hwnd);
}

/* --------------------------------------------------------------------- */
/* Open display by index                                                 */
/* --------------------------------------------------------------------- */
static BOOL OpenDisplayByIndex(int idx, HWND hwnd) {
    DisplayDevice devices[16];
    int count = EnumerateDisplays(devices, 16, "S142T16");
    if (idx < 0 || idx >= count) return FALSE;

    HANDLE hNew = OpenComPort(devices[idx].portName);
    if (!hNew) return FALSE;
    if (!SetupComPort(hNew, g_config.baudRate)) {
        CloseComPort(hNew);
        return FALSE;
    }

    EnterCriticalSection(&g_cs);
    if (g_hCom) {
        ClearFrame(g_hCom, g_config.columns, g_config.rows);
        CloseComPort(g_hCom);
    }
    g_hCom = hNew;
    g_menuData.currentDisplayIndex = idx;
    LeaveCriticalSection(&g_cs);

    UpdateTrayMenu(&g_menuData, hwnd);
    return TRUE;
}

/* --------------------------------------------------------------------- */
/* Watchdog thread for hot-plug                                          */
/* --------------------------------------------------------------------- */
static DWORD WINAPI WatchdogThread(LPVOID param) {
    HWND hwnd = (HWND)param;
    while (g_running) {
        Sleep(3000);

        DisplayDevice devices[16];
        int count = EnumerateDisplays(devices, 16, "S142T16");

        EnterCriticalSection(&g_cs);
        int currentIdx = g_menuData.currentDisplayIndex;
        HANDLE hCurrent = g_hCom;
        LeaveCriticalSection(&g_cs);

        BOOL stillPresent = (currentIdx >= 0 && currentIdx < count);
        if (!stillPresent) {
            if (count > 0) {
                OpenDisplayByIndex(0, hwnd);
            } else {
                EnterCriticalSection(&g_cs);
                if (g_hCom) {
                    ClearFrame(g_hCom, g_config.columns, g_config.rows);
                    CloseComPort(g_hCom);
                    g_hCom = NULL;
                    g_menuData.currentDisplayIndex = -1;
                }
                LeaveCriticalSection(&g_cs);
                UpdateTrayMenu(&g_menuData, hwnd);
            }
        } else {
            if (hCurrent == NULL) {
                OpenDisplayByIndex(currentIdx, hwnd);
            }
        }
    }
    return 0;
}

/* --------------------------------------------------------------------- */
/* Helper: load string from resources                                    */
/* --------------------------------------------------------------------- */
static void LoadStringToBuffer(int id, char* buffer, int size) {
    if (!LoadStringA(GetModuleHandle(NULL), id, buffer, size))
        buffer[0] = '\0';
}

/* --------------------------------------------------------------------- */
/* Именованное событие для завершения                                    */
/* --------------------------------------------------------------------- */
static DWORD WINAPI ShutdownWaitThread(LPVOID param) {
    (void)param;
    WaitForSingleObject(g_hShutdownEvent, INFINITE);
    g_running = FALSE;
    PostQuitMessage(0);
    return 0;
}

static void InitShutdownEvent(void) {
    g_hShutdownEvent = CreateEventA(NULL, TRUE, FALSE, "Global\\LCDClockShutdownEvent");
    if (g_hShutdownEvent && GetLastError() == ERROR_ALREADY_EXISTS) {
        /* Сигнализируем существующему процессу */
        SetEvent(g_hShutdownEvent);
        /* Даём время на очистку (опционально) */
        Sleep(2000);
        CloseHandle(g_hShutdownEvent);
        g_hShutdownEvent = CreateEventA(NULL, TRUE, FALSE, "Global\\LCDClockShutdownEvent");
    }
    if (g_hShutdownEvent) {
        g_hShutdownThread = CreateThread(NULL, 0, ShutdownWaitThread, NULL, 0, NULL);
    }
}

static void CleanupShutdownEvent(void) {
    if (g_hShutdownEvent) {
        SetEvent(g_hShutdownEvent);  /* разбудить поток, если он ещё ждёт */
        CloseHandle(g_hShutdownEvent);
    }
    if (g_hShutdownThread) {
        WaitForSingleObject(g_hShutdownThread, 1000);
        CloseHandle(g_hShutdownThread);
    }
}

/* --------------------------------------------------------------------- */
/* Window procedure                                                      */
/* --------------------------------------------------------------------- */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_APP + 1) {
        HandleTrayMessage(wParam, lParam, hwnd, (BOOL*)&g_running, &g_menuData);
        return 0;
    }
    if (msg == WM_APP + 2) {
        EnterCriticalSection(&g_cs);
        int newIdx = g_menuData.currentDisplayIndex;
        LeaveCriticalSection(&g_cs);
        if (newIdx >= 0) OpenDisplayByIndex(newIdx, hwnd);
        return 0;
    }
    if (msg == WM_QUERYENDSESSION) {
        return TRUE;
    }
    if (msg == WM_ENDSESSION) {
        if (wParam) {
            EnterCriticalSection(&g_cs);
            if (g_hCom) {
                ClearFrame(g_hCom, g_config.columns, g_config.rows);
            }
            LeaveCriticalSection(&g_cs);
        }
        g_running = FALSE;
        PostQuitMessage(0);
        return 0;
    }
    if (msg == WM_POWERBROADCAST) {
        switch (wParam) {
            case PBT_APMSUSPEND:
                EnterCriticalSection(&g_cs);
                if (g_hCom) {
                    ClearFrame(g_hCom, g_config.columns, g_config.rows);
                }
                LeaveCriticalSection(&g_cs);
                return TRUE;
            case PBT_APMRESUMEAUTOMATIC:
                return TRUE;
        }
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    if (msg == WM_DESTROY) {
        g_running = FALSE;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/* --------------------------------------------------------------------- */
/* Build INI file name from executable path                              */
/* --------------------------------------------------------------------- */
static void GetIniFileName(char* out, DWORD outSize) {
    GetModuleFileNameA(NULL, out, outSize);
    char* p = out + lstrlenA(out);
    while (p > out && *p != '.') --p;
    if (p > out) lstrcpyA(p, ".ini");
    else lstrcatA(out, ".ini");
}

/* --------------------------------------------------------------------- */
/* GUI mode (tray)                                                       */
/* --------------------------------------------------------------------- */
static int RunGui(void) {
    char iniPath[MAX_PATH];
    GetIniFileName(iniPath, MAX_PATH);
    LoadConfig(&g_config, iniPath);

    InitializeCriticalSection(&g_cs);
    RescanAndUpdateMenu(NULL);

    if (g_menuData.displayCount > 0) {
        OpenDisplayByIndex(0, NULL);
    } else {
        char msg[256];
        LoadStringToBuffer(IDS_ERR_NO_DISPLAY, msg, sizeof(msg));
        MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
    }

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "TrayClockClass";
    if (!RegisterClassA(&wc)) {
        char msg[256];
        LoadStringToBuffer(IDS_ERR_WINDOW_CLASS, msg, sizeof(msg));
        MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
        return 1;
    }

    g_hwnd = CreateWindowExA(0, "TrayClockClass", "TrayClock", 0,
                             0,0,0,0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);
    if (!g_hwnd) {
        char msg[256];
        LoadStringToBuffer(IDS_ERR_WINDOW_CREATE, msg, sizeof(msg));
        MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
        return 1;
    }

    HICON hIcon = LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCEA(IDI_MAIN));
    if (!hIcon) hIcon = LoadIconA(NULL, IDI_APPLICATION);
    char tooltip[64];
    LoadStringToBuffer(IDS_TRAY_TOOLTIP, tooltip, sizeof(tooltip));
    CreateTrayIcon(g_hwnd, WM_APP + 1, hIcon, tooltip);

    InitShutdownEvent();

    g_hThread = CreateThread(NULL, 0, DisplayThread, NULL, 0, NULL);
    if (!g_hThread) {
        char msg[256];
        LoadStringToBuffer(IDS_ERR_THREAD, msg, sizeof(msg));
        MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
        RemoveTrayIcon();
        DestroyWindow(g_hwnd);
        return 1;
    }
    HANDLE hWatchdog = CreateThread(NULL, 0, WatchdogThread, g_hwnd, 0, NULL);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    g_running = FALSE;
    WaitForSingleObject(g_hThread, 2000);
    WaitForSingleObject(hWatchdog, 2000);
    CloseHandle(g_hThread);
    CloseHandle(hWatchdog);
    RemoveTrayIcon();
    EnterCriticalSection(&g_cs);
    if (g_hCom) {
        ClearFrame(g_hCom, g_config.columns, g_config.rows);
        CloseComPort(g_hCom);
    }
    LeaveCriticalSection(&g_cs);
    DeleteCriticalSection(&g_cs);

    for (int i = 0; i < g_menuData.displayCount; ++i)
        if (g_menuData.displayNames[i]) HeapFree(GetProcessHeap(), 0, g_menuData.displayNames[i]);
    if (g_menuData.displayNames) HeapFree(GetProcessHeap(), 0, g_menuData.displayNames);

    CleanupShutdownEvent();
    return 0;
}

/* --------------------------------------------------------------------- */
/* Entry point                                                           */
/* --------------------------------------------------------------------- */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)nCmdShow;
    (void)lpCmdLine;
    g_hInst = hInstance;

    HANDLE hMutex = CreateMutexA(NULL, TRUE, "Global\\LCDClockAppMutex");
    BOOL alreadyRunning = (hMutex && GetLastError() == ERROR_ALREADY_EXISTS);

    char* cmdLine = GetCommandLineA();
    char* argv[32];
    int argc = ParseCommandLineA(cmdLine, argv, 32);

    for (int i = 1; i < argc; ++i) {
        if (lstrcmpA(argv[i], "--service") == 0 || lstrcmpA(argv[i], "-s") == 0) {
            if (alreadyRunning) {
                char msg[256];
                LoadStringToBuffer(IDS_ERR_MUTEX, msg, sizeof(msg));
                MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
                if (hMutex) CloseHandle(hMutex);
                return 1;
            }
            RunAsService();
            return 0;
        }
        else if (lstrcmpA(argv[i], "--install-start") == 0 || lstrcmpA(argv[i], "-is") == 0) {
            if (!IsNtAdmin()) {
                char msg[256];
                LoadStringToBuffer(IDS_ERR_NO_ADMIN, msg, sizeof(msg));
                MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
                if (hMutex) CloseHandle(hMutex);
                return 1;
            }
            if (InstallService()) {
                if (hMutex) {
                    ReleaseMutex(hMutex);
                    CloseHandle(hMutex);
                    hMutex = NULL;
                }
                SC_HANDLE hSCM = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
                if (hSCM) {
                    SC_HANDLE hSvc = OpenServiceA(hSCM, SERVICE_NAME, SERVICE_START);
                    if (hSvc) {
                        if (StartServiceA(hSvc, 0, NULL)) {
                            MessageBoxA(NULL, "Service installed and started successfully.", "Success", MB_OK);
                        } else {
                            MessageBoxA(NULL, "Service installed but failed to start.", "Warning", MB_ICONWARNING);
                        }
                        CloseServiceHandle(hSvc);
                    } else {
                        MessageBoxA(NULL, "Service installed but could not open for start.", "Warning", MB_ICONWARNING);
                    }
                    CloseServiceHandle(hSCM);
                } else {
                    MessageBoxA(NULL, "Service installed but could not start (SCM error).", "Warning", MB_ICONWARNING);
                }
            } else {
                char msg[256];
                LoadStringToBuffer(IDS_ERR_INSTALL_SERVICE, msg, sizeof(msg));
                MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
            }
            if (hMutex) CloseHandle(hMutex);
            return 0;
        }
        else if (lstrcmpA(argv[i], "--install") == 0 || lstrcmpA(argv[i], "-i") == 0) {
            if (!IsNtAdmin()) {
                char msg[256];
                LoadStringToBuffer(IDS_ERR_NO_ADMIN, msg, sizeof(msg));
                MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
                if (hMutex) CloseHandle(hMutex);
                return 1;
            }
            if (InstallService()) {
                char msg[256];
                LoadStringToBuffer(IDS_SUCCESS_INSTALL, msg, sizeof(msg));
                MessageBoxA(NULL, msg, "Success", MB_OK);
            } else {
                char msg[256];
                LoadStringToBuffer(IDS_ERR_INSTALL_SERVICE, msg, sizeof(msg));
                MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
            }
            if (hMutex) CloseHandle(hMutex);
            return 0;
        }
        else if (lstrcmpA(argv[i], "--uninstall") == 0 || lstrcmpA(argv[i], "-u") == 0) {
            if (!IsNtAdmin()) {
                char msg[256];
                LoadStringToBuffer(IDS_ERR_NO_ADMIN, msg, sizeof(msg));
                MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
                if (hMutex) CloseHandle(hMutex);
                return 1;
            }
            if (UninstallService()) {
                char msg[256];
                LoadStringToBuffer(IDS_SUCCESS_UNINSTALL, msg, sizeof(msg));
                MessageBoxA(NULL, msg, "Success", MB_OK);
            } else {
                char msg[256];
                LoadStringToBuffer(IDS_ERR_UNINSTALL_SERVICE, msg, sizeof(msg));
                MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
            }
            if (hMutex) CloseHandle(hMutex);
            return 0;
        }
    }

    /* GUI */
    if (alreadyRunning) {
        char msg[256];
        LoadStringToBuffer(IDS_ERR_MUTEX, msg, sizeof(msg));
        MessageBoxA(NULL, msg, "Warning", MB_ICONINFORMATION);
        if (hMutex) CloseHandle(hMutex);
        return 0;
    }
    int result = RunGui();
    if (hMutex) { ReleaseMutex(hMutex); CloseHandle(hMutex); }
    return result;
}