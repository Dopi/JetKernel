/** @file wlan_proc.c
  * @brief This file contains functions for proc fin proc file.
  * 
  *  Copyright © Marvell International Ltd. and/or its affiliates, 2003-2006
  */
/********************************************************
Change log:
	10/04/05: Add Doxygen format comments
	01/05/06: Add kernel 2.6.x support	
	
********************************************************/
#include 	"include.h"

#ifdef CONFIG_MARVELL_8686_PROC_FS
/********************************************************
		Local Variables
********************************************************/

static char *szModes[] = {
    "Ad-hoc",
    "Managed",
    "Auto",
    "Unknown"
};

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/

/** 
 *  @brief proc read function
 *
 *  @param page	   pointer to buffer
 *  @param start   read data starting position
 *  @param offset  offset
 *  @param count   counter 
 *  @param eof     end of file flag
 *  @param data    data to output
 *  @return 	   number of output data
 */
static int
wlan_proc_read(char *page, char **start, off_t offset,
               int count, int *eof, void *data)
{
    int i;
    char *p = page;
    struct net_device *netdev = data;
    struct dev_mc_list *mcptr = netdev->mc_list;
    char fmt[64];
    wlan_private *priv = netdev->priv;
    wlan_adapter *Adapter = priv->adapter;
    ulong flags;

    if (offset != 0) {
        *eof = 1;
        goto exit;
    }

    get_version(Adapter, fmt, sizeof(fmt) - 1);

    p += sprintf(p, "driver_name = " "\"wlan\"\n");
    p += sprintf(p, "driver_version = %s", fmt);
    p += sprintf(p, "\nInterfaceName=\"%s\"\n", netdev->name);
    p += sprintf(p, "Mode=\"%s\"\n", szModes[Adapter->InfrastructureMode]);
    p += sprintf(p, "State=\"%s\"\n",
                 ((Adapter->MediaConnectStatus ==
                   WlanMediaStateDisconnected) ? "Disconnected" :
                  "Connected"));
    p += sprintf(p, "MACAddress=\"%02x:%02x:%02x:%02x:%02x:%02x\"\n",
                 netdev->dev_addr[0], netdev->dev_addr[1],
                 netdev->dev_addr[2], netdev->dev_addr[3],
                 netdev->dev_addr[4], netdev->dev_addr[5]);

    p += sprintf(p, "MCCount=\"%d\"\n", netdev->mc_count);
    p += sprintf(p, "ESSID=\"%s\"\n",
                 (u8 *) Adapter->CurBssParams.BSSDescriptor.Ssid.Ssid);
    p += sprintf(p, "Channel=\"%d\"\n",
                 Adapter->CurBssParams.BSSDescriptor.Channel);
    p += sprintf(p, "region_code = \"%02x\"\n", (u32) Adapter->RegionCode);

    /*
     * Put out the multicast list 
     */
    for (i = 0; i < netdev->mc_count; i++) {
        p += sprintf(p,
                     "MCAddr[%d]=\"%02x:%02x:%02x:%02x:%02x:%02x\"\n",
                     i,
                     mcptr->dmi_addr[0], mcptr->dmi_addr[1],
                     mcptr->dmi_addr[2], mcptr->dmi_addr[3],
                     mcptr->dmi_addr[4], mcptr->dmi_addr[5]);

        mcptr = mcptr->next;
    }
    p += sprintf(p, "num_tx_bytes = %lu\n", priv->stats.tx_bytes);
    p += sprintf(p, "num_rx_bytes = %lu\n", priv->stats.rx_bytes);
    p += sprintf(p, "num_tx_pkts = %lu\n", priv->stats.tx_packets);
    p += sprintf(p, "num_rx_pkts = %lu\n", priv->stats.rx_packets);
    p += sprintf(p, "num_tx_pkts_dropped = %lu\n", priv->stats.tx_dropped);
    p += sprintf(p, "num_rx_pkts_dropped = %lu\n", priv->stats.rx_dropped);
    p += sprintf(p, "num_tx_pkts_err = %lu\n", priv->stats.tx_errors);
    p += sprintf(p, "num_rx_pkts_err = %lu\n", priv->stats.rx_errors);
    p += sprintf(p, "carrier %s\n",
                 ((netif_carrier_ok(priv->wlan_dev.netdev)) ? "on" : "off"));
    p += sprintf(p, "tx queue %s\n",
                 ((netif_queue_stopped(priv->wlan_dev.netdev)) ? "stopped" :
                  "started"));

    spin_lock_irqsave(&Adapter->QueueSpinLock, flags);
    if (Adapter->CurCmd) {
        HostCmd_DS_COMMAND *CmdPtr =
            (HostCmd_DS_COMMAND *) Adapter->CurCmd->BufVirtualAddr;
        p += sprintf(p, "CurCmd ID = 0x%x, 0x%x\n",
                     wlan_cpu_to_le16(CmdPtr->Command),
                     wlan_cpu_to_le16(*(u16 *) ((u8 *) CmdPtr + S_DS_GEN)));
    } else {
        p += sprintf(p, "CurCmd NULL\n");
    }
    spin_unlock_irqrestore(&Adapter->QueueSpinLock, flags);

  exit:
    return (p - page);
}

/********************************************************
		Global Functions
********************************************************/

/** 
 *  @brief create wlan proc file
 *
 *  @param priv	   pointer wlan_private
 *  @param dev     pointer net_device
 *  @return 	   N/A
 */
void
wlan_proc_entry(wlan_private * priv, struct net_device *dev)
{

    PRINTM(INFO, "Creating Proc Interface\n");

    if (!priv->proc_entry) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
        priv->proc_entry = proc_mkdir("wlan", init_net.proc_net);
#else
        priv->proc_entry = proc_mkdir("wlan", proc_net);
#endif
        

        if (priv->proc_entry) {
            priv->proc_dev = create_proc_read_entry
                ("info", 0, priv->proc_entry, wlan_proc_read, dev);
        }
    }
}

/** 
 *  @brief remove proc file
 *
 *  @param priv	   pointer wlan_private
 *  @return 	   N/A
 */
void
wlan_proc_remove(wlan_private * priv)
{

    if (priv->proc_entry) {
        remove_proc_entry("info", priv->proc_entry);
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
    remove_proc_entry("wlan", init_net.proc_net);
#else
    remove_proc_entry("wlan", proc_net);
#endif

}

/** 
 *  @brief convert string to number
 *
 *  @param s   	   pointer to numbered string
 *  @return 	   converted number from string s
 */
int
string_to_number(char *s)
{
    int r = 0;
    int base = 0;

    if ((strncmp(s, "0x", 2) == 0) || (strncmp(s, "0X", 2) == 0))
        base = 16;
    else
        base = 10;
    if (base == 16)
        s += 2;
    for (s = s; *s != 0; s++) {
        if ((*s >= 48) && (*s <= 57))
            r = (r * base) + (*s - 48);
        else if ((*s >= 65) && (*s <= 70))
            r = (r * base) + (*s - 55);
        else if ((*s >= 97) && (*s <= 102))
            r = (r * base) + (*s - 87);
        else
            break;
    }

    return r;
}

#endif
