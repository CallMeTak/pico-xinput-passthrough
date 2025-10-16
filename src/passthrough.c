
// This example runs both host and device concurrently. The USB host receive
// reports from HID device and print it out over USB Device CDC interface.
// For TinyUSB roothub port0 is native usb controller, roothub port1 is
// pico-pio-usb.

// Standard library headers
#include <string.h>
#include <stdlib.h>

// Pico SDK headers
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "pico/stdio/driver.h"
#include "pico/multicore.h"

// TinyUSB and board headers
#include "tusb.h"
#include "bsp/board_api.h"
#include "host/usbh.h"
#include "device/usbd_pvt.h"
#include "class/hid/hid.h"

// Hardware-specific headers
#include "hardware/clocks.h"
#include "pio_usb.h"

// Project-specific headers
#include "device_callbacks.h"
#include "host_callbacks.h"

// Cannot use pico/stdio_usb.h along with tinyusb host mode
// So we copy the file into our own project
// https://forums.raspberrypi.com/viewtopic.php?t=355601
#include "stdio_usb.h"


#include "xinput_host.h"
#include "xinput_device.h"

extern usbd_class_driver_t const usbd_xinput_driver;
uint32_t blink_interval_ms = 250;

void led_blinking_task(void);
void cdc_task(void);
void xusbd_task();

int main(void)
{

  set_sys_clock_khz(240000, true);
  board_init();

  tusb_rhport_init_t dev_init = {
      .role = TUSB_ROLE_DEVICE,
      .speed = TUSB_SPEED_AUTO};
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;

  // Reversed DP/DM for Pico board. Depends on your own board wiring
  pio_cfg.pinout = PIO_USB_PINOUT_DPDM;
  tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

  tusb_rhport_init_t host_init = {
      .role = TUSB_ROLE_HOST,
      .speed = TUSB_SPEED_AUTO};
  tusb_init(BOARD_TUH_RHPORT, &host_init);

  if (board_init_after_tusb)
  {
    board_init_after_tusb();
  }

  stdio_usb_init();

  // Main loop
  while (1)
  {
    // Host task
    tuh_task();

    // Device task
    tud_task();

    // led blink task
    led_blinking_task();
  }

  return 0;
}

//--------------------------------------------------------------------+
// Blinking Task
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if (board_millis() - start_ms < blink_interval_ms)
  {
    return; // not enough time
  }
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}

// Application level callbacks to register class drivers
usbh_class_driver_t const *usbh_app_driver_get_cb(uint8_t *driver_count)
{
  *driver_count = 1;
  return &usbh_xinput_driver;
}
usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count)
{
  *driver_count = 1;
  return &usbd_xinput_driver;
}

// Application callback invoked when XInput report is received
// For passthrough, we send a device report for every report we receive
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, xinputh_interface_t const *xid_itf, uint16_t len)
{
  (void)len; // unused
  const xinput_gamepad_t *p = &xid_itf->pad;
  const char *type_str;
  if (xid_itf->last_xfer_result == XFER_RESULT_SUCCESS)
  {
    if (xid_itf->connected && xid_itf->new_pad_data)
    {
      // Create a report to send to the PC.
      xinput_report_t report = {0};
      report.bReportID = 0;
      report.bSize = 0x14;
      report.bmButtons = p->wButtons;
      report.bLeftTrigger = p->bLeftTrigger;
      report.bRightTrigger = p->bRightTrigger;
      report.wThumbLeftX = p->sThumbLX;
      report.wThumbLeftY = p->sThumbLY;
      report.wThumbRightX = p->sThumbRX;
      report.wThumbRightY = p->sThumbRY;

      tud_xinput_report(&report);
    }
  }
  tuh_xinput_receive_report(dev_addr, instance);
}

// Application callback invoked when Xinput device is plugged in
void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf)
{
  tuh_xinput_set_led(dev_addr, instance, 0, true);
  tuh_xinput_set_led(dev_addr, instance, 1, true);
  tuh_xinput_set_rumble(dev_addr, instance, 0, 0, true);
  tuh_xinput_receive_report(dev_addr, instance);
}
