@defgroup    boards_max78000fthr MAX78000FTHR
@ingroup     boards
@brief       Support for the MAX78000FTHR board

## General information

The MAX78000FTHR is an Analog Devices (formerly Maxim Integrated) development
board in the Feather form factor, built around the MAX78000: an Arm
Cortex-M4F running at 100 MHz, a RISC-V coprocessor and a convolutional
neural network (CNN) accelerator, with 512 KiB of flash and 128 KiB of SRAM.
The board also has a camera, a microphone, a micro SD card slot and a power
management IC.

Currently, only the Cortex-M4F is supported by RIOT, see @ref cpu_max78000
for the supported peripherals.

| Feature   | Pins                                                      |
|:--------- |:--------------------------------------------------------- |
| LEDs      | RGB LED, active low: LED0 (red): P2.0, LED1 (green): P2.1, LED2 (blue): P2.2 |
| Buttons   | BTN0 (SW1): P0.2, BTN1 (SW2): P1.7, active low            |
| Console   | UART0, RX: P0.0, TX: P0.1 (115200 baud)                   |

## Links

- [MAX78000FTHR web page](https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/max78000fthr.html)
- [MAX78000 web page](https://www.analog.com/en/products/max78000.html)

## Flash the board

The board is flashed with OpenOCD through the on-board DAPLink debugger
(MAX32625, CMSIS-DAP). OpenOCD must provide the `target/max32xxx_common.cfg`
target file (upstream OpenOCD since 2025), the board configuration is in
`boards/max78000fthr/dist/openocd.cfg`.

The process is automated in the usual `make flash` target:

```shell
make BOARD=max78000fthr -C examples/basic/hello-world flash
```

## Accessing STDIO via UART

The STDIO is accessible via the USB port: the on-board DAPLink exposes a
virtual COM port connected to UART0 (generally mapped to `/dev/ttyACM0` on a
Linux host).

Use the `term` target to connect to the board serial port:

```shell
make BOARD=max78000fthr -C examples/basic/hello-world term
```
