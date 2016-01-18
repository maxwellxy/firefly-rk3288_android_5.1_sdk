/******************************************************************************
 *
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 *         OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM
 *         OR ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 *****************************************************************************/


/*****************************************************************************
 *
 *  Filename:      btif_fm.c
 *
 *  Description:   Bluedroid Fm implementation
 *
 *****************************************************************************/

#include <hardware/bluetooth.h>
#include <hardware/bt_fm.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "gki.h"

#define LOG_TAG "BTIF_FM"

#include "bta_api.h"
#include "bta_rds_api.h"
#include "bd.h"


#include "btif_common.h"
#include "btif_util.h"
#include "btif_fm.h"
#include "gki.h"



/*****************************************************************************
**  Constants & Macros
******************************************************************************/


/* originally 85. however 85 is quite low causing continuous jumps. NFL should be run too
 * to give a better estimation */
#ifndef BTIF_FM_AF_THRESH
#define BTIF_FM_AF_THRESH 95
#endif

#define BTIF_RDS_STR_LEN_8      8
#define BTIF_RDS_STR_LEN_64     64


#define CHECK_BTFM_INIT() if (bt_fm_callbacks == NULL)\
    {\
        BTIF_TRACE_EVENT1("BTFM: %s: BTFM not initialized", __func__);\
        return BTA_WRONG_MODE;\
    } else {\
        BTIF_TRACE_EVENT1("BTFM: %s", __func__);\
    }
/* remapping of bta values. BT over SCO and A2DP are missing. keep in sync with is sent from jni! */
#define BTIF_FM_AUDIO_PATH_NONE            0
#define BTIF_FM_AUDIO_PATH_SPEAKER         1
#define BTIF_FM_AUDIO_PATH_WIRED_HEADSET   2
#define BTIF_FM_AUDIO_PATH_DIGITAL         3


/*****************************************************************************
**  Local type definitions
******************************************************************************/



/* FM data */

typedef struct
{
    BOOLEAN             af_avail;
    BOOLEAN             af_on;
    UINT16              pi_code;
    tBTA_FM_AF_LIST     af_list;
}tBTIF_FM_AF_CB;

typedef struct
{
    BOOLEAN                 rds_on;
    BOOLEAN                 audio_mute;
    BOOLEAN                 search_in_progress;
    tBTA_FM_AUDIO_MODE      mode;
    tBTA_FM_AUDIO_PATH      path;       /* this relies on bta_fm_api.h (and bta_fm_act.c) channges */
    tBTA_FM_STEP_TYPE       scan_step;
    UINT16                  cur_freq;
    tBTIF_FM_AF_CB          af_cb;
    UINT16                  volume;
    int                     current_bta_path;
} tBTIF_FM_Data;

/*****************************************************************************
**  Static variables
******************************************************************************/
static btfm_callbacks_t *bt_fm_callbacks = NULL;
//static tBTIF_FM_PARAMS btif_fm_cb_params;
static tBTIF_FM_Data btif_fm_data;

/*************************************************************************
** Extern functions
*************************************************************************/
    extern void bta_fm_co_reset_rds_engine(void);


/*****************************************************************************
** Local helper functions
******************************************************************************/
    static void btif_fm_cback(tBTA_FM_EVT event, tBTA_FM *p_data);
    static int btif_fm_set_rds_mode(BOOLEAN rdsOn, BOOLEAN afOn);
    static void btif_fm_set_rds_mode_int(BOOLEAN rds_on, BOOLEAN af_on);

    static int btif_fm_tune(int freq);
    static int btif_fm_set_audio_path(int audioPath);
    static int btif_fm_set_volume(int volume);


    /*******************************************************************************
    **
    ** Function         btif_fm_af_merge_list
    **
    ** Description      For method-B AF encoding, merge up the AF list for seperate
    **                  short lists based on the same currently tuned service.
    **
    ** Returns          BOOLEAN: TRUE: when AF list is updated
    **                           FALSE: No AF list updating needed.
    **
    *******************************************************************************/
    static BOOLEAN btif_fm_af_merge_list(tBTA_FM_AF_LIST *p_af_data)
    {
        UINT8 i, j ;
        BOOLEAN duplicate, add_up = FALSE;
        tBTA_FM_AF_LIST *p_local_lst = &btif_fm_data.af_cb.af_list;
        BTIF_TRACE_DEBUG1("%s", __func__);

        for (i = 0; i < p_af_data->num_af; i ++)
        {
            duplicate = FALSE;

            for (j = 0; j < p_local_lst->num_af; j ++)
            {
                if (p_af_data->af_list[i] == p_local_lst->af_list[j])
                {
                    duplicate = TRUE;
                    break;
                }
            }
            if ( ! duplicate)
            {
                add_up = TRUE;
                p_local_lst->af_type[p_local_lst->num_af] = p_af_data->af_type[i];
                p_local_lst->af_list[p_local_lst->num_af ++] = p_af_data->af_list[i];
            }
        }
        return add_up;
    }


    /*******************************************************************************
    **
    ** Function         btif_fm_rdsp_cback
    **
    ** Description      RDS decoder call back function.
    **
    ** Returns          void
    *******************************************************************************/
    void btif_fm_rdsp_cback(tBTA_RDS_EVT event, tBTA_FM_RDS *p_data, UINT8 app_id)
    {
        UINT8   i = 0;
        BOOLEAN new_af = TRUE;

        switch(event)
        {
        case BTA_RDS_REG_EVT:
            BTIF_TRACE_DEBUG1("Client register status: %d", p_data->status);
            break;
        case BTA_RDS_PI_EVT:
            BTIF_TRACE_DEBUG2("%s: BTA_RDS_PI_EVT pi_code = %d", __func__, p_data->pi_code);
            btif_fm_data.af_cb.pi_code = p_data->pi_code;
            break;
        case BTA_RDS_AF_EVT:
            BTIF_TRACE_DEBUG3("BTA_RDS_AF_EVT received in %s, tn_freq = %d, num_af = %d",
                  (p_data->af_data.method == BTA_RDS_AF_M_B)? "Method-B" :"Method-A",
                  p_data->af_data.list.af_list[0], btif_fm_data.af_cb.af_list.num_af);

            if (btif_fm_data.af_cb.af_on)
            {
                new_af = TRUE; /* needs to be true handle first time setup of AF */

                if (p_data->af_data.method == BTA_RDS_AF_M_B)
                {
                    if (p_data->af_data.list.af_list[0] == btif_fm_data.cur_freq)
                        new_af = btif_fm_af_merge_list(&p_data->af_data.list);
                    else
                        /* Application can choose to remember other tuned frequency AF list */
                        /* btui_app, other TN af list, skip */
                        new_af = FALSE;
                }
                else
                    memcpy(&btif_fm_data.af_cb.af_list, &p_data->af_data.list, sizeof(tBTA_FM_AF_LIST));
                if (new_af)
                {
                    btif_fm_data.af_cb.af_avail = TRUE;
                    /* when a new AF arrives, update AF list by calling set RDS mode */
                    if (btif_fm_data.af_cb.af_on)
                    {
                        btif_fm_set_rds_mode_int(TRUE, TRUE);
                    }
                }

                i = 0;
                while (i < btif_fm_data.af_cb.af_list.num_af)
                {
                    BTIF_TRACE_DEBUG2(">>>>>>>>>>>>>>>>>>>>>>>>>AF[%d] [%d]", i+1,
                        btif_fm_data.af_cb.af_list.af_list[i]);
                    i++;
                }
            }
            break;
        case BTA_RDS_PS_EVT:
            {
                BTIF_TRACE_DEBUG0("BTA_RDS_PS_EVT received");
                /* Reformat for transport to app via standard callback event. */
                tBTA_FM rds_event_data;
                rds_event_data.rds_update.status = BTA_FM_OK;
                rds_event_data.rds_update.data = BTA_RDS_PS_EVT;
                rds_event_data.rds_update.index = 0;
                memcpy(rds_event_data.rds_update.text, p_data->p_data, BTIF_RDS_STR_LEN_8+1);

                btif_fm_cback(BTA_FM_RDS_UPD_EVT, (tBTA_FM *)&rds_event_data);
            }
            break;
        case BTA_RDS_TP_EVT:
            BTIF_TRACE_DEBUG0("BTA_RDS_TP_EVT received");
            break;
        case BTA_RDS_PTY_EVT:
            {
                BTIF_TRACE_DEBUG0("BTA_RDS_TPY_EVT received");
                /* Reformat for transport to app via standard callback event. */
                tBTA_FM rds_event_data;
                rds_event_data.rds_update.status = BTA_FM_OK;
                rds_event_data.rds_update.data = BTA_RDS_PTY_EVT;
                rds_event_data.rds_update.index = p_data->pty.pty_val;
                memcpy(rds_event_data.rds_update.text, p_data->pty.p_str, BTIF_RDS_STR_LEN_8+1);

                btif_fm_cback(BTA_FM_RDS_UPD_EVT, (tBTA_FM *)&rds_event_data);
            }
            break;
        case BTA_RDS_PTYN_EVT:
            {
                BTIF_TRACE_DEBUG0("BTA_RDS_TPYN_EVT received");
                /* Reformat for transport to app via standard callback event. */
                tBTA_FM rds_event_data;
                rds_event_data.rds_update.status = BTA_FM_OK;
                rds_event_data.rds_update.data = BTA_RDS_PTYN_EVT;
                rds_event_data.rds_update.index = 0;
                memcpy(rds_event_data.rds_update.text, p_data->p_data, BTIF_RDS_STR_LEN_8+1);

                btif_fm_cback(BTA_FM_RDS_UPD_EVT, (tBTA_FM *)&rds_event_data);
            }
            break;
        case BTA_RDS_RT_EVT:
            BTIF_TRACE_DEBUG0("BTA_RDS_RT_EVT received");
            if (p_data->rt.complete != FALSE) {
                /* Reformat for transport to app via standard callback event. */
                tBTA_FM rds_event_data;
                rds_event_data.rds_update.status = BTA_FM_OK;
                rds_event_data.rds_update.data = BTA_RDS_RT_EVT;
                rds_event_data.rds_update.index = p_data->rt.g_type;
                memcpy(rds_event_data.rds_update.text, p_data->rt.p_data, BTIF_RDS_STR_LEN_64+1);

                btif_fm_cback(BTA_FM_RDS_UPD_EVT, (tBTA_FM *)&rds_event_data);
            }
            break;
        case BTA_RDS_RTP_EVT:
            if (NULL != p_data) {
                int k = 0;
                BTIF_TRACE_DEBUG0("BTA_RDS_RTP_EVT");
                tBTA_RDS_RTP *p_rtp_data = (tBTA_RDS_RTP*)p_data;

                    BTIF_TRACE_DEBUG4("%s: bRunning: %d bToggle = %d tag_num - %d",__func__,
                       p_rtp_data->running,p_rtp_data->tag_toggle,
                       p_rtp_data->tag_num);
                   k = p_rtp_data->tag_num;
                   while(k){

                        BTIF_TRACE_DEBUG4("%s: content_type: %d start: %d len: %d",__func__,
                           p_rtp_data->tag[k].content_type,p_rtp_data->tag[k].start,
                           p_rtp_data->tag[k].len);
                        k--;
                   }
                btif_fm_cback(BTA_RDS_RTP_EVT, (tBTA_FM *)&p_rtp_data);

             } else {
                 BTIF_TRACE_DEBUG0("BTIF_FM: BTA_RDS_RTP_EVT FAILED!!! (DATA == NULL)");
             }

            break;
        default:
            BTIF_TRACE_DEBUG1("RDS register status: %d", event);
            break;
        }
    }





 /*******************************************************************************
 **
 ** Function         btif_fm_set_rds_mode_int
 **
 ** Description      Turn on/off RDS/AF mode.
 **
 ** Returns          void
 *******************************************************************************/

static void btif_fm_set_rds_mode_int(BOOLEAN rds_on, BOOLEAN af_on)
{
    UINT8   i = 0;
    tBTA_FM_AF_PARAM *p_af_param = NULL;
    tBTA_FM_AF_LIST af_list = {2, {BTA_FM_AF_FM, BTA_FM_AF_FM}, { 1079, 880}};

    BTIF_TRACE_DEBUG4("%s: rds_on = %d, af_on = %d, af_avail = %d",
          __func__, rds_on, af_on, btif_fm_data.af_cb.af_avail);

    if (af_on && btif_fm_data.af_cb.af_avail &&
        ((p_af_param = GKI_getbuf(sizeof(tBTA_FM_AF_PARAM))) != NULL))
    {
        p_af_param->af_thresh = (UINT8) BTIF_FM_AF_THRESH;
        p_af_param->pi_code = btif_fm_data.af_cb.pi_code;
        memcpy(&p_af_param->af_list, &btif_fm_data.af_cb.af_list, sizeof(tBTA_FM_AF_LIST));
        while(i < p_af_param->af_list.num_af)
        {
            BTIF_TRACE_DEBUG1(">>>>> pi_code [%d]", p_af_param->pi_code);
            BTIF_TRACE_DEBUG2(">>>>> AF[%d] [%d]", i+1, p_af_param->af_list.af_list[i]);
            i++;
        }
    }

    /* enable RDS data polling with once every 3 seconds */
    BTA_FmSetRDSMode(rds_on, af_on, p_af_param);

    if (p_af_param)
        GKI_freebuf((void *)p_af_param);
}

/*******************************************************************************
**
** Function         btif_fm_clear_af_info
**
** Description      Reset AF information if a new frequency is set.
**
** Returns          void
*******************************************************************************/
static void btif_fm_clear_af_info(void)
{
    BOOLEAN af_on = btif_fm_data.af_cb.af_on ;

    memset(&btif_fm_data.af_cb, 0, sizeof(tBTIF_FM_AF_CB));
    if ((btif_fm_data.af_cb.af_on = af_on) == TRUE)
        btif_fm_set_rds_mode_int(btif_fm_data.rds_on, btif_fm_data.af_cb.af_on);

}



static void btif_fm_upstreams_evt(UINT16 event, char* p_evt_data)
{
    tBTA_FM *p_fm_evt_data = (tBTA_FM *)p_evt_data;

    switch (event)
    {
        case BTA_FM_ENABLE_EVT:
            APPL_TRACE_DEBUG1("%s: event= BTA_FM_ENABLE_EVT", __func__);
            // wait 150 ms for the FM engine to stabilize in stress situation
            // TODO Fine tune
            usleep(150000);
            memset(&btif_fm_data, 0, sizeof(tBTIF_FM_Data));
            HAL_CBACK(bt_fm_callbacks, enable_cb, p_fm_evt_data->status);
            break;
        case BTA_FM_TUNE_EVT:
            APPL_TRACE_DEBUG4("%s: BTA_FM_TUNE_EVT status = %i rssi = %i freq = %i ",
                 __func__, (int)p_fm_evt_data->chnl_info.status, (int)p_fm_evt_data->chnl_info.rssi,
                 (int)p_fm_evt_data->chnl_info.freq);

            btif_fm_data.search_in_progress = FALSE;
            btif_fm_clear_af_info();

            if (btif_fm_data.rds_on)
            {
                // Reset the RDS engine
                bta_fm_co_reset_rds_engine();
            }

            /* keep track of current radio frequency as required by AF for example */
            btif_fm_data.cur_freq = p_fm_evt_data->chnl_info.freq;
            HAL_CBACK(bt_fm_callbacks, tune_cb,
                p_fm_evt_data->status, p_fm_evt_data->chnl_info.rssi,
                p_fm_evt_data->chnl_info.snr, p_fm_evt_data->chnl_info.freq);
            break;
        case BTA_FM_DISABLE_EVT:
            APPL_TRACE_DEBUG1("%s: event= BTA_FM_DISABLE_EVT", __func__);
            // TODO Fine tune
            usleep(300000);
            HAL_CBACK(bt_fm_callbacks, disable_cb, p_fm_evt_data->status);
            break;

        case BTA_FM_AF_JMP_EVT:
            APPL_TRACE_DEBUG4("%s: BTA_FM_AF_JMP_EVT status = %i rssi = %i freq = %i ",
                 __func__, (int)p_fm_evt_data->chnl_info.status, (int)p_fm_evt_data->chnl_info.rssi,
                 (int)p_fm_evt_data->chnl_info.freq);
            /* keep track of current radio frequency as required by AF */
            btif_fm_data.cur_freq = p_fm_evt_data->chnl_info.freq;
            HAL_CBACK(bt_fm_callbacks, af_jump_cb,
                p_fm_evt_data->status, p_fm_evt_data->chnl_info.rssi,
                p_fm_evt_data->chnl_info.snr, p_fm_evt_data->chnl_info.freq);
            break;

        case BTA_FM_SEARCH_CMPL_EVT:
            APPL_TRACE_DEBUG4("%s: BTA_FM_SEARCH_CMPL_EVT status = %i rssi = %i freq = %i ",
                 __func__, (int)p_fm_evt_data->chnl_info.status, (int)p_fm_evt_data->chnl_info.rssi,
                 (int)p_fm_evt_data->chnl_info.freq);

            btif_fm_data.search_in_progress = FALSE;
            btif_fm_clear_af_info();
            if (btif_fm_data.rds_on)
            {
                bta_fm_co_reset_rds_engine();
            }
            btif_fm_data.cur_freq = p_fm_evt_data->chnl_info.freq;

            HAL_CBACK(bt_fm_callbacks, search_complete_cb,
                p_fm_evt_data->status, p_fm_evt_data->chnl_info.rssi,
                p_fm_evt_data->chnl_info.snr, p_fm_evt_data->chnl_info.freq);
            break;

        case BTA_FM_SEARCH_EVT:
            APPL_TRACE_DEBUG4("%s: BTA_FM_SEARCH_EVT rssi = %i snr = %i freq = %i ",
                 __func__, (int)p_fm_evt_data->scan_data.rssi, (int)p_fm_evt_data->scan_data.snr,
                 (int)p_fm_evt_data->scan_data.freq);

            btif_fm_data.search_in_progress = FALSE;
            btif_fm_clear_af_info();

            btif_fm_data.cur_freq = p_fm_evt_data->scan_data.freq;

            //No status sent from stack so treat the event received as BTA_FM_OK
            HAL_CBACK(bt_fm_callbacks, search_cb,
                BTA_FM_OK, p_fm_evt_data->scan_data.rssi,
                p_fm_evt_data->scan_data.snr, p_fm_evt_data->scan_data.freq);
            break;

        case BTA_FM_AUD_MODE_EVT:
            APPL_TRACE_DEBUG2("%s: event= BTA_FM_AUD_MODE_EVT,audio_mode = %d",
                __func__, p_fm_evt_data->mode_info.audio_mode);

            btif_fm_data.mode = p_fm_evt_data->mode_info.audio_mode;
            HAL_CBACK(bt_fm_callbacks, audio_mode_cb, p_fm_evt_data->status,
                p_fm_evt_data->mode_info.audio_mode);
            break;

        case BTA_FM_RDS_UPD_EVT:
            HAL_CBACK(bt_fm_callbacks, rds_data_cb,
                p_fm_evt_data->status, p_fm_evt_data->rds_update.data,
                p_fm_evt_data->rds_update.index, p_fm_evt_data->rds_update.text);
            break;

        case BTA_RDS_RTP_EVT:
            HAL_CBACK(bt_fm_callbacks, rtp_data_cb, (btfm_rds_rtp_info_t *)p_fm_evt_data);
            break;

        case BTA_FM_AUD_DATA_EVT:
            APPL_TRACE_DEBUG4("%s: BTA_FM_SEARCH_EVT status = %i rssi = %i freq = %i ",
                 __func__, (int)p_fm_evt_data->audio_data.status, (int)p_fm_evt_data->audio_data.rssi,
                 (int)p_fm_evt_data->chnl_info.freq);

            if (btif_fm_data.search_in_progress == FALSE)
            {
                HAL_CBACK(bt_fm_callbacks, audio_data_cb,
                    p_fm_evt_data->audio_data.status, p_fm_evt_data->audio_data.rssi,
                    p_fm_evt_data->audio_data.snr, p_fm_evt_data->audio_data.audio_mode);
            }
            break;

        case BTA_FM_AUD_PATH_EVT:
            APPL_TRACE_DEBUG3("%s: BTA_FM_AUD_PATH_EVT, bta audio_path: %d, status: %d",
                  __func__, p_fm_evt_data->path_info.audio_path,
                  p_fm_evt_data->path_info.status);
            btif_fm_data.current_bta_path = p_fm_evt_data->path_info.audio_path;
            HAL_CBACK(bt_fm_callbacks, audio_path_cb, p_fm_evt_data->status,
                p_fm_evt_data->path_info.audio_path);
            break;

        case BTA_FM_RDS_MODE_EVT:
            APPL_TRACE_DEBUG4("%s: BTA_FM_RDS_MODE_EVT: status=%d, RDS=%s, AF=%s", __func__,
                  (UINT8)(p_fm_evt_data->rds_mode.status),
                  p_fm_evt_data->rds_mode.rds_on ? "TRUE" : "FALSE",
                  p_fm_evt_data->rds_mode.af_on  ? "TRUE" : "FALSE");

            btif_fm_data.af_cb.af_on = p_fm_evt_data->rds_mode.af_on;
            if (btif_fm_data.rds_on != p_fm_evt_data->rds_mode.rds_on)
            {
                btif_fm_data.rds_on = p_fm_evt_data->rds_mode.rds_on;
            }
            HAL_CBACK(bt_fm_callbacks,rds_mode_cb,p_fm_evt_data->rds_mode.status,
                p_fm_evt_data->rds_mode.rds_on, p_fm_evt_data->rds_mode.af_on);
            break;

        case BTA_FM_SET_DEEMPH_EVT:
            APPL_TRACE_DEBUG1("%s: BTA_FM_SET_DEEMPH_EVT", __func__);
            HAL_CBACK(bt_fm_callbacks, deemphasis_cb, p_fm_evt_data->deemphasis.status,
                p_fm_evt_data->deemphasis.time_const);
            break;

        case BTA_FM_MUTE_AUD_EVT:
            APPL_TRACE_DEBUG1("%s: BTA_FM_MUTE_AUD_EVT", __func__);

            btif_fm_data.audio_mute = p_fm_evt_data->mute_stat.is_mute;
            HAL_CBACK(bt_fm_callbacks, deemphasis_cb, p_fm_evt_data->mute_stat.status,
                p_fm_evt_data->mute_stat.is_mute);
            break;

        case BTA_FM_SCAN_STEP_EVT:
            APPL_TRACE_DEBUG1("%s: BTA_FM_SCAN_STEP_EVT", __func__);
            btif_fm_data.scan_step = p_fm_evt_data->scan_step ;
            HAL_CBACK(bt_fm_callbacks, scan_step_cb, p_fm_evt_data->status,
                 p_fm_evt_data->scan_step);
            break;

        case BTA_FM_SET_REGION_EVT:
            APPL_TRACE_DEBUG1("%s: BTA_FM_SET_REGION_EVT", __func__);
            HAL_CBACK(bt_fm_callbacks, region_cb, p_fm_evt_data->region_info.status,
                 p_fm_evt_data->region_info.region);
            break;

        case BTA_FM_NFL_EVT:
            APPL_TRACE_DEBUG2("%s: BTA_FM_NFL_EVT: NFL = %d", __func__, (int)p_fm_evt_data->nfloor.rssi);
            HAL_CBACK(bt_fm_callbacks, nfl_cb, p_fm_evt_data->region_info.status,
                 p_fm_evt_data->region_info.region);
            break;

        case BTA_FM_RDS_TYPE_EVT:
            APPL_TRACE_DEBUG3("%s: BTA_FM_RDS_TYPE_EVT: status=%d rds_type=0x%x", __func__,
                  (UINT8)p_fm_evt_data->rds_type.status, (UINT8)p_fm_evt_data->rds_type.type);
            HAL_CBACK(bt_fm_callbacks, rds_type_cb, p_fm_evt_data->rds_type.status,
                 p_fm_evt_data->rds_type.type);
            break;

        case BTA_FM_VOLUME_EVT:
            {
             APPL_TRACE_DEBUG3("%s: BTA_FM_VOLUME_EVT: status = %d, volume = %d", __func__,
                  p_fm_evt_data->volume.status, p_fm_evt_data->volume.volume);

            HAL_CBACK(bt_fm_callbacks, volume_cb, p_fm_evt_data->volume.status,
                         p_fm_evt_data->volume.volume);

            }
            break;

        default:
            APPL_TRACE_DEBUG2("%s: Unhandled event %i", __func__, event);
            break;
    }
}

/*******************************************************************************
**
** Function         btif_fm_cback
**
** Description      transfer context for processing  the events from FM
**
** Returns          void
*******************************************************************************/
static void btif_fm_cback(tBTA_FM_EVT event, tBTA_FM *p_data)
{
    bt_status_t status;
    status = btif_transfer_context(btif_fm_upstreams_evt, (UINT16) event,
        (void*)p_data, sizeof(tBTA_FM), NULL);
    ASSERTC(status == BT_STATUS_SUCCESS, "Context transfer failed!", status);
}


/**
 * Opens the interface and provides the callback routines
 * to the implemenation of this interface.
 */
static int btif_fm_init (btfm_callbacks_t* callbacks )
{
    BTIF_TRACE_EVENT1("%s", __func__);
    bt_fm_callbacks = callbacks;
    return BTA_SUCCESS;
}

/** Enable Fm. */
int btif_fm_enable (int functionalityMask)
{
    CHECK_BTFM_INIT();
    BTA_FmEnable(functionalityMask, btif_fm_cback, BTIF_RDS_APP_ID);
    return BTA_SUCCESS;
}


/** Tune Fm. */
static int btif_fm_tune(int freq)
{
    CHECK_BTFM_INIT();
    btif_fm_data.search_in_progress = TRUE;

    BTA_FmTuneFreq(freq);
    return BTA_SUCCESS;
}


/** Mute/unmute Fm. */
static int btif_fm_mute(BOOLEAN mute)
{
    CHECK_BTFM_INIT();
    BTA_FmMute(mute);
    return BTA_SUCCESS;
}


/** Search Fm. */
static int btif_fm_search(int scanMode, int rssiThresh, int condType, int CondValue)
{
    tBTA_FM_SCH_RDS_COND temp_cond;
    CHECK_BTFM_INIT();

    temp_cond.cond_type = condType;
    temp_cond.cond_val = CondValue;
    if ((0 == temp_cond.cond_type) && (0 == temp_cond.cond_val))
    {
        BTA_FmSearchFreq(scanMode,
                rssiThresh, NULL);
    }
    else
    {
        BTA_FmSearchFreq(scanMode,
                rssiThresh, (tBTA_FM_SCH_RDS_COND*) (&temp_cond));
    }
    return BTA_SUCCESS;
}


/** Search Fm.with frequency wrap */
static int btif_fm_combo_search(int startFreq, int endFreq, int rssiThresh, int direction, int scanMode,
                        BOOLEAN multiChannel, int condType, int condValue)
{
    tBTA_FM_SCH_RDS_COND temp_cond;
    CHECK_BTFM_INIT();

    temp_cond.cond_type = condType;
    temp_cond.cond_val = condValue;

    if ((0 == temp_cond.cond_type) && (0 == temp_cond.cond_val))
    {
         BTA_FmComboSearch(startFreq, endFreq,
             rssiThresh, direction, scanMode,
             multiChannel, NULL);
    }
    else
    {
        BTA_FmComboSearch(startFreq, endFreq,
            rssiThresh, direction, scanMode,
            multiChannel, (&temp_cond));
    }
    btif_fm_data.search_in_progress= TRUE;
    return BTA_SUCCESS;
}


/** Abort Search for Fm. */
static int btif_fm_search_abort()
{
    CHECK_BTFM_INIT();

    BTIF_TRACE_EVENT2("%s btif_fm_data.Current search_in_progress: %d",
        __func__,btif_fm_data.search_in_progress);
    btif_fm_data.search_in_progress = FALSE;
    /* Abort search for station. */
    BTA_FmSearchAbort();

    return BTA_SUCCESS;
}


/** Enable rds/af for Fm. */
static int btif_fm_set_rds_mode(BOOLEAN rdsOn, BOOLEAN afOn)
{
    tBTA_FM_AF_PARAM *p_af_param = NULL;

    CHECK_BTFM_INIT();
    btif_fm_data.rds_on = rdsOn;
    btif_fm_data.af_cb.af_on = afOn;

    btif_fm_set_rds_mode_int(rdsOn, afOn);
    return BTA_SUCCESS;
}


/** Set Rds type. */
static int btif_fm_set_rds_type(int rdsType)
{
    CHECK_BTFM_INIT();

    BTA_FmSetRdsRbds(rdsType);
    return BTA_SUCCESS;
}


/** Set audio mode */
static int btif_fm_set_audio_mode(int audioMode)
{
    CHECK_BTFM_INIT();

    BTA_FmSetAudioMode(audioMode);
    return BTA_SUCCESS;
}

/** Set audio path. */
static int btif_fm_set_audio_path(int audioPath)
{
    CHECK_BTFM_INIT();
    APPL_TRACE_DEBUG2(
        "btif_fm_set_audio_path(requested audioPath: %d) , current_bta_path: %d",
                       audioPath, btif_fm_data.current_bta_path );

    BTA_FmConfigAudioPath((tBTA_FM_AUDIO_PATH)audioPath);
    return BTA_SUCCESS;
}


/** Set Fm region */
static int btif_fm_set_region(int regionType)
{
    CHECK_BTFM_INIT();

    BTA_FmSetRegion(regionType);
    return BTA_SUCCESS;
}

/** Set Fm region */
static int btif_fm_set_scan_step(int scanStep)
{
    CHECK_BTFM_INIT();

    BTA_FmSetScanStep(scanStep);
    return BTA_SUCCESS;
}

/** Config de-emphasis param. */
static int btif_fm_config_deemphasis(int timeType)
{
    CHECK_BTFM_INIT();

    BTA_FmConfigDeemphasis(timeType);
    return BTA_SUCCESS;
}


/** Estimate noise floor. */
static int btif_fm_estimate_noise_floor(int level)
{
    CHECK_BTFM_INIT();

    BTA_FmEstNoiseFloor(level);
    return BTA_SUCCESS;
}


/** Reads audio quality of current station .Turn audio date live update on/off */
static int btif_fm_read_audio_quality(BOOLEAN turnOn)
{
    CHECK_BTFM_INIT();

    BTA_FmReadAudioQuality(turnOn);
    return BTA_SUCCESS;
}


/** Used to configure RSSI value polling interval */
static int btif_fm_config_signal_notification(int time)
{
    CHECK_BTFM_INIT();

    BTA_FmSetSignalNotifSetting(time);
    return BTA_SUCCESS;
}


/** Set fm volume. */
static int btif_fm_set_volume(int volume)
{
    CHECK_BTFM_INIT();

    APPL_TRACE_DEBUG2( "btif_fm_volume_control(volume: %d) on current_bta_path: %d",
                       volume, btif_fm_data.current_bta_path );

    BTA_FmVolumeControl(volume);

    return BTA_SUCCESS;
}


/** Set SNR thresh for search. */
static int btif_fm_set_search_criteria(int value)
{
    tBTA_FM_CVALUE      criteria;
    CHECK_BTFM_INIT();

    criteria.snr = value;

    return BTA_FmSetSchCriteria(BTA_FM_CTYPE_SNR, &criteria);
}


/** Disable Fm. */
static int btif_fm_disable(void)
{
    CHECK_BTFM_INIT();

    /* Disable FM */
    memset(&btif_fm_data, 0, sizeof(tBTIF_FM_Data));

    BTA_FmDisable();

    return BTA_SUCCESS;

}


/** Closes the interface. */
static void btif_fm_cleanup(void)
{
    BTIF_TRACE_EVENT2("%s bt_fm_callbacks: %d", __func__,bt_fm_callbacks);

    bt_fm_callbacks = NULL;
}


static const btfm_interface_t btfmInterface = {
    sizeof(btfmInterface),
    btif_fm_init,
    btif_fm_enable,
    btif_fm_tune,
    btif_fm_mute,
    btif_fm_search,
    btif_fm_combo_search,
    btif_fm_search_abort,
    btif_fm_set_rds_mode,
    btif_fm_set_rds_type,
    btif_fm_set_audio_mode,
    btif_fm_set_audio_path,
    btif_fm_set_region,
    btif_fm_set_scan_step,
    btif_fm_config_deemphasis,
    btif_fm_estimate_noise_floor,
    btif_fm_read_audio_quality,
    btif_fm_config_signal_notification,
    btif_fm_set_volume,
    btif_fm_set_search_criteria,
    btif_fm_disable,
    btif_fm_cleanup
};

/*******************************************************************************
**
** Function         btif_fm_get_interface
**
** Description      Get the fm callback interface
**
** Returns          btfm_interface_t
**
*******************************************************************************/
const btfm_interface_t *btif_fm_get_interface()
{
    BTIF_TRACE_EVENT1("%s", __func__);
    return &btfmInterface;
}


