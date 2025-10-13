
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
#include "xinput_host.h"
#include "xinput_device.h"
#include "host/usbh.h"
uint32_t blink_interval_ms = 250;

void led_blinking_task(void);
void cdc_task(void);
void xusbd_task();
// void print_device_info(uint8_t daddr, const tusb_desc_device_t* desc_device);

int main(void)
{

  set_sys_clock_khz(120000, true);
  board_init();

  tusb_rhport_init_t dev_init = {
      .role = TUSB_ROLE_DEVICE,
      .speed = TUSB_SPEED_AUTO};
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pinout = PIO_USB_PINOUT_DPDM;
  tuh_configure(BOARD_TUH_RHPORT, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

  tusb_rhport_init_t host_init = {
      .role = TUSB_ROLE_HOST,
      .speed = TUSB_SPEED_AUTO};
  // Pass our custom config to the host stack
  tusb_init(BOARD_TUH_RHPORT, &host_init);

  if (board_init_after_tusb)
  {
    board_init_after_tusb();
  }
 // stdio_usb_init();
  while (1)
  {
    tud_task();
    tuh_task();
   // xusbd_task();

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

usbh_class_driver_t const *usbh_app_driver_get_cb(uint8_t *driver_count)
{
  *driver_count = 1;
  return &usbh_xinput_driver;
}

void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, xinputh_interface_t const *xid_itf, uint16_t len)
{
  (void)len; // unused
  const xinput_gamepad_t *p = &xid_itf->pad;
  const char *type_str;
  printf("report received callback\r\n");
  printf("last xfer result: %d\r\n", xid_itf->last_xfer_result);
  if (xid_itf->last_xfer_result == XFER_RESULT_SUCCESS)
  {
    switch (xid_itf->type)
    {
    case 1:
      type_str = "Xbox One";
      break;
    case 2:
      type_str = "Xbox 360 Wireless";
      break;
    case 3:
      type_str = "Xbox 360 Wired";
      break;
    case 4:
      type_str = "Xbox OG";
      break;
    default:
      type_str = "Unknown";
    }
    if (xid_itf->connected && xid_itf->new_pad_data)
    {
     // TU_LOG1("[%02x, %02x], Type: %s, Buttons %04x, LT: %02x RT: %02x, LX: %d, LY: %d, RX: %d, RY: %d\n",
          //    dev_addr, instance, type_str, p->wButtons, p->bLeftTrigger, p->bRightTrigger, p->sThumbLX, p->sThumbLY, p->sThumbRX, p->sThumbRY);

      // How to check specific buttons
      if (p->wButtons & XINPUT_GAMEPAD_A){
        TU_LOG1("You are pressing A\n");
      }
    }
  }
  tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf)
{
  TU_LOG1("XINPUT MOUNTED %02x %d\n", dev_addr, instance);
  // If this is a Xbox 360 Wireless controller we need to wait for a connection packet
  // on the in pipe before setting LEDs etc. So just start getting data until a controller is connected.
  if (xinput_itf->type == XBOX360_WIRELESS && xinput_itf->connected == false)
  {
    printf("receive report success: %d\r\n", tuh_xinput_receive_report(dev_addr, instance));
    return;
  }
  tuh_xinput_set_led(dev_addr, instance, 0, true);
  tuh_xinput_set_led(dev_addr, instance, 1, true);
  tuh_xinput_set_rumble(dev_addr, instance, 0, 0, true);
  printf("receive report 2 success: %d\r\n", tuh_xinput_receive_report(dev_addr, instance));
  tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  TU_LOG1("XINPUT UNMOUNTED %02x %d\n", dev_addr, instance);
}
void xusbd_task()
{
  uint32_t current_time = board_millis();
  static uint32_t last_run_time = 0;
  if (current_time - last_run_time >= 1)
  {
    tud_xinput_press_button(BUTTON_A);
    tud_xinput_update();
    last_run_time = current_time;
  }
}