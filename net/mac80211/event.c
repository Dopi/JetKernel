/*
 * Copyright 2007	Johannes Berg <johannes@sipsolutions.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * mac80211 - events
 */

#include <net/iw_handler.h>
#include "ieee80211_i.h"

/*
 * indicate a failed Michael MIC to userspace; the passed packet
 * (in the variable hdr) must be long enough to extract the TKIP
 * fields like TSC
 */
void mac80211_ev_michael_mic_failure(struct ieee80211_sub_if_data *sdata, int keyidx,
				     struct ieee80211_hdr *hdr)
{
	union iwreq_data wrqu;
	char *buf = kmalloc(128, GFP_ATOMIC);

	if (buf) {
		/* TODO: needed parameters: count, key type, TSC */
		sprintf(buf, "MLME-MICHAELMICFAILURE.indication("
			"keyid=%d %scast addr=%pM)",
			keyidx, hdr->addr1[0] & 0x01 ? "broad" : "uni",
			hdr->addr2);
		memset(&wrqu, 0, sizeof(wrqu));
		wrqu.data.length = strlen(buf);
		wireless_send_event(sdata->dev, IWEVCUSTOM, &wrqu, buf);
		kfree(buf);
	}

	/*
	 * TODO: re-add support for sending MIC failure indication
	 * with all info via nl80211
	 */
}
