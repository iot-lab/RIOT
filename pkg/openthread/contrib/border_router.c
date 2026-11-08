/*
 * SPDX-FileCopyrightText: 2026 Inria
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @{
 * @ingroup     net
 * @file
 * @brief       OpenThread border router over ethos
 *
 * The node bridges the IPv6 datagrams between the Thread network and an
 * ethos link to a host, as `gnrc_border_router` with the `ethos_uhcpd.py`
 * or `start_network.sh` tools of the host:
 *
 * - the host routes the prefix to `fe80::2` on its tap interface: the node
 *   answers the neighbor solicitations of `fe80::2` on the ethos link;
 * - the prefix is requested with UHCP (port 12345) and published in the
 *   Thread Network Data as an on-mesh prefix with SLAAC and a default route:
 *   the Thread nodes get an address in it and route off-mesh traffic to the
 *   border router;
 * - the datagrams of the host are given to OpenThread (otIp6Send), the
 *   datagrams OpenThread gives to the host (otIp6SetReceiveCallback) are sent
 *   on the ethos link, except link-local and mesh-local ones.
 *
 * stdio is multiplexed on the same link (stdio_ethos).
 *
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 * @}
 */

#include <errno.h>
#include <string.h>

#include "byteorder.h"
#include "ethos.h"
#include "ethos_params.h"
#include "event.h"
#include "iolist.h"
#include "net/ethertype.h"
#include "net/inet_csum.h"
#include "net/netdev.h"
#include "ztimer.h"

#include "openthread/border_router.h"
#include "openthread/ip6.h"
#include "openthread/message.h"
#include "openthread/thread.h"
#include "ot.h"

#define ENABLE_DEBUG 0
#include "debug.h"

#define ETH_HDR_LEN         (14U)
#define IPV6_HDR_LEN        (40U)
#define IPV6_MTU            (1280U)
#define PROTO_UDP           (17U)
#define PROTO_ICMPV6        (58U)
#define ICMPV6_NS           (135U)
#define ICMPV6_NA           (136U)
#define UHCP_PORT           (12345U)
#define UHCP_MAGIC          (0x55484350UL)
#define UHCP_REQ            (0U)
#define UHCP_PUSH           (1U)
#define UHCP_PERIOD_MS      (10U * MS_PER_SEC)

/* used by stdio_ethos */
ethos_t ethos;

static uint8_t _inbuf[2048];
static uint8_t _frame[ETH_HDR_LEN + IPV6_MTU];
static uint8_t _host_mac[6];
static bool _host_mac_known;
static bool _prefix_set;
static ztimer_t _uhcp_timer;

/* address of the node on the ethos link, the next hop of the host */
static const uint8_t _ll_addr[16] = { 0xfe, 0x80, [15] = 0x02 };
/* UHCP servers */
static const uint8_t _uhcp_group[16] = { 0xff, 0x15, [14] = 0xab, [15] = 0xcd };

static void _ethos_isr_handler(event_t *event);
static void _uhcp_handler(event_t *event);

static event_t _ev_ethos_isr = { .handler = _ethos_isr_handler };
static event_t _ev_uhcp = { .handler = _uhcp_handler };

static bool _is_link_local(const uint8_t *addr)
{
    /* fe80::/10 or a multicast address with a scope up to link-local */
    return ((addr[0] == 0xfe) && ((addr[1] & 0xc0) == 0x80))
           || ((addr[0] == 0xff) && ((addr[1] & 0x0f) <= 2));
}

static bool _is_mesh_local(const uint8_t *addr)
{
    const otMeshLocalPrefix *prefix = otThreadGetMeshLocalPrefix(openthread_get_instance());

    return (prefix != NULL) && !memcmp(addr, prefix->m8, sizeof(prefix->m8));
}

static uint16_t _upper_csum(const uint8_t *ip, const uint8_t *payload, uint16_t len,
                            uint8_t proto)
{
    uint8_t pseudo[8] = { 0 };
    uint16_t sum;

    pseudo[2] = len >> 8;
    pseudo[3] = len & 0xff;
    pseudo[7] = proto;
    sum = inet_csum(0, &ip[8], 32);                 /* source and destination */
    sum = inet_csum(sum, pseudo, sizeof(pseudo));
    sum = inet_csum(sum, payload, len);
    return ~sum;
}

/* write the Ethernet and IPv6 headers in _frame, @p len is the payload */
static void _ipv6_header(const uint8_t *dst_mac, const uint8_t *dst, uint16_t len,
                         uint8_t proto, uint8_t hop_limit)
{
    uint8_t *ip = &_frame[ETH_HDR_LEN];

    memcpy(&_frame[0], dst_mac, 6);
    memcpy(&_frame[6], ethos.mac_addr, 6);
    _frame[12] = ETHERTYPE_IPV6 >> 8;
    _frame[13] = ETHERTYPE_IPV6 & 0xff;

    memset(ip, 0, IPV6_HDR_LEN);
    ip[0] = 0x60;
    ip[4] = len >> 8;
    ip[5] = len & 0xff;
    ip[6] = proto;
    ip[7] = hop_limit;
    memcpy(&ip[8], _ll_addr, 16);
    memcpy(&ip[24], dst, 16);
}

static void _send_frame(size_t len)
{
    iolist_t iol = { .iol_base = _frame, .iol_len = len };

    ethos.netdev.driver->send(&ethos.netdev, &iol);
}

static void _multicast_mac(uint8_t *mac, const uint8_t *addr)
{
    mac[0] = 0x33;
    mac[1] = 0x33;
    memcpy(&mac[2], &addr[12], 4);
}

/* neighbor advertisement of fe80::2 to the host */
static void _send_na(const uint8_t *ns_ip)
{
    static const uint8_t all_nodes[16] = { 0xff, 0x02, [15] = 0x01 };
    uint8_t src[16];
    bool unspecified = true;
    uint8_t dst_mac[6];

    /* the reply is built in the buffer of the solicitation */
    memcpy(src, &ns_ip[8], sizeof(src));
    for (unsigned i = 0; i < sizeof(src); i++) {
        unspecified &= (src[i] == 0);
    }
    const uint8_t *dst = unspecified ? all_nodes : src;

    if (unspecified || !_host_mac_known) {
        _multicast_mac(dst_mac, dst);
    }
    else {
        memcpy(dst_mac, _host_mac, 6);
    }

    /* type, code, checksum, flags, target, target link-layer address option */
    uint8_t *icmp = &_frame[ETH_HDR_LEN + IPV6_HDR_LEN];
    uint16_t len = 4 + 4 + 16 + 8;

    _ipv6_header(dst_mac, dst, len, PROTO_ICMPV6, 255);
    memset(icmp, 0, len);
    icmp[0] = ICMPV6_NA;
    /* router, solicited (unless unspecified source), override */
    icmp[4] = 0x80 | (unspecified ? 0 : 0x40) | 0x20;
    memcpy(&icmp[8], _ll_addr, 16);
    icmp[24] = 2;   /* target link-layer address */
    icmp[25] = 1;
    memcpy(&icmp[26], ethos.mac_addr, 6);
    uint16_t csum = _upper_csum(&_frame[ETH_HDR_LEN], icmp, len, PROTO_ICMPV6);

    icmp[2] = csum >> 8;
    icmp[3] = csum & 0xff;
    _send_frame(ETH_HDR_LEN + IPV6_HDR_LEN + len);
}

/* UHCP request of a /64 prefix to the UHCP servers of the link */
static void _send_uhcp_req(void)
{
    uint8_t dst_mac[6];
    uint8_t *udp = &_frame[ETH_HDR_LEN + IPV6_HDR_LEN];
    uint16_t len = 8 + 6;

    _multicast_mac(dst_mac, _uhcp_group);
    _ipv6_header(dst_mac, _uhcp_group, len, PROTO_UDP, 64);
    memset(udp, 0, len);
    udp[0] = UHCP_PORT >> 8;
    udp[1] = UHCP_PORT & 0xff;
    udp[2] = UHCP_PORT >> 8;
    udp[3] = UHCP_PORT & 0xff;
    udp[4] = len >> 8;
    udp[5] = len & 0xff;
    /* magic, version and type, prefix length */
    byteorder_htobebufl(&udp[8], UHCP_MAGIC);
    udp[12] = (0 << 4) | UHCP_REQ;
    udp[13] = 64;
    uint16_t csum = _upper_csum(&_frame[ETH_HDR_LEN], udp, len, PROTO_UDP);

    udp[6] = csum >> 8;
    udp[7] = csum & 0xff;
    _send_frame(ETH_HDR_LEN + IPV6_HDR_LEN + len);
}

static void _set_prefix(const uint8_t *prefix, uint8_t prefix_len)
{
    otInstance *ot = openthread_get_instance();
    otBorderRouterConfig config;
    char str[OT_IP6_PREFIX_STRING_SIZE];

    memset(&config, 0, sizeof(config));
    memcpy(config.mPrefix.mPrefix.mFields.m8, prefix, (prefix_len + 7) / 8);
    config.mPrefix.mLength = prefix_len;
    config.mPreference = OT_ROUTE_PREFERENCE_MED;
    config.mPreferred = true;
    config.mSlaac = true;
    config.mDefaultRoute = true;
    config.mOnMesh = true;
    config.mStable = true;

    otIp6PrefixToString(&config.mPrefix, str, sizeof(str));
    if ((otBorderRouterAddOnMeshPrefix(ot, &config) != OT_ERROR_NONE)
        || (otBorderRouterRegister(ot) != OT_ERROR_NONE)) {
        printf("openthread_br: cannot publish the prefix %s\n", str);
        return;
    }
    printf("openthread_br: prefix %s published\n", str);
    _prefix_set = true;
}

static void _handle_uhcp(const uint8_t *udp, uint16_t len)
{
    const uint8_t *uhcp = &udp[8];

    if ((len < 8 + 6) || (byteorder_bebuftohl(uhcp) != UHCP_MAGIC)
        || ((uhcp[4] & 0x0f) != UHCP_PUSH)) {
        return;
    }
    uint8_t prefix_len = uhcp[5];

    if ((prefix_len != 64) || (len < 8 + 6 + prefix_len / 8) || _prefix_set) {
        return;
    }
    _set_prefix(&uhcp[6], prefix_len);
}

/* IPv6 datagram of the host */
static void _handle_ipv6(uint8_t *ip, size_t len)
{
    uint16_t payload_len = byteorder_bebuftohs(&ip[4]);
    uint8_t proto = ip[6];
    const uint8_t *dst = &ip[24];
    uint8_t *payload = &ip[IPV6_HDR_LEN];

    if ((len < IPV6_HDR_LEN) || ((ip[0] >> 4) != 6)
        || (IPV6_HDR_LEN + payload_len > len)) {
        return;
    }

    if (_is_link_local(dst)) {
        if ((proto == PROTO_ICMPV6) && (payload_len >= 24) && (payload[0] == ICMPV6_NS)
            && !memcmp(&payload[8], _ll_addr, 16)) {
            _send_na(ip);
        }
        else if ((proto == PROTO_UDP) && (payload_len >= 8)
                 && (byteorder_bebuftohs(&payload[2]) == UHCP_PORT)) {
            _handle_uhcp(payload, payload_len);
        }
        return;
    }

    otInstance *ot = openthread_get_instance();
    otMessage *msg = otIp6NewMessage(ot, NULL);

    if (msg == NULL) {
        DEBUG("openthread_br: no message buffer\n");
        return;
    }
    if (otMessageAppend(msg, ip, IPV6_HDR_LEN + payload_len) != OT_ERROR_NONE) {
        otMessageFree(msg);
        return;
    }
    /* otIp6Send() frees the message */
    otIp6Send(ot, msg);
}

static void _ethos_rx(void)
{
    netdev_t *netdev = &ethos.netdev;
    int len = netdev->driver->recv(netdev, NULL, 0, NULL);

    if ((len <= 0) || (len > (int)sizeof(_frame))) {
        netdev->driver->recv(netdev, NULL, len, NULL);
        return;
    }
    len = netdev->driver->recv(netdev, _frame, len, NULL);
    if ((len < (int)(ETH_HDR_LEN + IPV6_HDR_LEN))
        || (byteorder_bebuftohs(&_frame[12]) != ETHERTYPE_IPV6)) {
        return;
    }
    memcpy(_host_mac, &_frame[6], 6);
    _host_mac_known = true;
    _handle_ipv6(&_frame[ETH_HDR_LEN], len - ETH_HDR_LEN);
}

static void _ethos_isr_handler(event_t *event)
{
    (void)event;
    ethos.netdev.driver->isr(&ethos.netdev);
}

static void _ethos_cb(netdev_t *netdev, netdev_event_t event)
{
    (void)netdev;
    switch (event) {
    case NETDEV_EVENT_ISR:
        event_post(openthread_get_evq(), &_ev_ethos_isr);
        break;
    case NETDEV_EVENT_RX_COMPLETE:
        _ethos_rx();
        break;
    case NETDEV_EVENT_LINK_UP:
        /* the host answered the hello: ask the prefix now */
        event_post(openthread_get_evq(), &_ev_uhcp);
        break;
    default:
        break;
    }
}

/* IPv6 datagram that OpenThread gives to the host */
static void _ot_receive(otMessage *msg, void *ctx)
{
    (void)ctx;
    uint16_t len = otMessageGetLength(msg);
    uint8_t *ip = &_frame[ETH_HDR_LEN];

    if ((len >= IPV6_HDR_LEN) && (len <= IPV6_MTU) && _host_mac_known
        && (otMessageRead(msg, 0, ip, len) == len)
        && !_is_link_local(&ip[24]) && !_is_mesh_local(&ip[24])) {
        memcpy(&_frame[0], _host_mac, 6);
        memcpy(&_frame[6], ethos.mac_addr, 6);
        _frame[12] = ETHERTYPE_IPV6 >> 8;
        _frame[13] = ETHERTYPE_IPV6 & 0xff;
        _send_frame(ETH_HDR_LEN + len);
    }
    otMessageFree(msg);
}

static void _uhcp_handler(event_t *event)
{
    (void)event;
    if (!_prefix_set) {
        _send_uhcp_req();
        ztimer_set(ZTIMER_MSEC, &_uhcp_timer, UHCP_PERIOD_MS);
    }
}

static void _uhcp_timer_cb(void *arg)
{
    (void)arg;
    event_post(openthread_get_evq(), &_ev_uhcp);
}

void openthread_border_router_init(otInstance *instance)
{
    ethos_setup(&ethos, &ethos_params[0], 0, _inbuf, sizeof(_inbuf));
    ethos.netdev.event_callback = _ethos_cb;
    if (ethos.netdev.driver->init(&ethos.netdev) < 0) {
        puts("openthread_br: cannot initialize ethos");
        return;
    }

    otIp6SetReceiveFilterEnabled(instance, true);
    otIp6SetReceiveCallback(instance, _ot_receive, NULL);

    _uhcp_timer.callback = _uhcp_timer_cb;
    event_post(openthread_get_evq(), &_ev_uhcp);
}
