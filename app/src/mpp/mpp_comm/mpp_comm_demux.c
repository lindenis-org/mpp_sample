/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_comm_demux.c
 * @brief 该目录是对mpp中DEMUX模块的公共操作,参数设置和获取类型进行简单抽象
 *        封装,以达到提高使用率和减少工作量的目的.
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-05-8
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
#define HLAY(chn, lyl)  (chn*4+lyl)


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
typedef struct tag_DEMUX_CFG_S {
    int demux_fd;
    int width;
    int height;
    DEMUX_MEDIA_INFO_S media_info;
} DEMUX_CFG_S;


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static DEMUX_CFG_S g_demux_cfg[DEMUX_MAX_CHN_NUM] = {0};


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/

#if 0
static int mpp_comm_demux_full_cfg(int fd, DEMUX_CHN_ATTR_S *p_attr)
{
    if (NULL == p_attr) {
        ERR_PRT("Input p_attr is NULL!\n");
        return -1;
    }

    p_attr->mStreamType = STREAMTYPE_LOCALFILE;
    p_attr->mSourceType = SOURCETYPE_FD;
    p_attr->mSourceUrl  = NULL;
    p_attr->mFd         = fd;
    p_attr->mDemuxDisableTrack = DEMUX_DISABLE_SUBTITLE_TRACK|DEMUX_DISABLE_AUDIO_TRACK;

    return 0;
}
#endif

/**
 * @brief  Create demux channel   .guixing
 * @param
 * - demux_chn    input
 * - file_path         input
 * - cb_info            input
 * - demux_info    output
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_demux_create(DEMUX_CHN demux_chn, const char *file_path, MPPCallbackInfo *cb_info, DEMUX_INFO_S *demux_info)
{
    int  ret = 0;
    DEMUX_CHN_ATTR_S demux_attr;

    if (demux_chn < 0 || demux_chn >= DEMUX_MAX_CHN_NUM) {
        ERR_PRT("Input demux_chn:%d is error! 0~%d \n", demux_chn, DEMUX_MAX_CHN_NUM);
        return -1;
    }

    if (NULL == cb_info || NULL == file_path || NULL == demux_info) {
        ERR_PRT("Input cb_info or file_path or demux_info is null!\n");
        return -1;
    }

    if (g_demux_cfg[demux_chn].demux_fd > 0) {
        ERR_PRT("The demux chn:%d file haved opened! error!\n", demux_chn);
        return -1;
    }

    g_demux_cfg[demux_chn].demux_fd = open(file_path, O_RDONLY);
    if (g_demux_cfg[demux_chn].demux_fd <= 0) {
        ERR_PRT("Open %s file error! ret:%d errno[%d] errinfo[%s] \n",
                file_path, g_demux_cfg[demux_chn].demux_fd, errno, strerror(errno));
        return -1;
    }

    memset(&demux_attr, 0, sizeof(DEMUX_CHN_ATTR_S));
    demux_attr.mStreamType = STREAMTYPE_LOCALFILE;
    demux_attr.mSourceType = SOURCETYPE_FD;
    demux_attr.mSourceUrl  = NULL;
    demux_attr.mFd         = g_demux_cfg[demux_chn].demux_fd;
    demux_attr.mDemuxDisableTrack = DEMUX_DISABLE_SUBTITLE_TRACK|DEMUX_DISABLE_AUDIO_TRACK;

    /* Step 1. Create demux channel */
    ret = AW_MPI_DEMUX_CreateChn(demux_chn, &demux_attr);
    if (ERR_DEMUX_EXIST == ret) {
        ERR_PRT("Demux chn[%d] is exist, find next!\n", demux_chn);
        return -1;
    } else if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_DEMUX_CreateChn demux_chn:%d fail! ret:0x%x\n", demux_chn, ret);
        return -1;
    }
    DB_PRT("Create demux_chn:%d success!\n", demux_chn);

    /* Step 2. Register demux call back function for control demux decode file. */
    ret = AW_MPI_DEMUX_RegisterCallback(demux_chn, cb_info);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_DEMUX_RegisterCallback demux_chn:%d fail! ret:0x%x\n", demux_chn, ret);
        return -1;
    }

    /* Step 3. Get demux file information, for setting vdec model. */
    DEMUX_MEDIA_INFO_S media_info;
    ret = AW_MPI_DEMUX_GetMediaInfo(demux_chn, &media_info);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_DEMUX_GetMediaInfo fail! demux_chn:%d  ret:0x%x\n", demux_chn, ret);
        return -1;
    }

    if ((media_info.mVideoNum >0 && media_info.mVideoIndex >= media_info.mVideoNum)
        || (media_info.mAudioNum >0 && media_info.mAudioIndex >= media_info.mAudioNum)
        || (media_info.mSubtitleNum >0 && media_info.mSubtitleIndex >= media_info.mSubtitleNum)) {
        ERR_PRT("fatal error, trackIndex wrong! [%d][%d],[%d][%d],[%d][%d]",
                media_info.mVideoNum, media_info.mVideoIndex, media_info.mAudioNum,
                media_info.mAudioIndex, media_info.mSubtitleNum, media_info.mSubtitleIndex);
        return -1;
    }

    memcpy(&g_demux_cfg[demux_chn].media_info, &media_info, sizeof(DEMUX_MEDIA_INFO_S));
    g_demux_cfg[demux_chn].width  = media_info.mVideoStreamInfo[0].mWidth;
    g_demux_cfg[demux_chn].height = media_info.mVideoStreamInfo[0].mHeight;
    demux_info->width      = media_info.mVideoStreamInfo[0].mWidth;
    demux_info->height     = media_info.mVideoStreamInfo[0].mHeight;
    demux_info->codec_type = media_info.mVideoStreamInfo[0].mCodecType;
    demux_info->frame_rate = media_info.mVideoStreamInfo[0].mFrameRate;

    return 0;
}


/**
 * @brief  Destroy demux channel   .guixing
 * @param
 * - demux_chn     input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_demux_destroy(DEMUX_CHN demux_chn)
{
    int  ret = 0;

    if (demux_chn < 0 || demux_chn >= DEMUX_MAX_CHN_NUM) {
        ERR_PRT("Input demux_chn:%d is error! 0~%d \n", demux_chn, DEMUX_MAX_CHN_NUM);
        return -1;
    }

    ret = AW_MPI_DEMUX_DestroyChn(demux_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_DEMUX_DestroyChn fail! demux_chn:%d  ret:0x%x \n", demux_chn, ret);
        return -1;
    }

    if (g_demux_cfg[demux_chn].demux_fd > 0) {
        close(g_demux_cfg[demux_chn].demux_fd);
        g_demux_cfg[demux_chn].demux_fd = -1;
    }

    return 0;
}


int mpp_comm_demux_bind_vdec(int demux_chn, int vdec_chn)
{
    int       ret = 0;
    MPP_CHN_S DemuxChn, VdecChn;

    DemuxChn.mModId  = MOD_ID_DEMUX;
    DemuxChn.mDevId  = 0;
    DemuxChn.mChnId  = demux_chn;

    VdecChn.mModId = MOD_ID_VDEC;
    VdecChn.mDevId = 0;
    VdecChn.mChnId = vdec_chn;

    ret = AW_MPI_SYS_Bind(&DemuxChn, &VdecChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind demux_chn:%d bind vdec_chn:%d fail! ret:0x%x\n",
                demux_chn, vdec_chn, ret);
        return -1;
    }

    return ret;
}


int mpp_comm_demux_unbind_vdec(int demux_chn, int vdec_chn)
{
    int       ret = 0;
    MPP_CHN_S DemuxChn, VdecChn;

    DemuxChn.mModId  = MOD_ID_DEMUX;
    DemuxChn.mDevId  = 0;
    DemuxChn.mChnId  = demux_chn;

    VdecChn.mModId = MOD_ID_VDEC;
    VdecChn.mDevId = 0;
    VdecChn.mChnId = vdec_chn;

    ret = AW_MPI_SYS_UnBind(&DemuxChn, &VdecChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind demux_chn:%d unbind vdec_chn:%d fail! ret:0x%x\n",
                demux_chn, vdec_chn, ret);
        return -1;
    }

    return ret;
}


int mpp_comm_demux_bind_adec(int demux_chn, int adec_chn)
{
    int       ret = 0;
    MPP_CHN_S DemuxChn, AdecChn;

    DemuxChn.mModId  = MOD_ID_DEMUX;
    DemuxChn.mDevId  = 0;
    DemuxChn.mChnId  = demux_chn;

    AdecChn.mModId = MOD_ID_ADEC;
    AdecChn.mDevId = 0;
    AdecChn.mChnId = adec_chn;

    ret = AW_MPI_SYS_Bind(&DemuxChn, &AdecChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind demux_chn:%d bind adec_chn:%d fail! ret:0x%x\n",
                demux_chn, adec_chn, ret);
        return -1;
    }

    return ret;
}

int mpp_comm_demux_unbind_adec(int demux_chn, int adec_chn)
{
    int       ret = 0;
    MPP_CHN_S DemuxChn, AdecChn;

    DemuxChn.mModId  = MOD_ID_DEMUX;
    DemuxChn.mDevId  = 0;
    DemuxChn.mChnId  = demux_chn;

    AdecChn.mModId = MOD_ID_ADEC;
    AdecChn.mDevId = 0;
    AdecChn.mChnId = adec_chn;

    ret = AW_MPI_SYS_UnBind(&DemuxChn, &AdecChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind demux_chn:%d unbind adec_chn:%d fail! ret:0x%x\n",
                demux_chn, adec_chn, ret);
        return -1;
    }

    return ret;
}


int mpp_comm_demux_bind_clock(int clock_chn, int demux_chn)
{
    int       ret = 0;
    MPP_CHN_S DmxChn, ClockChn;

    ClockChn.mModId = MOD_ID_CLOCK;
    ClockChn.mDevId = 0;
    ClockChn.mChnId = clock_chn;

    DmxChn.mModId  = MOD_ID_DEMUX;
    DmxChn.mDevId  = 0;
    DmxChn.mChnId  = demux_chn;

    ret = AW_MPI_SYS_Bind(&ClockChn, &DmxChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind demux_chn:%d bind clock_chn:%d fail! ret:0x%x\n",
                demux_chn, clock_chn, ret);
        return -1;
    }

    return ret;
}

int mpp_comm_demux_unbind_clock(int demux_chn, int clock_chn)
{
    int       ret = 0;
    MPP_CHN_S DmxChn, ClockChn;

    ClockChn.mModId = MOD_ID_CLOCK;
    ClockChn.mDevId = 0;
    ClockChn.mChnId = clock_chn;

    DmxChn.mModId  = MOD_ID_DEMUX;
    DmxChn.mDevId  = 0;
    DmxChn.mChnId  = demux_chn;

    ret = AW_MPI_SYS_UnBind(&ClockChn, &DmxChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind demux_chn:%d bind clock_chn:%d fail! ret:0x%x\n",
                demux_chn, clock_chn, ret);
        return -1;
    }

    return ret;
}


ERRORTYPE mpp_comm_demux_callbcak(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    int ret = 0;
    VO_LAYER  vo_layer = 0;
    DEMUX_CB_PARAM_S *pdemux_param = NULL;

    if (NULL != cookie) {
        pdemux_param = (DEMUX_CB_PARAM_S *)cookie;
    } else {
        return SUCCESS;
    }

    if (pChn->mModId == MOD_ID_DEMUX) {
        switch (event) {
        case MPP_EVENT_NOTIFY_EOF:
            DB_PRT("demux to end of file.\n");
            if (pdemux_param->vdec_chn >= 0) {
                ret = AW_MPI_VDEC_SetStreamEof(pdemux_param->vdec_chn, 1);
                if (ret) {
                    ERR_PRT("Do AW_MPI_VDEC_SetStreamEof fail! vdec_ch:%d ret:0x%x \n", pdemux_param->vdec_chn, ret);
                }
            }
            break;

        default:
            break;
        }
    } else if (pChn->mModId == MOD_ID_VDEC) {
        switch (event) {
        case MPP_EVENT_NOTIFY_EOF:
            DB_PRT("vdec to the end of file.\n");
            if (pdemux_param->vo_chn >= 0) {
                vo_layer = HLAY(pdemux_param->vo_chn, 0);
                AW_MPI_VO_SetStreamEof(vo_layer, 0, 1);
                pdemux_param->run_flag = 0;
            }
            //cdx_sem_up(&pDemux2Vdec2VoData->mSemExit);
            break;

        default:
            break;
        }
    } else if (pChn->mModId == MOD_ID_VOU) {
        switch (event) {
        case MPP_EVENT_NOTIFY_EOF:
            DB_PRT("vo to the end of file.\n");
            pdemux_param->run_flag = 0;
            break;

        case MPP_EVENT_RENDERING_START:
            DB_PRT("vo start to rendering.\n");
            break;

        default:
            break;
        }
    }

    return SUCCESS;
}

