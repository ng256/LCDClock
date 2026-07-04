/**
 * @file com.h
 * @brief Serial port communication and device discovery.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#ifndef COM_H
#define COM_H

#include <windows.h>

typedef struct {
    char portName[16];
    char instanceId[256];
    char friendlyName[256];
} DisplayDevice;

/**
 * @brief Enumerate all displays with given instance signature.
 * @param devices Array of DisplayDevice (user allocates).
 * @param maxDevices Max number of devices to fill.
 * @param signature Substring to match in instance path (e.g., "S142T16").
 * @return Number of devices found.
 */
int EnumerateDisplays(DisplayDevice* devices, int maxDevices, const char* signature);

/**
 * @brief Open COM port.
 * @return HANDLE or NULL.
 */
HANDLE OpenComPort(const char* port);

/**
 * @brief Configure port (8N1).
 */
BOOL SetupComPort(HANDLE hCom, DWORD baudRate);

/**
 * @brief Send text.
 */
void SendText(HANDLE hCom, const char* text);

/**
 * @brief Close port.
 */
void CloseComPort(HANDLE hCom);

#endif