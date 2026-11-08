/*
 * SPDX-FileCopyrightText: 2017 Fundacion Inria Chile
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @{
 * @ingroup     net
 * @file
 * @brief       Bootstrap of OpenThread: radio, thread and event queue
 *
 * @author      Jose Ignacio Alamos <jialamos@uc.cl>
 * @}
 */

#include <assert.h>
#include <errno.h>
#include <string.h>

#include "event.h"
#include "kernel_defines.h"
#include "thread.h"

#include "openthread/dataset.h"
#include "openthread/instance.h"
#include "openthread/ip6.h"
#include "openthread/tasklet.h"
#include "openthread/thread.h"

#include "ot.h"

#ifdef MODULE_AT86RF2XX
#  include "at86rf2xx.h"
#  include "at86rf2xx_params.h"
#endif
#ifdef MODULE_CC2538_RF
#  include "cc2538_rf.h"
#endif
#ifdef MODULE_ESP_IEEE802154
#  include "esp_ieee802154_hal.h"
#endif
#ifdef MODULE_KW2XRF
#  include "bhp/event.h"
#  include "kw2xrf.h"
#  include "kw2xrf_params.h"
#endif
#ifdef MODULE_KW41ZRF
#  include "kw41zrf.h"
#endif
#ifdef MODULE_MRF24J40
#  include "bhp/event.h"
#  include "mrf24j40.h"
#  include "mrf24j40_params.h"
#endif
#ifdef MODULE_NRF802154
#  include "nrf802154.h"
#endif
#ifdef MODULE_SOCKET_ZEP
#  include "socket_zep.h"
#  include "socket_zep_params.h"
#endif

#define ENABLE_DEBUG 0
#include "debug.h"

static otInstance *_instance;
static event_queue_t _queue;
static ieee802154_dev_t _radio;
static char _stack[OPENTHREAD_STACKSIZE];

#ifdef MODULE_AT86RF2XX
static at86rf2xx_bhp_ev_t _at86rf2xx_bhp;
#endif
#ifdef MODULE_KW2XRF
static kw2xrf_t _kw2xrf;
static bhp_event_t _kw2xrf_bhp;
#endif
#ifdef MODULE_MRF24J40
static mrf24j40_t _mrf24j40;
static bhp_event_t _mrf24j40_bhp;
#endif
#ifdef MODULE_SOCKET_ZEP
static socket_zep_t _socket_zep;
#endif

event_queue_t *openthread_get_evq(void)
{
    return &_queue;
}

otInstance *openthread_get_instance(void)
{
    return _instance;
}

static void _tasklets_handler(event_t *event)
{
    (void)event;
    while (otTaskletsArePending(_instance)) {
        otTaskletsProcess(_instance);
    }
}

static event_t _ev_tasklets = { .handler = _tasklets_handler };

void otTaskletsSignalPending(otInstance *instance)
{
    (void)instance;
    event_post(&_queue, &_ev_tasklets);
}

/* Set up the radio with a Radio HAL implementation; the bottom half of the
 * radios that have one runs in the OpenThread thread */
static int _radio_setup(ieee802154_dev_t *radio)
{
#if defined(MODULE_AT86RF2XX)
    return at86rf2xx_init_event(&_at86rf2xx_bhp, &at86rf2xx_params[0], radio, &_queue);
#elif defined(MODULE_CC2538_RF)
    cc2538_rf_hal_setup(radio);
    cc2538_init();
    return 0;
#elif defined(MODULE_ESP_IEEE802154)
    esp_ieee802154_setup(radio);
    esp_ieee802154_init();
    return 0;
#elif defined(MODULE_KW2XRF)
    bhp_event_init(&_kw2xrf_bhp, &_queue, &kw2xrf_radio_hal_irq_handler, radio);
    return kw2xrf_init(&_kw2xrf, &kw2xrf_params[0], radio, bhp_event_isr_cb, &_kw2xrf_bhp);
#elif defined(MODULE_KW41ZRF)
    kw41zrf_hal_setup(radio);
    return kw41zrf_init();
#elif defined(MODULE_MRF24J40)
    bhp_event_init(&_mrf24j40_bhp, &_queue, &mrf24j40_radio_irq_handler, radio);
    return mrf24j40_init(&_mrf24j40, &mrf24j40_params[0], radio, bhp_event_isr_cb,
                         &_mrf24j40_bhp);
#elif defined(MODULE_NRF802154)
    nrf802154_hal_setup(radio);
    return nrf802154_init();
#elif defined(MODULE_SOCKET_ZEP)
    socket_zep_hal_setup(&_socket_zep, radio);
    socket_zep_setup(&_socket_zep, &socket_zep_params[0]);
    return 0;
#else
    (void)radio;
    return -ENODEV;
#endif
}

#if !IS_USED(MODULE_OPENTHREAD_RCP) && defined(OPENTHREAD_PANID) && defined(OPENTHREAD_CHANNEL)
/* Start the network described by the OPENTHREAD_* macros */
static void _start_network(void)
{
    static const uint8_t ext_panid[] = OPENTHREAD_EXT_PANID;
    static const uint8_t key[] = OPENTHREAD_NETWORK_KEY;
    static const uint8_t ml_prefix[] = OPENTHREAD_MESH_LOCAL_PREFIX;
    otOperationalDataset dataset;

    memset(&dataset, 0, sizeof(dataset));
    dataset.mActiveTimestamp.mSeconds = 1;
    dataset.mComponents.mIsActiveTimestampPresent = true;
    dataset.mChannel = OPENTHREAD_CHANNEL;
    dataset.mComponents.mIsChannelPresent = true;
    dataset.mPanId = OPENTHREAD_PANID;
    dataset.mComponents.mIsPanIdPresent = true;
    memcpy(dataset.mExtendedPanId.m8, ext_panid, sizeof(dataset.mExtendedPanId.m8));
    dataset.mComponents.mIsExtendedPanIdPresent = true;
    memcpy(dataset.mNetworkKey.m8, key, sizeof(dataset.mNetworkKey.m8));
    dataset.mComponents.mIsNetworkKeyPresent = true;
    memcpy(dataset.mMeshLocalPrefix.m8, ml_prefix, sizeof(dataset.mMeshLocalPrefix.m8));
    dataset.mComponents.mIsMeshLocalPrefixPresent = true;
    otNetworkNameFromString(&dataset.mNetworkName, OPENTHREAD_NETWORK_NAME);
    dataset.mComponents.mIsNetworkNamePresent = true;

    if (otDatasetSetActive(_instance, &dataset) != OT_ERROR_NONE) {
        DEBUG("openthread: cannot set the active dataset\n");
        return;
    }
    otIp6SetEnabled(_instance, true);
    otThreadSetEnabled(_instance, true);
}
#endif

static void *_openthread_thread(void *arg)
{
    (void)arg;

    event_queue_claim(&_queue);

    if (openthread_radio_init(&_radio) < 0) {
        DEBUG("openthread: radio initialization failed\n");
        return NULL;
    }

    _instance = otInstanceInitSingle();
    assert(_instance);

#if IS_USED(MODULE_OPENTHREAD_RCP)
    openthread_rcp_init(_instance);
#else
#  if IS_USED(MODULE_SHELL_CMD_OPENTHREAD)
    ot_shell_init(_instance);
#  endif
#  if IS_USED(MODULE_OPENTHREAD_BORDER_ROUTER)
    openthread_border_router_init(_instance);
#  endif
#  if defined(OPENTHREAD_PANID) && defined(OPENTHREAD_CHANNEL)
    _start_network();
#  endif
#endif

    event_loop(&_queue);

    return NULL;
}

void openthread_bootstrap(void)
{
    event_queue_init_detached(&_queue);

    if (_radio_setup(&_radio) < 0) {
        DEBUG("openthread: no radio\n");
        return;
    }

    thread_create(_stack, sizeof(_stack), OPENTHREAD_PRIORITY, 0,
                  _openthread_thread, NULL, "openthread");
}
