#include "tusb_option.h"
#if (TUSB_OPT_HOST_ENABLED && CFG_TUH_XINPUT)
#include "xinput_device.h"
#include "device/usbd.h"
#include "device/usbd_pvt.h"
typedef struct
{
    xinput_state_t gamepad_state;
    xinput_report_t gamepad_report;
    uint8_t ep_in;
    uint8_t ep_out;
    uint8_t epin_buf[CFG_TUD_XINPUT_EPIN_BUFSIZE];
    uint8_t epout_buf[CFG_TUD_XINPUT_EPOUT_BUFSIZE];
    xfer_result_t last_xfer_result;
    uint32_t last_xferred_bytes;
} xinputd_interface_t;

static xinputd_interface_t xinputd_itf;

bool tud_xinput_report(xinput_report_t *report)
{
    // Return if fail to claim endpoint
    TU_VERIFY(usbd_edpt_claim(0, xinputd_itf.ep_in));

    // Copy the report data into the endpoint buffer
    memcpy(xinputd_itf.epin_buf, report, sizeof(xinput_report_t));

    // Return if tud not ready or endpoint busy
    if (!tud_ready() || usbd_edpt_busy(0, xinputd_itf.ep_in))
    {
        usbd_edpt_release(0, xinputd_itf.ep_in);
        return false;
    }
    // Queue the transfer
    return usbd_edpt_xfer(0, xinputd_itf.ep_in, xinputd_itf.epin_buf, sizeof(xinput_report_t));
}

bool tud_xinput_update()
{
    memcpy((uint8_t*)&xinputd_itf.gamepad_report + 2, &xinputd_itf.gamepad_state, sizeof(xinput_state_t));
    return tud_xinput_report(&xinputd_itf.gamepad_report);
}

// May delete later. helper function to update the entire state at once
void tud_xinput_update_state(xinput_state_t *state)
{
    // Copy state into the local gamepad_report buffer
    memcpy((uint8_t *)&xinputd_itf.gamepad_report + 2, state, sizeof(xinput_state_t));
}

// Application helper methods
// May separate into a separate header/source file later
void tud_xinput_press_button(XboxButton button)
{
    xinputd_itf.gamepad_state.bmButtons |= button;
}

void tud_xinput_release_button(XboxButton button)
{
    xinputd_itf.gamepad_state.bmButtons &= ~button;
}
void tud_xinput_joystick_lx(int16_t value)
{
    xinputd_itf.gamepad_state.wThumbLeftX = value;
}
void tud_xinput_joystick_ly(int16_t value)
{
    xinputd_itf.gamepad_state.wThumbLeftY = value;
}
void tud_xinput_joystick_rx(int16_t value)
{
    xinputd_itf.gamepad_state.wThumbRightX = value;
}
void tud_xinput_joystick_ry(int16_t value)
{
    xinputd_itf.gamepad_state.wThumbRightY = value;
}

//--------------------------------------------------------------------+
// USBD Driver Callbacks
//--------------------------------------------------------------------+
uint16_t tud_xinput_open_cb(uint8_t rhport, tusb_desc_interface_t const *itf_descriptor, uint16_t max_length)
{
    // Verify that this is the XInput interface
    TU_VERIFY(itf_descriptor->bInterfaceClass == TUD_XINPUT_CLASS &&
                  itf_descriptor->bInterfaceSubClass == TUD_XINPUT_SUBCLASS &&
                  itf_descriptor->bInterfaceProtocol == TUD_XINPUT_PROTOCOL,
              0);

    uint16_t driver_length = sizeof(tusb_desc_interface_t) + (itf_descriptor->bNumEndpoints * sizeof(tusb_desc_endpoint_t)) + 16;

    TU_VERIFY(max_length >= driver_length, 0);

    uint8_t const *current_descriptor = tu_desc_next(itf_descriptor);
    uint8_t found_endpoints = 0;
    while ((found_endpoints < itf_descriptor->bNumEndpoints) && (driver_length <= max_length))
    {
        tusb_desc_endpoint_t const *endpoint_descriptor = (tusb_desc_endpoint_t const *)current_descriptor;
        // If endpoint found, open it
        if (TUSB_DESC_ENDPOINT == tu_desc_type(endpoint_descriptor))
        {
            TU_ASSERT(usbd_edpt_open(rhport, endpoint_descriptor));

            if (tu_edpt_dir(endpoint_descriptor->bEndpointAddress) == TUSB_DIR_IN)
                xinputd_itf.ep_in = endpoint_descriptor->bEndpointAddress;
            else
                xinputd_itf.ep_out = endpoint_descriptor->bEndpointAddress;

            ++found_endpoints;
        }

        current_descriptor = tu_desc_next(current_descriptor);
    }
    return driver_length;
}

void tud_xinput_init_cb(void)
{
    // Reset the interface state
    memset(&xinputd_itf, 0, sizeof(xinputd_itf));

    // Initialize the gamepad report with a compound literal
    xinputd_itf.gamepad_report = (xinput_report_t){
        .bReportID = 0x00,
        .bSize = 0x14, // 20 bytes for a standard XInput report
        // The rest of the fields will be zero-initialized
    };
}
bool tud_xinput_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request)
{
    (void)rhport;
    (void)stage;
    (void)request;
    return true;
}
bool tud_xinput_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes)
{
    (void)result;
    (void)xferred_bytes;
    return true;
    // IN transfer completed, release the endpoint
    if (ep_addr == xinputd_itf.ep_in)
    {
        usbd_edpt_release(rhport, xinputd_itf.ep_in);
    }
    return true;
}
void tud_xinput_reset_cb(uint8_t rhport)
{
    (void)rhport;
    tud_xinput_init_cb();
}
usbd_class_driver_t const usbd_xinput_driver =
    {
        .name = "XINPUT_DEVICE",
        .init = tud_xinput_init_cb,
        .open = tud_xinput_open_cb,
        .control_xfer_cb = tud_xinput_control_xfer_cb,
        .xfer_cb = tud_xinput_xfer_cb,
        .reset = tud_xinput_reset_cb,
        .sof = NULL,
};

#endif