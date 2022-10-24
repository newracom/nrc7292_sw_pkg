/*
 * Copyright (c) 2016-2019 Newracom, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <net/genetlink.h>
#include "nrc-mac80211.h"
#include "nrc-netlink.h"
#include "nrc.h"
#include "wim.h"
#include "nrc-debug.h"
#include "nrc-hif.h"
#include "nrc-stats.h"

#define NRC_NETLINK_FAMILY_NAME	("NRC-NL-FAM")
#define MAX_CAPIREQ_SIZE (32)
#define MAX_HALOW_SIZE (32)

static struct nrc *nrc_nw;


#ifdef CONFIG_SUPPORT_NEW_NETLINK
static int nrc_nl_pre_doit(const struct genl_ops *ops,
			   struct sk_buff *skb, struct genl_info *info)
#else
static int nrc_nl_pre_doit(struct genl_ops *ops,
			   struct sk_buff *skb, struct genl_info *info)
#endif
{
	return 0;
}

#ifdef CONFIG_SUPPORT_NEW_NETLINK
static void nrc_nl_post_doit(const struct genl_ops *ops,
			     struct sk_buff *skb, struct genl_info *info)
#else
static void nrc_nl_post_doit(struct genl_ops *ops,
			     struct sk_buff *skb, struct genl_info *info)
#endif
{
}

static bool nrc_set_stbc_rx(struct nrc *nw, u8 stream)
{
	struct ieee80211_supported_band *sband = NULL;

	sband = &nw->bands[NL80211_BAND_2GHZ];

	if (stream > 4)
		return false;

	if (stream == 0) {
		sband->ht_cap.cap &= ~(IEEE80211_HT_CAP_RX_STBC);
		return true;
	}

	sband->ht_cap.cap |= (stream << IEEE80211_HT_CAP_RX_STBC_SHIFT);

	return true;
}


static const struct nla_policy nl_umac_policy[NL_WFA_CAPI_ATTR_LAST] = {
	[NL_WFA_CAPI_INTF_ID]		= {.type = NLA_NUL_STRING},
	[NL_WFA_CAPI_PARAM_NAME]	= {.type = NLA_NUL_STRING},
	[NL_WFA_CAPI_PARAM_STR_VAL]	= {.type = NLA_NUL_STRING},
	[NL_WFA_CAPI_PARAM_DESTADDR]	= {.type = NLA_UNSPEC, .len = ETH_ALEN},
	[NL_WFA_CAPI_PARAM_MCS]		= {.type = NLA_U8},
	[NL_WFA_CAPI_PARAM_TID]		= {.type = NLA_U8},
	[NL_WFA_CAPI_PARAM_SMPS]	= {.type = NLA_U8},
	[NL_WFA_CAPI_PARAM_STBC]	= {.type = NLA_U8},
	[NL_WFA_CAPI_PARAM_VENDOR1]	= {.type = NLA_NUL_STRING},
	[NL_WFA_CAPI_PARAM_VENDOR2]	= {.type = NLA_NUL_STRING},
	[NL_WFA_CAPI_PARAM_VENDOR3]	= {.type = NLA_NUL_STRING},
	[NL_WFA_CAPI_PARAM_RESPONSE]	= {.type = NLA_NUL_STRING},
	[NL_WFA_CAPI_PARAM_VIF_ID]  = {.type = NLA_S32},
	[NL_WFA_CAPI_PARAM_BSS_MAX_IDLE_OFFSET]	= {.type = NLA_S32},
	[NL_WFA_CAPI_PARAM_BSS_MAX_IDLE]		= {.type = NLA_S32},
	[NL_SHELL_RUN_CMD]		= {.type = NLA_NUL_STRING},
	[NL_CMD_LOG_MSG]		= {.type = NLA_NUL_STRING},
	[NL_CMD_LOG_TYPE]		= {.type = NLA_U8},
	[NL_MGMT_FRAME_INJECTION]	= {.type = NLA_U8},
	[NL_HALOW_PARAM_NAME]		= {.type = NLA_NUL_STRING},
	[NL_HALOW_PARAM_STR_VAL]	= {.type = NLA_NUL_STRING},
	[NL_MIC_SCAN_CHANNEL_START] = {.type = NLA_S32},
	[NL_MIC_SCAN_CHANNEL_END] = {.type = NLA_S32},
	[NL_CMD_RECOVERY_MSG] = {.type = NLA_NUL_STRING},
	[NL_FRAME_INJECTION_BUFFER] = {.type = NLA_NUL_STRING},
	[NL_SET_IE_EID] = {.type = NLA_U16},
	[NL_SET_IE_LENGTH] = {.type = NLA_U8},
	[NL_SET_IE_DATA] = {.type = NLA_NUL_STRING},
	[NL_SET_SAE_EID] = {.type = NLA_U16},
	[NL_SET_SAE_LENGTH] = {.type = NLA_U16},
	[NL_SET_SAE_DATA] = {.type = NLA_NUL_STRING},
	[NL_SHELL_RUN_CMD_RAW]		= {.type = NLA_NUL_STRING},
	[NL_AUTO_BA_ON]		= {.type = NLA_U8},
};

static const struct genl_multicast_group nl_umac_mcast_grps[] = {
	[NL_MCGRP_WFA_CAPI_RESPONSE] = { .name = "response", },
	[NL_MCGRP_NRC_LOG]			= { .name = "nrc-log", },
};

static struct genl_family nrc_nl_fam = {
	.id		= GENL_ID_GENERATE,
	.hdrsize	= 0,
	.name		= NRC_NETLINK_FAMILY_NAME,
	.version	= 1,
	.maxattr	= MAX_NL_WFA_CAPI_ATTR,
#if KERNEL_VERSION(5, 2, 0) <= NRC_TARGET_KERNEL_VERSION
	.policy = nl_umac_policy,
#endif
#ifdef CONFIG_SUPPORT_NEW_NETLINK
	.parallel_ops	= false,
#endif
	.netnsok	= true,
	.pre_doit	= nrc_nl_pre_doit,
	.post_doit	= nrc_nl_post_doit,
#ifdef CONFIG_SUPPORT_AFTER_KERNEL_3_0_36
	.mcgrps = nl_umac_mcast_grps,
	.n_mcgrps = ARRAY_SIZE(nl_umac_mcast_grps),
#endif
};


int nrc_netlink_rx(struct nrc *nw, struct sk_buff *skb, u8 subtype)
{
	struct sk_buff *mcast_skb;
	void *data;

#ifdef CONFIG_SUPPORT_GENLMSG_DEFAULT
	mcast_skb = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
#else
	mcast_skb = genlmsg_new(NLMSG_DEFAULT_SIZE - GENL_HDRLEN, GFP_KERNEL);
#endif

	if (!mcast_skb)
		return -1;

	data = genlmsg_put(mcast_skb, 0, 0, &nrc_nl_fam, 0, NL_CMD_LOG_EVENT);

	skb_put(skb, 1);
	skb->data[skb->len-1] = 0;

	nla_put_string(mcast_skb, NL_CMD_LOG_MSG, skb->data);
	nla_put_u8(mcast_skb, NL_CMD_LOG_TYPE, subtype);
	genlmsg_end(mcast_skb, data);

#ifdef CONFIG_SUPPORT_GENLMSG_DEFAULT
	genlmsg_multicast(&nrc_nl_fam,
			mcast_skb,
			0,
			NL_MCGRP_NRC_LOG,
			GFP_KERNEL);
#else
	genlmsg_multicast(mcast_skb,
			0,
			NL_MCGRP_NRC_LOG,
			GFP_KERNEL);
#endif

	dev_kfree_skb(skb);

	return 0;
}

int nrc_netlink_trigger_recovery(struct nrc *nw)
{
	struct sk_buff *mcast_skb;
	void *data;

#ifdef CONFIG_SUPPORT_GENLMSG_DEFAULT
	mcast_skb = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
#else
	mcast_skb = genlmsg_new(NLMSG_DEFAULT_SIZE - GENL_HDRLEN, GFP_KERNEL);
#endif

	if (!mcast_skb)
		return -1;

	data = genlmsg_put(mcast_skb, 0, 0, &nrc_nl_fam, 0, NL_CMD_RECOVERY);
	nla_put_string(mcast_skb, NL_CMD_RECOVERY_MSG, "recovery");
	genlmsg_end(mcast_skb, data);

#ifdef CONFIG_SUPPORT_GENLMSG_DEFAULT
	genlmsg_multicast(&nrc_nl_fam,
			mcast_skb,
			0,
			NL_MCGRP_NRC_LOG,
			GFP_KERNEL);
#else
	genlmsg_multicast(mcast_skb,
			0,
			NL_MCGRP_NRC_LOG,
			GFP_KERNEL);
#endif

	return 0;
}

static int capi_sta_reply(int id, struct genl_info *info, const char *response)
{
	struct sk_buff *msg;
	void *hdr;

#ifdef CONFIG_SUPPORT_GENLMSG_DEFAULT
	msg = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
	hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq, &nrc_nl_fam,
			0 /*no flags*/, id);
#else
	msg = genlmsg_new(NLMSG_DEFAULT_SIZE - GENL_HDRLEN, GFP_KERNEL);
	hdr = genlmsg_put(msg, info->snd_pid, info->snd_seq, &nrc_nl_fam,
			0 /*no flags*/, id);
#endif
	nla_put_string(msg, NL_WFA_CAPI_PARAM_RESPONSE, response);
	genlmsg_end(msg, hdr);

	return genlmsg_reply(msg, info);
}

static int capi_sta_reply_ok(int id, struct genl_info *info)
{
	return capi_sta_reply(id, info, NL_WFA_CAPI_RESP_OK);
}

static int capi_sta_reply_fail(int id, struct genl_info *info)
{
	return capi_sta_reply(id, info, NL_WFA_CAPI_RESP_ERR);
}

static int halow_reply(int id, struct genl_info *info, const char *response)
{
	struct sk_buff *msg;
	void *hdr;

#ifdef CONFIG_SUPPORT_GENLMSG_DEFAULT
	msg = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
	hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq, &nrc_nl_fam,
			0 /*no flags*/, id);
#else
	msg = genlmsg_new(NLMSG_DEFAULT_SIZE - GENL_HDRLEN, GFP_KERNEL);
	hdr = genlmsg_put(msg, info->snd_pid, info->snd_seq, &nrc_nl_fam,
			0 /*no flags*/, id);
#endif

	nla_put_string(msg, NL_HALOW_RESPONSE, response);
	genlmsg_end(msg, hdr);

	return genlmsg_reply(msg, info);
}


/* capi_sta_get_info - return vendor specific information
 *
 * Wi-Fi Test Suite Control API Spec 7.17
 *
 * Return: vendor specfic information (can be multiple)
 */
static int capi_sta_get_info(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *msg;
	void *hdr;
	int rc = 0;

	nrc_dbg(NRC_DBG_CAPI, "%s()", __func__);

#ifdef CONFIG_SUPPORT_GENLMSG_DEFAULT
	msg = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);

	hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq, &nrc_nl_fam,
			0, NL_WFA_CAPI_STA_GET_INFO);
#else
	msg = genlmsg_new(NLMSG_DEFAULT_SIZE - GENL_HDRLEN, GFP_KERNEL);

	hdr = genlmsg_put(msg, info->snd_pid, info->snd_seq, &nrc_nl_fam,
			0, NL_WFA_CAPI_STA_GET_INFO);
#endif

	if (hdr == NULL) {
		nrc_dbg(NRC_DBG_CAPI, "Failed to generate resp. nlmsg");
		return -EMSGSIZE;
	}

	nla_put_string(msg, NL_WFA_CAPI_PARAM_VENDOR1, "vendor_test1");
	nla_put_string(msg, NL_WFA_CAPI_PARAM_VENDOR2, "vendor_test2");
	nla_put_string(msg, NL_WFA_CAPI_PARAM_VENDOR3, "vendor_test3");

	genlmsg_end(msg, hdr);

	rc = genlmsg_reply(msg, info);

	return rc;
}

static int halow_set_dut(struct sk_buff *skb, struct genl_info *info)
{
	uint8_t param_name[MAX_HALOW_SIZE] = {0,};
	uint8_t str_value[MAX_HALOW_SIZE] = {0,};
#if KERNEL_VERSION(5, 11, 0) <= NRC_TARGET_KERNEL_VERSION
	nla_strscpy(param_name, info->attrs[NL_HALOW_PARAM_NAME],
			nla_len(info->attrs[NL_HALOW_PARAM_NAME]));

	nla_strscpy(str_value, info->attrs[NL_HALOW_PARAM_STR_VAL],
			nla_len(info->attrs[NL_HALOW_PARAM_STR_VAL]));
#else
	nla_strlcpy(param_name, info->attrs[NL_HALOW_PARAM_NAME],
			nla_len(info->attrs[NL_HALOW_PARAM_NAME]));

	nla_strlcpy(str_value, info->attrs[NL_HALOW_PARAM_STR_VAL],
			nla_len(info->attrs[NL_HALOW_PARAM_STR_VAL]));
#endif

	nrc_dbg(NRC_DBG_CAPI, "%s(name:\"%s\",val:\"%s\")", __func__,
			param_name, str_value);

	if (!strcasecmp(param_name, "AMPDU")) {
		/* Enable or disable the AMPDU Aggregation feature
		 * param = ["Enable"|"Disable"]
		 */
		if (!strcasecmp(str_value, "Enable")) {
			nrc_nw->ampdu_supported = true;
			nrc_nw->ampdu_reject = false;
			nrc_nw->amsdu_supported = true;
		} else if (!strcasecmp(str_value, "Disable")) {
			nrc_nw->ampdu_supported = false;
			nrc_nw->ampdu_reject = true;
			nrc_nw->amsdu_supported = false;
		} else {
			nrc_dbg(NRC_DBG_CAPI, "%s Unsupported value (%s)",
					__func__, str_value);
			goto halow_not_supported;
		}
		skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SET, WIM_MAX_SIZE);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_AMSDU_SUPPORT,
				sizeof(nrc_nw->ampdu_supported),
				&nrc_nw->ampdu_supported);
		nrc_xmit_wim_request(nrc_nw, skb);
	} else if (!strcasecmp(param_name, "AMSDU")) {
		/* Enable or disable the AMSDU Aggregation feature
		 * param = ["Enable"|"Disable"]
		 */
		if (!strcasecmp(str_value, "enable"))
			nrc_nw->amsdu_supported = false;
		else if (!strcasecmp(str_value, "disable"))
			nrc_nw->amsdu_supported = true;
		else {
			nrc_dbg(NRC_DBG_CAPI, "%s Unsupported value (%s)",
					__func__, str_value);
			goto halow_not_supported;
		}
		skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SET, WIM_MAX_SIZE);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_AMSDU_SUPPORT,
				sizeof(nrc_nw->amsdu_supported),
				&nrc_nw->amsdu_supported);
		nrc_xmit_wim_request(nrc_nw, skb);
	} else if (!strcasecmp(param_name, "Enable_SGI")) {
		/* param = ["1M"|"2M"|"4M"|"8M"|"16M"|"All"] */
		u32 sgi = BIT(WIM_BW_1M) | BIT(WIM_BW_2M) | BIT(WIM_BW_4M);

		if (strcasecmp(str_value, "All") != 0)
			goto halow_not_supported;

		skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SET, WIM_MAX_SIZE);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_SGI, sizeof(sgi), &sgi);
		nrc_xmit_wim_request(nrc_nw, skb);
	} else if (!strcasecmp(param_name, "1mhz_ctrl_resp")) {
		u32 val = 0;

		if (!strcasecmp(str_value, "Enable"))
			val = 1;
		else if (!strcasecmp(str_value, "Disable"))
			val = 0;
		else
			goto halow_not_supported;

		skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SET, WIM_MAX_SIZE);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_1MHZ_CTRL_RSP, sizeof(val),
				&val);
		nrc_xmit_wim_request(nrc_nw, skb);

	} else if (!strcasecmp(param_name, "Color_Indication")) {
		long lcolor  = 0;
		u32 color = 0;

		if (kstrtol(str_value, 10, &lcolor) < 0)
			goto halow_not_supported;
		color = lcolor;
		skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SET, WIM_MAX_SIZE);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_COLOR_IND, sizeof(color),
				&color);
		nrc_xmit_wim_request(nrc_nw, skb);

	} else if (!strcasecmp(param_name, "Tim_Mode")) {
		u32 val = 0;

		if (!strcasecmp(str_value, "OLB"))
			val = WIM_TIM_OLB;
		else if (!strcasecmp(str_value, "Single_AID"))
			val = WIM_TIM_SINGLE_AID;
		else if (!strcasecmp(str_value, "BlockBitmap"))
			val = WIM_TIM_BLOCK_BMP;
		else
			goto halow_not_supported;

		skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SET, WIM_MAX_SIZE);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_S1G_TIM_MODE, sizeof(val),
				&val);
		nrc_xmit_wim_request(nrc_nw, skb);

	} else if (!strcasecmp(param_name, "Disable_SGI")) {
		u32 sgi = 0;

		if (strcasecmp(str_value, "All") != 0)
			goto halow_not_supported;

		skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SET, WIM_MAX_SIZE);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_SGI, sizeof(sgi), &sgi);
		nrc_xmit_wim_request(nrc_nw, skb);
	} else if (!strcasecmp(param_name, "Block_Frame")) {
		if (!strcasecmp(str_value, "Enable"))
			nrc_nw->block_frame = true;
		else if (!strcasecmp(str_value, "Disable"))
			nrc_nw->block_frame = false;
		else
			goto halow_not_supported;
	} else if (!strcasecmp(param_name, "Keep_Alive")) {
		if (!strcasecmp(str_value, "data")) {
			int band;
			struct sk_buff *b;
			struct ieee80211_vif *vif = nrc_nw->vif[0];
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
			struct ieee80211_chanctx_conf *chanctx_conf;
#else
			struct ieee80211_conf *chanctx_conf;
#endif
			struct ieee80211_tx_control control = {
				.sta = ieee80211_find_sta(vif,
						vif->bss_conf.bssid)
			};
#if KERNEL_VERSION(4, 14, 17) <= NRC_TARGET_KERNEL_VERSION
			b = ieee80211_nullfunc_get(nrc_nw->hw, vif, false);
#else
			b = ieee80211_nullfunc_get(nrc_nw->hw, vif);
#endif
			skb_set_queue_mapping(b, IEEE80211_AC_VO);
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
			chanctx_conf = rcu_dereference(vif->chanctx_conf);
			band = chanctx_conf->def.chan->band;

			if (!ieee80211_tx_prepare_skb(nrc_nw->hw,
				   vif, b, band, NULL))
				goto halow_not_supported;
#else
			chanctx_conf = &nrc_nw->hw->conf;
			band = chanctx_conf->channel->band;
#endif
			nrc_xmit_frame(nrc_nw, hw_vifindex(vif), (!!control.sta ? control.sta->aid : 0), b);
		} else
			goto halow_not_supported;

	} else if (!strcasecmp(param_name, "STA_Type")) {
		/* SGI20 Enable or disable the Short Guard Interval feature
		 * param = ["Sensor"|"Non-sensor"|"Both"]
		 */
		u32 sta = 0;
		bool pv1_enable = false;

		if (!strcasecmp(str_value, "Sensor")) {
			sta = WIM_S1G_STA_TYPE_SENSOR;
			pv1_enable = true;
		} else if (!strcasecmp(str_value, "Non_Sensor")) {
			sta = WIM_S1G_STA_TYPE_NON_SENSOR;
		} else if (!strcasecmp(str_value, "Both")) {
			sta = WIM_S1G_STA_TYPE_BOTH;
			pv1_enable = true;
		} else
			goto halow_not_supported;

		skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SET, WIM_MAX_SIZE);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_S1G_STA_TYPE, sizeof(sta),
				&sta);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_S1G_PV1, sizeof(pv1_enable),
				&pv1_enable);
		nrc_xmit_wim_request(nrc_nw, skb);
	} else if (!strcasecmp(param_name, "Operating_Channel")) {
		goto halow_not_supported;
	} else if (!strcasecmp(param_name, "Primary_Channel")) {
		goto halow_not_supported;
	} else if (!strcasecmp(param_name, "Bandwidth")) {
		goto halow_not_supported;
	} else if (!strcasecmp(param_name, "1Mhz_Duplicated_PPDU")) {
		goto halow_not_supported;
	} else if (!strcasecmp(param_name, "ACK_Policy")) {
		goto halow_not_supported;
	} else
		goto halow_not_supported;

	return halow_reply(NL_HALOW_SET_DUT, info, NL_HALOW_RESP_OK);

halow_not_supported:
	return halow_reply(NL_HALOW_SET_DUT, info, NL_HALOW_RESP_NOT_SUPP);
}


/* capi_sta_set_11n - set 11n STA settings
 *
 * This function must not be called during RIFS testing.
 *
 * According to the spec, CA should send "ERROR" response back
 * when the request command is not supported,
 * otherwise CA does not need to response
 *
 * Wi-Fi Test Suite Control API Spec 7.47
 *
 * Return: "COMPLETE" or "ERROR"
 *
 */
static int capi_sta_set_11n(struct sk_buff *skb, struct genl_info *info)
{
	uint8_t param_name[MAX_CAPIREQ_SIZE] = {0,};
	uint8_t str_value[MAX_CAPIREQ_SIZE] = {0,};
	uint16_t u8_value = 0;

#if KERNEL_VERSION(5, 11, 0) <= NRC_TARGET_KERNEL_VERSION
	nla_strscpy(param_name, info->attrs[NL_WFA_CAPI_PARAM_NAME],
			nla_len(info->attrs[NL_WFA_CAPI_PARAM_NAME]));

	nla_strscpy(str_value, info->attrs[NL_WFA_CAPI_PARAM_STR_VAL],
			nla_len(info->attrs[NL_WFA_CAPI_PARAM_STR_VAL]));
#else
	nla_strlcpy(param_name, info->attrs[NL_WFA_CAPI_PARAM_NAME],
			nla_len(info->attrs[NL_WFA_CAPI_PARAM_NAME]));

	nla_strlcpy(str_value, info->attrs[NL_WFA_CAPI_PARAM_STR_VAL],
			nla_len(info->attrs[NL_WFA_CAPI_PARAM_STR_VAL]));
#endif

	nrc_dbg(NRC_DBG_CAPI, "%s(name:\"%s\",val:\"%s\")", __func__,
			param_name, str_value);

	if (!strcasecmp(param_name, "40_Intolerant")) {
		/* Enable or disable the 40 MHz Intolerant feature
		 * param = ["Enable"|"Disable"]
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %s)", __func__, param_name,
				str_value);
		goto wfa_not_supported;
	} else if (!strcasecmp(param_name, "ADDBA_Reject")) {
		/* Enable or disable the rejecting any ADDBA request by sending
		 * ADDBA response with the status decline
		 * param = ["Enable"|"Disable"]
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %s)", __func__, param_name,
				str_value);

		if (!strcasecmp(str_value, "enable")) {
			nrc_nw->ampdu_supported = false;

		} else if (!strcasecmp(str_value, "disable")) {
			nrc_nw->ampdu_supported = true;
		} else {
			nrc_dbg(NRC_DBG_CAPI, "%s Unsupported value (%s)",
					__func__, str_value);
		}

	} else if (!strcasecmp(param_name, "AMPDU")) {
		/* Enable or disable the AMPDU Aggregation feature
		 * param = ["Enable"|"Disable"]
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %s)", __func__, param_name,
				str_value);
		if (!strcasecmp(str_value, "enable")) {
			nrc_nw->ampdu_supported = false;

		} else if (!strcasecmp(str_value, "disable")) {
			nrc_nw->ampdu_supported = true;
		} else {
			nrc_dbg(NRC_DBG_CAPI, "%s Unsupported value (%s)",
					__func__, str_value);
		}

	} else if (!strcasecmp(param_name, "AMSDU")) {
		/* Enable or disable the AMSDU Aggregation feature
		 * param = ["Enable"|"Disable"]
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %s)", __func__, param_name,
				str_value);

		if (!strcasecmp(str_value, "enable")) {
			nrc_nw->amsdu_supported = false;

		} else if (!strcasecmp(str_value, "disable")) {
			nrc_nw->amsdu_supported = true;
		} else {
			nrc_dbg(NRC_DBG_CAPI, "%s Unsupported value (%s)",
					__func__, str_value);
		}

	} else if (!strcasecmp(param_name, "Greenfield")) {
		/* Enable or disable the HT Greenfield feature
		 * param = ["Enable"|"Disable"]
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %s)", __func__, param_name,
				str_value);
		goto wfa_not_supported;
	} else if (!strcasecmp(param_name, "MCS_FixedRate")) {
		/* Fixed MCS rate
		 * param = [0...31]
		 */
		long lmcs = 0;
		u32 mcs = 0;
		int ret;

		if (kstrtol(str_value, 10, &lmcs) < 0)
			goto wfa_not_supported;
		if (lmcs < 0 || lmcs > 32)
			goto wfa_not_supported;
		mcs = lmcs;
		if (!nrc_nw)
			goto wfa_not_supported;
		skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SET, WIM_MAX_SIZE);
		nrc_wim_skb_add_tlv(skb, WIM_TLV_MCS, sizeof(mcs), &mcs);
		ret = nrc_xmit_wim_request(nrc_nw, skb);

		if (ret < 0) {
			nrc_dbg(NRC_DBG_CAPI, "%s: Failed to send WIM_CMD (MCS) ",
				__func__);
			goto wfa_not_supported;
		}

	} else if (!strcasecmp(param_name, "MCS32")) {
		/* Enable or disable HT Duplicate Mode
		 * param = ["Enable"|"Disable"]
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %s)", __func__, param_name,
				str_value);
		goto wfa_not_supported;
	} else if (!strcasecmp(param_name, "Reset_Default")) {
		/* Reset the station to program's default configuration
		 * param = "11n"
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %s)", __func__, param_name,
				str_value);
	} else if (!strcasecmp(param_name, "RIFS")) {
		/* Enable/disable the RIFS feature
		 * param = ["Enable"|"Disable"]
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %s)", __func__, param_name,
				str_value);
	} else if (!strcasecmp(param_name, "RXSP_Stream")) {
		/* Rx spatial streams
		 * param = ["1SS"|"2SS"|"3SS"]
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %s)", __func__, param_name,
				str_value);

	} else if (!strcasecmp(param_name, "SGI20")) {
		/* SGI20 Enable or disable the Short Guard Interval feature
		 * param = ["Enable"|"Disable"]
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %s)", __func__, param_name,
				str_value);
	} else if (!strcasecmp(param_name, "SMPS")) {
		/* SM Power Save Mode
		 * param = [0...2]
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %d)", __func__, param_name,
				u8_value);
		goto wfa_not_supported;
	} else if (!strcasecmp(param_name, "STBC_RX")) {
		/* STBC receive streams
		 * param = [0...4]
		 */
		long lstbc;

		if (kstrtol(str_value, 10, &lstbc) < 0)
			goto wfa_not_supported;

		if (lstbc < 0 || lstbc > 4)
			goto wfa_not_supported;

		if (!nrc_set_stbc_rx(nrc_nw, (u8)lstbc))
			goto wfa_not_supported;
	} else if (!strcasecmp(param_name, "STBC_TX")) {
		/* STBC transmit streams
		 * param = [0...4]
		 */
		u8 stbc;

		if (!info->attrs[NL_WFA_CAPI_PARAM_STBC])
			return -EINVAL;
		stbc = nla_get_u8(info->attrs[NL_WFA_CAPI_PARAM_STBC]);

		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %d)", __func__, param_name,
				stbc);
		goto wfa_not_supported;
	} else if (!strcasecmp(param_name, "TXSP_Stream")) {
		/* Tx spatial stream
		 * param = ["1SS"|"2SS"|"3SS"]
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %s)", __func__, param_name,
				str_value);
	} else if (!strcasecmp(param_name, "Width")) {
		/* 802.11n channel width
		 * param = ["20"|"40"|"Auto"]
		 */
		nrc_dbg(NRC_DBG_CAPI, "%s(%s, %s)", __func__, param_name,
				str_value);
	} else {
		return capi_sta_reply_fail(NL_WFA_CAPI_STA_SET_11N, info);
	}
	return capi_sta_reply_ok(NL_WFA_CAPI_STA_SET_11N, info);

wfa_not_supported:
		return capi_sta_reply_fail(NL_WFA_CAPI_STA_SET_11N, info);
}

/**
 * Design: find the station vif, and sta corresponding to the
 * serving AP. If there are more than one active station interface,
 * we simply find the first one.
 */
struct capi_data {
	struct ieee80211_vif *vif;
	struct ieee80211_sta *sta;
	u8 tid;
	u8 *addr;
	bool done;
};

static void capi_send_addba(void *data, u8 *mac, struct ieee80211_vif *vif)
{
	struct capi_data *c = data;
	struct ieee80211_sta *sta;
	u64 now = 0, diff = 0;

	now = ktime_to_us(ktime_get_real());

	rcu_read_lock();
	if (vif->type == NL80211_IFTYPE_STATION) {
		if (!vif->bss_conf.assoc)
			goto out;

		if (c->addr && !ether_addr_equal(c->addr, vif->bss_conf.bssid))
			goto out;

		sta = ieee80211_find_sta(vif, vif->bss_conf.bssid);
	} else {
		if (!c->addr)
			goto out;
		sta = ieee80211_find_sta(vif, c->addr);
	}

	if (!sta)
		goto out;

	ieee80211_start_tx_ba_session(sta, c->tid, 0);
	c->done = true;

 out:
	rcu_read_unlock();
	diff = ktime_to_us(ktime_get_real()) - now;
	if ((!diff) || (diff > NRC_MAC80211_RCU_LOCK_THRESHOLD))
		nrc_mac_dbg("%s, diff=%lu", __func__, (unsigned long)diff);
}

/* capi_sta_send_addba - Send ADDBA Request to associated AP
 *
 * Wi-Fi Test Suite Control API Spec 7.47
 *
 * @Interface : Interface ID
 * @TID : Traffic ID
 * Return: "None"
 */
static int capi_sta_send_addba(struct sk_buff *skb, struct genl_info *info)
{
	struct capi_data param = { 0 };

	nrc_nw->ampdu_supported = true;

	if (!info->attrs[NL_WFA_CAPI_PARAM_TID])
		return capi_sta_reply(NL_WFA_CAPI_SEND_ADDBA, info,
				NL_WFA_CAPI_RESP_ERR);

	param.tid  = nla_get_u8(info->attrs[NL_WFA_CAPI_PARAM_TID]);
	if (info->attrs[NL_WFA_CAPI_PARAM_DESTADDR])
		param.addr = nla_data(info->attrs[NL_WFA_CAPI_PARAM_DESTADDR]);

	nrc_dbg(NRC_DBG_CAPI, "%s(%d,%pM)", __func__, param.tid, param.addr);

#ifdef CONFIG_SUPPORT_ITERATE_INTERFACE
	ieee80211_iterate_active_interfaces(nrc_nw->hw, 0, capi_send_addba,
					    &param);
#else
	ieee80211_iterate_active_interfaces(nrc_nw->hw, capi_send_addba,
					    &param);
#endif
	if (!param.done)
		pr_err("WFA_CAPI: failed to send ADDBA");

	return capi_sta_reply(NL_WFA_CAPI_SEND_ADDBA, info,
			      param.done ?
			      NL_WFA_CAPI_RESP_OK : NL_WFA_CAPI_RESP_ERR);
}

static void capi_send_delba(void *data, u8 *mac, struct ieee80211_vif *vif)
{
	struct capi_data *c = data;
	struct ieee80211_sta *sta;
	u64 now = 0, diff = 0;

	now = ktime_to_us(ktime_get_real());

	rcu_read_lock();
	if (vif->type == NL80211_IFTYPE_STATION) {
		if (!vif->bss_conf.assoc)
			goto out;

		if (c->addr && !ether_addr_equal(c->addr, vif->bss_conf.bssid))
			goto out;

		sta = ieee80211_find_sta(vif, vif->bss_conf.bssid);
	} else {
		if (!c->addr)
			goto out;
		sta = ieee80211_find_sta(vif, c->addr);
	}

	if (!sta)
		goto out;

	ieee80211_stop_tx_ba_session(sta, c->tid);
	c->done = true;

 out:
	rcu_read_unlock();
	diff = ktime_to_us(ktime_get_real()) - now;
	if ((!diff) || (diff > NRC_MAC80211_RCU_LOCK_THRESHOLD))
		nrc_mac_dbg("%s, diff=%lu", __func__, (unsigned long)diff);
}

/* capi_sta_send_delba - Send DELBA Request to associated AP
 */
static int capi_sta_send_delba(struct sk_buff *skb, struct genl_info *info)
{
	struct capi_data param = { 0 };

	nrc_nw->ampdu_supported = true;

	if (!info->attrs[NL_WFA_CAPI_PARAM_TID])
		return capi_sta_reply(NL_WFA_CAPI_SEND_DELBA, info,
				NL_WFA_CAPI_RESP_ERR);

	param.tid  = nla_get_u8(info->attrs[NL_WFA_CAPI_PARAM_TID]);
	if (info->attrs[NL_WFA_CAPI_PARAM_DESTADDR])
		param.addr = nla_data(info->attrs[NL_WFA_CAPI_PARAM_DESTADDR]);

	nrc_dbg(NRC_DBG_CAPI, "%s(%d,%pM)", __func__, param.tid, param.addr);

#ifdef CONFIG_SUPPORT_ITERATE_INTERFACE
	ieee80211_iterate_active_interfaces(nrc_nw->hw, 0, capi_send_delba,
					    &param);
#else
	ieee80211_iterate_active_interfaces(nrc_nw->hw, capi_send_delba,
					    &param);
#endif
	if (!param.done)
		pr_err("WFA_CAPI: failed to send DELBA");

	return capi_sta_reply(NL_WFA_CAPI_SEND_DELBA, info,
			      param.done ?
			      NL_WFA_CAPI_RESP_OK : NL_WFA_CAPI_RESP_ERR);
}

/**************************************************************************//**
 * FunctionName	: capi_bss_max_idle_offset
 * Description	: Set the offset of max bss idle timer through netlink.
 *				  Use below parameter passed via Netlink
 *				  NL_WFA_CAPI_PARAM_BSS_MAX_IDLE_OFFSET: S32
 * Parameters	: skb(struct sk_buff*)
 *				: info(struct genl_info*)
 * Returns		: int(capi_sta_reply)
 */
static int capi_bss_max_idle_offset(struct sk_buff *skb, struct genl_info *info)
{
	int32_t bss_max_idle_offset;

	if (!info->attrs[NL_WFA_CAPI_PARAM_BSS_MAX_IDLE_OFFSET])
		return capi_sta_reply(NL_WFA_CAPI_BSS_MAX_IDLE_OFFSET, info,
				NL_WFA_CAPI_RESP_ERR);

	bss_max_idle_offset = nla_get_s32(
		   info->attrs[NL_WFA_CAPI_PARAM_BSS_MAX_IDLE_OFFSET]);
	nrc_set_bss_max_idle_offset(bss_max_idle_offset);

	nrc_dbg(NRC_DBG_CAPI, "%s(%d)", __func__, bss_max_idle_offset);

	return capi_sta_reply(NL_WFA_CAPI_BSS_MAX_IDLE_OFFSET, info,
			NL_WFA_CAPI_RESP_NONE);
}

static int s1g_unscaled_interval_max = 0x3fff;
static int convert_usf(int interval)
{
	int ui, usf = 0, interval_usf;

	if (interval <= s1g_unscaled_interval_max) {
		ui = interval;
		usf = 0;
	} else if (interval / 10 <= s1g_unscaled_interval_max) {
		ui= interval / 10;
		usf = 1;
	} else if (interval / 1000 <= s1g_unscaled_interval_max) {
		ui = interval / 1000;
		usf = 2;
        } else if (interval / 10000 <= s1g_unscaled_interval_max) {
		ui = interval / 10000;
		usf = 3;
	} else {
		ui = 0;
		usf = 0;
        }

	interval_usf = (usf << 14) + ui;

	return interval_usf;
}

/*
 * FunctionName	: capi_bss_max_idle
 * Description	: Set the max bss idle through netlink.
 *					Use below parameter passed via Netlink
 *					NL_WFA_CAPI_PARAM_BSS_MAX_IDLE: S32
 *					NL_WFA_CAPI_PARAM_VIF_ID: S32
 *					NL_WFA_CAPI_PARAM_BSS_MAX_IDLE_OFFSET: S32 //AUTO_USF Setting
 * Parameters	: skb(struct sk_buff*)
 *				: info(struct genl_info*)
 * Returns		: int(capi_sta_reply)
 */
static int capi_bss_max_idle(struct sk_buff *skb, struct genl_info *info)
{
	int32_t max_idle, vif_id, no_usf_auto_convert;
	struct ieee80211_vif *vif;

	if ((!info->attrs[NL_WFA_CAPI_PARAM_BSS_MAX_IDLE]) ||
		(!info->attrs[NL_WFA_CAPI_PARAM_VIF_ID])) {
		return capi_sta_reply(NL_WFA_CAPI_BSS_MAX_IDLE, info,
				NL_WFA_CAPI_RESP_ERR);
	}

	max_idle = nla_get_s32(info->attrs[NL_WFA_CAPI_PARAM_BSS_MAX_IDLE]);
	vif_id = nla_get_s32(info->attrs[NL_WFA_CAPI_PARAM_VIF_ID]);
	no_usf_auto_convert = nla_get_s32(info->attrs[NL_WFA_CAPI_PARAM_BSS_MAX_IDLE_OFFSET]);

	vif = nrc_nw->vif[vif_id];

	nrc_dbg(NRC_DBG_CAPI, "%s (no_usf_auto_convert:%d) (max_idle:%d)",
		__func__, no_usf_auto_convert, max_idle);

	if (vif) {
		struct nrc_vif *i_vif;
		i_vif = to_i_vif(vif);
		if (nrc_mac_is_s1g(nrc_nw->hw->priv) && max_idle) {
			/* bss_max_idle: in unit of 1000 TUs (1024ms = 1.024 seconds) */
			if (max_idle > 16383 * 10000 || max_idle <= 0) {
				i_vif->max_idle_period = 0;
			} else {
				/* (Default) Convert in USF Format (Value (14bit) * USF(2bit)) and save it */
				if (no_usf_auto_convert) {
					i_vif->max_idle_period = max_idle;
				} else {
					i_vif->max_idle_period = convert_usf(max_idle);
				}
			}
		} else {
			if (max_idle > 65535 || max_idle <= 0) {
				i_vif->max_idle_period = 0;
			} else {
				i_vif->max_idle_period = max_idle;
			}
		}
		nrc_dbg(NRC_DBG_CAPI, "%s max_idle(%d) vs converted_max_idle(%d)", __func__, max_idle, i_vif->max_idle_period);
	}

	return capi_sta_reply(NL_WFA_CAPI_BSS_MAX_IDLE, info,
			vif != NULL ?
			NL_WFA_CAPI_RESP_NONE : NL_WFA_CAPI_RESP_ERR);
}

static void generate_mmic_error(void *data, u8 *mac, struct ieee80211_vif *vif)
{
	struct capi_data *c = data;
	struct ieee80211_sta *sta;
	struct sk_buff *skb;
	struct ieee80211_rx_status *rx_status;
	struct ieee80211_hdr *hdr;
#ifdef CONFIG_SUPPORT_CHANNEL_INFO
	struct ieee80211_chanctx_conf *chan;
#else
	struct ieee80211_conf *chan;
#endif
	u64 now = 0, diff = 0;

	if (vif->type != NL80211_IFTYPE_STATION || !vif->bss_conf.assoc)
		return;

	if (c->addr && !ether_addr_equal(c->addr, vif->bss_conf.bssid))
		return;

	now = ktime_to_us(ktime_get_real());

	rcu_read_lock();
	sta = ieee80211_find_sta(vif, vif->bss_conf.bssid);
	if (!sta)
		goto out;

	skb = dev_alloc_skb(1000);

	hdr = (struct ieee80211_hdr *)skb_put(skb, sizeof(*hdr));
	skb_put(skb, 500);

	hdr->frame_control = cpu_to_le16(IEEE80211_FTYPE_DATA |
					 IEEE80211_STYPE_DATA |
					 IEEE80211_FCTL_FROMDS);

	ether_addr_copy(hdr->addr1, mac);
	ether_addr_copy(hdr->addr2, vif->bss_conf.bssid);
	ether_addr_copy(hdr->addr3, vif->bss_conf.bssid);

	rx_status = IEEE80211_SKB_RXCB(skb);

#ifdef CONFIG_SUPPORT_CHANNEL_INFO
	chan = rcu_dereference(vif->chanctx_conf);
	if (!chan)
		goto out;

	rx_status->freq = chan->def.chan->center_freq;
	rx_status->band = chan->def.chan->band;
#else
	chan = &nrc_nw->hw->conf;
	if (!chan)
		goto out;

	rx_status->freq = chan->channel->center_freq;
	rx_status->band = chan->channel->band;
#endif
	rx_status->flag = (RX_FLAG_DECRYPTED | RX_FLAG_MMIC_STRIPPED |
			   RX_FLAG_MMIC_ERROR);

	ieee80211_rx_irqsafe(nrc_nw->hw, skb);

	c->done = true;

 out:
	rcu_read_unlock();
	diff = ktime_to_us(ktime_get_real()) - now;
	if ((!diff) || (diff > NRC_MAC80211_RCU_LOCK_THRESHOLD))
		nrc_mac_dbg("%s, diff=%lu", __func__, (unsigned long)diff);
}

/* test_mmic_failure - Trigger MMIC failture event to cfg80211
 *
 * @ TID : Traffic ID, "0" will be used if TID is omitted.
 * @ SA	: Source Address,  BSSID will be used if SA is omitted.
 * Return: "Ok" if success, "Fail" if failed
 */
static int test_mmic_failure(struct sk_buff *skb, struct genl_info *info)
{
	struct capi_data param = { 0 };

	if (info->attrs[NL_WFA_CAPI_PARAM_TID])
		param.tid  = nla_get_u8(info->attrs[NL_WFA_CAPI_PARAM_TID]);

	if (info->attrs[NL_WFA_CAPI_PARAM_DESTADDR])
		param.addr = nla_data(info->attrs[NL_WFA_CAPI_PARAM_DESTADDR]);

	nrc_dbg(NRC_DBG_CAPI, "%s(%d,%pM)", __func__, param.tid, param.addr);

#ifdef CONFIG_SUPPORT_ITERATE_INTERFACE
	ieee80211_iterate_active_interfaces(nrc_nw->hw, 0, generate_mmic_error,
					    &param);
#else
	ieee80211_iterate_active_interfaces(nrc_nw->hw, generate_mmic_error,
					    &param);
#endif

	if (!param.done)
		pr_err("WFA_CAPI: failed to generate MMIC failure");

	return capi_sta_reply(NL_TEST_MMIC_FAILURE, info,
			      param.done ?
			      NL_WFA_CAPI_RESP_NONE : NL_WFA_CAPI_RESP_ERR);
}

static int nrc_inject_mgmt_frame(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *skb_mgmt = NULL;
	struct ieee80211_hdr_3addr *hdr = NULL;
	const int tailroom = 100;
	u8 injector[ETH_ALEN] = {0x2, 0x0, 0x1, 0x1, 0x1, 0x1};
	/*u8 *pos;*/
	u8 stype = 0;

	if (info->attrs[NL_MGMT_FRAME_INJECTION_STYPE])
		stype = nla_get_u8(info->attrs[NL_MGMT_FRAME_INJECTION_STYPE]);
	else
		goto fail_over;

	skb_mgmt = dev_alloc_skb(nrc_nw->hw->extra_tx_headroom + sizeof(*hdr) +
			+ tailroom);
	skb_reserve(skb_mgmt, nrc_nw->hw->extra_tx_headroom);

	hdr = (struct ieee80211_hdr_3addr *) skb_put(skb_mgmt, sizeof(*hdr));
	hdr->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT | (stype << 4));
	/* TO DO - consider multiple vifs */
	ether_addr_copy(hdr->addr1, nrc_nw->mac_addr[0].addr);
	ether_addr_copy(hdr->addr2, injector);
	/* TO DO - consider multiple vifs */
	ether_addr_copy(hdr->addr3, nrc_nw->mac_addr[0].addr);

	switch (stype << 4) {
	case IEEE80211_STYPE_AUTH:
		/* Authentication Algorithm */
		put_unaligned_le16(0, skb_put(skb_mgmt, 2));
		/* Authentication SEQ */
		put_unaligned_le16(1, skb_put(skb_mgmt, 2));
		/* Status code */
		put_unaligned_le16(0, skb_put(skb_mgmt, 2));
		break;
	case IEEE80211_STYPE_ASSOC_REQ:
		/* Capabilities Information */
		put_unaligned_le16(0x0421, skb_put(skb_mgmt, 2));
		/* Listen Interval */
		put_unaligned_le16(10, skb_put(skb_mgmt, 2));
		break;
	default:
		goto fail_over;
	}
	ieee80211_rx_irqsafe(nrc_nw->hw, skb_mgmt);
	return 0;

fail_over:
	if (skb_mgmt)
		dev_kfree_skb(skb_mgmt);

	return 0;
}

static int nrc_shell_run_simple(struct sk_buff *skb, struct genl_info *info)
{
	char *cmd = NULL;
	struct sk_buff *wim_skb;

	if (!nrc_access_vif(nrc_nw)) {
		nrc_dbg(NRC_DBG_CAPI, "%s Can't send command", __func__);
		return -EIO;
	}

	if (info->attrs[NL_SHELL_RUN_CMD])
		cmd = nla_data(info->attrs[NL_SHELL_RUN_CMD]);

	if (!cmd)
		return -EINVAL;

	wim_skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SHELL, WIM_MAX_SIZE);

	if (!wim_skb)
		return -EINVAL;

	nrc_wim_skb_add_tlv(wim_skb, WIM_TLV_SHELL_CMD, strlen(cmd)+1, cmd);
	nrc_xmit_wim_request(nrc_nw, wim_skb);

	return 0;
}

enum nrc_shell_run_state {
	NRC_SHELL_IDLE,
	NRC_SHELL_RUNNING
};

static int nrc_shell_running_state;

static void set_shell_run_state(int state)
{
	nrc_shell_running_state = state;
}

static int get_shell_run_state(void)
{
	return nrc_shell_running_state;
}

static int nrc_shell_run(struct sk_buff *skb, struct genl_info *info)
{
	char *cmd = NULL;
	char cmd_resp[512];
	struct sk_buff *msg, *wim_skb, *wim_resp;
	void *hdr;

	if (!nrc_access_vif(nrc_nw)) {
		nrc_dbg(NRC_DBG_CAPI, "%s Can't send command", __func__);
		return -EIO;
	}

#ifdef CONFIG_SUPPORT_GENLMSG_DEFAULT
	msg = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
	hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq, &nrc_nl_fam,
			0 /*no flags*/, NL_SHELL_RUN_CMD);
#else
	msg = genlmsg_new(NLMSG_DEFAULT_SIZE - GENL_HDRLEN, GFP_KERNEL);
	hdr = genlmsg_put(msg, info->snd_pid, info->snd_seq, &nrc_nl_fam,
			0 /*no flags*/, NL_SHELL_RUN_CMD);
#endif

	if (info->attrs[NL_SHELL_RUN_CMD])
		cmd = nla_data(info->attrs[NL_SHELL_RUN_CMD]);

	if (get_shell_run_state() != NRC_SHELL_IDLE) {
		strcpy(cmd_resp, "Failed");
		nla_put_string(msg, NL_SHELL_RUN_CMD_RESP, cmd_resp);
		genlmsg_end(msg, hdr);
		return genlmsg_reply(msg, info);
	}
	set_shell_run_state(NRC_SHELL_RUNNING);

	if (!cmd)
		return -EINVAL;

	wim_skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SHELL, WIM_MAX_SIZE);

	if (!wim_skb)
		return -EINVAL;

	nrc_wim_skb_add_tlv(wim_skb, WIM_TLV_SHELL_CMD, strlen(cmd)+1, cmd);

	wim_resp = nrc_xmit_wim_request_wait(nrc_nw, wim_skb,
			(WIM_RESP_TIMEOUT * 300));

	if (wim_resp) {
		struct wim *wim = (struct wim *)wim_resp->data;

		if (wim->cmd == WIM_CMD_SHELL) {
			struct wim_tlv *tlv = (struct wim_tlv *)(wim + 1);

			memcpy(cmd_resp, &tlv->v, tlv->l);
			cmd_resp[tlv->l] = 0;
			nrc_dbg(NRC_DBG_CAPI, "%s[%s]", __func__,
					cmd_resp);
		}
		dev_kfree_skb(wim_resp);
	} else {
		strcpy(cmd_resp, "Failed");
	}

	nla_put_string(msg, NL_SHELL_RUN_CMD_RESP, cmd_resp);
	genlmsg_end(msg, hdr);

	set_shell_run_state(NRC_SHELL_IDLE);

	return genlmsg_reply(msg, info);
}

static int nrc_shell_run_raw(struct sk_buff *skb, struct genl_info *info)
{
	char *cmd = NULL;
	char cmd_resp[512];
	struct sk_buff *msg, *wim_skb, *wim_resp;
	void *hdr;

	if (!nrc_access_vif(nrc_nw)) {
		nrc_dbg(NRC_DBG_CAPI, "%s Can't send command", __func__);
		return -EIO;
	}

#ifdef CONFIG_SUPPORT_GENLMSG_DEFAULT
	msg = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
	hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq, &nrc_nl_fam,
			0 /*no flags*/, NL_SHELL_RUN_CMD_RAW);
#else
	msg = genlmsg_new(NLMSG_DEFAULT_SIZE - GENL_HDRLEN, GFP_KERNEL);
	hdr = genlmsg_put(msg, info->snd_pid, info->snd_seq, &nrc_nl_fam,
			0 /*no flags*/, NL_SHELL_RUN_CMD);
#endif
	if (info->attrs[NL_SHELL_RUN_CMD_RAW])
		cmd = nla_data(info->attrs[NL_SHELL_RUN_CMD_RAW]);

	if (get_shell_run_state() != NRC_SHELL_IDLE) {
		strcpy(cmd_resp, "Failed");
		nla_put_string(msg, NL_SHELL_RUN_CMD_RESP_RAW, cmd_resp);
		genlmsg_end(msg, hdr);
		return genlmsg_reply(msg, info);
	}
	set_shell_run_state(NRC_SHELL_RUNNING);
	if (!cmd)
		return -EINVAL;

	wim_skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SHELL_RAW, WIM_MAX_SIZE);
	if (!wim_skb)
		return -EINVAL;

	nrc_wim_skb_add_tlv(wim_skb, WIM_TLV_SHELL_CMD, strlen(cmd)+1, cmd);
	wim_resp = nrc_xmit_wim_request_wait(nrc_nw, wim_skb,
			(WIM_RESP_TIMEOUT * 300));
	if (wim_resp) {
		struct wim *wim = (struct wim *)wim_resp->data;
		if (wim->cmd == WIM_CMD_SHELL_RAW) {
			struct wim_tlv *tlv = (struct wim_tlv *)(wim + 1);
			memcpy(cmd_resp, &tlv->v, tlv->l);
			//cmd_resp[tlv->l] = 0;
			//nrc_dbg(NRC_DBG_CAPI, "%s[%s]", __func__,
			//		cmd_resp);
			nla_put(msg, NL_SHELL_RUN_CMD_RESP_RAW, tlv->l, cmd_resp);
		}
		dev_kfree_skb(wim_resp);
	} else {
		strcpy(cmd_resp, "Failed");
	}

	//nla_put_string(msg, NL_SHELL_RUN_CMD_RESP_RAW, cmd_resp);

	genlmsg_end(msg, hdr);

	set_shell_run_state(NRC_SHELL_IDLE);

	return genlmsg_reply(msg, info);
}

/* cli_app_get_info - api for cli application
 *
 * Return: rssi & snr
 */
static int cli_app_get_info(struct sk_buff *skb, struct genl_info *info)
{
	char *cmd = NULL;
	char cmd_resp[512];
	struct sk_buff *msg;
	void *hdr;
	int total_count = 0;
	int start_point = 0;
	const int max_number_per_response = 20;
	char *str = NULL;
	int i = 0;

	memset(cmd_resp, 0x0, sizeof(cmd_resp));

	if (!nrc_access_vif(nrc_nw)) {
		nrc_dbg(NRC_DBG_CAPI, "%s Can't send command", __func__);
		return -EIO;
	}

#ifdef CONFIG_SUPPORT_GENLMSG_DEFAULT
	msg = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
	hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq, &nrc_nl_fam,
			0 /*no flags*/, NL_SHELL_RUN_CMD);
#else
	msg = genlmsg_new(NLMSG_DEFAULT_SIZE - GENL_HDRLEN, GFP_KERNEL);
	hdr = genlmsg_put(msg, info->snd_pid, info->snd_seq, &nrc_nl_fam,
			0 /*no flags*/, NL_SHELL_RUN_CMD);
#endif

	if (info->attrs[NL_SHELL_RUN_CMD])
		cmd = nla_data(info->attrs[NL_SHELL_RUN_CMD]);

	if (!cmd)
		return -EINVAL;

	if (strcmp(cmd, "show signal -sr -num") == 0) {
		//start monitoring
		signal_monitor = true;
		nrc_dbg(NRC_DBG_CAPI, "%s Start Signal Monitor (%d)", __func__, signal_monitor);
		total_count = nrc_stats_report_count();
		sprintf(cmd_resp, "%d,%d", total_count,
				   max_number_per_response);
	} else if (strcmp(cmd, "show signal stop") == 0) {
		//stop monitoring
		signal_monitor = false;
		nrc_dbg(NRC_DBG_CAPI, "%s Stop Signal Monitor (%d)", __func__, signal_monitor);
		sprintf(cmd_resp, "okay");
	} else {
		str = strrchr(cmd, ' ');
		for (i = 1; str[i] != '\0'; ++i)
			start_point = start_point * 10 + str[i] - '0';

		nrc_stats_report(cmd_resp, start_point, max_number_per_response);
	}
	nla_put_string(msg, NL_SHELL_RUN_CMD_RESP, cmd_resp);
	genlmsg_end(msg, hdr);

	return genlmsg_reply(msg, info);
}

static int nrc_mic_scan(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *msg, *wim_skb, *wim_resp;
	struct wim_channel_1m_param channel;
	struct wim_channel_1m_param resp;
	void *hdr;
	int count = 0;

	channel.channel_start = nla_get_s32(
				   info->attrs[NL_MIC_SCAN_CHANNEL_START]);
	channel.channel_end = nla_get_s32(
				   info->attrs[NL_MIC_SCAN_CHANNEL_END]);

	wim_skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_MIC_SCAN,
				   sizeof(struct wim_channel_1m_param));

	if (!wim_skb)
		return -EINVAL;

	count = (channel.channel_end - channel.channel_start) / 10;
	nrc_wim_skb_add_tlv(wim_skb, WIM_TLV_CCA_1M,
				   sizeof(struct wim_channel_1m), &channel);

	wim_resp = nrc_xmit_wim_request_wait(nrc_nw, wim_skb,
				   (WIM_RESP_TIMEOUT * 10 * count));

	if (wim_resp) {
		struct wim *wim = (struct wim *)wim_resp->data;

		if (wim->cmd == WIM_CMD_MIC_SCAN) {
			struct wim_tlv *tlv = (struct wim_tlv *)(wim + 1);

			memcpy(&resp, &tlv->v, tlv->l);
		}
		dev_kfree_skb(wim_resp);
	}

	msg = genlmsg_new(NLMSG_DEFAULT_SIZE - GENL_HDRLEN, GFP_KERNEL);
	hdr = genlmsg_put(msg, info->snd_portid, info->snd_seq, &nrc_nl_fam,
			0 /*no flags*/, NL_MIC_SCAN);

	nla_put_u32(msg, NL_MIC_SCAN_CHANNEL_BITMAP, resp.cca_bitmap);
	genlmsg_end(msg, hdr);

	return genlmsg_reply(msg, info);
}

static int nrc_inject_frame(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *buffer;
	uint8_t *frame;
	int length = nla_len(info->attrs[NL_FRAME_INJECTION_BUFFER]);

	buffer = dev_alloc_skb(nrc_nw->hw->extra_tx_headroom + length);
	skb_reserve(buffer, nrc_nw->hw->extra_tx_headroom);
	frame = skb_put(buffer, length - 1);
#if KERNEL_VERSION(5, 11, 0) <= NRC_TARGET_KERNEL_VERSION
	nla_strscpy(frame, info->attrs[NL_FRAME_INJECTION_BUFFER], length);
#else
	nla_strlcpy(frame, info->attrs[NL_FRAME_INJECTION_BUFFER], length);
#endif

	nrc_xmit_injected_frame(nrc_nw, NULL, NULL, buffer);

	return 0;
}

static int nrc_set_ie(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *wim_skb;
	struct wim_set_ie_param ie;

	ie.eid = nla_get_u16(info->attrs[NL_SET_IE_EID]);
	ie.length = nla_get_u8(info->attrs[NL_SET_IE_LENGTH]);

#if KERNEL_VERSION(5, 11, 0) <= NRC_TARGET_KERNEL_VERSION
	nla_strscpy(ie.data, info->attrs[NL_SET_IE_DATA],
		nla_len(info->attrs[NL_SET_IE_DATA]));
#else
	nla_strlcpy(ie.data, info->attrs[NL_SET_IE_DATA],
		nla_len(info->attrs[NL_SET_IE_DATA]));
#endif
	wim_skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SET_IE,
		sizeof(struct wim_set_ie_param));

	if (!wim_skb)
		return -EINVAL;

	nrc_wim_skb_add_tlv(wim_skb, WIM_TLV_IE_PARAM,
		sizeof(struct wim_set_ie_param), &ie);
	nrc_xmit_wim_request(nrc_nw, wim_skb);

	return 0;
}

static int nrc_set_sae(struct sk_buff *skb, struct genl_info *info)
{
	// 10/27/2020 Shinwoo Lee
	// Annotated debug messages out, but left them for future debugging

	struct sk_buff *wim_skb;
	struct wim_set_sae_param sae;
	int i;
	// nrc_dbg(NRC_DBG_WIM, "nrc-netlink driver Log (before copying eid)\n");
	sae.eid = nla_get_u16(info->attrs[NL_SET_SAE_EID]);
	// nrc_dbg(NRC_DBG_WIM, "nrc-netlink driver Log (before copying length)\n");
	sae.length = nla_get_u16(info->attrs[NL_SET_SAE_LENGTH]);

	for (i=0; i< sae.length; i++) {
		if (i==0) nrc_dbg(NRC_DBG_WIM, "Data: %x", *(info->attrs[NL_SET_SAE_DATA]));
		else nrc_dbg(NRC_DBG_WIM, "%x", *(info->attrs[NL_SET_SAE_DATA]+i));
	}
	// nrc_dbg(NRC_DBG_WIM, "nrc-netlink driver Log (before copying data)\n");
#if KERNEL_VERSION(5, 11, 0) <= NRC_TARGET_KERNEL_VERSION
	nla_strscpy(sae.data, info->attrs[NL_SET_SAE_DATA], sae.length+1);
#else
	nla_strlcpy(sae.data, info->attrs[NL_SET_SAE_DATA], sae.length+1);
#endif
	wim_skb = nrc_wim_alloc_skb(nrc_nw, WIM_CMD_SET_SAE,
		sizeof(struct wim_set_sae_param));

	// nrc_dbg_enable(NRC_DBG_WIM);
	// nrc_dbg(NRC_DBG_WIM, "nrc-netlink driver log (after copying data)\n");
	// nrc_dbg(NRC_DBG_WIM, "----------------------\n");
	// nrc_dbg(NRC_DBG_WIM, "EID: %d\n", sae.eid);
	// nrc_dbg(NRC_DBG_WIM, "Length: %d\n", sae.length);
	// for (i=0; i<sae.length; i++) {
	// 	if (i==0) nrc_dbg(NRC_DBG_WIM, "Data: %x", *(sae.data+i));
	// 	else nrc_dbg(NRC_DBG_WIM, "%x", *(sae.data+i));
	// }
	// nrc_dbg(NRC_DBG_WIM, "\n----------------------\n");

	if (!wim_skb)
		return -EINVAL;

	// nrc_dbg(NRC_DBG_WIM, "nrc-netlink driver log (add tlv)\n");
	// nrc_dbg(NRC_DBG_WIM, "----------------------\n");
	// nrc_dbg(NRC_DBG_WIM, "size of tlv : %d\n", sizeof(struct wim_set_sae_param));

	nrc_wim_skb_add_tlv(wim_skb, WIM_TLV_SAE_PARAM, sizeof(struct wim_set_sae_param), &sae);
	nrc_xmit_wim_request(nrc_nw, wim_skb);

	return 0;
}

static int nrc_auto_ba_toggle(struct sk_buff *skb, struct genl_info *info)
{
	nrc_set_auto_ba(nla_get_u8(info->attrs[NL_AUTO_BA_ON]) ? true : false);
	return 0;
}

#ifdef CONFIG_SUPPORT_GENLMSG_DEFAULT
static const struct genl_ops nl_umac_nl_ops[] = {
#else
static struct genl_ops nl_umac_nl_ops[] = {
#endif
	{
		.cmd	= NL_WFA_CAPI_STA_GET_INFO,
		.doit	= capi_sta_get_info,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_WFA_CAPI_STA_SET_11N,
		.doit	= capi_sta_set_11n,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_WFA_CAPI_SEND_ADDBA,
		.doit	= capi_sta_send_addba,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_WFA_CAPI_SEND_DELBA,
		.doit	= capi_sta_send_delba,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_WFA_CAPI_BSS_MAX_IDLE,
		.doit	= capi_bss_max_idle,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_WFA_CAPI_BSS_MAX_IDLE_OFFSET,
		.doit	= capi_bss_max_idle_offset,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_TEST_MMIC_FAILURE,
		.doit	= test_mmic_failure,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_SHELL_RUN,
		.doit	= nrc_shell_run,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_SHELL_RUN_SIMPLE,
		.doit	= nrc_shell_run_simple,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_MGMT_FRAME_INJECTION,
		.doit	= nrc_inject_mgmt_frame,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_HALOW_SET_DUT,
		.doit	= halow_set_dut,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_CLI_APP_GET_INFO,
		.doit	= cli_app_get_info,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_MIC_SCAN,
		.doit	= nrc_mic_scan,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_FRAME_INJECTION,
		.doit	= nrc_inject_frame,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_SET_IE,
		.doit	= nrc_set_ie,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_SET_SAE_DATA,
		.doit	= nrc_set_sae,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_SHELL_RUN_RAW,
		.doit	= nrc_shell_run_raw,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
	{
		.cmd	= NL_AUTO_BA_TOGGLE,
		.doit	= nrc_auto_ba_toggle,
#if KERNEL_VERSION(5, 2, 0) > NRC_TARGET_KERNEL_VERSION
		.policy = nl_umac_policy,
#endif
	},
};


static int nl_umac_notify(struct notifier_block *nb, unsigned long state,
		void *notifier)
{
	if (state != NETLINK_URELEASE)
		return NOTIFY_DONE;
	return NOTIFY_DONE;
}

static struct notifier_block nl_umac_netlink_notifier = {
	.notifier_call = nl_umac_notify,
};


int nrc_netlink_init(struct nrc *nw)
{
	int rc = 0;
#if KERNEL_VERSION(4, 10, 0) <= NRC_TARGET_KERNEL_VERSION
	nrc_nl_fam.ops = nl_umac_nl_ops;
	nrc_nl_fam.n_ops = ARRAY_SIZE(nl_umac_nl_ops);
	rc = genl_register_family(&nrc_nl_fam);
#else
#ifdef CONFIG_SUPPORT_GENLMSG_DEFAULT
	rc = genl_register_family_with_ops_groups(&nrc_nl_fam,
						  nl_umac_nl_ops,
						  nl_umac_mcast_grps);
#else
	rc = genl_register_family_with_ops(&nrc_nl_fam,
					  nl_umac_nl_ops,
					  ARRAY_SIZE(nl_umac_nl_ops));
#endif
#endif

	if (rc) {
		pr_err("genl_register_family_with_ops_groups() is failed (%d).",
				rc);
		return -EINVAL;
	}

	rc = netlink_register_notifier(&nl_umac_netlink_notifier);

	if (rc) {
		pr_err("netlink_register_notifier() is failed (%d).",
				rc);
		genl_unregister_family(&nrc_nl_fam);
		return -EINVAL;
	}

	nrc_nw = nw;
	return 0;
}

void nrc_netlink_exit(void)
{
	pr_err("%s", __func__);
	netlink_unregister_notifier(&nl_umac_netlink_notifier);
	genl_unregister_family(&nrc_nl_fam);
}
