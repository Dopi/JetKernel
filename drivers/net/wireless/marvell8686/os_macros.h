/*
 * Copyright © Marvell International Ltd. and/or its affiliates, 2003-2006
 */
#ifndef	_OS_MACROS_H
#define _OS_MACROS_H

#define os_time_get()	jiffies

extern spinlock_t driver_lock;
extern unsigned long driver_flags;
#define OS_INT_DISABLE	spin_lock_irqsave(&driver_lock, driver_flags)
#define	OS_INT_RESTORE	spin_unlock_irqrestore(&driver_lock, driver_flags); \
			driver_lock = SPIN_LOCK_UNLOCKED

#define UpdateTransStart(dev) { \
	dev->trans_start = jiffies; \
}

#define OS_SET_THREAD_STATE(x)		set_current_state(x)

#define MODULE_GET	if(try_module_get(THIS_MODULE)==0) return WLAN_STATUS_FAILURE;
#define MODULE_PUT	module_put(THIS_MODULE)

#define OS_INIT_SEMAPHORE(x)    	init_MUTEX(x)
#define OS_ACQ_SEMAPHORE_BLOCK(x)	down_interruptible(x)
#define OS_ACQ_SEMAPHORE_NOBLOCK(x)	down_trylock(x)
#define OS_REL_SEMAPHORE(x) 		up(x)

/* Definitions below are needed for other OS like threadx */
#define	TX_DISABLE
#define TX_RESTORE
#define	ConfigureThreadPriority()
#define OS_INTERRUPT_SAVE_AREA
#define OS_FREE_LOCK(x)
#define TX_EVENT_FLAGS_SET(x, y, z)

#define os_wait_interruptible_timeout(waitq, cond, timeout) \
	wait_event_interruptible_timeout(waitq, cond, timeout)

static inline void
os_sched_timeout(u32 millisec)
{
    set_current_state(TASK_INTERRUPTIBLE);

    schedule_timeout((millisec * HZ) / 1000);
}

static inline void
os_schedule(u32 millisec)
{
    schedule_timeout((millisec * HZ) / 1000);
}

static inline int
CopyMulticastAddrs(wlan_adapter * Adapter, struct net_device *dev)
{
    int i = 0;
    struct dev_mc_list *mcptr = dev->mc_list;

    for (i = 0; i < dev->mc_count; i++) {
        memcpy(&Adapter->MulticastList[i], mcptr->dmi_addr, ETH_ALEN);
        mcptr = mcptr->next;
    }

    return i;
}

static inline u32
get_utimeofday(void)
{
    struct timeval t;
    u32 ut;

    do_gettimeofday(&t);
    ut = (u32) t.tv_sec * 1000000 + ((u32) t.tv_usec);
    return ut;
}

static inline int
os_upload_rx_packet(wlan_private * priv, struct sk_buff *skb)
{

#define IPFIELD_ALIGN_OFFSET	2

    skb->dev = priv->wlan_dev.netdev;
    skb->protocol = eth_type_trans(skb, priv->wlan_dev.netdev);
    skb->ip_summed = CHECKSUM_UNNECESSARY;

    netif_rx(skb);

    return 0;
}

static inline void
os_free_tx_packet(wlan_private * priv)
{
    ulong flags;

    if (priv->adapter->CurrentTxSkb) {
        kfree_skb(priv->adapter->CurrentTxSkb);
        spin_lock_irqsave(&priv->adapter->CurrentTxLock, flags);
        priv->adapter->CurrentTxSkb = NULL;
        spin_unlock_irqrestore(&priv->adapter->CurrentTxLock, flags);
    }
}

/*
 *  netif carrier_on/off and start(wake)/stop_queue handling
 *
 *           carrier_on      carrier_off     start_queue     stop_queue
 * open           x(connect)      x(disconnect)   x
 * close                          x                               x
 * assoc          x                               x
 * deauth                         x                               x
 * adhoc-start
 * adhoc-join
 * adhoc-link     x                               x
 * adhoc-bcnlost                  x                               x
 * scan-begin                     x                               x
 * scan-end       x                               x
 * ds-enter                       x                               x
 * ds-exit        x                               x
 * xmit                                                           x
 * xmit-done                                      x
 * tx-timeout
 */
static inline void
os_carrier_on(wlan_private * priv)
{
    if (!netif_carrier_ok(priv->wlan_dev.netdev) &&
        (priv->adapter->MediaConnectStatus == WlanMediaStateConnected) &&
        ((priv->adapter->InfrastructureMode != Wlan802_11IBSS) ||
         (priv->adapter->AdhocLinkSensed))) {
        netif_carrier_on(priv->wlan_dev.netdev);
    }
}

static inline void
os_carrier_off(wlan_private * priv)
{
    if (netif_carrier_ok(priv->wlan_dev.netdev)) {
        netif_carrier_off(priv->wlan_dev.netdev);
    }
}

static inline void
os_start_queue(wlan_private * priv)
{
    if (netif_queue_stopped(priv->wlan_dev.netdev) &&
        (priv->adapter->MediaConnectStatus == WlanMediaStateConnected) &&
        ((priv->adapter->InfrastructureMode != Wlan802_11IBSS) ||
         (priv->adapter->AdhocLinkSensed))) {
        netif_wake_queue(priv->wlan_dev.netdev);
    }
}

static inline void
os_stop_queue(wlan_private * priv)
{
    if (!netif_queue_stopped(priv->wlan_dev.netdev)) {
        netif_stop_queue(priv->wlan_dev.netdev);
    }
}

#endif /* _OS_MACROS_H */
