#ifndef DEVICE_CALLBACKS_H
#define DEVICE_CALLBACKS_H

#include "tusb.h"

// Invoked when device is mounted
void tud_mount_cb(void);

// Invoked when device is unmounted
void tud_umount_cb(void);

// Invoked when usb bus is suspended
void tud_suspend_cb(bool remote_wakeup_en);

// Invoked when usb bus is resumed
void tud_resume_cb(void);

void tud_cdc_rx_cb(uint8_t itf);

#endif
