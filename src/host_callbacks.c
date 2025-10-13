#include "host_callbacks.h"
#include "tusb.h"
#include "bsp/board_api.h"
#include "pico/bootrom.h"
#include <stdio.h>

extern uint32_t blink_interval_ms;
extern bool is_print[CFG_TUH_DEVICE_MAX+1];
extern tusb_desc_device_t descriptor_device[CFG_TUH_DEVICE_MAX+1];
extern uint8_t g_dev_addr;
extern bool g_descriptors_fetched;
extern uint8_t g_dev_desc_buf[];
extern uint8_t g_config_desc_buf[];

#define DESC_BUF_SIZE 256

static void print_utf16(uint16_t *temp_buf, size_t buf_len);
static void _convert_utf16le_to_utf8(const uint16_t *utf16, size_t utf16_len, uint8_t *utf8, size_t utf8_len);
static int _count_utf8_bytes(const uint16_t *buf, size_t len);

void tuh_enum_descriptor_device_cb(uint8_t daddr, tusb_desc_device_t const* desc_device) {
  (void) daddr;
  descriptor_device[daddr] = *desc_device; // save device descriptor
}
// Reset to bootsel mode when baud rate set to 300
void tud_cdc_line_coding_cb(__unused uint8_t itf, cdc_line_coding_t const* p_line_coding) {
    if (p_line_coding->bit_rate == 300) {    
        //board_led_write(1);
        reset_usb_boot(1, 0);
    }
}
void tuh_mount_cb(uint8_t daddr) {
  blink_interval_ms = 100;

}

void tuh_descriptor_get_device_cb(tuh_xfer_t* xfer) {
  (void)xfer; // xfer is not used in this function, so we cast it to void to prevent a compiler warning.
}
void tuh_descriptor_get_configuration_cb(tuh_xfer_t* xfer) {
  (void)xfer; // xfer is not used in this function.
}
void tuh_umount_cb(uint8_t daddr) {
  blink_interval_ms = 1000;

}