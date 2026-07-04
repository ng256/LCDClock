/**
 * @file service.c
 * @brief Windows service implementation.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#include "service.h"
#include "com.h"
#include "display.h"
#include "config.h"

static SERVICE_STATUS_HANDLE g_hStatusHandle = NULL;
static SERVICE_STATUS g_Status = {0};
static HANDLE g_hStopEvent = NULL;
static HANDLE g_hCom = NULL;
static AppConfig g_config;
static volatile BOOL g_running = FALSE;

/* Forward declarations */
static DWORD WINAPI ServiceWorkerThread(LPVOID lpParam);
static void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
static DWORD WINAPI ServiceCtrlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

/* --------------------------------------------------------------------- */
/* Service control handler                                               */
/* --------------------------------------------------------------------- */
static DWORD WINAPI ServiceCtrlHandler(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext) {
    (void)dwEventType;
    (void)lpEventData;
    (void)lpContext;
    switch (dwControl) {
        case SERVICE_CONTROL_STOP:
            g_Status.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus(g_hStatusHandle, &g_Status);
            g_running = FALSE;
            SetEvent(g_hStopEvent);
            break;
        case SERVICE_CONTROL_SHUTDOWN:
            g_running = FALSE;
            SetEvent(g_hStopEvent);
            break;
        case SERVICE_CONTROL_INTERROGATE:
            SetServiceStatus(g_hStatusHandle, &g_Status);
            break;
        default:
            break;
    }
    return NO_ERROR;
}

/* --------------------------------------------------------------------- */
/* Worker thread: updates display every second                           */
/* --------------------------------------------------------------------- */
static DWORD WINAPI ServiceWorkerThread(LPVOID lpParam) {
    (void)lpParam;
    char frame[128];
    while (g_running) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        BuildFrame(frame, &st, g_config.columns, g_config.displayMode);
        if (g_hCom) {
            SendText(g_hCom, frame);
        }
        WaitForSingleObject(g_hStopEvent, 1000);
    }
    return 0;
}

/* --------------------------------------------------------------------- */
/* ServiceMain - entry point for SCM                                     */
/* --------------------------------------------------------------------- */
static void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    (void)argc;
    (void)argv;
    HANDLE hThread = NULL;
    char iniPath[MAX_PATH];

    g_Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_Status.dwCurrentState = SERVICE_START_PENDING;
    g_Status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    g_Status.dwWin32ExitCode = NO_ERROR;
    g_Status.dwServiceSpecificExitCode = 0;
    g_Status.dwCheckPoint = 0;
    g_Status.dwWaitHint = 30000;

    g_hStatusHandle = RegisterServiceCtrlHandlerEx(SERVICE_NAME, ServiceCtrlHandler, NULL);
    if (!g_hStatusHandle) {
        return;
    }

    SetServiceStatus(g_hStatusHandle, &g_Status);

    GetModuleFileNameA(NULL, iniPath, MAX_PATH);
    char* p = iniPath + lstrlenA(iniPath);
    while (p > iniPath && *p != '.') --p;
    if (p > iniPath) lstrcpyA(p, ".ini");
    else lstrcatA(iniPath, ".ini");
    LoadConfig(&g_config, iniPath);

    DisplayDevice devices[16];
    int count = EnumerateDisplays(devices, 16, "S142T16");
    if (count == 0) {
        g_Status.dwCurrentState = SERVICE_STOPPED;
        g_Status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        g_Status.dwServiceSpecificExitCode = 1;
        SetServiceStatus(g_hStatusHandle, &g_Status);
        return;
    }
    g_hCom = OpenComPort(devices[0].portName);
    if (!g_hCom || !SetupComPort(g_hCom, g_config.baudRate)) {
        if (g_hCom) CloseComPort(g_hCom);
        g_Status.dwCurrentState = SERVICE_STOPPED;
        g_Status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        g_Status.dwServiceSpecificExitCode = 2;
        SetServiceStatus(g_hStatusHandle, &g_Status);
        return;
    }

    g_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!g_hStopEvent) {
        CloseComPort(g_hCom);
        g_Status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_hStatusHandle, &g_Status);
        return;
    }

    g_running = TRUE;
    g_Status.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_hStatusHandle, &g_Status);

    hThread = CreateThread(NULL, 0, ServiceWorkerThread, NULL, 0, NULL);
    if (!hThread) {
        g_running = FALSE;
        CloseHandle(g_hStopEvent);
        CloseComPort(g_hCom);
        g_Status.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_hStatusHandle, &g_Status);
        return;
    }

    WaitForSingleObject(g_hStopEvent, INFINITE);

    g_running = FALSE;
    WaitForSingleObject(hThread, 2000);
    CloseHandle(hThread);
    CloseHandle(g_hStopEvent);
    if (g_hCom) {
        ClearFrame(g_hCom, g_config.columns, g_config.rows);
        CloseComPort(g_hCom);
    }

    g_Status.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_hStatusHandle, &g_Status);
}

/* --------------------------------------------------------------------- */
/* Public functions                                                      */
/* --------------------------------------------------------------------- */
BOOL InstallService(void) {
    SC_HANDLE hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!hSCManager) return FALSE;

    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char cmdLine[MAX_PATH + 16];
    wsprintfA(cmdLine, "\"%s\" --service", path);

    SC_HANDLE hService = CreateServiceA(
        hSCManager,
        SERVICE_NAME,
        SERVICE_DISPLAY_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        cmdLine,
        NULL, NULL, NULL, NULL, NULL
    );
    if (!hService) {
        CloseServiceHandle(hSCManager);
        return FALSE;
    }
    SERVICE_DESCRIPTIONA desc = { SERVICE_DESCRIPTION };
    ChangeServiceConfig2A(hService, SERVICE_CONFIG_DESCRIPTION, &desc);
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return TRUE;
}

BOOL UninstallService(void) {
    SC_HANDLE hSCManager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCManager) return FALSE;
    SC_HANDLE hService = OpenServiceA(hSCManager, SERVICE_NAME, DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!hService) {
        CloseServiceHandle(hSCManager);
        return FALSE;
    }

    SERVICE_STATUS status;
    if (QueryServiceStatus(hService, &status) && status.dwCurrentState == SERVICE_RUNNING) {
        ControlService(hService, SERVICE_CONTROL_STOP, &status);
        for (int i = 0; i < 30; i++) {
            Sleep(1000);
            if (!QueryServiceStatus(hService, &status)) break;
            if (status.dwCurrentState == SERVICE_STOPPED) break;
        }
    }

    BOOL deleted = DeleteService(hService);
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    return deleted;
}

void RunAsService(void) {
    SERVICE_TABLE_ENTRY dispatchTable[] = {
        { SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { NULL, NULL }
    };
    StartServiceCtrlDispatcher(dispatchTable);
}