#include "host_callbacks.h"
#include "tusb.h"
#include "bsp/board_api.h"
#include "pico/bootrom.h"
#include <stdio.h>

extern uint32_t blink_interval_ms;

// Reset to bootsel mode when baud rate set to 300
void tud_cdc_line_coding_cb(__unused uint8_t itf, cdc_line_coding_t const* p_line_coding) {
    if (p_line_coding->bit_rate == 300) {    
        //board_led_write(1);
        reset_usb_boot(1, 0);
    }
}
void tuh_mount_cb(uint8_t daddr) {
  (void)daddr;
  blink_interval_ms = 100;

}
void tuh_umount_cb(uint8_t daddr) {
  (void)daddr;
  blink_interval_ms = 1000;

}