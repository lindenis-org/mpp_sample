/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file sample_ai_aenc_mux_ao.cpp
 * @brief 该目录是对mpp中的 AI+AENC+MUX+AO 通路sample
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
#include <pthread.h>

#include "common.h"
#include "menu.h"
#include "mpp_comm.h"
#include "mpp_menu.h"


/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
#define RECORDE_PATH_DIR   "/tmp"
#define RECORDE_FILE_MP3_0 "/tmp/ch0_0.mp3"
#define RECORDE_FILE_MP3_1 "/tmp/ch0_1.mp3"

#define RECORDE_PATH_DIR   "/tmp"
#define RECORDE_FILE_AAC_0 "/tmp/ch0_0.aac"
#define RECORDE_FILE_AAC_1 "/tmp/ch0_1.aac"

#define AUDIO_RECORDE_TIME 30 /* audio recorde 30s */


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
typedef struct tag_AUDIO_PROC_CFG_S {
    pthread_t       thd_id;
    int run_flag;
    int audio_dev;
    int audio_chn;
    int record_time;
    char file_name[128];
} AUDIO_PROC_CFG_S;


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static AUDIO_PROC_CFG_S g_audio_cfg;

static MENU_INODE g_menu_top[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"AUDIO control",   NULL,   NULL, NULL},
    {(char *)"Quite ai+aenc+mux+ao sample",  ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
static void MenuCtrl(void)
{
    int  ret = 0;

    ret = mpp_menu_audio_get(&g_menu_top[0].subMenu);
    if (ret) {
        ERR_PRT("Do mpp_menu_audio_get fail! ret:%d\n", ret);
    }

    RunMenuCtrl(g_menu_top);
}


static int components_bind(void)
{
    int ret = 0;

    ret = mpp_comm_ai_bind_aenc(AI_CHN_0, AENC_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_ai_bind_aenc fail! ret:%d \n", ret);
        return -1;
    }

    ret = mpp_comm_ai_bind_ao(AI_CHN_1, AO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_ai_bind_ao fail! ret:%d \n", ret);
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

    ret = mpp_comm_ai_unbind_aenc(AI_CHN_0, AENC_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_ai_unbind_aenc fail! ret:%d \n", ret);
        return -1;
    }

    ret = mpp_comm_ai_unbind_ao(AI_CHN_1, AO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_ai_unbind_ao fail! ret:%d \n", ret);
        return -1;
    }

    ret = mpp_comm_ao_unbind_clock(CLOCK_CHN_0, AO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_ao_unbind_clock fail! ret:%d \n", ret);
        return -1;
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

    ret = AW_MPI_AI_EnableChn(AI_DEV_0, AI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_AI_EnableChn fail! ret:%d\n", ret);
        return -1;
    }

    ret = AW_MPI_AI_EnableChn(AI_DEV_0, AI_CHN_1);
    if (ret) {
        ERR_PRT("Do AW_MPI_AI_EnableChn fail! ret:%d\n", ret);
        return -1;
    }

    ret = AW_MPI_AENC_StartRecvPcm(AENC_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_AENC_StartRecvPcm fail! ret:%d\n", ret);
        return -1;
    }

    ret = AW_MPI_AO_StartChn(AO_DEV_0, AO_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_AO_EnableChn fail! ret:%d\n", ret);
        return -1;
    }

    return 0;
}


static int components_stop(void)
{
    int ret = 0;

    ret = AW_MPI_AENC_StopRecvPcm(AENC_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_AENC_StopRecvPcm fail! ret:%d\n", ret);
    }

    ret = AW_MPI_AO_StopChn(AO_DEV_0, AO_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_AO_StopChn fail! ret:%d\n", ret);
    }

    ret = AW_MPI_AI_DisableChn(AI_DEV_0, AI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_AI_DisableChn fail! ret:%d\n", ret);
    }
    ret = AW_MPI_AI_DisableChn(AI_DEV_0, AI_CHN_1);
    if (ret) {
        ERR_PRT("Do AW_MPI_AI_DisableChn fail! ret:%d\n", ret);
    }

    ret = AW_MPI_CLOCK_Stop(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_CLOCK_Stop fail! ret:%d\n", ret);
    }

    return 0;
}


static int audio_record(AENC_CHN aenc_chn, const char *file_name, int recorde_time)
{
    int   ret = 0;
    FILE *fp  = NULL;
    AUDIO_STREAM_S stream;
    struct timeval tv_begin, tv_tmp;

    fp = fopen(file_name, "wb");
    if (NULL == fp) {
        ERR_PRT("Open %s file error! errno[%d] errinfo[%s] \n",
                file_name, errno, strerror(errno));
        return -1;
    }

    gettimeofday(&tv_begin, NULL);
    while (1) {
        ret = AW_MPI_AENC_GetStream(aenc_chn, &stream, 100);
        if (!ret) {
            fwrite(stream.pStream, 1, stream.mLen, fp);
            AW_MPI_AENC_ReleaseStream(aenc_chn, &stream);
        }

        gettimeofday(&tv_tmp, NULL);
        if ((tv_tmp.tv_sec - tv_begin.tv_sec) > recorde_time) {
            DB_PRT("The file:%s audio recorde end!\n", file_name);
            break;
        }
    }

    fflush(fp);
    fclose(fp);
    fp = NULL;

    return 0;
}


static void *audio_record_proc(void *param)
{
    int   ret = 0;
    FILE *fp  = NULL;
    AUDIO_STREAM_S stream;
    AUDIO_PROC_CFG_S *p_audio_cfg;

    if (NULL == param) {
        ERR_PRT("Input param is NULL!\n");
    }

    p_audio_cfg = (AUDIO_PROC_CFG_S *)param;

    fp = fopen(p_audio_cfg->file_name, "wb");
    if (NULL == fp) {
        ERR_PRT("Open %s file error! errno[%d] errinfo[%s] \n",
                p_audio_cfg->file_name, errno, strerror(errno));
        return NULL;
    }

    while (p_audio_cfg->run_flag) {
        ret = AW_MPI_AENC_GetStream(p_audio_cfg->audio_chn, &stream, 100);
        if (!ret) {
            fwrite(stream.pStream, 1, stream.mLen, fp);
            AW_MPI_AENC_ReleaseStream(p_audio_cfg->audio_chn, &stream);
        }
    }

    fflush(fp);
    fclose(fp);
    fp = NULL;

    DB_PRT("Return audio record process thread...\n");
    return NULL;
}


#ifdef SAMPLE_MODE
int main()
#else
int SampleAiAencMuxAo(void *pData, char *pTitle)
#endif
{
    int ret = 0;
    AUDIO_CFG_S audio_cfg;

    DB_PRT("Do sample_vi_venc start!\n");

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

    /* Step 3. Create ai device and channel. */
    ret = mpp_comm_ai_create(AI_CHN_0, &audio_cfg);
    if (ret) {
        ERR_PRT("Do mpp_comm_ai_create fail! ai_chn:%d ret:%d \n", AI_CHN_0, ret);
        goto _exit_1;
    }
    ret = AW_MPI_AI_CreateChn(AI_DEV_0, AI_CHN_1);

    /* Step 4. Create aenc channel. */
    ret = mpp_comm_aenc_create(AENC_CHN_0, &audio_cfg);
    if (ret) {
        ERR_PRT("Do mpp_comm_aenc_create fail! aenc_chn:%d ret:%d \n", AENC_CHN_0, ret);
        goto _exit_2;
    }

    /* Step 5. Create ao  channel  and clock . */
    ret = mpp_comm_ao_create(AO_CHN_0, &audio_cfg, NULL);
    if (ret) {
        ERR_PRT("Do mpp_comm_ao_create fail! ao_chn:%d ret:%d \n", AO_CHN_0, ret);
        goto _exit_3;
    }
    ret = mpp_comm_ao_clock_create(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_ao_clock_create fail! clock_chn:%d ret:%d \n", CLOCK_CHN_0, ret);
        goto _exit_4;
    }

    /* Setup 6. Bind ai to ao, and clock */
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

    /* Setup 8. Do audio recorde function */
#if 1
    memset(&g_audio_cfg, 0, sizeof(g_audio_cfg));
    g_audio_cfg.run_flag  = 1;
    g_audio_cfg.audio_dev = AI_DEV_0;
    g_audio_cfg.audio_chn = AI_CHN_0;
    strncpy(g_audio_cfg.file_name, RECORDE_FILE_AAC_0, sizeof(g_audio_cfg.file_name));
    ret = pthread_create(&g_audio_cfg.thd_id, NULL, audio_record_proc, &g_audio_cfg);
    if (ret) {
        ERR_PRT("Do pthread_create fail! ret:%d\n", ret);
    }

    MenuCtrl();

    g_audio_cfg.run_flag = 0;
    pthread_join(g_audio_cfg.thd_id, 0);

#else
    printf("\n\n Start audio recorde. aenc_chn:%d  recorde_time:%d  file:%s\n\n",
           AENC_CHN_0, AUDIO_RECORDE_TIME, RECORDE_FILE_AAC_0);
    audio_record(AENC_CHN_0, RECORDE_FILE_AAC_0, AUDIO_RECORDE_TIME);
    printf("\n\n Audio recorde the end ...... \n\n");
#endif

_exit_6:
    components_stop();
_exit_5:
    components_unbind();
_exit_4:
    mpp_comm_ao_clock_destroy(CLOCK_CHN_0);
_exit_3:
    mpp_comm_ao_destroy(AO_CHN_0);
_exit_2:
    mpp_comm_aenc_destroy(AENC_CHN_0);
_exit_1:
    mpp_comm_ai_destroy(AI_CHN_0);
    mpp_comm_ai_destroy(AI_CHN_1);

_exit_0:
    ret = mpp_comm_sys_exit();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_exit fail! ret:%d \n", ret);
    }

    return ret;
}

