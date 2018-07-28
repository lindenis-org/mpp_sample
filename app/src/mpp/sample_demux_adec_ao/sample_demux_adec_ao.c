/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file sample_demux_adec_ao.cpp
 * @brief 该目录是对mpp中的 DEMUX+ADEC+AO 通路sample
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-05-08
 */

/************************************************************************************************/
/*                                      Include Files                                           */
/************************************************************************************************/

#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "mpp_comm.h"


/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
#define DEMUX_FILE_AAC  "/tmp/16k_16bit_mono.aac"


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static MPPCallbackInfo  g_cb_info;
static DEMUX_CB_PARAM_S g_demux_param;


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/

static int components_bind(void)
{
    int ret = 0;

    ret = mpp_comm_demux_bind_adec(DEMUX_CHN_0, ADEC_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_demux_bind_adec fail! ret:%d \n", ret);
        return -1;
    }

    ret = mpp_comm_demux_bind_clock(CLOCK_CHN_0, DEMUX_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_demux_bind_clock fail! ret:%d \n", ret);
        return -1;
    }

    ret = mpp_comm_adec_bind_ao(ADEC_CHN_0, AO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_adec_bind_ao fail! ret:%d \n", ret);
        return -1;
    }

    ret = mpp_comm_ao_bind_clock(CLOCK_CHN_0, AO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_ao_bind_clock fail! ret:%d \n", ret);
        return -1;
    }

    return 0;
}


static int components_unbind(void)
{
    int ret = 0;

    ret = mpp_comm_ao_unbind_clock(CLOCK_CHN_0, AO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_ao_unbind_clock fail! ret:%d \n", ret);
    }

    ret = mpp_comm_adec_unbind_ao(ADEC_CHN_0, AO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_adec_unbind_ao fail! ret:%d \n", ret);
    }

    ret = mpp_comm_demux_unbind_clock(CLOCK_CHN_0, DEMUX_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_demux_unbind_clock fail! ret:%d \n", ret);
    }

    ret = mpp_comm_demux_unbind_adec(DEMUX_CHN_0, ADEC_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_demux_unbind_adec fail! ret:%d \n", ret);
    }

    return 0;
}


static int components_start(void)
{
    int ret = 0;

    ret = AW_MPI_CLOCK_Start(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_CLOCK_Start fail! ret:%d\n", ret);
        return -1;
    }

    ret = AW_MPI_ADEC_StartRecvStream(ADEC_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_ADEC_StartRecvStream fail! ret:%d\n", ret);
        return -1;
    }

    ret = AW_MPI_AO_StartChn(AO_DEV_0, AO_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_AO_EnableChn fail! ret:%d\n", ret);
        return -1;
    }

    ret = AW_MPI_DEMUX_Start(DEMUX_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_DEMUX_Start fail! ret:%d\n", ret);
        return -1;
    }

    return 0;
}


static int components_stop(void)
{
    int ret = 0;

    ret = AW_MPI_DEMUX_Stop(DEMUX_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_DEMUX_Stop fail! ret:%d\n", ret);
    }

    ret = AW_MPI_ADEC_StopRecvStream(ADEC_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_ADEC_StopRecvStream fail! ret:%d\n", ret);
    }

    ret = AW_MPI_AO_StopChn(AO_DEV_0, AO_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_AO_StopChn fail! ret:%d\n", ret);
    }

    ret = AW_MPI_CLOCK_Stop(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_CLOCK_Stop fail! ret:%d\n", ret);
    }

    return 0;
}



#ifdef SAMPLE_MODE
int main()
#else
int SampleDemuxAdecAo(void *pData, char *pTitle)
#endif
{
    int ret = 0;
    AUDIO_CFG_S audio_cfg;

    DB_PRT("Do SampleDemuxAdecAo start!\n");

    memset(&g_cb_info,     0, sizeof(MPPCallbackInfo));
    memset(&g_demux_param, 0, sizeof(DEMUX_CB_PARAM_S));
    g_cb_info.cookie   = &g_demux_param;
    g_cb_info.callback = mpp_comm_audio_demux_callbcak;

    g_demux_param.demux_chn  = DEMUX_CHN_0;
    g_demux_param.demux_type = 1; /* audio demux type */
    g_demux_param.adec_chn   = ADEC_CHN_0;
    g_demux_param.ao_chn     = AO_CHN_0;
    g_demux_param.run_flag   = 1;
    g_demux_param.fd         = -1;

    /* Step 1. Init mpp system */
    ret = mpp_comm_sys_init();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_init fail! ret:%d \n", ret);
        return -1;
    }

    /* Step 2.  Get audio config by type */
    ret = mpp_comm_get_audio_cfg(AUDIO_AAC_16BIT_16K_MONO, &audio_cfg);
    if (ret) {
        ERR_PRT("Do mpp_comm_get_audio_cfg fail! ret:%d \n", ret);
        goto _exit_0;
    }

    /* Step 3. Create audio demux channel. */
    ret = mpp_comm_audio_demux_create(DEMUX_CHN_0, DEMUX_FILE_AAC, &g_cb_info);
    if (ret) {
        ERR_PRT("Do mpp_comm_audio_demux_create file:%s fail! demux_chn:%d ret:%d \n", DEMUX_FILE_AAC, DEMUX_CHN_0, ret);
        goto _exit_1;
    }

    /* Step 4. Create adec channel. */
    ret = mpp_comm_adec_create(ADEC_CHN_0, &audio_cfg, &g_cb_info);
    if (ret) {
        ERR_PRT("Do mpp_comm_adec_create fail! adec_chn:%d ret:%d \n", ADEC_CHN_0, ret);
        goto _exit_2;
    }

    /* Step 5. Create ao  channel  and clock . */
    ret = mpp_comm_ao_create(AO_CHN_0, &audio_cfg, &g_cb_info);
    if (ret) {
        ERR_PRT("Do mpp_comm_ao_create fail! ao_chn:%d ret:%d \n", AO_CHN_0, ret);
        goto _exit_3;
    }
    ret = mpp_comm_ao_clock_create(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_ao_clock_create fail! clock_chn:%d ret:%d \n", CLOCK_CHN_0, ret);
        goto _exit_4;
    }

    /* Setup 6. Bind demux to adec,  ao and clock */
    ret = components_bind();
    if (ret) {
        ERR_PRT("Do components_bind fail! ret:%d \n", ret);
        goto _exit_5;
    }

    /* Setup 7. Start ai  ao and clock */
    ret = components_start();
    if (ret) {
        ERR_PRT("Do components_start fail! ret:%d \n", ret);
        goto _exit_6;
    }

    /* Setup 8. Do this block menu control function */
    AW_MPI_AO_SetDevVolume(AO_DEV_0, 90);
    while (0 != g_demux_param.run_flag) {
        DB_PRT("Running decode audio file ......\n");
        sleep(1);
    }
    DB_PRT("The audio src file:%s decode end!\n", DEMUX_FILE_AAC);

_exit_6:
    components_stop();
_exit_5:
    components_unbind();
_exit_4:
    mpp_comm_ao_clock_destroy(CLOCK_CHN_0);
_exit_3:
    mpp_comm_ao_destroy(AO_CHN_0);
_exit_2:
    mpp_comm_adec_destroy(ADEC_CHN_0);
_exit_1:
    mpp_comm_audio_demux_destroy(DEMUX_CHN_0);
_exit_0:
    ret = mpp_comm_sys_exit();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_exit fail! ret:%d \n", ret);
    }

    return ret;
}

