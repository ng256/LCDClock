/**
 * @file com.c
 * @brief Implementation of COM discovery and communication.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#include "com.h"
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>

static size_t StrLen(const char* s) {
    const char* p = s;
    while (*p) ++p;
    return (size_t)(p - s);
}

static BOOL StrStr(const char* str, const char* sub) {
    if (!*sub) return TRUE;
    while (*str) {
        const char* s1 = str, *s2 = sub;
        while (*s1 && *s2 && *s1 == *s2) ++s1, ++s2;
        if (!*s2) return TRUE;
        ++str;
    }
    return FALSE;
}

int EnumerateDisplays(DisplayDevice* devices, int maxDevices, const char* signature) {
    if (!devices || maxDevices <= 0) return 0;
    HDEVINFO devInfoSet = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, 0, 0, DIGCF_PRESENT);
    if (devInfoSet == INVALID_HANDLE_VALUE) return 0;

    SP_DEVINFO_DATA devInfoData;
    ZeroMemory(&devInfoData, sizeof(devInfoData));
    devInfoData.cbSize = sizeof(devInfoData);
    int count = 0;
    DWORD i;

    for (i = 0; SetupDiEnumDeviceInfo(devInfoSet, i, &devInfoData) && count < maxDevices; ++i) {
        char instanceId[256];
        if (!SetupDiGetDeviceInstanceIdA(devInfoSet, &devInfoData, instanceId, sizeof(instanceId), NULL))
            continue;
        if (!StrStr(instanceId, signature))
            continue;

        /* Get friendly name (e.g. "Prolific PL2303GS USB Serial COM Port (COM3)") */
        char friendlyName[256] = {0};
        if (!SetupDiGetDeviceRegistryPropertyA(devInfoSet, &devInfoData,
                                               SPDRP_FRIENDLYNAME,
                                               NULL, (PBYTE)friendlyName, sizeof(friendlyName), NULL)) {
            /* Fallback to device description if friendly name not available */
            SetupDiGetDeviceRegistryPropertyA(devInfoSet, &devInfoData,
                                              SPDRP_DEVICEDESC,
                                              NULL, (PBYTE)friendlyName, sizeof(friendlyName), NULL);
        }

        /* Get COM port name from registry */
        HKEY hKey = SetupDiOpenDevRegKey(devInfoSet, &devInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
        if (hKey) {
            char portName[16];
            DWORD size = sizeof(portName);
            if (RegQueryValueExA(hKey, "PortName", NULL, NULL, (LPBYTE)portName, &size) == ERROR_SUCCESS) {
                lstrcpynA(devices[count].portName, portName, sizeof(devices[count].portName));
                lstrcpynA(devices[count].instanceId, instanceId, sizeof(devices[count].instanceId));
                lstrcpynA(devices[count].friendlyName, friendlyName, sizeof(devices[count].friendlyName));
                ++count;
            }
            RegCloseKey(hKey);
        }
    }
    SetupDiDestroyDeviceInfoList(devInfoSet);
    return count;
}

HANDLE OpenComPort(const char* port) {
    char full[32];
    wsprintfA(full, "\\\\.\\%s", port);
    HANDLE h = CreateFileA(full, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    return (h == INVALID_HANDLE_VALUE) ? NULL : h;
}

BOOL SetupComPort(HANDLE hCom, DWORD baudRate) {
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(hCom, &dcb)) return FALSE;
    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    return SetCommState(hCom, &dcb);
}

void SendText(HANDLE hCom, const char* text) {
    if (!hCom || hCom == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(hCom, text, (DWORD)StrLen(text), &written, NULL);
}

void CloseComPort(HANDLE hCom) {
    if (hCom && hCom != INVALID_HANDLE_VALUE) CloseHandle(hCom);
}