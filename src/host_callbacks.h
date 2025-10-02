#ifndef HOST_CALLBACKS_H
#define HOST_CALLBACKS_H

#include "tusb.h"

// Invoked when a device is mounted
void tuh_mount_cb(uint8_t daddr);

// Invoked when a device is unmounted
void tuh_umount_cb(uint8_t daddr);

void tuh_enum_descriptor_device_cb(uint8_t daddr, tusb_desc_device_t const* desc_device);

void tuh_descriptor_get_device_cb(tuh_xfer_t* xfer);

void tuh_descriptor_get_configuration_cb(tuh_xfer_t* xfer);

#endif
