#ifndef P54_H
#define P54_H

/*
 * Shared defines for all mac80211 Prism54 code
 *
 * Copyright (c) 2006, Michael Wu <flamingice@sourmilk.net>
 *
 * Based on the islsm (softmac prism54) driver, which is:
 * Copyright 2004-2006 Jean-Baptiste Note <jbnote@gmail.com>, et al.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

enum p54_control_frame_types {
	P54_CONTROL_TYPE_SETUP = 0,
	P54_CONTROL_TYPE_SCAN,
	P54_CONTROL_TYPE_TRAP,
	P54_CONTROL_TYPE_DCFINIT,
	P54_CONTROL_TYPE_RX_KEYCACHE,
	P54_CONTROL_TYPE_TIM,
	P54_CONTROL_TYPE_PSM,
	P54_CONTROL_TYPE_TXCANCEL,
	P54_CONTROL_TYPE_TXDONE,
	P54_CONTROL_TYPE_BURST,
	P54_CONTROL_TYPE_STAT_READBACK,
	P54_CONTROL_TYPE_BBP,
	P54_CONTROL_TYPE_EEPROM_READBACK,
	P54_CONTROL_TYPE_LED,
	P54_CONTROL_TYPE_GPIO,
	P54_CONTROL_TYPE_TIMER,
	P54_CONTROL_TYPE_MODULATION,
	P54_CONTROL_TYPE_SYNTH_CONFIG,
	P54_CONTROL_TYPE_DETECTOR_VALUE,
	P54_CONTROL_TYPE_XBOW_SYNTH_CFG,
	P54_CONTROL_TYPE_CCE_QUIET,
	P54_CONTROL_TYPE_PSM_STA_UNLOCK,
	P54_CONTROL_TYPE_PCS,
	P54_CONTROL_TYPE_BT_BALANCER = 28,
	P54_CONTROL_TYPE_GROUP_ADDRESS_TABLE = 30,
	P54_CONTROL_TYPE_ARPTABLE = 31,
	P54_CONTROL_TYPE_BT_OPTIONS = 35
};

#define P54_HDR_FLAG_CONTROL		BIT(15)
#define P54_HDR_FLAG_CONTROL_OPSET	(BIT(15) + BIT(0))

struct p54_hdr {
	__le16 flags;
	__le16 len;
	__le32 req_id;
	__le16 type;	/* enum p54_control_frame_types */
	u8 rts_tries;
	u8 tries;
	u8 data[0];
} __attribute__ ((packed));

#define FREE_AFTER_TX(skb)						\
	((((struct p54_hdr *) ((struct sk_buff *) skb)->data)->		\
	flags) == cpu_to_le16(P54_HDR_FLAG_CONTROL_OPSET))

struct p54_edcf_queue_param {
	__le16 aifs;
	__le16 cwmin;
	__le16 cwmax;
	__le16 txop;
} __attribute__ ((packed));

struct p54_rssi_linear_approximation {
	s16 mul;
	s16 add;
	s16 longbow_unkn;
	s16 longbow_unk2;
};

#define EEPROM_READBACK_LEN 0x3fc

#define ISL38XX_DEV_FIRMWARE_ADDR 0x20000

#define FW_FMAC 0x464d4143
#define FW_LM86 0x4c4d3836
#define FW_LM87 0x4c4d3837
#define FW_LM20 0x4c4d3230

struct p54_common {
	struct ieee80211_hw *hw;
	u32 rx_start;
	u32 rx_end;
	struct sk_buff_head tx_queue;
	void (*tx)(struct ieee80211_hw *dev, struct sk_buff *skb);
	int (*open)(struct ieee80211_hw *dev);
	void (*stop)(struct ieee80211_hw *dev);
	int mode;
	u16 rx_mtu;
	u8 headroom;
	u8 tailroom;
	struct mutex conf_mutex;
	u8 mac_addr[ETH_ALEN];
	u8 bssid[ETH_ALEN];
	struct pda_iq_autocal_entry *iq_autocal;
	unsigned int iq_autocal_len;
	struct pda_channel_output_limit *output_limit;
	unsigned int output_limit_len;
	struct pda_pa_curve_data *curve_data;
	struct p54_rssi_linear_approximation rssical_db[IEEE80211_NUM_BANDS];
	unsigned int filter_flags;
	bool use_short_slot;
	u16 rxhw;
	u8 version;
	unsigned int tx_hdr_len;
	unsigned int fw_var;
	unsigned int fw_interface;
	unsigned int output_power;
	u32 tsf_low32;
	u32 tsf_high32;
	u64 basic_rate_mask;
	u16 wakeup_timer;
	u16 aid;
	struct ieee80211_tx_queue_stats tx_stats[8];
	struct p54_edcf_queue_param qos_params[8];
	struct ieee80211_low_level_stats stats;
	struct delayed_work work;
	struct sk_buff *cached_beacon;
	int noise;
	void *eeprom;
	struct completion eeprom_comp;
	u8 privacy_caps;
	u8 rx_keycache_size;
};

int p54_rx(struct ieee80211_hw *dev, struct sk_buff *skb);
void p54_free_skb(struct ieee80211_hw *dev, struct sk_buff *skb);
int p54_parse_firmware(struct ieee80211_hw *dev, const struct firmware *fw);
int p54_read_eeprom(struct ieee80211_hw *dev);
struct ieee80211_hw *p54_init_common(size_t priv_data_len);
void p54_free_common(struct ieee80211_hw *dev);

#endif /* P54_H */
