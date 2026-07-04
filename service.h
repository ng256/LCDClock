/**
 * @file service.h
 * @brief Windows service declarations.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#ifndef SERVICE_H
#define SERVICE_H

#include <windows.h>

#define SERVICE_NAME "LCDClockService"
#define SERVICE_DISPLAY_NAME "LCD Clock Service"
#define SERVICE_DESCRIPTION "Displays current time and date on a USB LCD display"

BOOL InstallService(void);
BOOL UninstallService(void);
void RunAsService(void);

#endif /* SERVICE_H */