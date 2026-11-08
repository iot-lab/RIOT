# OpenThread Radio Co-Processor

This application is an OpenThread Radio Co-Processor (RCP): the radio of an
OpenThread host such as the border router `otbr-agent` or `ot-cli` of the
POSIX platform of OpenThread, over spinel on HDLC on the UART of stdio
(stdio is disabled).

Build and flash it:

```
make BOARD=nrf52840dk flash
```

Then start the host on the serial port of the board, e.g. with `ot-cli`
built from the OpenThread sources (`script/cmake-build posix`):

```
ot-cli 'spinel+hdlc+uart:///dev/ttyACM0?uart-baudrate=115200'
```

On native, the RCP uses a ZEP socket as radio (`dist/tools/zep_dispatch`)
and the host can start it on a pseudo terminal:

```
ot-cli 'spinel+hdlc+forkpty://bin/native64/tests_openthread_rcp.elf?forkpty-arg=-z&forkpty-arg=[::1]:17754'
```

The baudrate of the link is set with `OPENTHREAD_RCP_UART_BAUDRATE` (e.g.
500000 for the A8-M3 of IoT-LAB, on `/dev/ttyA8_M3` of the A8).
