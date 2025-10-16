#include "device_callbacks.h"
#include "bsp/board_api.h"

extern uint32_t blink_interval_ms;
//--------------------------------------------------------------------
// Device CDC
//--------------------------------------------------------------------
void tud_cdc_rx_cb(uint8_t itf)
{
  // allocate buffer for the data in the stack
  uint8_t buf[CFG_TUD_CDC_RX_BUFSIZE];

  printf("RX CDC %d\n", itf);

  // read the available data
  // | IMPORTANT: also do this for CDC0 because otherwise
  // | you won't be able to print anymore to CDC0
  // | next time this function is called
  uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

  // check if the data was received on the second cdc interface
  if (itf == 0)
  {
    // process the received data
    buf[count] = 0; // null-terminate the string
    // now echo data back to the console on CDC 0
    printf("Received on CDC 1: %s\n", buf);

    // and echo back OK on CDC 1
    tud_cdc_n_write(itf, (uint8_t const *)"OK\r\n", 4);
    tud_cdc_n_write_flush(itf);
  }
}
