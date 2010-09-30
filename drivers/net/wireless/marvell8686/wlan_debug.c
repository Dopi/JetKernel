/** @file wlan_debug.c
  * @brief This file contains functions for debug proc file.
  * 
  *  Copyright © Marvell International Ltd. and/or its affiliates, 2003-2007
  */
/********************************************************
Change log:
	10/04/05: Add Doxygen format comments
	01/05/06: Add kernel 2.6.x support	
	
********************************************************/

#include  "include.h"

#ifdef CONFIG_MARVELL_8686_PROC_FS
/********************************************************
		Local Variables
********************************************************/

#define item_size(n) (sizeof ((wlan_adapter *)0)->n)
#define item_addr(n) ((u32) &((wlan_adapter *)0)->n)

#define item_dbg_size(n) (sizeof (((wlan_adapter *)0)->dbg.n))
#define item_dbg_addr(n) ((u32) &(((wlan_adapter *)0)->dbg.n))

#define item1_size(n) (sizeof ((wlan_dev_t *)0)->n)
#define item1_addr(n) ((u32) &((wlan_dev_t *)0)->n)

struct debug_data
{
    char name[32];
    u32 size;
    u32 addr;
    u32 offset;
};

/* To debug any member of wlan_adapter or wlan_dev_t, simply add one line here.
 */
#define ITEMS_FROM_WLAN_DEV		1

static struct debug_data items[] = {
    {"IntCounter", item_size(IntCounter), 0, item_addr(IntCounter)},
    {"ConnectStatus", item_size(MediaConnectStatus), 0,
     item_addr(MediaConnectStatus)},
    {"wmmQStp", item_size(wmm.queueStopped), 0, item_addr(wmm.queueStopped)},
    {"wmmPkts", item_size(wmm.packetsQueued), 0,
     item_addr(wmm.packetsQueued)},
    {"wmmAcVo", item_size(wmm.packetsOut[WMM_AC_VO]), 0,
     item_addr(wmm.packetsOut[WMM_AC_VO])},
    {"wmmAcVi", item_size(wmm.packetsOut[WMM_AC_VI]), 0,
     item_addr(wmm.packetsOut[WMM_AC_VI])},
    {"wmmAcBE", item_size(wmm.packetsOut[WMM_AC_BE]), 0,
     item_addr(wmm.packetsOut[WMM_AC_BE])},
    {"wmmAcBK", item_size(wmm.packetsOut[WMM_AC_BK]), 0,
     item_addr(wmm.packetsOut[WMM_AC_BK])},
    {"PSMode", item_size(PSMode), 0, item_addr(PSMode)},
    {"PSState", item_size(PSState), 0, item_addr(PSState)},
    {"IsDeepSleep", item_size(IsDeepSleep), 0, item_addr(IsDeepSleep)},
    {"IsAutoDeepSleepEnabled", item_size(IsAutoDeepSleepEnabled), 0,
     item_addr(IsAutoDeepSleepEnabled)},
    {"WakeupDevReq", item_size(bWakeupDevRequired), 0,
     item_addr(bWakeupDevRequired)},
    {"WakeupTries", item_size(WakeupTries), 0, item_addr(WakeupTries)},
    {"HS_Configured", item_size(bHostSleepConfigured), 0,
     item_addr(bHostSleepConfigured)},
    {"HS_Activated", item_size(HS_Activated), 0, item_addr(HS_Activated)},
    {"num_tx_timeout", item_dbg_size(num_tx_timeout), 0,
     item_dbg_addr(num_tx_timeout)},
    {"num_cmd_timeout", item_dbg_size(num_cmd_timeout), 0,
     item_dbg_addr(num_cmd_timeout)},
    {"TimeoutCmdId", item_dbg_size(TimeoutCmdId), 0,
     item_dbg_addr(TimeoutCmdId)},
    {"TimeoutCmdAct", item_dbg_size(TimeoutCmdAct), 0,
     item_dbg_addr(TimeoutCmdAct)},
    {"LastCmdId", item_dbg_size(LastCmdId), 0, item_dbg_addr(LastCmdId)},
    {"LastCmdAct", item_dbg_size(LastCmdAct), 0, item_dbg_addr(LastCmdAct)},
    {"LastCmdIndex", item_dbg_size(LastCmdIndex), 0,
     item_dbg_addr(LastCmdIndex)},
    {"LastCmdRespId", item_dbg_size(LastCmdRespId), 0,
     item_dbg_addr(LastCmdRespId)},
    {"LastCmdRespIndex", item_dbg_size(LastCmdRespIndex), 0,
     item_dbg_addr(LastCmdRespIndex)},
    {"LastEvent", item_dbg_size(LastEvent), 0, item_dbg_addr(LastEvent)},
    {"LastEventIndex", item_dbg_size(LastEventIndex), 0,
     item_dbg_addr(LastEventIndex)},
    {"num_cmd_h2c_fail", item_dbg_size(num_cmd_host_to_card_failure), 0,
     item_dbg_addr(num_cmd_host_to_card_failure)},
    {"num_cmd_sleep_cfm_fail",
     item_dbg_size(num_cmd_sleep_cfm_host_to_card_failure), 0,
     item_dbg_addr(num_cmd_sleep_cfm_host_to_card_failure)},
    {"num_tx_h2c_fail", item_dbg_size(num_tx_host_to_card_failure), 0,
     item_dbg_addr(num_tx_host_to_card_failure)},
    {"num_evt_deauth", item_dbg_size(num_event_deauth), 0,
     item_dbg_addr(num_event_deauth)},
    {"num_evt_disassoc", item_dbg_size(num_event_disassoc), 0,
     item_dbg_addr(num_event_disassoc)},
    {"num_evt_link_lost", item_dbg_size(num_event_link_lost), 0,
     item_dbg_addr(num_event_link_lost)},
    {"num_cmd_deauth", item_dbg_size(num_cmd_deauth), 0,
     item_dbg_addr(num_cmd_deauth)},
    {"num_cmd_assoc_ok", item_dbg_size(num_cmd_assoc_success), 0,
     item_dbg_addr(num_cmd_assoc_success)},
    {"num_cmd_assoc_fail", item_dbg_size(num_cmd_assoc_failure), 0,
     item_dbg_addr(num_cmd_assoc_failure)},

    {"dnld_sent", item1_size(dnld_sent), 0, item1_addr(dnld_sent)},
};

static int num_of_items = sizeof(items) / sizeof(items[0]);

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
 *  @param s       read data starting position
 *  @param off     offset
 *  @param cnt     counter 
 *  @param eof     end of file flag
 *  @param data    data to output
 *  @return 	   number of output data
 */
static int
wlan_debug_read(char *page, char **s, off_t off, int cnt, int *eof,
                void *data)
{
    int val = 0;
    char *p = page;
    int i;

    struct debug_data *d = (struct debug_data *) data;

    MODULE_GET;

    for (i = 0; i < num_of_items; i++) {
        if (d[i].size == 1)
            val = *((u8 *) d[i].addr);
        else if (d[i].size == 2)
            val = *((u16 *) d[i].addr);
        else if (d[i].size == 4)
            val = *((u32 *) d[i].addr);
        else {
            int j;
            p += sprintf(p, "%s=", d[i].name);
            for (j = 0; j < d[i].size; j += 2) {
                val = *(u16 *) (d[i].addr + j);
                p += sprintf(p, "0x%x ", val);
            }
            p += sprintf(p, "\n");
            continue;
        }

        if (strstr(d[i].name, "Id"))
            p += sprintf(p, "%s=0x%x\n", d[i].name, val);
        else
            p += sprintf(p, "%s=%d\n", d[i].name, val);
    }
    MODULE_PUT;
    return p - page;
}

/** 
 *  @brief proc write function
 *
 *  @param f	   file pointer
 *  @param buf     pointer to data buffer
 *  @param cnt     data number to write
 *  @param data    data to write
 *  @return 	   number of data
 */
static int
wlan_debug_write(struct file *f, const char *buf, unsigned long cnt,
                 void *data)
{
    int r, i;
    char *pdata;
    char *p;
    char *p0;
    char *p1;
    char *p2;
    struct debug_data *d = (struct debug_data *) data;

    MODULE_GET;

    pdata = (char *) kmalloc(cnt, GFP_KERNEL);
    if (pdata == NULL) {
        MODULE_PUT;
        return 0;
    }

    if (copy_from_user(pdata, buf, cnt)) {
        PRINTM(INFO, "Copy from user failed\n");
        kfree(pdata);
        MODULE_PUT;
        return 0;
    }

    p0 = pdata;
    for (i = 0; i < num_of_items; i++) {
        do {
            p = strstr(p0, d[i].name);
            if (p == NULL)
                break;
            p1 = strchr(p, '\n');
            if (p1 == NULL)
                break;
            p0 = p1++;
            p2 = strchr(p, '=');
            if (!p2)
                break;
            p2++;
            r = string_to_number(p2);
            if (d[i].size == 1)
                *((u8 *) d[i].addr) = (u8) r;
            else if (d[i].size == 2)
                *((u16 *) d[i].addr) = (u16) r;
            else if (d[i].size == 4)
                *((u32 *) d[i].addr) = (u32) r;
            break;
        } while (TRUE);
    }
    kfree(pdata);
    MODULE_PUT;
    return cnt;
}

/********************************************************
		Global Functions
********************************************************/
/** 
 *  @brief create debug proc file
 *
 *  @param priv	   pointer wlan_private
 *  @param dev     pointer net_device
 *  @return 	   N/A
 */
void
wlan_debug_entry(wlan_private * priv, struct net_device *dev)
{
    int i;
    struct proc_dir_entry *r;

    if (priv->proc_entry == NULL)
        return;

    for (i = 0; i < (num_of_items - ITEMS_FROM_WLAN_DEV); i++) {
        items[i].addr = items[i].offset + (u32) priv->adapter;
    }
    for (i = num_of_items - ITEMS_FROM_WLAN_DEV; i < num_of_items; i++) {
        items[i].addr = items[i].offset + (u32) & priv->wlan_dev;
    }
    r = create_proc_entry("debug", 0644, priv->proc_entry);
    if (r == NULL)
        return;

    r->data = &items[0];
    r->read_proc = wlan_debug_read;
    r->write_proc = wlan_debug_write;
    r->owner = THIS_MODULE;

}

/** 
 *  @brief remove proc file
 *
 *  @param priv	   pointer wlan_private
 *  @return 	   N/A
 */
void
wlan_debug_remove(wlan_private * priv)
{
    remove_proc_entry("debug", priv->proc_entry);
}

#endif
