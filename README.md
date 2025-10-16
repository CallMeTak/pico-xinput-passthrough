# Pico XInput Passthrough

This is an example application for the Raspberry Pi Pico (RP2040) that demonstrates reading an XInput controller and outputting/presenting as an XInput controller simultaneously.

I use a basic Pico board and had to solder wires from a female usb type-a connector to pins on my board to connect a controller.

Instead, you can get a board from Adafruit that already has a connector attached. https://www.adafruit.com/product/5723

This application reads a report from a physical XInput controller connected to the Pico and immediately sends a corresponding report to the host computer, effectively "passing through" the inputs from the controller to the PC.

I wrote a custom XInput device driver and included it in here, but I will later separate it into its own repo.

## Libraries used

*   **[TinyUSB](https://github.com/hathach/tinyusb):** An open-source cross-platform USB stack.
*   **[Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB):** A library that allows the Pico's PIO state machines to act as a USB host port.
*   **[TinyUSB XInput Host Driver](https://github.com/Ryzee119/tusb_xinput):** A third-party driver for handling XInput devices in host mode.

## Purpose

While this application doesn't have much practical use on its own, it serves as foundation for more advanced projects such as an input remapper.
