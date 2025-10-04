
// This example runs both host and device concurrently. The USB host receive
// reports from HID device and print it out over USB Device CDC interface.
// For TinyUSB roothub port0 is native usb controller, roothub port1 is
// pico-pio-usb.

#include <string.h>
#include <stdlib.h>
#include "pico/stdio/driver.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include "bsp/board_api.h"

#include "pico/stdio.h"
#include "pico/multicore.h"
#include "pio_usb.h"
#include "hardware/clocks.h"
#include "device_callbacks.h"
#include "host_callbacks.h"
#include "stdio_usb.h"
#define LANGUAGE_ID 0x0409


/*stdio_driver_t stdio_driver = {
  .crlf_enabled = false,
  .in_chars = stdio_usb_in_chars,
  .out_chars = stdio_usb_out_chars,
  .out_flush = stdio_usb_out_flush,
  .set_chars_available_callback = stdio_set_chars_available_callback
};*/
// Buffer to hold the fetched device descriptor
#define DESC_BUF_SIZE 256
uint8_t g_dev_desc_buf[DESC_BUF_SIZE];

// Buffer to hold the fetched configuration descriptor
uint8_t g_config_desc_buf[DESC_BUF_SIZE];

// Address of the connected downstream device
uint8_t g_dev_addr = 0;

// Flag to indicate that we have fetched the descriptors
bool g_descriptors_fetched = false;

// To manage the deferred control transfer
static tusb_control_request_t g_deferred_req;
// ####################################################################

uint32_t blink_interval_ms = 250;

bool is_print[CFG_TUH_DEVICE_MAX+1] = { 0 };
tusb_desc_device_t descriptor_device[CFG_TUH_DEVICE_MAX+1];

void led_blinking_task(void);
void cdc_task(void);
void print_device_info(uint8_t daddr, const tusb_desc_device_t* desc_device);

int main(void) {
 
  set_sys_clock_khz(120000, true);
  board_init();

 // printf("TinyUSB Host Information -> Device CDC Example\r\n");

  // init device and host stack on configured roothub port
  tusb_rhport_init_t dev_init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO
  };
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pinout = PIO_USB_PINOUT_DPDM;
  tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

  tusb_rhport_init_t host_init = {
    .role = TUSB_ROLE_HOST,
    .speed = TUSB_SPEED_AUTO
  };
  // Pass our custom config to the host stack
  tusb_init(BOARD_TUH_RHPORT, &host_init);

  if (board_init_after_tusb) {
        board_init_after_tusb();
    }
   //stdio_set_driver_enabled(&stdio_driver, true);
   //stdio_filter_driver(&stdio_driver);
   stdio_usb_init();
  while (1) {
    tud_task(); // tinyusb device task
    tuh_task(); // tinyusb host task
    cdc_task();
    led_blinking_task();
  }

  return 0;
}

void cdc_task(void) {
  if (!tud_cdc_connected()) {
    // delay a bit otherwise we can outpace host's terminal. Linux will set LineState (DTR) then Line Coding.
    // If we send data before Linux's terminal set Line Coding, it can be ignored --> missing data with hardware test loop
    board_delay(20);
  }

  for (uint8_t daddr = 1; daddr <= CFG_TUH_DEVICE_MAX; daddr++) {
    if (tuh_mounted(daddr)) {
      if (is_print[daddr]) {
        is_print[daddr] = false;
        print_device_info(daddr, &descriptor_device[daddr]);
        tud_cdc_write_flush();
      }
    }
  }
}

//--------------------------------------------------------------------+
// Blinking Task
//--------------------------------------------------------------------+
void led_blinking_task(void) {
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if (board_millis() - start_ms < blink_interval_ms) {
    return;// not enough time
  }
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}