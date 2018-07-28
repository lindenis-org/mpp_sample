/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     :
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2016/11/4
  Last Modified :
  Description   :
  Function List :
  History       :
******************************************************************************/

#define LOG_NDEBUG 0
#define LOG_TAG "SampleVi2Venc2Muxer"

#include <unistd.h>
#include "plat_log.h"
#include <time.h>
#include <mm_common.h>

#include "mpi_isp.h"

#include "sample_vi2venc2muxer.h"
#include "sample_vi2venc2muxer_conf.h"


#define DEFAULT_SRC_SIZE   1080
#define DEFAULT_DST_VIDEO_FILE      "/mnt/extsd/sample_vi2venc2muxer/1080p.mp4"
#define DEFAULT_SRC_SIZE   1080

#define DEFAULT_MAX_DURATION  60*1000
#define DEFAULT_DST_VIDEO_FRAMERATE 30
#define DEFAULT_DST_VIDEO_BITRATE 12*1000*1000

#define DEFAULT_SRC_PIXFMT   MM_PIXEL_FORMAT_YUV_PLANAR_420
#define DEFAULT_DST_PIXFMT   MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420
#define DEFAULT_ENCODER   PT_H264

#define DEFAULT_SIMPLE_CACHE_SIZE_VFS       (4*1024)


// #define DOUBLE_ENCODER_FILE_OUT


static SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData;


static int setOutputFileSync(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData, char* path, int64_t fallocateLength, int muxerId);


static ERRORTYPE InitVi2Venc2MuxerData(void)
{
    pVi2Venc2MuxerData = (SAMPLE_VI2VENC2MUXER_S* )malloc(sizeof(SAMPLE_VI2VENC2MUXER_S));
    if (pVi2Venc2MuxerData == NULL) {
        aloge("malloc struct fail");
        return FAILURE;
    }

    memset(pVi2Venc2MuxerData, 0, sizeof(SAMPLE_VI2VENC2MUXER_S));

    pVi2Venc2MuxerData->mConfigPara.srcSize = pVi2Venc2MuxerData->mConfigPara.dstSize = DEFAULT_SRC_SIZE;
    if (pVi2Venc2MuxerData->mConfigPara.srcSize == 3840) {
        pVi2Venc2MuxerData->mConfigPara.srcWidth = 3840;
        pVi2Venc2MuxerData->mConfigPara.srcHeight = 2160;
    } else if (pVi2Venc2MuxerData->mConfigPara.srcSize == 1080) {
        pVi2Venc2MuxerData->mConfigPara.srcWidth = 1920;
        pVi2Venc2MuxerData->mConfigPara.srcHeight = 1080;
    } else if (pVi2Venc2MuxerData->mConfigPara.srcSize == 720) {
        pVi2Venc2MuxerData->mConfigPara.srcWidth = 1280;
        pVi2Venc2MuxerData->mConfigPara.srcHeight = 720;
    }

    if (pVi2Venc2MuxerData->mConfigPara.dstSize == 3840) {
        pVi2Venc2MuxerData->mConfigPara.dstWidth = 3840;
        pVi2Venc2MuxerData->mConfigPara.dstHeight = 2160;
    } else if (pVi2Venc2MuxerData->mConfigPara.dstSize == 1080) {
        pVi2Venc2MuxerData->mConfigPara.dstWidth = 1920;
        pVi2Venc2MuxerData->mConfigPara.dstHeight = 1080;
    } else if (pVi2Venc2MuxerData->mConfigPara.dstSize == 720) {
        pVi2Venc2MuxerData->mConfigPara.dstWidth = 1280;
        pVi2Venc2MuxerData->mConfigPara.dstHeight = 720;
    }

    pVi2Venc2MuxerData->mConfigPara.mMaxFileDuration = DEFAULT_MAX_DURATION;
    pVi2Venc2MuxerData->mConfigPara.mVideoFrameRate = DEFAULT_DST_VIDEO_FRAMERATE;
    pVi2Venc2MuxerData->mConfigPara.mVideoBitRate = DEFAULT_DST_VIDEO_BITRATE;

    pVi2Venc2MuxerData->mConfigPara.srcPixFmt = DEFAULT_SRC_PIXFMT;
    pVi2Venc2MuxerData->mConfigPara.dstPixFmt = DEFAULT_DST_PIXFMT;
    pVi2Venc2MuxerData->mConfigPara.mVideoEncoderFmt = DEFAULT_ENCODER;
    pVi2Venc2MuxerData->mConfigPara.mField = VIDEO_FIELD_FRAME;

    pVi2Venc2MuxerData->mConfigPara.mColor2Grey = FALSE;
    pVi2Venc2MuxerData->mConfigPara.m3DNR = FALSE;

    pVi2Venc2MuxerData->mMuxGrp = MM_INVALID_CHN;
    pVi2Venc2MuxerData->mVeChn = MM_INVALID_CHN;
    pVi2Venc2MuxerData->mViChn = MM_INVALID_CHN;
    pVi2Venc2MuxerData->mViDev = MM_INVALID_DEV;

    strcpy(pVi2Venc2MuxerData->mConfigPara.dstVideoFile, DEFAULT_DST_VIDEO_FILE);

    pVi2Venc2MuxerData->mCurrentState = REC_NOT_PREPARED;



    return SUCCESS;
}

static ERRORTYPE parseCmdLine(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData, int argc, char** argv)
{
    ERRORTYPE ret = FAILURE;

    while (*argv) {
        if (!strcmp(*argv, "-path")) {
            argv++;
            if (*argv) {
                ret = SUCCESS;
                if (strlen(*argv) >= MAX_FILE_PATH_LEN) {
                    aloge("fatal error! file path[%s] too long:!", *argv);
                }

                strncpy(pVi2Venc2MuxerData->mCmdLinePara.mConfigFilePath, *argv, MAX_FILE_PATH_LEN-1);
                pVi2Venc2MuxerData->mCmdLinePara.mConfigFilePath[MAX_FILE_PATH_LEN-1] = '\0';
            }
        } else if(!strcmp(*argv, "-h")) {
            printf("CmdLine param:\n"
                   "\t-path /home/sample_vi2venc2muxer.conf\n");
            break;
        } else if (*argv) {
            argv++;
        }
    }

    return ret;
}

static ERRORTYPE loadConfigPara(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData, const char *conf_path)
{
    int ret;
    char *ptr;
    CONFPARSER_S mConf;

    ret = createConfParser(conf_path, &mConf);
    if (ret < 0) {
        aloge("load conf fail");
        return FAILURE;
    }

    pVi2Venc2MuxerData->mConfigPara.mDevNo = GetConfParaInt(&mConf, CFG_SRC_DEV_NODE, 0);

    pVi2Venc2MuxerData->mConfigPara.srcSize = GetConfParaInt(&mConf, CFG_SRC_SIZE, 0);
    if (pVi2Venc2MuxerData->mConfigPara.srcSize == 3840) {
        pVi2Venc2MuxerData->mConfigPara.srcWidth = 3840;
        pVi2Venc2MuxerData->mConfigPara.srcHeight = 2160;
    } else if (pVi2Venc2MuxerData->mConfigPara.srcSize == 1080) {
        pVi2Venc2MuxerData->mConfigPara.srcWidth = 1920;
        pVi2Venc2MuxerData->mConfigPara.srcHeight = 1080;
    } else if (pVi2Venc2MuxerData->mConfigPara.srcSize == 720) {
        pVi2Venc2MuxerData->mConfigPara.srcWidth = 1280;
        pVi2Venc2MuxerData->mConfigPara.srcHeight = 720;
    }
    pVi2Venc2MuxerData->mConfigPara.srcWidth = GetConfParaInt(&mConf, CFG_SRC_WIDTH, 0);
    pVi2Venc2MuxerData->mConfigPara.srcHeight = GetConfParaInt(&mConf, CFG_SRC_HEIGHT, 0);
    pVi2Venc2MuxerData->mConfigPara.mode = GetConfParaInt(&mConf, CFG_STABLE_MODE, 0);

    pVi2Venc2MuxerData->mConfigPara.vsWidth = GetConfParaInt(&mConf, CFG_VS_WIDTH, 0);
    pVi2Venc2MuxerData->mConfigPara.vsHeight = GetConfParaInt(&mConf, CFG_VS_HEIGHT, 0);

    pVi2Venc2MuxerData->mConfigPara.dstSize = GetConfParaInt(&mConf, CFG_DST_VIDEO_SIZE, 0);
    if (pVi2Venc2MuxerData->mConfigPara.dstSize == 3840) {
        pVi2Venc2MuxerData->mConfigPara.dstWidth = 3840;
        pVi2Venc2MuxerData->mConfigPara.dstHeight = 2160;
    } else if (pVi2Venc2MuxerData->mConfigPara.dstSize == 1080) {
        pVi2Venc2MuxerData->mConfigPara.dstWidth = 1920;
        pVi2Venc2MuxerData->mConfigPara.dstHeight = 1080;
    } else if (pVi2Venc2MuxerData->mConfigPara.dstSize == 720) {
        pVi2Venc2MuxerData->mConfigPara.dstWidth = 1280;
        pVi2Venc2MuxerData->mConfigPara.dstHeight = 720;
    }
    pVi2Venc2MuxerData->mConfigPara.dstWidth = GetConfParaInt(&mConf, CFG_DST_WIDTH, 0);
    pVi2Venc2MuxerData->mConfigPara.dstHeight = GetConfParaInt(&mConf, CFG_DST_HEIGHT, 0);
    pVi2Venc2MuxerData->mConfigPara.mVippBufNum = GetConfParaInt(&mConf, CFG_VIPP_BUF_NUM, 5);
    pVi2Venc2MuxerData->mConfigPara.mRq = GetConfParaInt(&mConf, CFG_EIS_RQ, 0);
    pVi2Venc2MuxerData->mConfigPara.mEisInputBufNum = GetConfParaInt(&mConf, CFG_EIS_INPUT_BUF_NUM, 5);
    pVi2Venc2MuxerData->mConfigPara.mEisOutputBufNum = GetConfParaInt(&mConf, CFG_EIS_OUTPUT_BUF_NUM, 10);

    ptr = (char *)GetConfParaString(&mConf, CFG_DST_VIDEO_FILE_STR, NULL);
    strcpy(pVi2Venc2MuxerData->mConfigPara.dstVideoFile, ptr);

    pVi2Venc2MuxerData->mConfigPara.mVideoFrameRate = GetConfParaInt(&mConf, CFG_DST_VIDEO_FRAMERATE, 0);
    pVi2Venc2MuxerData->mConfigPara.mVideoBitRate = GetConfParaInt(&mConf, CFG_DST_VIDEO_BITRATE, 0);
    pVi2Venc2MuxerData->mConfigPara.mMaxFileDuration = GetConfParaInt(&mConf, CFG_DST_VIDEO_DURATION, 0);

    pVi2Venc2MuxerData->mConfigPara.mRcMode = GetConfParaInt(&mConf, CFG_RC_MODE, 0);
    pVi2Venc2MuxerData->mConfigPara.mEnableFastEnc = GetConfParaInt(&mConf, CFG_FAST_ENC, 0);
    pVi2Venc2MuxerData->mConfigPara.mEnableRoi = GetConfParaInt(&mConf, CFG_ROI, 0);

    ptr	= (char *)GetConfParaString(&mConf, CFG_DST_VIDEO_ENCODER, NULL);
    if (!strcmp(ptr, "H.264")) {
        pVi2Venc2MuxerData->mConfigPara.mVideoEncoderFmt = PT_H264;
    } else if (!strcmp(ptr, "H.265")) {
        pVi2Venc2MuxerData->mConfigPara.mVideoEncoderFmt = PT_H265;
    } else if (!strcmp(ptr, "MJPEG")) {
        pVi2Venc2MuxerData->mConfigPara.mVideoEncoderFmt = PT_MJPEG;
    } else {
        aloge("error conf encoder type");
    }

    pVi2Venc2MuxerData->mConfigPara.mTestDuration = GetConfParaInt(&mConf, CFG_TEST_DURATION, 0);

    alogd("dev_node:%d, frame rate:%d, bitrate:%d, video_duration=%d, test_time=%d", pVi2Venc2MuxerData->mConfigPara.mDevNo,\
          pVi2Venc2MuxerData->mConfigPara.mVideoFrameRate, pVi2Venc2MuxerData->mConfigPara.mVideoBitRate,\
          pVi2Venc2MuxerData->mConfigPara.mMaxFileDuration, pVi2Venc2MuxerData->mConfigPara.mTestDuration);

    ptr	= (char *)GetConfParaString(&mConf, CFG_COLOR2GREY, NULL);
    if(!strcmp(ptr, "yes")) {
        pVi2Venc2MuxerData->mConfigPara.mColor2Grey = TRUE;
    } else {
        pVi2Venc2MuxerData->mConfigPara.mColor2Grey = FALSE;
    }

    ptr	= (char *)GetConfParaString(&mConf, CFG_3DNR, NULL);
    if(!strcmp(ptr, "yes")) {
        pVi2Venc2MuxerData->mConfigPara.m3DNR = TRUE;
    } else {
        pVi2Venc2MuxerData->mConfigPara.m3DNR = FALSE;
    }

    destroyConfParser(&mConf);
    return SUCCESS;
}

static unsigned long long GetNowTimeUs(void)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * 1000000 + now.tv_usec;
}

static int getFileNameByCurTime(char *pNameBuf)
{
#if 0
    sprintf(pNameBuf, "%s", "/mnt/extsd/sample_mux/");
    sprintf(pNameBuf, "%s%llud.mp4", pNameBuf, GetNowTimeUs());
#else
    static int file_cnt = 0;
    int len = strlen(pVi2Venc2MuxerData->mConfigPara.dstVideoFile);
    char *ptr = pVi2Venc2MuxerData->mConfigPara.dstVideoFile;
    while (*(ptr+len-1) != '.') {
        len--;
    }

    ++file_cnt;
    strncpy(pNameBuf, pVi2Venc2MuxerData->mConfigPara.dstVideoFile, len-1);
    sprintf(pNameBuf, "%s_%d.mp4", pNameBuf, file_cnt);
#endif
    return 0;
}

static ERRORTYPE MPPCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData = (SAMPLE_VI2VENC2MUXER_S *)cookie;

    if(MOD_ID_VENC == pChn->mModId) {
        switch(event) {
        case MPP_EVENT_RELEASE_VIDEO_BUFFER: {
            break;
        }
        default: {
            break;
        }
        }
    } else if(MOD_ID_MUX == pChn->mModId) {
        switch(event) {
        case MPP_EVENT_RECORD_DONE: {
            int muxerId = *(int*)pEventData;
            alogd("file done, mux_id=%d", muxerId);
            break;
        }
        case MPP_EVENT_NEED_NEXT_FD: {
            int muxerId = *(int*)pEventData;
            SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData = (SAMPLE_VI2VENC2MUXER_S *)cookie;
            char fileName[128] = {0};

            if (muxerId == pVi2Venc2MuxerData->mMuxId[0]) {
                getFileNameByCurTime(fileName);
            }
#ifdef DOUBLE_ENCODER_FILE_OUT
            else if(muxerId == pVi2Venc2MuxerData->mMuxId[1]) {
                strcpy(fileName, "/mnt/extsd/sample_vi2venc2muxer/");
                static int cnt = 0;
                cnt++;
                sprintf(fileName, "%s%d.ts", fileName, cnt);
            }
#endif
            alogd("mux set next fd, filepath=%s", fileName);
            setOutputFileSync(pVi2Venc2MuxerData, fileName, 0, muxerId);
            break;
        }
        case MPP_EVENT_BSFRAME_AVAILABLE: {
            alogd("mux bs frame available");
            break;
        }
        default: {
            break;
        }
        }
    }

    return SUCCESS;
}

static ERRORTYPE configMuxGrpAttr(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData)
{
    memset(&pVi2Venc2MuxerData->mMuxGrpAttr, 0, sizeof(MUX_GRP_ATTR_S));

    pVi2Venc2MuxerData->mMuxGrpAttr.mVideoEncodeType = pVi2Venc2MuxerData->mConfigPara.mVideoEncoderFmt;
    pVi2Venc2MuxerData->mMuxGrpAttr.mWidth = pVi2Venc2MuxerData->mConfigPara.dstWidth;
    pVi2Venc2MuxerData->mMuxGrpAttr.mHeight = pVi2Venc2MuxerData->mConfigPara.dstHeight;
    pVi2Venc2MuxerData->mMuxGrpAttr.mVideoFrmRate = pVi2Venc2MuxerData->mConfigPara.mVideoFrameRate*1000;
    //pVi2Venc2MuxerData->mMuxGrpAttr.mMaxKeyInterval =
    pVi2Venc2MuxerData->mMuxGrpAttr.mAudioEncodeType = PT_MAX;

    return SUCCESS;
}

static ERRORTYPE createMuxGrp(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData)
{
    ERRORTYPE ret;
    BOOL nSuccessFlag = FALSE;

    configMuxGrpAttr(pVi2Venc2MuxerData);
    pVi2Venc2MuxerData->mMuxGrp = 0;
    while (pVi2Venc2MuxerData->mMuxGrp < MUX_MAX_GRP_NUM) {
        ret = AW_MPI_MUX_CreateGrp(pVi2Venc2MuxerData->mMuxGrp, &pVi2Venc2MuxerData->mMuxGrpAttr);
        if (SUCCESS == ret) {
            nSuccessFlag = TRUE;
            alogd("create mux group[%d] success!", pVi2Venc2MuxerData->mMuxGrp);
            break;
        } else if (ERR_MUX_EXIST == ret) {
            alogd("mux group[%d] is exist, find next!", pVi2Venc2MuxerData->mMuxGrp);
            pVi2Venc2MuxerData->mMuxGrp++;
        } else {
            alogd("create mux group[%d] ret[0x%x], find next!", pVi2Venc2MuxerData->mMuxGrp, ret);
            pVi2Venc2MuxerData->mMuxGrp++;
        }
    }

    if (FALSE == nSuccessFlag) {
        pVi2Venc2MuxerData->mMuxGrp = MM_INVALID_CHN;
        aloge("fatal error! create mux group fail!");
        return FAILURE;
    } else {
        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pVi2Venc2MuxerData;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_MUX_RegisterCallback(pVi2Venc2MuxerData->mMuxGrp, &cbInfo);
        return SUCCESS;
    }
}

static int addOutputFormatAndOutputSink_1(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData, OUTSINKINFO_S *pSinkInfo)
{
    int retMuxerId = -1;
    MUX_CHN_INFO_S *pEntry, *pTmp;

    alogd("fmt:0x%x, fd:%d, FallocateLen:%d, callback_out_flag:%d", pSinkInfo->mOutputFormat, pSinkInfo->mOutputFd, pSinkInfo->mFallocateLen, pSinkInfo->mCallbackOutFlag);
    if(pSinkInfo->mOutputFd >= 0 && TRUE == pSinkInfo->mCallbackOutFlag) {
        aloge("fatal error! one muxer cannot support two sink methods!");
        return -1;
    }

    //find if the same output_format sinkInfo exist or callback out stream is exist.
    pthread_mutex_lock(&pVi2Venc2MuxerData->mMuxChnListLock);
    if (!list_empty(&pVi2Venc2MuxerData->mMuxChnList)) {
        list_for_each_entry_safe(pEntry, pTmp, &pVi2Venc2MuxerData->mMuxChnList, mList) {
            if (pEntry->mSinkInfo.mOutputFormat == pSinkInfo->mOutputFormat) {
                alogd("Be careful! same outputForamt[0x%x] exist in array", pSinkInfo->mOutputFormat);
            }
            if (pEntry->mSinkInfo.mCallbackOutFlag == pSinkInfo->mCallbackOutFlag) {
                aloge("fatal error! only support one callback out stream");
            }
        }
    }
    pthread_mutex_unlock(&pVi2Venc2MuxerData->mMuxChnListLock);

    MUX_CHN_INFO_S *p_node = (MUX_CHN_INFO_S *)malloc(sizeof(MUX_CHN_INFO_S));
    if (p_node == NULL) {
        aloge("alloc mux chn info node fail");
        return -1;
    }

    memset(p_node, 0, sizeof(MUX_CHN_INFO_S));
    p_node->mSinkInfo.mMuxerId = pVi2Venc2MuxerData->mMuxerIdCounter;
    p_node->mSinkInfo.mOutputFormat = pSinkInfo->mOutputFormat;
    if (pSinkInfo->mOutputFd > 0) {
        p_node->mSinkInfo.mOutputFd = dup(pSinkInfo->mOutputFd);
    } else {
        p_node->mSinkInfo.mOutputFd = -1;
    }
    p_node->mSinkInfo.mFallocateLen = pSinkInfo->mFallocateLen;
    p_node->mSinkInfo.mCallbackOutFlag = pSinkInfo->mCallbackOutFlag;

    p_node->mMuxChnAttr.mMuxerId = p_node->mSinkInfo.mMuxerId;
    p_node->mMuxChnAttr.mMediaFileFormat = p_node->mSinkInfo.mOutputFormat;
    p_node->mMuxChnAttr.mMaxFileDuration = pVi2Venc2MuxerData->mConfigPara.mMaxFileDuration *1000;
    p_node->mMuxChnAttr.mFallocateLen = p_node->mSinkInfo.mFallocateLen;
    p_node->mMuxChnAttr.mCallbackOutFlag = p_node->mSinkInfo.mCallbackOutFlag;
    p_node->mMuxChnAttr.mFsWriteMode = FSWRITEMODE_SIMPLECACHE;
    p_node->mMuxChnAttr.mSimpleCacheSize = DEFAULT_SIMPLE_CACHE_SIZE_VFS;

    p_node->mMuxChn = MM_INVALID_CHN;

    if ((pVi2Venc2MuxerData->mCurrentState == REC_PREPARED) || (pVi2Venc2MuxerData->mCurrentState == REC_RECORDING)) {
        ERRORTYPE ret;
        BOOL nSuccessFlag = FALSE;
        MUX_CHN nMuxChn = 0;
        while (nMuxChn < MUX_MAX_CHN_NUM) {
            ret = AW_MPI_MUX_CreateChn(pVi2Venc2MuxerData->mMuxGrp, nMuxChn, &p_node->mMuxChnAttr, p_node->mSinkInfo.mOutputFd);
            if (SUCCESS == ret) {
                nSuccessFlag = TRUE;
                alogd("create mux group[%d] channel[%d] success, muxerId[%d]!", pVi2Venc2MuxerData->mMuxGrp, nMuxChn, p_node->mMuxChnAttr.mMuxerId);
                break;
            } else if (ERR_MUX_EXIST == ret) {
                alogd("mux group[%d] channel[%d] is exist, find next!", pVi2Venc2MuxerData->mMuxGrp, nMuxChn);
                nMuxChn++;
            } else {
                aloge("fatal error! create mux group[%d] channel[%d] fail ret[0x%x], find next!", pVi2Venc2MuxerData->mMuxGrp, nMuxChn, ret);
                nMuxChn++;
            }
        }

        if (nSuccessFlag) {
            retMuxerId = p_node->mSinkInfo.mMuxerId;
            p_node->mMuxChn = nMuxChn;
            pVi2Venc2MuxerData->mMuxerIdCounter++;
        } else {
            aloge("fatal error! create mux group[%d] channel fail!", pVi2Venc2MuxerData->mMuxGrp);
            if (p_node->mSinkInfo.mOutputFd >= 0) {
                close(p_node->mSinkInfo.mOutputFd);
                p_node->mSinkInfo.mOutputFd = -1;
            }

            retMuxerId = -1;
        }

        pthread_mutex_lock(&pVi2Venc2MuxerData->mMuxChnListLock);
        list_add_tail(&p_node->mList, &pVi2Venc2MuxerData->mMuxChnList);
        pthread_mutex_unlock(&pVi2Venc2MuxerData->mMuxChnListLock);
    } else {
        retMuxerId = p_node->mSinkInfo.mMuxerId;
        pVi2Venc2MuxerData->mMuxerIdCounter++;
        pthread_mutex_lock(&pVi2Venc2MuxerData->mMuxChnListLock);
        list_add_tail(&p_node->mList, &pVi2Venc2MuxerData->mMuxChnList);
        pthread_mutex_unlock(&pVi2Venc2MuxerData->mMuxChnListLock);
    }

    return retMuxerId;
}

static int addOutputFormatAndOutputSink(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData, char* path, MEDIA_FILE_FORMAT_E format)
{
    int muxerId = -1;
    OUTSINKINFO_S sinkInfo = {0};

    if (path != NULL) {
        sinkInfo.mFallocateLen = 0;
        sinkInfo.mCallbackOutFlag = FALSE;
        sinkInfo.mOutputFormat = format;
        sinkInfo.mOutputFd = open(path, O_RDWR | O_CREAT, 0666);
        if (sinkInfo.mOutputFd < 0) {
            aloge("Failed to open %s", path);
            return -1;
        }

        muxerId = addOutputFormatAndOutputSink_1(pVi2Venc2MuxerData, &sinkInfo);
        close(sinkInfo.mOutputFd);
    }

    return muxerId;
}

static int setOutputFileSync_1(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData, int fd, int64_t fallocateLength, int muxerId)
{
    MUX_CHN_INFO_S *pEntry, *pTmp;

    if (pVi2Venc2MuxerData->mCurrentState != REC_RECORDING) {
        aloge("must be in recording state");
        return -1;
    }

    alogv("setOutputFileSync fd=%d", fd);
    if (fd < 0) {
        aloge("Invalid parameter");
        return -1;
    }

    MUX_CHN muxChn = MM_INVALID_CHN;
    pthread_mutex_lock(&pVi2Venc2MuxerData->mMuxChnListLock);
    if (!list_empty(&pVi2Venc2MuxerData->mMuxChnList)) {
        list_for_each_entry_safe(pEntry, pTmp, &pVi2Venc2MuxerData->mMuxChnList, mList) {
            if (pEntry->mMuxChnAttr.mMuxerId == muxerId) {
                muxChn = pEntry->mMuxChn;
                break;
            }
        }
    }
    pthread_mutex_unlock(&pVi2Venc2MuxerData->mMuxChnListLock);

    if (muxChn != MM_INVALID_CHN) {
        alogd("switch fd");
        AW_MPI_MUX_SwitchFd(pVi2Venc2MuxerData->mMuxGrp, muxChn, fd, fallocateLength);
        return 0;
    } else {
        aloge("fatal error! can't find muxChn which muxerId[%d]", muxerId);
        return -1;
    }
}

static int setOutputFileSync(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData, char* path, int64_t fallocateLength, int muxerId)
{
    int ret;

    if (pVi2Venc2MuxerData->mCurrentState != REC_RECORDING) {
        aloge("not in recording state");
        return -1;
    }

    if(path != NULL) {
        int fd = open(path, O_RDWR | O_CREAT, 0666);
        if (fd < 0) {
            aloge("fail to open %s", path);
            return -1;
        }
        ret = setOutputFileSync_1(pVi2Venc2MuxerData, fd, fallocateLength, muxerId);
        close(fd);

        return ret;
    } else {
        return -1;
    }
}

static ERRORTYPE configVencChnAttr(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData)
{
    memset(&pVi2Venc2MuxerData->mVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));

    pVi2Venc2MuxerData->mVencChnAttr.VeAttr.Type = pVi2Venc2MuxerData->mConfigPara.mVideoEncoderFmt;
    //pVi2Venc2MuxerData->mVencChnAttr.VeAttr.MaxKeyInterval = pVi2Venc2MuxerData->mVideoMaxKeyItl;
    pVi2Venc2MuxerData->mVencChnAttr.VeAttr.SrcPicWidth  = pVi2Venc2MuxerData->mConfigPara.vsWidth;
    pVi2Venc2MuxerData->mVencChnAttr.VeAttr.SrcPicHeight = pVi2Venc2MuxerData->mConfigPara.vsHeight;
    pVi2Venc2MuxerData->mVencChnAttr.VeAttr.PixelFormat = pVi2Venc2MuxerData->mConfigPara.dstPixFmt;
    pVi2Venc2MuxerData->mVencChnAttr.VeAttr.Field = pVi2Venc2MuxerData->mConfigPara.mField;

    if (PT_H264 == pVi2Venc2MuxerData->mVencChnAttr.VeAttr.Type) {
        pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrH264e.bByFrame = TRUE;
        pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrH264e.Profile = 2;//0:base 1:main 2:high
        pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrH264e.PicWidth  = pVi2Venc2MuxerData->mConfigPara.dstWidth;
        pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrH264e.PicHeight = pVi2Venc2MuxerData->mConfigPara.dstHeight;
        switch (pVi2Venc2MuxerData->mConfigPara.mRcMode) {
        case 1:
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264VBR;
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH264Vbr.mMinQp = 10;
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH264Vbr.mMaxQp = 52;
            break;
        case 2:
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264FIXQP;
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH264FixQp.mIQp = 28;
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH264FixQp.mPQp = 28;
            break;
        case 3:
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264QPMAP;
            break;
        case 0:
        default:
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264CBR;
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH264Cbr.mSrcFrmRate = pVi2Venc2MuxerData->mConfigPara.mVideoFrameRate;
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH264Cbr.mBitRate = pVi2Venc2MuxerData->mConfigPara.mVideoBitRate;
            break;
        }
        if (pVi2Venc2MuxerData->mConfigPara.mEnableFastEnc) {
            pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrH264e.FastEncFlag = TRUE;
        }
    } else if (PT_H265 == pVi2Venc2MuxerData->mVencChnAttr.VeAttr.Type) {
        pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrH265e.mbByFrame = TRUE;
        pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrH265e.mProfile = 1;//1:main 2:main10 3:sti11
        pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrH265e.mPicWidth = pVi2Venc2MuxerData->mConfigPara.dstWidth;
        pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrH265e.mPicHeight = pVi2Venc2MuxerData->mConfigPara.dstHeight;
        pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH265Cbr.mBitRate = pVi2Venc2MuxerData->mConfigPara.mVideoBitRate;
        switch (pVi2Venc2MuxerData->mConfigPara.mRcMode) {
        case 1:
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264VBR;
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH265Vbr.mMinQp = 10;
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH265Vbr.mMaxQp = 52;
            break;
        case 2:
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264FIXQP;
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH265FixQp.mIQp = 28;
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH265FixQp.mPQp = 28;
            break;
        case 3:
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264QPMAP;
            break;
        case 0:
        default:
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264CBR;
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH265Cbr.mSrcFrmRate = pVi2Venc2MuxerData->mConfigPara.mVideoFrameRate;
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrH265Cbr.mBitRate = pVi2Venc2MuxerData->mConfigPara.mVideoBitRate;
            break;
        }
        if (pVi2Venc2MuxerData->mConfigPara.mEnableFastEnc) {
            pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrH265e.mFastEncFlag = TRUE;
        }
    } else if (PT_MJPEG == pVi2Venc2MuxerData->mVencChnAttr.VeAttr.Type) {
        pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrMjpeg.mbByFrame = TRUE;
        pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrMjpeg.mPicWidth = pVi2Venc2MuxerData->mConfigPara.dstWidth;
        pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrMjpeg.mPicHeight = pVi2Venc2MuxerData->mConfigPara.dstHeight;
        switch (pVi2Venc2MuxerData->mConfigPara.mRcMode) {
        case 0:
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGCBR;
            break;
        case 1:
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGFIXQP;
            break;
        case 2:
        case 3:
            aloge("not support! use default cbr mode");
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGCBR;
            break;
        default:
            pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGCBR;
            break;
        }
        pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mAttrMjpegeCbr.mBitRate = pVi2Venc2MuxerData->mConfigPara.mVideoBitRate;
    }

    alogd("venc ste Rcmode=%d", pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode);

    return SUCCESS;
}

static ERRORTYPE createVencChn(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData)
{
    ERRORTYPE ret;
    BOOL nSuccessFlag = FALSE;

    configVencChnAttr(pVi2Venc2MuxerData);
    pVi2Venc2MuxerData->mVeChn = 0;
    while (pVi2Venc2MuxerData->mVeChn < VENC_MAX_CHN_NUM) {
        ret = AW_MPI_VENC_CreateChn(pVi2Venc2MuxerData->mVeChn, &pVi2Venc2MuxerData->mVencChnAttr);
        if (SUCCESS == ret) {
            nSuccessFlag = TRUE;
            alogd("create venc channel[%d] success!", pVi2Venc2MuxerData->mVeChn);
            break;
        } else if (ERR_VENC_EXIST == ret) {
            alogd("venc channel[%d] is exist, find next!", pVi2Venc2MuxerData->mVeChn);
            pVi2Venc2MuxerData->mVeChn++;
        } else {
            alogd("create venc channel[%d] ret[0x%x], find next!", pVi2Venc2MuxerData->mVeChn, ret);
            pVi2Venc2MuxerData->mVeChn++;
        }
    }

    if (nSuccessFlag == FALSE) {
        pVi2Venc2MuxerData->mVeChn = MM_INVALID_CHN;
        aloge("fatal error! create venc channel fail!");
        return FAILURE;
    } else {
        VENC_FRAME_RATE_S stFrameRate;
        stFrameRate.SrcFrmRate = stFrameRate.DstFrmRate = pVi2Venc2MuxerData->mConfigPara.mVideoFrameRate;
        alogd("set venc framerate:%d", stFrameRate.DstFrmRate);
        AW_MPI_VENC_SetFrameRate(pVi2Venc2MuxerData->mVeChn, &stFrameRate);

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pVi2Venc2MuxerData;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VENC_RegisterCallback(pVi2Venc2MuxerData->mVeChn, &cbInfo);

        if ( ((PT_H264 == pVi2Venc2MuxerData->mVencChnAttr.VeAttr.Type) || (PT_H265 == pVi2Venc2MuxerData->mVencChnAttr.VeAttr.Type))
             && ((VENC_RC_MODE_H264QPMAP == pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode) || (VENC_RC_MODE_H265QPMAP == pVi2Venc2MuxerData->mVencChnAttr.RcAttr.mRcMode))
           ) {
            aloge("fatal error! not support qpmap currently!");
            /*
            unsigned int width, heigth;
            int num;
            VENC_PARAM_H264_QPMAP_S QpMap;

            width = pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrH264e.PicWidth;
            heigth = pVi2Venc2MuxerData->mVencChnAttr.VeAttr.AttrH264e.PicHeight;
            num = (ALIGN(width, 16) >> 4) * (ALIGN(heigth, 16) >> 4);

            QpMap.num = num;
            QpMap.p_info = (VENC_QPMAP_BLOCK_QP_INFO *)malloc(sizeof(VENC_QPMAP_BLOCK_QP_INFO) * num);
            if (QpMap.p_info == NULL)
            {
                aloge("QPmap buffer malloc fail!!");
            }
            else
            {
                int i;
                for (i = 0; i < num / 2; i++)
                {
                    QpMap.p_info[i].mb_en = 1;
                    QpMap.p_info[i].mb_skip_flag = 0;
                    QpMap.p_info[i].mb_qp = 10;
                }
                for (; i < num; i++)
                {
                    QpMap.p_info[i].mb_en = 1;
                    QpMap.p_info[i].mb_skip_flag = 0;
                    QpMap.p_info[i].mb_qp = 42;
                }

                alogd("set QPMAP");
                AW_MPI_VENC_SetH264QPMAP(pVi2Venc2MuxerData->mVeChn, (const VENC_PARAM_H264_QPMAP_S *)&QpMap);
                free(QpMap.p_info);
            }
            */
        }

        return SUCCESS;
    }
}

static ERRORTYPE createViChn(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData)
{
    ERRORTYPE ret;

    //create vi channel
    pVi2Venc2MuxerData->mViDev = pVi2Venc2MuxerData->mConfigPara.mDevNo;
    pVi2Venc2MuxerData->mViChn = 0;

    ret = AW_MPI_VI_CreateVipp(pVi2Venc2MuxerData->mViDev);
    if (ret != SUCCESS) {
        aloge("fatal error! AW_MPI_VI CreateVipp failed");
    }

    memset(&pVi2Venc2MuxerData->mViAttr, 0, sizeof(VI_ATTR_S));
    pVi2Venc2MuxerData->mViAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pVi2Venc2MuxerData->mViAttr.memtype = V4L2_MEMORY_MMAP;
    //pVi2Venc2MuxerData->mViAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT();
    pVi2Venc2MuxerData->mViAttr.format.pixelformat = V4L2_PIX_FMT_NV21M;
    pVi2Venc2MuxerData->mViAttr.format.field = V4L2_FIELD_NONE;
    //pVi2Venc2MuxerData->mViAttr.format.colorspace = V4L2_COLORSPACE_JPEG;
    pVi2Venc2MuxerData->mViAttr.format.width =  pVi2Venc2MuxerData->mConfigPara.srcWidth;//
    pVi2Venc2MuxerData->mViAttr.format.height = pVi2Venc2MuxerData->mConfigPara.srcHeight;//
    pVi2Venc2MuxerData->mViAttr.nbufs = pVi2Venc2MuxerData->mConfigPara.mVippBufNum; // 10;// 38; // 10; //5
    aloge("use %d v4l2 buffers!!!", pVi2Venc2MuxerData->mViAttr.nbufs);
    pVi2Venc2MuxerData->mViAttr.nplanes = 2;
    pVi2Venc2MuxerData->mViAttr.fps = pVi2Venc2MuxerData->mConfigPara.mVideoFrameRate;
    pVi2Venc2MuxerData->mViAttr.antishake_enable = 1;
    pVi2Venc2MuxerData->mViAttr.antishake_attr.number_of_input_buffers = pVi2Venc2MuxerData->mConfigPara.mEisInputBufNum; // 5; // 27; //5 ; // 4;
    pVi2Venc2MuxerData->mViAttr.antishake_attr.number_of_output_buffers = pVi2Venc2MuxerData->mConfigPara.mEisOutputBufNum; //10;
    pVi2Venc2MuxerData->mViAttr.antishake_attr.operation_mode = pVi2Venc2MuxerData->mConfigPara.mode;
    pVi2Venc2MuxerData->mViAttr.antishake_attr.rq = pVi2Venc2MuxerData->mConfigPara.mRq;

    ret = AW_MPI_VI_SetVippAttr(pVi2Venc2MuxerData->mViDev, &pVi2Venc2MuxerData->mViAttr);
    if (ret != SUCCESS) {
        aloge("fatal error! AW_MPI_VI SetVippAttr failed");
    }

    // AW_MPI_VI_SetVippFlip(0, 0);//0,1
    // AW_MPI_VI_SetVippMirror(0, 0);//0,1

    AW_MPI_VI_SetVippFlip(0, 1);//0,1
    AW_MPI_VI_SetVippMirror(0, 1);//0,1

    // AW_MPI_VI_SetVippFlip(0, 0);//0,1
    // AW_MPI_VI_SetVippMirror(0, 1);//0,1

    // AW_MPI_VI_SetVippFlip(0, 1);//0,1
    // AW_MPI_VI_SetVippMirror(0, 0);//0,1

    ret = AW_MPI_VI_EnableVipp(pVi2Venc2MuxerData->mViDev);
    if (ret != SUCCESS) {
        aloge("fatal error! enableVipp fail!");
    }
    /* open isp */
    int isp_dev;
    if (pVi2Venc2MuxerData->mViDev == 0 || pVi2Venc2MuxerData->mViDev == 2) {
        isp_dev = 1;
    } else if (pVi2Venc2MuxerData->mViDev == 1 || pVi2Venc2MuxerData->mViDev == 3) {
        isp_dev = 0;
    }
    AW_MPI_ISP_Init();
    AW_MPI_ISP_Run(0);

    // AW_MPI_ISP_SetMirror(0, 1);//0,1

    ret = AW_MPI_VI_CreateVirChn(pVi2Venc2MuxerData->mViDev, pVi2Venc2MuxerData->mViChn, NULL);
    if (ret != SUCCESS) {
        aloge("fatal error! createVirChn[%d] fail!", pVi2Venc2MuxerData->mViChn);
    }

    return ret;
}

static ERRORTYPE prepare(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData)
{
    BOOL nSuccessFlag;
    MUX_CHN nMuxChn;
    MUX_CHN_INFO_S *pEntry, *pTmp;
    ERRORTYPE ret;
    ERRORTYPE result = FAILURE;

    if (createViChn(pVi2Venc2MuxerData) != SUCCESS) {
        aloge("create vi chn fail");
        return result;
    }

    if (createVencChn(pVi2Venc2MuxerData) != SUCCESS) {
        aloge("create venc chn fail");
        return result;
    }

    if (createMuxGrp(pVi2Venc2MuxerData) != SUCCESS) {
        aloge("create mux group fail");
        return result;
    }

    //set spspps
    if (pVi2Venc2MuxerData->mConfigPara.mVideoEncoderFmt == PT_H264) {
        VencHeaderData H264SpsPpsInfo;
        AW_MPI_VENC_GetH264SpsPpsInfo(pVi2Venc2MuxerData->mVeChn, &H264SpsPpsInfo);
        AW_MPI_MUX_SetH264SpsPpsInfo(pVi2Venc2MuxerData->mMuxGrp, &H264SpsPpsInfo);
    } else if(pVi2Venc2MuxerData->mConfigPara.mVideoEncoderFmt == PT_H265) {
        VencHeaderData H265SpsPpsInfo;
        AW_MPI_VENC_GetH265SpsPpsInfo(pVi2Venc2MuxerData->mVeChn, &H265SpsPpsInfo);
        AW_MPI_MUX_SetH265SpsPpsInfo(pVi2Venc2MuxerData->mMuxGrp, &H265SpsPpsInfo);
    }

    if (pVi2Venc2MuxerData->mConfigPara.mEnableRoi) {
        VENC_ROI_CFG_S VencRoiCfg;

        VencRoiCfg.bEnable = TRUE;
        VencRoiCfg.Index = 0;
        VencRoiCfg.Qp = 10;
        VencRoiCfg.bAbsQp = 0;
        VencRoiCfg.Rect.X = 0;
        VencRoiCfg.Rect.Y = 0;
        VencRoiCfg.Rect.Width = 1280;
        VencRoiCfg.Rect.Height = 320;
        AW_MPI_VENC_SetRoiCfg(pVi2Venc2MuxerData->mVeChn, &VencRoiCfg);

        VencRoiCfg.Index = 1;
        VencRoiCfg.Rect.X = 200;
        VencRoiCfg.Rect.Y = 600;
        VencRoiCfg.Rect.Width = 1000;
        VencRoiCfg.Rect.Height = 200;
        AW_MPI_VENC_SetRoiCfg(pVi2Venc2MuxerData->mVeChn, &VencRoiCfg);

        VencRoiCfg.Index = 2;
        VencRoiCfg.Rect.X = 600;
        VencRoiCfg.Rect.Y = 900;
        VencRoiCfg.Rect.Width = 300;
        VencRoiCfg.Rect.Height = 100;
        VencRoiCfg.Qp = 40;
        VencRoiCfg.bAbsQp = 0;
        AW_MPI_VENC_SetRoiCfg(pVi2Venc2MuxerData->mVeChn, &VencRoiCfg);

        /*VencRoiCfg.Index = 4;
        VencRoiCfg.Rect.X = 800;
        VencRoiCfg.Rect.Y = 200;
        VencRoiCfg.Rect.Width = 600;
        VencRoiCfg.Rect.Height = 100;
        AW_MPI_VENC_SetRoiCfg(pVi2Venc2MuxerData->mVeChn, &VencRoiCfg);*/
    }

    pthread_mutex_lock(&pVi2Venc2MuxerData->mMuxChnListLock);
    if (!list_empty(&pVi2Venc2MuxerData->mMuxChnList)) {
        list_for_each_entry_safe(pEntry, pTmp, &pVi2Venc2MuxerData->mMuxChnList, mList) {
            nMuxChn = 0;
            nSuccessFlag = FALSE;
            while (pEntry->mMuxChn < MUX_MAX_CHN_NUM) {
                ret = AW_MPI_MUX_CreateChn(pVi2Venc2MuxerData->mMuxGrp, nMuxChn, &pEntry->mMuxChnAttr, pEntry->mSinkInfo.mOutputFd);
                if (SUCCESS == ret) {
                    nSuccessFlag = TRUE;
                    alogd("create mux group[%d] channel[%d] success, muxerId[%d]!", pVi2Venc2MuxerData->mMuxGrp, \
                          nMuxChn, pEntry->mMuxChnAttr.mMuxerId);
                    break;
                } else if(ERR_MUX_EXIST == ret) {
                    nMuxChn++;
                    //break;
                } else {
                    nMuxChn++;
                }
            }

            if (FALSE == nSuccessFlag) {
                pEntry->mMuxChn = MM_INVALID_CHN;
                aloge("fatal error! create mux group[%d] channel fail!", pVi2Venc2MuxerData->mMuxGrp);
            } else {
                result = SUCCESS;
                pEntry->mMuxChn = nMuxChn;
            }
        }
    } else {
        aloge("maybe something wrong,mux chn list is empty");
    }
    pthread_mutex_unlock(&pVi2Venc2MuxerData->mMuxChnListLock);

    if ((pVi2Venc2MuxerData->mViDev >= 0 && pVi2Venc2MuxerData->mViChn >= 0) && pVi2Venc2MuxerData->mVeChn >= 0) {
        MPP_CHN_S ViChn = {MOD_ID_VIU, pVi2Venc2MuxerData->mViDev, pVi2Venc2MuxerData->mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pVi2Venc2MuxerData->mVeChn};

        AW_MPI_SYS_Bind(&ViChn, &VeChn);
    }

    if (pVi2Venc2MuxerData->mVeChn >= 0 && pVi2Venc2MuxerData->mMuxGrp >= 0) {
        MPP_CHN_S MuxGrp = {MOD_ID_MUX, 0, pVi2Venc2MuxerData->mMuxGrp};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pVi2Venc2MuxerData->mVeChn};

        AW_MPI_SYS_Bind(&VeChn, &MuxGrp);
        pVi2Venc2MuxerData->mCurrentState = REC_PREPARED;
    }

    return result;
}

static ERRORTYPE start(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData)
{
    ERRORTYPE ret = SUCCESS;

    alogd("start");

    ret = AW_MPI_VI_EnableVirChn(pVi2Venc2MuxerData->mViDev, pVi2Venc2MuxerData->mViChn);
    if (ret != SUCCESS) {
        alogd("VI enable error!");
        return FAILURE;
    }

    if (pVi2Venc2MuxerData->mVeChn >= 0) {
        AW_MPI_VENC_StartRecvPic(pVi2Venc2MuxerData->mVeChn);
    }

    if (pVi2Venc2MuxerData->mMuxGrp >= 0) {
        AW_MPI_MUX_StartGrp(pVi2Venc2MuxerData->mMuxGrp);
    }

    pVi2Venc2MuxerData->mCurrentState = REC_RECORDING;

    return ret;
}

static ERRORTYPE stop(SAMPLE_VI2VENC2MUXER_S *pVi2Venc2MuxerData)
{
    MUX_CHN_INFO_S *pEntry, *pTmp;
    ERRORTYPE ret = SUCCESS;
    alogd("stop");

    if (pVi2Venc2MuxerData->mMuxGrp >= 0) {
        alogd("stop mux grp");
        AW_MPI_MUX_StopGrp(pVi2Venc2MuxerData->mMuxGrp);
    }
    if (pVi2Venc2MuxerData->mVeChn >= 0) {
        alogd("stop venc");
        AW_MPI_VENC_StopRecvPic(pVi2Venc2MuxerData->mVeChn);
    }
    if (pVi2Venc2MuxerData->mViChn >= 0) {
        AW_MPI_VI_DisableVirChn(pVi2Venc2MuxerData->mViDev, pVi2Venc2MuxerData->mViChn);
    }

    if (pVi2Venc2MuxerData->mMuxGrp >= 0) {
        alogd("destory mux grp");
        AW_MPI_MUX_DestroyGrp(pVi2Venc2MuxerData->mMuxGrp);
        pVi2Venc2MuxerData->mMuxGrp = MM_INVALID_CHN;
    }
    pthread_mutex_lock(&pVi2Venc2MuxerData->mMuxChnListLock);
    if (!list_empty(&pVi2Venc2MuxerData->mMuxChnList)) {
        alogd("free chn list node");
        list_for_each_entry_safe(pEntry, pTmp, &pVi2Venc2MuxerData->mMuxChnList, mList) {
            if (pEntry->mSinkInfo.mOutputFd > 0) {
                alogd("close file");
                close(pEntry->mSinkInfo.mOutputFd);
                pEntry->mSinkInfo.mOutputFd = -1;
            }

            list_del(&pEntry->mList);
            free(pEntry);
        }
    }
    pthread_mutex_unlock(&pVi2Venc2MuxerData->mMuxChnListLock);

    if (pVi2Venc2MuxerData->mVeChn >= 0) {
        alogd("destory venc");
        AW_MPI_VENC_ResetChn(pVi2Venc2MuxerData->mVeChn);
        AW_MPI_VENC_DestroyChn(pVi2Venc2MuxerData->mVeChn);
        pVi2Venc2MuxerData->mVeChn = MM_INVALID_CHN;
    }
    if (pVi2Venc2MuxerData->mViChn >= 0) {
        AW_MPI_VI_DestoryVirChn(pVi2Venc2MuxerData->mViDev, pVi2Venc2MuxerData->mViChn);
        int isp_dev;
        if (pVi2Venc2MuxerData->mViDev == 0 || pVi2Venc2MuxerData->mViDev == 2) {
            isp_dev = 1;
        } else if (pVi2Venc2MuxerData->mViDev == 1 || pVi2Venc2MuxerData->mViDev == 3) {
            isp_dev = 0;
        }
        AW_MPI_ISP_Stop(0);
        AW_MPI_ISP_Exit();
        AW_MPI_VI_DisableVipp(pVi2Venc2MuxerData->mViDev);
        AW_MPI_VI_DestoryVipp(pVi2Venc2MuxerData->mViDev);
    }

    return SUCCESS;
}


int main(int argc, char** argv)
{
    int result = -1;
    MUX_CHN_INFO_S *pEntry, *pTmp;

    printf("sample_virvi2venc2muxer running!\n");
    if (InitVi2Venc2MuxerData() != SUCCESS) {
        return -1;
    }

    cdx_sem_init(&pVi2Venc2MuxerData->mSemExit, 0);

    if (parseCmdLine(pVi2Venc2MuxerData, argc, argv) != SUCCESS) {
        aloge("parse cmdline fail");
        goto err_out_0;
    }

    if (loadConfigPara(pVi2Venc2MuxerData, pVi2Venc2MuxerData->mCmdLinePara.mConfigFilePath) != SUCCESS) {
        aloge("load config file fail");
        goto err_out_0;
    }

    INIT_LIST_HEAD(&pVi2Venc2MuxerData->mMuxChnList);
    pthread_mutex_init(&pVi2Venc2MuxerData->mMuxChnListLock, NULL);

    pVi2Venc2MuxerData->mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&pVi2Venc2MuxerData->mSysConf);
    AW_MPI_SYS_Init();

    pVi2Venc2MuxerData->mMuxId[0] = addOutputFormatAndOutputSink(pVi2Venc2MuxerData, pVi2Venc2MuxerData->mConfigPara.dstVideoFile, MEDIA_FILE_FORMAT_MP4);
    if (pVi2Venc2MuxerData->mMuxId[0] < 0) {
        aloge("add first out file fail");
        goto err_out_1;
    }

#ifdef DOUBLE_ENCODER_FILE_OUT
    char mov_path[128];
    strcpy(mov_path, "/mnt/extsd/sample_vi2venc2muxer/0.ts");
    pVi2Venc2MuxerData->mMuxId[1] = addOutputFormatAndOutputSink(pVi2Venc2MuxerData, mov_path, MEDIA_FILE_FORMAT_TS);
    if (pVi2Venc2MuxerData->mMuxId[1] < 0) {
        alogd("add mMuxId[1] ts file sink fail");
    }
#endif

    if (prepare(pVi2Venc2MuxerData) != SUCCESS) {
        aloge("prepare fail!");
        goto err_out_2;
    }

    start(pVi2Venc2MuxerData);
    /*
        sleep(10);

        int b3DNR;
        VENC_COLOR2GREY_S bColor2Grey;

        AW_MPI_VENC_Get3DNR(pVi2Venc2MuxerData->mVeChn, &b3DNR);
        AW_MPI_VENC_GetColor2Grey(pVi2Venc2MuxerData->mVeChn, &bColor2Grey);

        if(!b3DNR)
        {
            alogd("now,let us open 3dnr ");
            b3DNR = TRUE;
            AW_MPI_VENC_Set3DNR(pVi2Venc2MuxerData->mVeChn, b3DNR);
        }
        if(!bColor2Grey.bColor2Grey)
        {
            alogd("now, let us open color2grey!");
            bColor2Grey.bColor2Grey = TRUE;
            AW_MPI_VENC_SetColor2Grey(pVi2Venc2MuxerData->mVeChn, &bColor2Grey);
        }

        sleep(10);

        AW_MPI_VENC_Set3DNR(pVi2Venc2MuxerData->mVeChn, 0);

        bColor2Grey.bColor2Grey = FALSE;
        AW_MPI_VENC_SetColor2Grey(pVi2Venc2MuxerData->mVeChn, &bColor2Grey);
    */
    if (pVi2Venc2MuxerData->mConfigPara.mTestDuration > 0) {
        cdx_sem_down_timedwait(&pVi2Venc2MuxerData->mSemExit, pVi2Venc2MuxerData->mConfigPara.mTestDuration*1000);
    } else {
        cdx_sem_down(&pVi2Venc2MuxerData->mSemExit);
    }

    alogd("start stop vi venc mux.");
    stop(pVi2Venc2MuxerData);
    result = 0;

    alogd("start to free res");
err_out_2:
    pthread_mutex_lock(&pVi2Venc2MuxerData->mMuxChnListLock);
    if (!list_empty(&pVi2Venc2MuxerData->mMuxChnList)) {
        alogd("chn list not empty");
        list_for_each_entry_safe(pEntry, pTmp, &pVi2Venc2MuxerData->mMuxChnList, mList) {
            if (pEntry->mSinkInfo.mOutputFd > 0) {
                close(pEntry->mSinkInfo.mOutputFd);
                pEntry->mSinkInfo.mOutputFd = -1;
            }
        }

        list_del(&pEntry->mList);
        free(pEntry);
    }
    pthread_mutex_unlock(&pVi2Venc2MuxerData->mMuxChnListLock);
err_out_1:
    AW_MPI_SYS_Exit();

    pthread_mutex_destroy(&pVi2Venc2MuxerData->mMuxChnListLock);
err_out_0:
    cdx_sem_deinit(&pVi2Venc2MuxerData->mSemExit);
    free(pVi2Venc2MuxerData);
    pVi2Venc2MuxerData = NULL;
    if (result == 0) {
        printf("sample_virvi2venc2muxer exit!\n");
    }

    return result;
}
