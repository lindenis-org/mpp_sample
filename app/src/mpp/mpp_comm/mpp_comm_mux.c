/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_comm_mux.c
 * @brief 该目录是对mpp中MUX模块的公共操作,参数设置和获取类型进行简单抽象
 *        封装,以达到提高使用率和减少工作量的目的.
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-04-14
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
/* None */


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static int g_muxer_id = 0;


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/

int mpp_comm_mux_video_full_cfg(PAYLOAD_TYPE_E payload_type, int width, int height, int frm_rate, MUX_GRP_ATTR_S *p_grp_attr)
{
    if (NULL == p_grp_attr) {
        ERR_PRT("Input p_grp_attr is NULL!\n");
        return -1;
    }

    switch (payload_type) {
    case PT_H264:
    case PT_H265:
        p_grp_attr->mVideoEncodeType = payload_type;
        break;

    default:
        ERR_PRT("Input payload_type:%d is not support!\n", payload_type);
        return -1;
    }
    p_grp_attr->mWidth           = width;
    p_grp_attr->mHeight          = height;
    p_grp_attr->mVideoFrmRate    = frm_rate * 1000;
    p_grp_attr->mAudioEncodeType = PT_MAX;

    return 0;
}


int mpp_comm_mux_audio_full_cfg(AUDIO_CFG_TYPE_E audio_cfg_type, MUX_GRP_ATTR_S *p_grp_attr)
{
    if (NULL == p_grp_attr) {
        ERR_PRT("Input p_grp_attr is NULL!\n");
        return -1;
    }

    switch (audio_cfg_type) {
    case AUDIO_AAC_16BIT_8K_MONO:
        p_grp_attr->mChannels         = 1;
        p_grp_attr->mBitsPerSample    = 16;
        p_grp_attr->mSampleRate       = 8000;
        p_grp_attr->mAudioEncodeType  = PT_AAC;
        p_grp_attr->mSamplesPerFrame  = 1024;
        break;

    case AUDIO_AAC_16BIT_16K_MONO:
        p_grp_attr->mChannels         = 1;
        p_grp_attr->mBitsPerSample    = 16;
        p_grp_attr->mSampleRate       = 16000;
        p_grp_attr->mAudioEncodeType  = PT_AAC;
        p_grp_attr->mSamplesPerFrame  = 1024;
        break;

    case AUDIO_ADPCM_16BIT_8K_MONO:
        p_grp_attr->mChannels         = 1;
        p_grp_attr->mBitsPerSample    = 16;
        p_grp_attr->mSampleRate       = 8000;
        p_grp_attr->mAudioEncodeType  = PT_ADPCMA;
        p_grp_attr->mSamplesPerFrame  = 1024;
        break;

    case AUDIO_ADPCM_16BIT_16K_MONO:
        p_grp_attr->mChannels         = 1;
        p_grp_attr->mBitsPerSample    = 16;
        p_grp_attr->mSampleRate       = 16000;
        p_grp_attr->mAudioEncodeType  = PT_ADPCMA;
        p_grp_attr->mSamplesPerFrame  = 1024;
        break;

    case AUDIO_MP3_16BIT_8K_MONO:
        p_grp_attr->mChannels         = 1;
        p_grp_attr->mBitsPerSample    = 16;
        p_grp_attr->mSampleRate       = 8000;
        p_grp_attr->mAudioEncodeType  = PT_MP3;
        p_grp_attr->mSamplesPerFrame  = 1024;
        break;

    case AUDIO_MP3_16BIT_16K_MONO:
        p_grp_attr->mChannels         = 1;
        p_grp_attr->mBitsPerSample    = 16;
        p_grp_attr->mSampleRate       = 16000;
        p_grp_attr->mAudioEncodeType  = PT_MP3;
        p_grp_attr->mSamplesPerFrame  = 1024;
        break;

    case AUDIO_MP3_16BIT_16K_STEREO:
        p_grp_attr->mChannels         = 2;
        p_grp_attr->mBitsPerSample    = 16;
        p_grp_attr->mSampleRate       = 16000;
        p_grp_attr->mAudioEncodeType  = PT_MP3;
        p_grp_attr->mSamplesPerFrame  = 1024;
        break;

    default:
        ERR_PRT("Input audio_cfg_type:%d is not support!\n", audio_cfg_type);
        return -1;
    }

    return 0;
}


/**
 * @brief  Create mux grp   .guixing
 * @param
 * - mux_grp       input
 * - cb_info          input
 * - p_grp_attr    input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_mux_grp_create(MUX_GRP mux_grp, MUX_GRP_ATTR_S *p_grp_attr, MPPCallbackInfo *cb_info)
{
    int  ret = 0;

    if (mux_grp < 0 || mux_grp >= MUX_MAX_GRP_NUM) {
        ERR_PRT("Input mux_grp:%d is error! 0~%d \n", mux_grp, MUX_MAX_GRP_NUM);
        return -1;
    }

    if (NULL == cb_info || NULL == p_grp_attr) {
        ERR_PRT("Input cb_info:%p  p_grp_attr:%p is null!\n", cb_info, p_grp_attr);
        return -1;
    }

    ret = AW_MPI_MUX_CreateGrp(mux_grp, p_grp_attr);
    if (ERR_MUX_EXIST == ret) {
        ERR_PRT("mux group[%d] is exist, find next!\n", mux_grp);
        return -1;
    } else if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_MUX_CreateGrp mux_grp:%d fail! ret:0x%x\n", mux_grp, ret);
        return -1;
    }
    DB_PRT("Create mux group:%d success!\n", mux_grp);

    ret = AW_MPI_MUX_RegisterCallback(mux_grp, cb_info);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_MUX_RegisterCallback mux_grp:%d fail! ret:0x%x\n", mux_grp, ret);
        return -1;
    }

    return 0;
}


int mpp_comm_mux_grp_destroy(MUX_GRP mux_grp)
{
    int ret = 0;

    if (mux_grp < 0 || mux_grp >= MUX_MAX_GRP_NUM) {
        ERR_PRT("Input mux_grp:%d is error! 0~%d \n", mux_grp, MUX_MAX_GRP_NUM);
        return -1;
    }

    ret = AW_MPI_MUX_DestroyGrp(mux_grp);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_MUX_DestroyGrp fail! mux_grp:%d ret:0x%x \n", mux_grp, ret);
        return ret;
    }

    return ret;
}
int mpp_comm_mux_chn_create(MUX_GRP mux_grp, MUX_CHN mux_chn, MEDIA_FILE_FORMAT_E file_format, const char* file_path)
{
    int ret = 0;
    int fd  = -1;
    MUX_CHN_ATTR_S chn_attr;

    if (mux_grp < 0 || mux_grp >= MUX_MAX_GRP_NUM) {
        ERR_PRT("Input mux_grp:%d is error! 0~%d \n", mux_grp, MUX_MAX_GRP_NUM);
        return -1;
    }

    if (mux_chn < 0 || mux_chn >= MUX_MAX_CHN_NUM) {
        ERR_PRT("Input mux_chn:%d is error! 0~%d \n", mux_chn, MUX_MAX_CHN_NUM);
        return -1;
    }

    if (NULL == file_path) {
        ERR_PRT("Input file_path is NULL!\n");
        return -1;
    }

    fd = open(file_path, O_RDWR | O_CREAT, 0666);
    if (fd <= 0) {
        ERR_PRT("Open %s file error! ret:%d errno[%d] errinfo[%s] \n",
                file_path, fd, errno, strerror(errno));
        return -1;
    }

    memset(&chn_attr, 0, sizeof(MUX_CHN_ATTR_S));
    chn_attr.mMuxerId          = g_muxer_id;
    chn_attr.mMediaFileFormat  = file_format;
    chn_attr.mMaxFileDuration  = MUX_FILE_DURATION;     //unit:ms
    chn_attr.mMaxFileSizeBytes = MUX_MAX_FILE_SIZE;     //unit:byte
    chn_attr.mFallocateLen     = MUX_FILE_ALLOCATE_LEN;
    chn_attr.mCallbackOutFlag  = MUX_CALL_BACK_FLAG;
    chn_attr.mFsWriteMode      = FSWRITEMODE_SIMPLECACHE;
    chn_attr.mSimpleCacheSize  = MUX_CACHE_SIZE;

    /* In this function , fd will be dup, so user sure be close fd, after function. gui xing wang */
    ret = AW_MPI_MUX_CreateChn(mux_grp, mux_chn, &chn_attr, fd);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_MUX_CreateChn fail! mux_grp:%d ret:0x%x \n", mux_grp, ret);
        close(fd);
        return ret;
    }

    if (fd > 0) {
        close(fd);
        fd = -1;
    }

    g_muxer_id++;

    return 0;
}


int mpp_comm_mux_chn_destroy(MUX_GRP mux_grp, MUX_CHN mux_chn)
{
    int ret = 0;

    if (mux_grp < 0 || mux_grp >= MUX_MAX_GRP_NUM) {
        ERR_PRT("Input mux_grp:%d is error! 0~%d \n", mux_grp, MUX_MAX_GRP_NUM);
        return -1;
    }

    if (mux_chn < 0 || mux_chn >= MUX_MAX_CHN_NUM) {
        ERR_PRT("Input mux_chn:%d is error! 0~%d \n", mux_chn, MUX_MAX_CHN_NUM);
        return -1;
    }

    ret = AW_MPI_MUX_DestroyChn(mux_grp, mux_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_MUX_DestroyGrp fail! mux_grp:%d mux_chn:%d ret:0x%x\n", mux_grp, mux_chn, ret);
        return ret;
    }

    return ret;
}


ERRORTYPE mpp_comm_mux_callbcak(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    int ret = 0;
    int fd  = -1;
    char file_name[256] = {0};
    MUX_CB_PARAM_S *p_param;

    if (NULL == cookie || NULL == pChn || NULL == pEventData) {
        DB_PRT("Input cookie:%p  pChn:%p  pEventData:%p is NULL!\n", cookie, pChn, pEventData);
        //return SUCCESS;
    }

    if(MOD_ID_VENC == pChn->mModId) {
        DB_PRT("MOD_ID_VENC  -- event:%d\n", event);
        switch(event) {
        case MPP_EVENT_RELEASE_VIDEO_BUFFER: {
            break;
        }
        default: {
            break;
        }
        }
    } else if(MOD_ID_MUX == pChn->mModId) {
        p_param = (MUX_CB_PARAM_S *)cookie;

        switch(event) {
        case MPP_EVENT_RECORD_DONE: {
            DB_PRT("MOD_ID_MUX  File is done!\n");
            break;
        }
        case MPP_EVENT_NEED_NEXT_FD: {
            p_param->file_cnt++;
            DB_PRT("MOD_ID_MUX  -- MPP_EVENT_NEED_NEXT_FD  mux_grp:%d mux_chn:%d  ... ... ... ...\n",
                   p_param->grp_id, p_param->chn_id);
            sprintf(file_name, "%s/venc%d_grp%d_ch%d_%d.mp4",
                    p_param->file_dir, p_param->venc_id, p_param->grp_id, p_param->chn_id, p_param->file_cnt);
            DB_PRT(" New file:%s \n", file_name);

            fd = open(file_name, O_RDWR | O_CREAT, 0666);
            if (fd <= 0) {
                ERR_PRT("Open %s file error! ret:%d errno[%d] errinfo[%s] \n",
                        file_name, fd, errno, strerror(errno));
                return -1;
            }

            ret = AW_MPI_MUX_SwitchFd(p_param->grp_id, p_param->chn_id, fd, 0);
            if (SUCCESS != ret) {
                ERR_PRT("Do AW_MPI_MUX_SwitchFd fail! mux_grp:%d mux_chn:%d ret:0x%x\n", p_param->grp_id, p_param->chn_id, ret);
            }

            close(fd);
            fd = -1;
            break;
        }
        case MPP_EVENT_BSFRAME_AVAILABLE: {
            DB_PRT("MOD_ID_MUX  -- mux bs frame available  MPP_EVENT_BSFRAME_AVAILABLE \n");
            break;
        }
        default: {
            break;
        }
        }
    }

    return SUCCESS;
}

