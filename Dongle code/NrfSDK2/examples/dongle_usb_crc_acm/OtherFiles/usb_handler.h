#ifndef USB_HANDLER_H
#define USB_HANDLER_H

#include "app_usbd.h"
#include "app_usbd_cdc_acm.h"  // Include this header to ensure the enum types are defined
#include "common.h"
#include "ble_nus2.h"
#include <stdarg.h>

extern const app_usbd_cdc_acm_t m_app_cdc_acm;

void usb_init(void);
void usb_processs(void);
void rtc_init(void);
//char* format_string_va(const char* format, va_list args);
#endif // USB_HANDLER_H
