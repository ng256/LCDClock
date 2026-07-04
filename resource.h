/**
 * @file resource.h
 * @brief Resource IDs for strings and icons.
 * @license MIT
 * @copyright Copyright (c) 2025 Bashkardin Pavel
 */

#ifndef RESOURCE_IDS_H
#define RESOURCE_IDS_H

/* Icon IDs */
#define IDI_MAIN                    100

/* String IDs for messages */
#define IDS_APP_TITLE               200
#define IDS_TRAY_TOOLTIP            201
#define IDS_MENU_EXIT               202
#define IDS_MENU_SELECT_DISPLAY     203

/* Error messages */
#define IDS_ERR_NO_ADMIN            210
#define IDS_ERR_INSTALL_SERVICE     211
#define IDS_ERR_UNINSTALL_SERVICE   212
#define IDS_ERR_NO_DISPLAY          213
#define IDS_ERR_COM_INIT            214
#define IDS_ERR_WINDOW_CLASS        215
#define IDS_ERR_WINDOW_CREATE       216
#define IDS_ERR_TRAY_ICON           217
#define IDS_ERR_THREAD              218
#define IDS_ERR_MUTEX               219
#define IDS_ERR_CONFIG              220

/* Success messages */
#define IDS_SUCCESS_INSTALL         230
#define IDS_SUCCESS_UNINSTALL       231

/* Service strings */
#define IDS_SERVICE_NAME            240
#define IDS_SERVICE_DISPLAY_NAME    241
#define IDS_SERVICE_DESCRIPTION     242

/* Display signature (optional, keep as is) */
/* #define IDS_SIGNATURE               250 */ /* not needed, hardcoded */

#endif /* RESOURCE_IDS_H */