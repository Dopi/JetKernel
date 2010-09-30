/** @file wlan_join.h
 *
 *  @brief Interface for the wlan infrastructure and adhoc join routines
 *
 *  Driver interface functions and type declarations for the join module
 *    implemented in wlan_join.c.  Process all start/join requests for
 *    both adhoc and infrastructure networks
 *
 *  Copyright © Marvell International Ltd. and/or its affiliates, 2003-2006
 *
 *  @sa wlan_join.c
 */
/*************************************************************
Change Log:
    01/11/06: Initial revision. Match new scan code, relocate related functions

************************************************************/

#ifndef _WLAN_JOIN_H
#define _WLAN_JOIN_H

//! Size of buffer allocated to store the association response from firmware
#define MRVDRV_ASSOC_RSP_BUF_SIZE  500

//! Size of buffer allocated to store IEs passed to firmware in the assoc req
#define MRVDRV_GENIE_BUF_SIZE      256

//! Size of buffer allocated to store TLVs passed to firmware in the assoc req
#define MRVDRV_ASSOC_TLV_BUF_SIZE  256

extern int wlan_cmd_802_11_authenticate(wlan_private * priv,
                                        HostCmd_DS_COMMAND * cmd,
                                        void *pdata_buf);
extern int wlan_cmd_802_11_ad_hoc_join(wlan_private * priv,
                                       HostCmd_DS_COMMAND * cmd,
                                       void *pdata_buf);
extern int wlan_cmd_802_11_ad_hoc_stop(wlan_private * priv,
                                       HostCmd_DS_COMMAND * cmd);
extern int wlan_cmd_802_11_ad_hoc_start(wlan_private * priv,
                                        HostCmd_DS_COMMAND * cmd,
                                        void *pssid);
extern int wlan_cmd_802_11_deauthenticate(wlan_private * priv,
                                          HostCmd_DS_COMMAND * cmd);
extern int wlan_cmd_802_11_associate(wlan_private * priv,
                                     HostCmd_DS_COMMAND * cmd,
                                     void *pdata_buf);
extern int wlan_ret_802_11_authenticate(wlan_private * priv,
                                        HostCmd_DS_COMMAND * resp);
extern int wlan_ret_802_11_ad_hoc(wlan_private * priv,
                                  HostCmd_DS_COMMAND * resp);
extern int wlan_ret_802_11_ad_hoc_stop(wlan_private * priv,
                                       HostCmd_DS_COMMAND * resp);
extern int wlan_ret_802_11_disassociate(wlan_private * priv,
                                        HostCmd_DS_COMMAND * resp);
extern int wlan_ret_802_11_associate(wlan_private * priv,
                                     HostCmd_DS_COMMAND * resp);

extern int wlan_associate(wlan_private * priv, BSSDescriptor_t * pBSSDesc);
extern int wlan_associate_to_table_idx(wlan_private * priv, int tableIdx);

extern int wlanidle_on(wlan_private * priv);
extern int wlanidle_off(wlan_private * priv);

extern int wlan_do_adhocstop_ioctl(wlan_private * priv);
extern int wlan_reassociation_thread(void *data);

extern int StartAdhocNetwork(wlan_private * priv,
                             WLAN_802_11_SSID * AdhocSSID);
extern int JoinAdhocNetwork(wlan_private * priv, BSSDescriptor_t * pBSSDesc);
extern int StopAdhocNetwork(wlan_private * priv);
extern int SendDeauthentication(wlan_private * priv);

extern int wlan_do_adhocstop_ioctl(wlan_private * priv);
extern int wlan_get_assoc_rsp_ioctl(wlan_private * priv, struct iwreq *wrq);
extern int wlan_set_mrvl_tlv_ioctl(wlan_private * priv, struct iwreq *wrq);

#ifdef __KERNEL__
extern int wlan_set_wap(struct net_device *dev, struct iw_request_info *info,
                        struct sockaddr *awrq, char *extra);
#endif

extern int sendADHOCBSSIDQuery(wlan_private * priv);

#endif
