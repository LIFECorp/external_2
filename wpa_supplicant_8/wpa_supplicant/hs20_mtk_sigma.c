/*! \file   "Hs20_mtk_sigma.c"
*    \brief  For HS20-SIGMA
*
*    This file provided the macros and functions library support for the
*    protocol layer hotspot 2.0 related function.
*
*/

/*******************************************************************************
* Copyright (c) 2007 MediaTek Inc.
*
* All rights reserved. Copying, compilation, modification, distribution
* or any other use whatsoever of this material is strictly prohibited
* except in accordance with a Software License Agreement with
* MediaTek Inc.
********************************************************************************
*/

/*******************************************************************************
* LEGAL DISCLAIMER
*
* BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND
* AGREES THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK
* SOFTWARE") RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE
* PROVIDED TO BUYER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY
* DISCLAIMS ANY AND ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT
* LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE OR NONINFRINGEMENT. NEITHER DOES MEDIATEK PROVIDE
* ANY WARRANTY WHATSOEVER WITH RESPECT TO THE SOFTWARE OF ANY THIRD PARTY
* WHICH MAY BE USED BY, INCORPORATED IN, OR SUPPLIED WITH THE MEDIATEK
* SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH THIRD PARTY FOR ANY
* WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE
* FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S SPECIFICATION OR TO
* CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
* BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
* LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL
* BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
* ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY
* BUYER TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
* THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
* WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT
* OF LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING
* THEREOF AND RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN
* FRANCISCO, CA, UNDER THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE
* (ICC).
********************************************************************************
*/

/*
** $Log: Hs20_mtk_sigma.c $
 *
 */

  /*******************************************************************************
 *						   C O M P I L E R	 F L A G S
 ********************************************************************************
 */

 /*******************************************************************************
 *					  E X T E R N A L	R E F E R E N C E S
 ********************************************************************************
 */
 
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "includes.h"

#include "common.h"
#include "eloop.h"
#include "common/ieee802_11_common.h"
#include "common/ieee802_11_defs.h"
#include "common/gas.h"
#include "common/wpa_ctrl.h"
#include "wpa_supplicant_i.h"
#include "driver_i.h"
#include "config.h"
#include "bss.h"
#include "gas_query.h"
#include "interworking.h"
#include "hs20_supplicant.h"


/*******************************************************************************
*								C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*							   D A T A	 T Y P E S
********************************************************************************
*/

/*******************************************************************************
*							  P U B L I C	D A T A
********************************************************************************
*/

/*******************************************************************************
*							 P R I V A T E	 D A T A
********************************************************************************
*/

/*******************************************************************************
*								   M A C R O S
********************************************************************************
*/

/*******************************************************************************
*					 F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/

/*******************************************************************************
*								F U N C T I O N S
********************************************************************************
*/

#ifdef CONFIG_MTK_HS20R1_SIGMA


int sigma_hs20_init (struct wpa_supplicant *wpa_s)
{
	struct hs20_sigma *sigma;
	struct sigma_cmd_sta_cred_list *prCredLst;

	if(wpa_s->global->sigma == NULL) {
		wpa_hs20_printf(MSG_INFO, "HS20 SIGMA: sigma_init");
		
		sigma = os_zalloc(sizeof(struct hs20_sigma));
		if (sigma == NULL) {
			return -1;
		}

#if 0
	    //sigma->cred_count = 0;
	    prCredLst = os_zalloc(sizeof(struct sigma_cmd_sta_cred_list));
		if (prCredLst == NULL) {
			return -1;
		}
		prCredLst->prNext = NULL;
		prCredLst->cred_count = 0;
		sigma->prCredLst = prCredLst;
#endif	
	    
	    sigma->is_running= 0;
		wpa_s->global->sigma = sigma;		

	}
	else {
		wpa_hs20_printf(MSG_INFO, "HS20 SIGMA: sigma is already init");
	}

	return 0;
}


void sigma_hs20_deinit(struct wpa_supplicant *wpa_s)
{
	size_t i;
	struct hs20_credential *prCredTmp;

	wpa_hs20_printf(MSG_INFO, "HS20 SIGMA: sigma_deinit");

	if(wpa_s->global->sigma != NULL) {
#if 0
		if (sigma->bssid_pool) {
			for (i = 0; i < sigma->bssid_pool_count; i++) {
				os_free(sigma->bssid_pool[i]);
			}
			os_free(sigma->bssid_pool);
			sigma->bssid_pool_count = 0;
			sigma->bssid_pool = NULL;
		}

		if (wpa_s->bss_pool) {
			os_free(wpa_s->bss_pool);
			wpa_s->bss_pool = NULL;
		}
		

		if (sigma->prCredLst) {
			sigma->prCredLst->cred_count = 0;
			while(sigma->prCredLst->prNext != NULL){
				prCredTmp = sigma->prCredLst->prNext->prNext;
				os_free(sigma->prCredLst->prNext);
				sigma->prCredLst->prNext = prCredTmp;
			}
			os_free(sigma->prCredLst);
			sigma->prCredLst = NULL;
		}	
#endif
		os_free(wpa_s->global->sigma);
		wpa_s->global->sigma = NULL;	
	}
	else {
		wpa_hs20_printf(MSG_INFO, "HS20 SIGMA: sigma is already deinit");
	}
}


/************ 
UCC: HS2-5.1 
CAPI: sta_scan
************/
int sigma_hs20_add_scan_info(struct wpa_supplicant *wpa_s, char *cmd, char *buf, size_t buflen)
{

	char *str;
	int ret = 3;

	wpa_hs20_printf(MSG_INFO, "[%s] Add Scan info...", __func__);

	for(;;)
	{
		str = strtok_r(NULL, " ", &cmd);
		if(str == NULL || str[0] == '\0')
			break;

		if(os_strncmp(str, "-h", 2) == 0){
			if(hwaddr_aton(str+2, wpa_s->conf->hessid) == -1){ 
				ret = -1;
			}
			wpa_hs20_printf(MSG_INFO, "[%s] Set hessid : " MACSTR, __func__, MAC2STR(wpa_s->conf->hessid));
		}
		else if(os_strncmp(str, "-a", 2) == 0){
			wpa_s->conf->access_network_type = atoi(str+2);
			if(wpa_s->conf->access_network_type > 15 || wpa_s->conf->access_network_type < 0){
				ret = -1;
			}
			wpa_hs20_printf(MSG_INFO, "[%s] Set access_network_type : %d", __func__, wpa_s->conf->access_network_type);
		}
	}

	
	if (!wpa_s->conf->update_config) {
		wpa_hs20_printf(MSG_DEBUG, "CTRL_IFACE: SAVE_CONFIG - Not allowed "
			   "to update configuration (update_config=0)");
		return -1;
	}

	ret = wpa_config_write(wpa_s->confname, wpa_s->conf);
	if (ret) {
		wpa_hs20_printf(MSG_DEBUG, "CTRL_IFACE: SAVE_CONFIG - Failed to "
			   "update configuration");
	} else {
		wpa_hs20_printf(MSG_DEBUG, "CTRL_IFACE: SAVE_CONFIG - Configuration"
			   " updated");
	}
	
	
	return ret;
}


/************ 
UCC: Almost all
CAPI: set_bssid_pool
************/
int sigma_hs20_set_bssid_pool(struct wpa_supplicant *wpa_s, char *cmd, char *buf, size_t buflen)
{
	struct param_hs20_set_bssid_pool rBssidPool;
	u8 bssid_buf[ETH_ALEN];
	int res;
	char *pos;
	char *end;
	//struct hs20_sigma *sigma = wpa_s->global->sigma;
	size_t i;
	u8 **bssids;
	u8 *bssid;
	int ret = 3;

	wpa_hs20_printf(MSG_INFO, "[%s] Set Bssid Pool...", __func__);

	/* clear old bssid pool */
	/*
	if (sigma->bssid_pool) {
		for (i = 0; i < sigma->bssid_pool_count; i++) {
			os_free(sigma->bssid_pool[i]);
		}
		os_free(sigma->bssid_pool);
		sigma->bssid_pool_count = 0;
		sigma->bssid_pool = NULL; 
	}
	*/

	pos = cmd;
	end = os_strchr(pos, '\0');
	
	rBssidPool.fgBssidPoolIsEnable = 0;
	rBssidPool.ucNumBssidPool = 0;

	// disable the bssid pool
	if (os_strncmp(pos, "0", 1) == 0) {
	    wpas_hs20_test_mode(wpa_s, HS20_CMD_ID_SET_BSSID_POOL, &rBssidPool);
		return 0;
	}
	// enable the bssid pool
	else if (os_strncmp(pos, "1 ", 2) == 0) {
		rBssidPool.fgBssidPoolIsEnable = 1;
		pos += 2;
		while (pos < end) {
			if (hwaddr_aton(pos, bssid_buf)) {
				return -1;
			}

			os_memcpy(rBssidPool.arBssidPool[rBssidPool.ucNumBssidPool++], bssid_buf, ETH_ALEN);

			if(rBssidPool.ucNumBssidPool >= 8)
				break;

			pos += 18;
		}
	}
	else {
		return -1;
	}

    for (i = 0; i < rBssidPool.ucNumBssidPool; i++) {
    	wpa_hs20_printf(MSG_DEBUG, "Test: bssid_pool[%d] " MACSTR, i, MAC2STR(rBssidPool.arBssidPool[i]));
    }

    wpas_hs20_test_mode(wpa_s, HS20_CMD_ID_SET_BSSID_POOL, &rBssidPool);
	
	return ret;
}

int sigma_get_current_assoc(struct wpa_supplicant *wpa_s, char *cmd, char *buf, size_t buflen)
{
    struct wpa_ssid *cur_ssid = wpa_s->current_ssid;

	char *pos, *end;
	int ret = -1;
	int i = 0;

	pos = buf;
	end = buf + buflen;

	if((cur_ssid != NULL) && (wpa_s->wpa_state == WPA_COMPLETED)){
		ret = os_snprintf(pos, end - pos, "ssid=%s\nbssid=" MACSTR "\n", cur_ssid->ssid, MAC2STR(wpa_s->bssid));
	}

	return ret;
}


#if 0

int sigma_hs20_reset(P_WPA_SUPPLICANT_T wpa_s){
    struct wpa_ssid *ssid;

	// Reset variables
	wpa_printf(MSG_INFO, "[%s] HS20 Reset...", __func__);

	//1 TODO More things to reset?
	os_memset(wpa_s->global->pHs20, 0, sizeof(wpa_s->global->pHs20));
	os_memset(wpa_s->global->sigma, 0, sizeof(wpa_s->global->sigma));

    if(!wpa_s->disconnected) {
        wpa_s->reassociate = 0;
        wpa_s->disconnected = 1;
        wpa_supplicant_disassociate(wpa_s, WLAN_REASON_DEAUTH_LEAVING);
    }

	/* Mark all networks disabled*/
	ssid = wpa_s->conf->ssid;
	while (ssid) {
		ssid->disabled = 1;
		ssid = ssid->next;
	}

	return 1;
}


#if 1
int sigma_set_bssid_pool(struct wpa_supplicant *wpa_s, char *cmd)
{
	u8 bssid_buf[ETH_ALEN];
	int res;
	char *pos;
	char *end;
	struct hs20_sigma *sigma;
	sigma = wpa_s->global->sigma;
	size_t i;
	u8 **bssids;
	u8 *bssid;

	/* obsolete the old bssid pool */
	if (sigma->bssid_pool) {
		for (i = 0; i < sigma->bssid_pool_count; i++) {
			os_free(sigma->bssid_pool[i]);
		}
		os_free(sigma->bssid_pool);
		sigma->bssid_pool_count = 0;
		sigma->bssid_pool = NULL; 
	}

	pos = cmd;
	end = os_strchr(pos, '\0');

    wpa_printf(MSG_ERROR, "Clear current hs20 scan results.");
    hs20_scan_results_free(wpa_s->hs20_scan_res);
    wpa_s->hs20_scan_res = NULL;

	// disable the bssid pool
	if (os_strncmp(pos, "0", 1) == 0) {
	    hs20_set_bssid_pool(wpa_s, sigma->bssid_pool_count, sigma->bssid_pool);
		return 0;
	}
	// enable the bssid pool
	else if (os_strncmp(pos, "1 ", 2) == 0) {
		pos += 2;
		while (pos < end) {
			if (hwaddr_aton(pos, bssid_buf)) {
				return -1;
			}

			bssid = os_zalloc(ETH_ALEN);
			if (bssid == NULL) {
				return -1;
			}
			os_memcpy(bssid, bssid_buf, ETH_ALEN);

			bssids = os_realloc(sigma->bssid_pool,
					 (sigma->bssid_pool_count + 1) * sizeof(u8 *));
			if (bssids == NULL) {
				os_free(bssid);
				return -1;
			}
			bssids[sigma->bssid_pool_count++] = bssid;
			sigma->bssid_pool = bssids;

			pos += 18;
		}
	}
	else {
		return -1;
	}

    for (i = 0; i < sigma->bssid_pool_count; i++) {
    	wpa_printf(MSG_ERROR, "Test: bssid_pool[%d] " MACSTR, i, MAC2STR(sigma->bssid_pool[i]));
    }

    hs20_set_bssid_pool(wpa_s, sigma->bssid_pool_count, sigma->bssid_pool);

	//Puff
    wpa_printf(MSG_ERROR, "[Puff] Clear current hs20 scan results.");
    hs20_scan_results_free(wpa_s->hs20_scan_res);
    wpa_s->hs20_scan_res = NULL;
	
	//hs20_get_scan_results_filter_by_bssid_pool(wpa_s);
	
	return 0;
}
#else
int sigma_set_bssid_pool(P_WPA_SUPPLICANT_T wpa_s, char *cmd, char *buf, size_t buflen)
{
	struct sigma_cmd_sta_bssid_pool *prBssidPool = NULL;
	struct bssid_list *prList = NULL, *prFirst = NULL;
	Boolean fgFirst;
	char *pos, *end;
	char *comma = ",";
	u8 *bssid;
	int ret = 0;
	int i = 0;

	wpa_printf(MSG_INFO, "[%s] Set bssid list...", __func__);

	prBssidPool = os_zalloc(sizeof(struct sigma_cmd_sta_bssid_pool));
	pos = buf;
	end = buf + buflen;

	// Enable or Disable filter
	if(os_strncmp(cmd, "0", 1) == 0){
		wpa_printf(MSG_ERROR, "[%s] The bssid_filter is disabled...", __func__);
		prBssidPool->fgEnabled = FALSE;
		prBssidPool->BssidNo   = 0;
		return 1;
	}
	else if(os_strncmp(cmd, "1 ", 2) == 0){
		wpa_printf(MSG_ERROR, "[%s] The bssid_filter is enabled...", __func__);
		prBssidPool->fgEnabled = TRUE;
		prBssidPool->BssidNo   = 0;
		fgFirst = TRUE;
		cmd += 2;
	}
	else {
		wpa_printf(MSG_ERROR, "[%s] Wrong parameters!!!", __func__);
		return -1;
	}

	// Add bssid_list
	bssid = strtok(cmd, comma);
	prList = os_zalloc(sizeof(struct bssid_list));

	do {

		if(hwaddr_aton(bssid, prList->bssid)){
			wpa_printf(MSG_INFO ,"[%s] Not a bssid format...", __func__);
		}
		else {
			prBssidPool->BssidNo ++;

			if(fgFirst){
				prFirst = prList;
				fgFirst = FALSE;
			}
			prList->prNext = os_zalloc(sizeof(struct bssid_list));
			prList = prList->prNext;
		}
		bssid = strtok(NULL, comma);
	} while(bssid != NULL);

	prBssidPool->prNext = prFirst;

	if(prBssidPool->BssidNo == 0){
		wpa_printf(MSG_ERROR, "[%s] If filter is enabled, please add at least one bssid!!!", __func__);
		return -1;
	}
	else {
		prList = prBssidPool->prNext;
		for(i=1 ; i<=(prBssidPool->BssidNo); i++){
				wpa_printf(MSG_INFO ,"[%s] Add [No. %d] bssid-[" MACSTR "]\n",
					__func__,
					i,
					MAC2STR(prList->bssid));
				prList = prList->prNext;
		}
	}

	wpa_s->global->pSigma->prBssidPool = prBssidPool;

	return 1;
}
#endif


int sigma_get_gtk_ptk_key(P_WPA_SUPPLICANT_T wpa_s, char *cmd, char *buf, size_t buflen)
{
	struct wpa_gtk_data *gd = wpa_s->wpa->gd;
	char *pos, *end;
	int ret = 0;
	int i = 0;

	pos = buf;
	end = buf + buflen;

	if(os_strncmp(cmd, "GTK", 3) == 0){
		if(gd->gtk_len == 16){
			ret = os_snprintf(pos, end - pos, "KEY: " CCMPGTKSTR, CCMPGTK2STR(gd->gtk));
		}
		else if(gd->gtk_len == 32){
			ret = os_snprintf(pos, end - pos, TKIPGTKSTR, TKIPGTK2STR(gd->gtk));
		}
		else {
			//1 TODO add WEP
			return -1;
		}

		wpa_hexdump(MSG_INFO, "[GTK3] Success to set Group Key", gd->gtk, gd->gtk_len);

	}
	else if(os_strncmp(cmd, "ptk", 3) == 0){
		//1 TODO PTK
		ret = -1;
	}
	else {
		wpa_printf(MSG_ERROR, "[%s]ERROR...parameter 0 (%s) is invalid!!!\n", __func__, *cmd);
		return -1;
	}

	return ret;
}


/* return 0 means SUCCESS, -1 means FAIL */
int sigma_sta_hs2_associate(struct wpa_supplicant *wpa_s)
{
#if 0
    wpa_s->global->round = 1;
    wpa_s->global->query_round = 1;
    wpa_s->global->sigma->is_running = 1;
    //wpa_s->conf->ap_scan = 0;
    struct wpa_ssid *ssid = NULL;

    hs20_reset_all_query_state(wpa_s);
    //hs20_printf_anqp_data_all_networks(wpa_s);

    if (wpa_s->current_ssid)
        wpa_supplicant_disassociate(wpa_s, WLAN_REASON_DEAUTH_LEAVING);

    ssid = wpa_s->conf->ssid;
    while (ssid) {
        ssid->disabled = 1 /*(nid != ssid->id)*/;
        ssid = ssid->next;
    }

    //hs20_query_all_anqp_caps_all_networks(wpa_s, NULL);
    hs20_query_anqp_elem_all_networks(wpa_s, NULL);
#else
	wpa_msg_ctrl(wpa_s, MSG_INFO, HS2_EVENT_HS20_CONNECT);
#endif

    return 0;
	
}


int sigma_get_current_assoc(P_WPA_SUPPLICANT_T wpa_s, char *cmd, char *buf, size_t buflen)
{
	struct hs20_sigma *sigma = wpa_s->global->sigma;
    struct wpa_ssid *cur_ssid = wpa_s->current_ssid;

	char *pos, *end;
	int ret = -1;
	int i = 0;

	pos = buf;
	end = buf + buflen;

	if((cur_ssid != NULL) && (wpa_s->wpa_state == WPA_COMPLETED)){
		ret = os_snprintf(pos, end - pos, "ssid=%s\nbssid=" MACSTR "\n", cur_ssid->ssid, MAC2STR(wpa_s->bssid));
	}

	return ret;
}

int sigma_hs20_add_scan_info(P_WPA_SUPPLICANT_T wpa_s, char *cmd, char *buf, size_t buflen)
{
	char *str;
	int ret = 1;
	struct wpabuf *ie;

	wpa_printf(MSG_INFO, "[%s] Add Scan info...", __func__);

	for(;;)
	{
		str = strtok_r(NULL, " ", &cmd);
		if(str == NULL || str[0] == '\0')
			break;

		if(os_strncmp(str, "-h", 2) == 0){
			if(hwaddr_aton(str+2, wpa_s->global->home_chars->hessid) == -1){ 
				ret = -1;
			}
			wpa_printf(MSG_INFO, "[%s] Set hessid : " MACSTR, __func__, MAC2STR(wpa_s->global->home_chars->hessid));
		}
		else if(os_strncmp(str, "-a", 2) == 0){
			wpa_s->global->home_chars->access_network_opt_ant = atoi(str+2);
			if(wpa_s->global->home_chars->access_network_opt_ant > 15 || wpa_s->global->home_chars->access_network_opt_ant < 0){
				ret = -1;
			}
			wpa_printf(MSG_INFO, "[%s] Set access_network_opt_ant : %d", __func__, wpa_s->global->home_chars->access_network_opt_ant);
		}
	}

    //Set IE template to driver
    ie = hs20_build_related_ie(wpa_s);
    wpa_printf(MSG_INFO, "Build HS20 related IE done!");
    if(ie) {
        wpa_printf(MSG_INFO, "Set HS20 related IE to driver!");
        wpa_drv_set_gen_ie(wpa_s, wpabuf_head(ie), wpabuf_len(ie));
        wpa_printf(MSG_INFO, "Free HS20 related IE!");
        wpabuf_free(ie);
    }	

	return ret;
}


int sigma_hs20_sta_add_cred(P_WPA_SUPPLICANT_T wpa_s, char *cmd, char *buf, size_t buflen)
{
#if 0
	struct hs20_credential *prCred = NULL;

	char *pos, *end;
	char *str;
	int ret = 0;
	int i = 0;

	wpa_printf(MSG_INFO, "[%s] Set STA_ADD_CREDENTIAL...", __func__);
	
    prCred = os_zalloc(sizeof(struct hs20_credential));
	if (prCred == NULL) {
		return -1;
	}
	wpa_s->global->sigma->prCredLst->cred_count++;
	//prCred = os_zalloc(sizeof(struct sigma_cmd_sta_add_credential));
    //prCred = &(wpa_s->global->sigma->arCred[wpa_s->global->sigma->cred_count]);
    //wpa_s->global->sigma->cred_count++;

	pos = buf;
	end = buf + buflen;

	/*
	* "<-tTYPE> <-uUSERNAME> <-pPASSWORD> <-iIMSI> <-nPLMN_MNC>\n"
	* "<-cPLMN_MCC> <-aROOT_CA> <-rREALM> <-fPREFER> <-qFQDN> <-lCLIENT_CA>= add credential" 
	*/
	prCred->type[0] = '\0';
	prCred->username[0] = '\0';
	prCred->password[0] = '\0';
	prCred->imsi[0] = '\0';
	prCred->plmn_mnc[0] = '\0';
	prCred->plmn_mcc[0] = '\0';
	prCred->root_ca[0] = '\0';
	prCred->realm[0] = '\0';
	prCred->prefer = 0;
	prCred->fqdn[0] = '\0';
	prCred->clientCA[0] = '\0';

	for(;;)
	{
		str = strtok_r(NULL, " ", &cmd);
		if(str == NULL || str[0] == '\0')
			break;

		if(os_strncmp(str, "-t", 2) == 0){
			strncpy(prCred->type, str+2, WFA_HS20_TYPE_LEN-1);
			prCred->type[os_strlen(str + 2)]='\0';
			wpa_printf(MSG_INFO, "[%s] Set prCred->type : %s", __func__, prCred->type);
		}
		else if(os_strncmp(str, "-u", 2) == 0){
			strncpy(prCred->username, str+2, WFA_HS20_LEN_32-1);
			prCred->username[os_strlen(str + 2)]='\0';
			wpa_printf(MSG_INFO, "[%s] Set prCred->username : %s", __func__, prCred->username);
		}
		else if(os_strncmp(str, "-p", 2) == 0){
			strncpy(prCred->password, str+2, WFA_HS20_LEN_256-1);
			prCred->password[os_strlen(str + 2)]='\0';
			wpa_printf(MSG_INFO, "[%s] Set prCred->password : %s", __func__, prCred->password);
		}
		else if(os_strncmp(str, "-i", 2) == 0){
			strncpy(prCred->imsi, str+2, WFA_HS20_LEN_32-1);
			prCred->imsi[os_strlen(str + 2)]='\0';
			wpa_printf(MSG_INFO, "[%s] Set prCred->imsi : %s", __func__, prCred->imsi);
		}
		else if(os_strncmp(str, "-n", 2) == 0){
			strncpy(prCred->plmn_mnc, str+2, WFA_HS20_LEN_32-1);
			prCred->plmn_mnc[os_strlen(str + 2)]='\0';
			wpa_printf(MSG_INFO, "[%s] Set prCred->plmn_mnc : %s", __func__, prCred->plmn_mnc);
		}
		else if(os_strncmp(str, "-c", 2) == 0){
			strncpy(prCred->plmn_mcc, str+2, WFA_HS20_LEN_32-1);
			prCred->plmn_mcc[os_strlen(str + 2)]='\0';
			wpa_printf(MSG_INFO, "[%s] Set prCred->plmn_mcc : %s", __func__, prCred->plmn_mcc);
		}
		else if(os_strncmp(str, "-a", 2) == 0){
			strncpy(prCred->root_ca, str+2, WFA_HS20_LEN_32-1);
			prCred->root_ca[os_strlen(str + 2)]='\0';
			wpa_printf(MSG_INFO, "[%s] Set prCred->root_ca : %s", __func__, prCred->root_ca);
		}
		else if(os_strncmp(str, "-r", 2) == 0){
			strncpy(prCred->realm, str+2, WFA_HS20_LEN_32-1);
			prCred->realm[os_strlen(str + 2)] = '\0';
			wpa_printf(MSG_INFO, "[%s] Set prCred->realm : %s [%d]", __func__, prCred->realm, os_strlen(prCred->realm));
		}
		else if(os_strncmp(str, "-f", 2) == 0){
			switch(atoi(str+2)){
				case 0:
					prCred->prefer = 0;
					break;
				case 1:
					prCred->prefer = 1;
					break;
				default:
					wpa_printf(MSG_ERROR, "[%s] Wrong prefer parameter(%d)", __func__, atoi(str+2));
					prCred->prefer = 0;
					break;
			}
			wpa_printf(MSG_INFO, "[%s] Set prCred->prefer : %d", __func__, prCred->prefer);
		}
		else if(os_strncmp(str, "-q", 2) == 0){
			strncpy(prCred->fqdn, str+2, WFA_HS20_LEN_32-1);
			prCred->fqdn[os_strlen(str + 2)]='\0';
			wpa_printf(MSG_INFO, "[%s] Set prCred->fqdn : %s", __func__, prCred->fqdn);
		}
		else if(os_strncmp(str, "-l", 2) == 0){
			strncpy(prCred->clientCA, str+2, WFA_HS20_LEN_32-1);
			prCred->clientCA[os_strlen(str + 2)]='\0';
			wpa_printf(MSG_INFO, "[%s] Set prCred->clientCA : %s", __func__, prCred->clientCA);
		}
	}

	prCred->prNext = wpa_s->global->sigma->prCredLst->prNext;
	wpa_s->global->sigma->prCredLst->prNext = prCred;

/*
	if(wpa_s->global->sigma->arCred != NULL){
		os_free(wpa_s->global->sigma->arCred);
	}

	wpa_s->global->sigma->arCred = prCred;
*/
	return 1;
#else
	if(cmd != NULL)
		wpa_msg_ctrl(wpa_s, MSG_INFO, HS2_EVENT_ADD_CREDENTIAL "%s", cmd);
	return 0;
#endif
}

int sigma_hs20_sta_dump_cred(P_WPA_SUPPLICANT_T wpa_s, char *buf, size_t buflen)
{
	struct hs20_credential *prCred = wpa_s->global->sigma->prCredLst->prNext;
	size_t cred_cnt = wpa_s->global->sigma->prCredLst->cred_count;
	char *pos, *end;
	int ret = 0;
	int idx = 1;

	pos = buf;
	end = buf + buflen;	

	wpa_printf(MSG_INFO, "[%s] Dump existing credentials (Total: %d)...", __func__, cred_cnt);

	ret = os_snprintf(pos, end - pos, "Total credentials: %d\n", cred_cnt);
	if (ret < 0 || ret >= end - pos)
		return pos - buf;
	pos += ret;	

	while(prCred != NULL){	
		ret = os_snprintf(pos, end - pos, "----------------\n");
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;		
		ret = os_snprintf(pos, end - pos, "Credential:%d\n", idx++);
		if (ret < 0 || ret >= end - pos)
			return pos - buf;
		pos += ret;		
		if(prCred->type[0] != '\0'){
			ret = os_snprintf(pos, end - pos, "Type:%s\n", prCred->type);
			if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;				
		}
		if(prCred->username[0] != '\0'){
			ret = os_snprintf(pos, end - pos, "Username:%s\n", prCred->username);
			if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;
		}
		if(prCred->password[0] != '\0'){
			ret = os_snprintf(pos, end - pos, "Password:%s\n", prCred->password);
			if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;
		}
		if(prCred->imsi[0] != '\0'){
			ret = os_snprintf(pos, end - pos, "IMSI:%s\n", prCred->imsi);
			if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;
		}	
		if(prCred->plmn_mnc[0] != '\0'){
			ret = os_snprintf(pos, end - pos, "PLMN_MNC:%s\n", prCred->plmn_mnc);
			if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;
		}
		if(prCred->plmn_mcc[0] != '\0'){
			ret = os_snprintf(pos, end - pos, "PLMN_MCC:%s\n", prCred->plmn_mcc);
			if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;
		}
		if(prCred->root_ca[0] != '\0'){
			ret = os_snprintf(pos, end - pos, "Root_ca:%s\n", prCred->root_ca);
			if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;
		}
		if(prCred->realm[0] != '\0'){
			ret = os_snprintf(pos, end - pos, "Realm:%s\n", prCred->realm);
			if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;
		}				
		ret = os_snprintf(pos, end - pos, "Prefer:%d\n", prCred->prefer);
		if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;
		if(prCred->fqdn[0] != '\0'){
			ret = os_snprintf(pos, end - pos, "FQDN:%s\n", prCred->fqdn);
			if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;
		}
		if(prCred->clientCA[0] != '\0'){
			ret = os_snprintf(pos, end - pos, "ClientCA:%s\n", prCred->clientCA);
			if (ret < 0 || ret >= end - pos)
				return pos - buf;
			pos += ret;
		}		
		
		prCred = prCred->prNext;
	}

	if(cred_cnt != (idx-1)){
		wpa_printf(MSG_ERROR, "[%s] CRED_CNT(%d) is not identical to idx(%d)", 
			__func__,
			cred_cnt,
			idx);
	}

	return pos - buf;
}

int sigma_hs20_sta_del_cred(P_WPA_SUPPLICANT_T wpa_s, char *cmd, char *buf, size_t buflen)
{
	struct hs20_credential *prCred = wpa_s->global->sigma->prCredLst->prNext;
	struct hs20_credential *prPreCred = NULL;
	
	size_t cred_cnt = wpa_s->global->sigma->prCredLst->cred_count;
	char *pos, *end;
	int ret = 0;
	int idx = atoi(cmd);
	int temp_idx = 1;

	wpa_printf(MSG_INFO, "[%s] Delete credential (%d)...", __func__, idx);

	if(idx == 9999){
		while(prCred != NULL){
			prPreCred = prCred;
			prCred = prCred->prNext;
			os_free(prPreCred);
		}
		wpa_s->global->sigma->prCredLst->prNext = NULL;
		wpa_s->global->sigma->prCredLst->cred_count = 0;
		return 3;
	}

	if(idx > cred_cnt || idx <= 0){
		wpa_printf(MSG_ERROR, "[%s] idx(%d) is not within cred_cnt(%d)", __func__, idx, cred_cnt);
		return -1;
	}

	if(idx == 1){
		wpa_s->global->sigma->prCredLst->prNext = prCred->prNext;
		os_free(prCred);
		wpa_s->global->sigma->prCredLst->cred_count --;
		return 3;
	}

	while(temp_idx < idx){
		prPreCred = prCred;
		prCred = prCred->prNext;
		temp_idx++;
	}
	prPreCred->prNext = prCred->prNext;
	os_free(prCred);
	wpa_s->global->sigma->prCredLst->cred_count --;
	return 3;	

}


int sigma_hs20_lock_scan(P_WPA_SUPPLICANT_T wpa_s, char *cmd, char *buf, size_t buflen)
{
	struct wpa_scan_res *res = NULL;
	size_t i, j, k, m, n;
	char *pos, *end;
	int ret = 0;

	n = 0;

	pos = buf;
	end = buf + buflen;
	j=0;

	wpa_printf(MSG_INFO, "[%s] HS20 Lock scan...", __func__);

	/*
	if(wpa_s->global->sigma->prBssidPool->BssidNo == 0){
		wpa_printf(MSG_ERROR, "[%s] There is no items in BSSID_POOL...", __func__);
		return 1;
	}
	*/

	for(i=0; i<1; i++){
		struct bssid_list *prCurr;
		int total_match = 0;

		//wpa_supplicant_notify_scanning(wpa_s, 1);

		wpa_s->conf->ap_scan = 0;


		wpa_printf(MSG_INFO, "[%s] test %d\n", __func__, n++);

#if 1


#if CONFIG_HS20_SIGMA

        wpa_printf(MSG_ERROR, "Clear current hs20 scan results.");
        hs20_scan_results_free(wpa_s->hs20_scan_res);
        wpa_s->hs20_scan_res = NULL;

		if (wpa_s->global->sigma->bssid_pool_count > 0) {
			struct wpa_scan_res **tmp;
			struct wpa_scan_res *r;
			size_t j;
			struct hs20_sigma *sigma;
			sigma = wpa_s->global->sigma;
			struct wpa_scan_results *results;
			struct wpa_scan_res *bss;

			wpa_printf(MSG_INFO, "[%s] test %d\n", __func__, n++);

			// find the specific BSS +++
			if (wpa_s->scan_res == NULL &&
				wpa_supplicant_get_scan_results(wpa_s) < 0)
				return -1;

			wpa_printf(MSG_INFO, "[%s] test %d\n", __func__, n++);

			if (wpa_s->scan_res == NULL)
				return -1;

			wpa_printf(MSG_INFO, "[%s] test %d\n", __func__, n++);

			results = wpa_s->scan_res;

			wpa_printf(MSG_INFO, "[%s] test %d\n", __func__, n++);

			/* obsolete the old bssid pool */
			if (wpa_s->bss_pool) {
				os_free(wpa_s->bss_pool);
				wpa_s->bss_pool = NULL;
			}
			wpa_printf(MSG_INFO, "[%s] test A\n", __func__);

			/* renew the bssid pool */
			for (i = 0; i < results->num; i++) {
				bss = results->res[i];

				for (j = 0; j < sigma->bssid_pool_count; j++) {
					if (os_memcmp(bss->bssid, sigma->bssid_pool[j], ETH_ALEN) == 0) {
						r = bss;

						if (wpa_s->bss_pool == NULL) {
							wpa_s->bss_pool = os_zalloc(sizeof(struct wpa_scan_results));
							if (wpa_s->bss_pool == NULL) {
								return -1;
							}
						}

						tmp = os_realloc(wpa_s->bss_pool->res,
								 (wpa_s->bss_pool->num + 1) * sizeof(struct wpa_scan_res *));
						if (tmp == NULL) {
							return -1;
						}

						tmp[wpa_s->bss_pool->num++] = r;
						wpa_s->bss_pool->res = tmp;
					}
				}
			}

			//results = wpa_s->bss_pool;
		}
#endif

	if(wpa_s->bss_pool == NULL){
        wpa_printf(MSG_INFO, "[%s] bss_pool is empty...", __func__);
        ret = os_snprintf(pos, end - pos, "RESULT:FAIL\n");
        wpa_s->conf->ap_scan = 1;

        if (!wpa_s->scanning && ((wpa_s->wpa_state <= WPA_SCANNING) ||
            (wpa_s->wpa_state >= WPA_COMPLETED))) {
            wpa_s->scan_req = 2;
            wpa_supplicant_req_scan(wpa_s, 0, 0);
        } else {
            wpa_printf(MSG_ERROR, "Ongoing Scan action...");
        }

        return ret;
	}

	wpa_printf(MSG_INFO, "[%s] wpa_s->global->sigma->bssid_pool_count = %d\n", __func__, wpa_s->global->sigma->bssid_pool_count);

		// All match
		if(wpa_s->bss_pool != NULL && wpa_s->global->sigma->bssid_pool_count == wpa_s->bss_pool->num){
			wpa_printf(MSG_INFO, "[%s] ALL match (%d)!!!", __func__, wpa_s->bss_pool->num);
			ret = os_snprintf(pos, end - pos, "RESULT:PASS\n");
			return ret;
		}
		else {
			wpa_printf(MSG_ERROR, "[%s] Match failed...bssid_pool_count(%d), (%d)",
				__func__, wpa_s->global->sigma->bssid_pool_count, wpa_s->bss_pool->num);
			ret = os_snprintf(pos, end - pos, "RESULT:FAIL\n");
			wpa_s->conf->ap_scan = 1;

    		if (!wpa_s->scanning && ((wpa_s->wpa_state <= WPA_SCANNING) ||
    			(wpa_s->wpa_state >= WPA_COMPLETED))) {
    			wpa_s->scan_req = 2;
    			wpa_supplicant_req_scan(wpa_s, 0, 0);
    		} else {
    			wpa_printf(MSG_ERROR, "Ongoing Scan action...");
    		}

			return ret;

		}
	}

#else

		prCurr = wpa_s->global->sigma->prBssidPool->prNext;

		do {
			int match = 0;

			for (k = 0; k < wpa_s->scan_res->num; k++) {
				struct wpa_scan_res *search_res = wpa_s->scan_res->res[k];

				for(m=0; m<ETH_ALEN; m++){
					if(prCurr->bssid[m] != search_res->bssid[m]){
						match = 0;
						break;
					}
					else {
						match = 1;
					}
				}

				if(match == 1){
					total_match++;
					break;
				}
			}

			if(match == 1){
				prCurr = prCurr->prNext;
			}
			else {
				break;
			}
		} while(prCurr != NULL);

		wpa_printf(MSG_INFO, "[%s] total_match: %d, BssidNo: %d!!!", __func__, total_match, wpa_s->global->sigma->prBssidPool->BssidNo);

		if(total_match == wpa_s->global->sigma->prBssidPool->BssidNo){
			wpa_printf(MSG_INFO, "[%s] ALL match!!!", __func__);
			goto ALL_MATCH;
		}


		wpa_s->conf->ap_scan = 1;
	}

	wpa_printf(MSG_ERROR, "[%s] Match failed...", __func__);
	ret = os_snprintf(pos, end - pos, "RESULT:FAIL\n");
	return ret;


ALL_MATCH:
	ret = os_snprintf(pos, end - pos, "RESULT:PASS\n");
	return ret;
#endif

	return -1;

}


int sigma_set_policy_update(P_WPA_SUPPLICANT_T wpa_s, char *cmd, char *buf, size_t buflen)
{

	wpa_msg_ctrl(wpa_s, MSG_INFO, HS2_EVENT_POLICY_UPDATE "%s", cmd);	

	return 0;
}


int sigma_sta_install_ppsmo(P_WPA_SUPPLICANT_T wpa_s, char *cmd, char *buf, size_t buflen)
{

	wpa_msg_ctrl(wpa_s, MSG_INFO, HS2_EVENT_INSTALL_PPSMO "%s", cmd);	

	return 0;
}


int sigma_sta_default_reset(P_WPA_SUPPLICANT_T wpa_s, char *cmd, char *buf, size_t buflen)
{

	wpa_msg_ctrl(wpa_s, MSG_INFO, HS2_EVENT_DEFAULT_RESET);	

	return 0;
}

int sigma_sta_install_webppsmo(P_WPA_SUPPLICANT_T wpa_s, char *cmd, char *buf, size_t buflen)
{

	wpa_msg_ctrl(wpa_s, MSG_INFO, HS2_EVENT_WEB_INSTALL_PPSMO "%s", cmd);	

	return 0;
}

#endif


#endif




