/**
 * @file install.c
 * @brief Installer for LCD Clock (without CRT).
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#define WIN32_LEAN_AND_MEAN
#include <initguid.h>   /* Must come before windows.h to define GUIDs */
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include "resource_install.h"

/* --------------------------------------------------------------------- */
/* CRT stubs                                                            */
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

void __chkstk(void) { }
void __main(void) { }
int _fltused = 0;

/* --------------------------------------------------------------------- */
/* Entry point stub                                                     */
/* --------------------------------------------------------------------- */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);

void WinMainCRTStartup(void) {
    HINSTANCE hInstance = GetModuleHandleA(NULL);
    LPSTR cmdLine = GetCommandLineA();
    int result = WinMain(hInstance, NULL, cmdLine, SW_SHOWDEFAULT);
    ExitProcess(result);
}

/* --------------------------------------------------------------------- */
/* Helper functions                                                     */
/* --------------------------------------------------------------------- */
static void LoadStringToBuffer(int id, char* buffer, int size) {
    if (!LoadStringA(GetModuleHandleA(NULL), id, buffer, size))
        buffer[0] = '\0';
}

static BOOL CopyFileWithProgress(const char* src, const char* dst) {
    return CopyFileA(src, dst, FALSE);
}

static BOOL CreateDirectoryRecursive(const char* path) {
    char tmp[MAX_PATH];
    char* p = NULL;
    if (!path) return FALSE;
    lstrcpynA(tmp, path, MAX_PATH);
    size_t len = lstrlenA(tmp);
    if (tmp[len-1] == '\\') tmp[len-1] = '\0';
    for (p = tmp + 1; *p; p++) {
        if (*p == '\\') {
            *p = '\0';
            CreateDirectoryA(tmp, NULL);
            *p = '\\';
        }
    }
    return CreateDirectoryA(tmp, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

static BOOL GetSpecialFolderPath(int csidl, char* out, DWORD size) {
    (void)size; /* unused, caller is responsible for buffer length */
    return SHGetFolderPathA(NULL, csidl, NULL, SHGFP_TYPE_CURRENT, out) == S_OK;
}

static BOOL CreateShortcut(const char* targetPath, const char* shortcutPath) {
    IShellLinkA* psl = NULL;
    IPersistFile* ppf = NULL;
    HRESULT hr;
    BOOL result = FALSE;
    hr = CoInitialize(NULL);
    if (FAILED(hr)) return FALSE;
    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkA, (void**)&psl);
    if (SUCCEEDED(hr)) {
        psl->lpVtbl->SetPath(psl, targetPath);
        psl->lpVtbl->SetWorkingDirectory(psl, targetPath);
        hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (void**)&ppf);
        if (SUCCEEDED(hr)) {
            wchar_t wShortcut[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, shortcutPath, -1, wShortcut, MAX_PATH);
            hr = ppf->lpVtbl->Save(ppf, wShortcut, TRUE);
            if (SUCCEEDED(hr)) result = TRUE;
            ppf->lpVtbl->Release(ppf);
        }
        psl->lpVtbl->Release(psl);
    }
    CoUninitialize();
    return result;
}

/* --------------------------------------------------------------------- */
/* Installation logic                                                   */
/* --------------------------------------------------------------------- */
static BOOL InstallNormal(const char* srcPath, const char* exeName) {
    char destFolder[MAX_PATH], iniSrc[MAX_PATH], icoSrc[MAX_PATH];
    char destExe[MAX_PATH], destIni[MAX_PATH], destIco[MAX_PATH];
    char desktopPath[MAX_PATH], shortcutPath[MAX_PATH];
    BOOL ok = TRUE;

    if (!GetSpecialFolderPath(CSIDL_LOCAL_APPDATA, destFolder, MAX_PATH))
        return FALSE;
    lstrcatA(destFolder, "\\LCDClock");
    if (!CreateDirectoryRecursive(destFolder)) return FALSE;

    lstrcpynA(iniSrc, srcPath, MAX_PATH);
    lstrcpynA(icoSrc, srcPath, MAX_PATH);
    char* p = iniSrc + lstrlenA(iniSrc);
    while (p > iniSrc && *p != '\\') --p;
    if (p > iniSrc) p++;
    lstrcpyA(p, "clock.ini");
    p = icoSrc + lstrlenA(icoSrc);
    while (p > icoSrc && *p != '\\') --p;
    if (p > icoSrc) p++;
    lstrcpyA(p, "clock.ico");
    wsprintfA(destExe, "%s\\%s", destFolder, exeName);
    wsprintfA(destIni, "%s\\clock.ini", destFolder);
    wsprintfA(destIco, "%s\\clock.ico", destFolder);

    if (!CopyFileWithProgress(srcPath, destExe)) ok = FALSE;
    if (ok && !CopyFileWithProgress(iniSrc, destIni)) ok = FALSE;
    if (ok) CopyFileWithProgress(icoSrc, destIco);

    if (ok && GetSpecialFolderPath(CSIDL_DESKTOP, desktopPath, MAX_PATH)) {
        wsprintfA(shortcutPath, "%s\\LCD Clock.lnk", desktopPath);
        if (!CreateShortcut(destExe, shortcutPath)) {
            char msg[256];
            LoadStringToBuffer(IDS_SHORTCUT_ERROR, msg, sizeof(msg));
            MessageBoxA(NULL, msg, "Warning", MB_ICONWARNING);
        }
    }
    return ok;
}

static BOOL InstallService(const char* srcPath, const char* exeName) {
    char destFolder[MAX_PATH], iniSrc[MAX_PATH], icoSrc[MAX_PATH];
    char destExe[MAX_PATH], destIni[MAX_PATH], destIco[MAX_PATH];
    BOOL ok = TRUE;

    if (!GetSpecialFolderPath(CSIDL_PROGRAM_FILES, destFolder, MAX_PATH))
        return FALSE;
    lstrcatA(destFolder, "\\LCDClock");
    if (!CreateDirectoryRecursive(destFolder)) return FALSE;

    lstrcpynA(iniSrc, srcPath, MAX_PATH);
    lstrcpynA(icoSrc, srcPath, MAX_PATH);
    char* p = iniSrc + lstrlenA(iniSrc);
    while (p > iniSrc && *p != '\\') --p;
    if (p > iniSrc) p++;
    lstrcpyA(p, "clock.ini");
    p = icoSrc + lstrlenA(icoSrc);
    while (p > icoSrc && *p != '\\') --p;
    if (p > icoSrc) p++;
    lstrcpyA(p, "clock.ico");
    wsprintfA(destExe, "%s\\%s", destFolder, exeName);
    wsprintfA(destIni, "%s\\clock.ini", destFolder);
    wsprintfA(destIco, "%s\\clock.ico", destFolder);

    if (!CopyFileWithProgress(srcPath, destExe)) ok = FALSE;
    if (ok && !CopyFileWithProgress(iniSrc, destIni)) ok = FALSE;
    if (ok) CopyFileWithProgress(icoSrc, destIco);

    if (ok) {
        char cmdLine[MAX_PATH + 32];
        wsprintfA(cmdLine, "\"%s\" -is", destExe);
        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        if (!CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            ok = FALSE;
        } else {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        if (!ok) {
            char msg[256];
            LoadStringToBuffer(IDS_SERVICE_ERROR, msg, sizeof(msg));
            MessageBoxA(NULL, msg, "Error", MB_ICONERROR);
        }
    }
    return ok;
}

/* --------------------------------------------------------------------- */
/* Dialog procedure                                                     */
/* --------------------------------------------------------------------- */
static INT_PTR CALLBACK InstallDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
        case WM_INITDIALOG: {
            HICON hIcon = LoadIconA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(IDI_INSTALL));
            SendMessageA(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            CheckRadioButton(hDlg, IDC_RADIO_NORMAL, IDC_RADIO_SERVICE, IDC_RADIO_NORMAL);
            return TRUE;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BTN_INSTALL: {
                    int type = IsDlgButtonChecked(hDlg, IDC_RADIO_SERVICE) ? 1 : 0;
                    char srcPath[MAX_PATH];
                    GetModuleFileNameA(NULL, srcPath, MAX_PATH);
                    char exeName[] = "clock.exe";
                    char* lastSlash = srcPath + lstrlenA(srcPath);
                    while (lastSlash > srcPath && *lastSlash != '\\') --lastSlash;
                    if (lastSlash > srcPath) *(lastSlash + 1) = '\0';
                    lstrcatA(srcPath, exeName);
                    BOOL success;
                    if (type == 0)
                        success = InstallNormal(srcPath, exeName);
                    else
                        success = InstallService(srcPath, exeName);
                    if (success) {
                        char msg[256];
                        LoadStringToBuffer(IDS_SUCCESS, msg, sizeof(msg));
                        MessageBoxA(hDlg, msg, "Success", MB_OK);
                        EndDialog(hDlg, IDOK);
                    } else {
                        char msg[256];
                        LoadStringToBuffer(IDS_COPY_ERROR, msg, sizeof(msg));
                        MessageBoxA(hDlg, msg, "Error", MB_ICONERROR);
                    }
                    return TRUE;
                }
                case IDC_BTN_CANCEL:
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
            }
            break;
    }
    return FALSE;
}

/* --------------------------------------------------------------------- */
/* Entry point                                                          */
/* --------------------------------------------------------------------- */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    DialogBoxA(hInstance, MAKEINTRESOURCEA(IDD_INSTALL_DIALOG), NULL, InstallDlgProc);
    return 0;
}