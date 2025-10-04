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
void tud_cdc_line_coding_cb(__unused uint8_t itf, cdc_line_coding_t const* p_line_coding) {
    if (p_line_coding->bit_rate == 300) {    
        //board_led_write(1);
        reset_usb_boot(1, 0);
    }
}
void tuh_mount_cb(uint8_t daddr) {
  blink_interval_ms = 100;
  printf("mounted device %u\r\n", daddr);
  
  is_print[daddr] = true;
  g_dev_addr = daddr;
  g_descriptors_fetched = false;

  tuh_descriptor_get_device(daddr, g_dev_desc_buf, DESC_BUF_SIZE, tuh_descriptor_get_device_cb, 0);
}

void tuh_descriptor_get_device_cb(tuh_xfer_t* xfer) {
  (void)xfer; // xfer is not used in this function, so we cast it to void to prevent a compiler warning.
  tud_cdc_write_str("Device descriptor retrieved. Getting configuration descriptor...\r\n");
  tud_cdc_write_flush();
  tuh_descriptor_get_configuration(g_dev_addr, 0, g_config_desc_buf, DESC_BUF_SIZE, tuh_descriptor_get_configuration_cb, 0);
}
void tuh_descriptor_get_configuration_cb(tuh_xfer_t* xfer) {
  (void)xfer; // xfer is not used in this function.
  g_descriptors_fetched = true;
  tud_cdc_write_str("Configuration descriptor received.\r\n");
  tud_cdc_write_flush();
}
void tuh_umount_cb(uint8_t daddr) {
  blink_interval_ms = 1000;
  printf("Printf: unmounted device %u\r\n", daddr);
  fflush(stdout);
  tud_cdc_write_str("tud cdc write str: unmounted device \r\n");
  tud_cdc_write_flush();
  is_print[daddr] = false;
}

void print_device_info(uint8_t daddr, const tusb_desc_device_t* desc_device) {
  // Get String descriptor using Sync API
  uint16_t serial[64];
  uint16_t buf[128];
  (void) buf;

#define LANGUAGE_ID 0x0409
  printf("Device %u: ID %04x:%04x SN ", daddr, desc_device->idVendor, desc_device->idProduct);
  uint8_t xfer_result = tuh_descriptor_get_serial_string_sync(daddr, LANGUAGE_ID, serial, sizeof(serial));
  if (XFER_RESULT_SUCCESS != xfer_result) {
    serial[0] = 'n';
    serial[1] = '/';
    serial[2] = 'a';
    serial[3] = 0;
  }
  print_utf16(serial, TU_ARRAY_SIZE(serial));
  printf("\r\n");

  printf("Device Descriptor:\r\n");
  printf("  bLength             %u\r\n"     , desc_device->bLength);
  printf("  bDescriptorType     %u\r\n"     , desc_device->bDescriptorType);
  printf("  bcdUSB              %04x\r\n"   , desc_device->bcdUSB);
  printf("  bDeviceClass        %u\r\n"     , desc_device->bDeviceClass);
  printf("  bDeviceSubClass     %u\r\n"     , desc_device->bDeviceSubClass);
  printf("  bDeviceProtocol     %u\r\n"     , desc_device->bDeviceProtocol);
  printf("  bMaxPacketSize0     %u\r\n"     , desc_device->bMaxPacketSize0);
  printf("  idVendor            0x%04x\r\n" , desc_device->idVendor);
  printf("  idProduct           0x%04x\r\n" , desc_device->idProduct);
  printf("  bcdDevice           %04x\r\n"   , desc_device->bcdDevice);

  printf("  iManufacturer       %u     "     , desc_device->iManufacturer);
  xfer_result = tuh_descriptor_get_manufacturer_string_sync(daddr, LANGUAGE_ID, buf, sizeof(buf));
  if (XFER_RESULT_SUCCESS == xfer_result) {
    print_utf16(buf, TU_ARRAY_SIZE(buf));
  }
  printf("\r\n");

  printf("  iProduct            %u     "     , desc_device->iProduct);
  xfer_result = tuh_descriptor_get_product_string_sync(daddr, LANGUAGE_ID, buf, sizeof(buf));
  if (XFER_RESULT_SUCCESS == xfer_result) {
    print_utf16(buf, TU_ARRAY_SIZE(buf));
  }
  printf("\r\n");

  printf("  iSerialNumber       %u     "     , desc_device->iSerialNumber);
  printf((char*)serial); // serial is already to UTF-8
  printf("\r\n");

  printf("  bNumConfigurations  %u\r\n"     , desc_device->bNumConfigurations);
}

static void _convert_utf16le_to_utf8(const uint16_t *utf16, size_t utf16_len, uint8_t *utf8, size_t utf8_len) {
  // TODO: Check for runover.
  (void)utf8_len;
  // Get the UTF-16 length out of the data itself.

  for (size_t i = 0; i < utf16_len; i++) {
    uint16_t chr = utf16[i];
    if (chr < 0x80) {
      *utf8++ = chr & 0xffu;
    } else if (chr < 0x800) {
      *utf8++ = (uint8_t)(0xC0 | (chr >> 6 & 0x1F));
      *utf8++ = (uint8_t)(0x80 | (chr >> 0 & 0x3F));
    } else {
      // TODO: Verify surrogate.
      *utf8++ = (uint8_t)(0xE0 | (chr >> 12 & 0x0F));
      *utf8++ = (uint8_t)(0x80 | (chr >> 6 & 0x3F));
      *utf8++ = (uint8_t)(0x80 | (chr >> 0 & 0x3F));
    }
    // TODO: Handle UTF-16 code points that take two entries.
  }
}

// Count how many bytes a utf-16-le encoded string will take in utf-8.
static int _count_utf8_bytes(const uint16_t *buf, size_t len) {
  size_t total_bytes = 0;
  for (size_t i = 0; i < len; i++) {
    uint16_t chr = buf[i];
    if (chr < 0x80) {
      total_bytes += 1;
    } else if (chr < 0x800) {
      total_bytes += 2;
    } else {
      total_bytes += 3;
    }
    // TODO: Handle UTF-16 code points that take two entries.
  }
  return (int) total_bytes;
}

static void print_utf16(uint16_t *temp_buf, size_t buf_len) {
  if ((temp_buf[0] & 0xff) == 0) {
    return;// empty
  }
  size_t utf16_len = ((temp_buf[0] & 0xff) - 2) / sizeof(uint16_t);
  size_t utf8_len = (size_t) _count_utf8_bytes(temp_buf + 1, utf16_len);
  _convert_utf16le_to_utf8(temp_buf + 1, utf16_len, (uint8_t *) temp_buf, sizeof(uint16_t) * buf_len);
  ((uint8_t*) temp_buf)[utf8_len] = '\0';

  printf((char*) temp_buf);
}

