# zenoh-pico

A zenoh client in the shell: it connects to a zenoh router over TCP,
publishes values and prints the samples of a subscription.

With GNRC, the subscription needs the `sock_tcp` read timeouts in
microseconds (fix of `gnrc_sock_tcp`): without it the reads of zenoh-pico
don't time out and the samples of the router are not received.

## Usage on native

Create a tap interface and start a zenoh router (`zenohd`, the same version
as the package) on the host, listening on IPv6:

```
sudo ip tuntap add tap0 mode tap user $(id -un)
sudo ip link set tap0 up
zenohd -l 'tcp/[::]:7447' --no-multicast-scouting
```

Build and run the application, then connect to the link local address of
`tap0` (`ip -6 addr show dev tap0`):

```
make BOARD=native64 all term
> zenoh open tcp/[fe80::<host>%5]:7447
zenoh: connected to tcp/[fe80::<host>%5]:7447
> zenoh sub demo/**
> zenoh put demo/riot hello
```

`5` is the interface of the application (`ifconfig`). A subscriber on the
host (e.g. `z_sub -k 'demo/**'` of the zenoh examples) receives `hello`, and a
publication of the host on `demo/...` is printed by the application:

```
zenoh: received 'hi' on demo/host
```

On a board, connect to the global address of the router through a border
router.
