@defgroup pkg_nanomodbus  nanoMODBUS Modbus library
@ingroup  pkg
@ingroup  net
@brief    Compact Modbus RTU and TCP library, client and server

[nanoMODBUS](https://github.com/debevv/nanoMODBUS) is a small Modbus library
for microcontrollers. It implements the client and the server roles over the
RTU and TCP framings, without dynamic memory allocation.

Supported function codes: read coils (1), read discrete inputs (2), read
holding registers (3), read input registers (4), write single coil (5), write
single register (6), write multiple coils (15), write multiple registers (16),
read file record (20), write file record (21), read/write multiple registers
(23) and read device identification (43/14).

## Usage

Add the package and at least one transport to the application `Makefile`:

```makefile
USEPKG += nanomodbus
USEMODULE += nanomodbus_udp
```

nanoMODBUS does its I/O through read and write callbacks. The following
transports are provided by `nanomodbus_riot.h`, each of them fills a
`nmbs_platform_conf` passed to `nmbs_client_create()` or
`nmbs_server_create()`:

| Module            | Function                 | Framing                          |
|:------------------|:-------------------------|:---------------------------------|
| `nanomodbus_uart` | `nanomodbus_uart_init()` | Modbus RTU over `periph_uart`, optional RS-485 driver enable pin |
| `nanomodbus_tcp`  | `nanomodbus_tcp_init()`  | Modbus TCP over a connected `sock_tcp` |
| `nanomodbus_udp`  | `nanomodbus_udp_init()`  | Modbus TCP frames over `sock_udp`, one frame per datagram |

The network stack providing `sock_tcp` or `sock_udp` (e.g. GNRC or lwIP) is
selected by the application. Modbus over UDP is not part of the Modbus
specification but is supported by many gateways and tools. It is lighter
than TCP on constrained networks such as 6LoWPAN over IEEE 802.15.4 or BLE.

Client over UDP:

```c
#include "nanomodbus_riot.h"

static sock_udp_t sock;
static nanomodbus_udp_t udp;
static nmbs_t client;

nmbs_platform_conf conf;
sock_udp_ep_t server = { .family = AF_INET6, .port = 502 };
uint16_t regs[2];

/* set server.addr.ipv6 */
sock_udp_create(&sock, NULL, NULL, 0);
nanomodbus_udp_init(&udp, &sock, &server, &conf);
nmbs_client_create(&client, &conf);
nmbs_set_read_timeout(&client, 1000);
nmbs_set_byte_timeout(&client, 1000);
nmbs_read_holding_registers(&client, 0, 2, regs);
```

A server is created with `nmbs_server_create()` and callbacks for the
supported requests, `nmbs_server_poll()` is then called in a loop to handle
the requests. With the UDP transport, the server replies to the sender of
each request when `nanomodbus_udp_init()` is called with a `NULL` remote.

## Configuration

Client or server support, as well as individual server function codes, can be
disabled to save memory with the `NMBS_CLIENT_DISABLED`,
`NMBS_SERVER_DISABLED` and `NMBS_SERVER_<FUNCTION>_DISABLED` macros, see
`nanomodbus.h`. They change the layout of `nmbs_t` and must be set for the
whole application:

```makefile
CFLAGS += -DNMBS_SERVER_DISABLED
```

## Limitations

- The read and write file record functions (20 and 21) access 16 bit
  registers through pointers into the message buffer that are not always
  aligned. Don't use them on CPUs that fault on unaligned accesses (e.g.
  Cortex-M0/M0+).
- The RTU transport does not enforce the 3.5 character silence between frames
  when sending. The driver enable pin is released when `uart_write()` returns,
  which must happen only once the last byte has been sent.

## License

nanoMODBUS is licensed under the MIT license.
