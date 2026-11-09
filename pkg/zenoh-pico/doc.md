@defgroup pkg_zenoh-pico  zenoh-pico
@ingroup  pkg
@brief    Zenoh protocol (pub/sub, queries) for constrained devices

[zenoh-pico](https://github.com/eclipse-zenoh/zenoh-pico) with a RIOT platform
layer (`ZENOH_GENERIC`): RIOT threads, mutexes and a scheduler based condition
variable, `ztimer64` clocks and TCP links over `sock_tcp`.

The build configuration is in `include/zenoh_generic_config.h`, every value
can be overridden with CFLAGS (for the package and the application). The
defaults target the client mode over TCP; peer mode needs
`-DZ_FEATURE_UNICAST_PEER=1`.

With GNRC, reads use the `sock_tcp` timeouts, which require the fix of the
GNRC sock_tcp timeout unit (microseconds were used as milliseconds). A write
waits for the current read timeout (`Z_CONFIG_SOCKET_TIMEOUT`, 100 ms) as GNRC
TCP serializes the calls on a connection.

License: EPL-2.0 OR Apache-2.0.
