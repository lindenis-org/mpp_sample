/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     :
  Version       : Initial Draft
  Author        : Allwinner BU3-XAPSW Team
  Created       : 2018/01/23
  Last Modified :
  Description   :
  Function List :
  History       :
******************************************************************************/

#define LOG_NDEBUG 1
#define LOG_TAG "SampleDemux2Muxer"

#include <unistd.h>
#include <fcntl.h>
#include "plat_log.h"
#include <time.h>
#include <mm_common.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <cdx_list.h>
#include <pthread.h>

#include "mm_comm_sys.h"
#include "mpi_sys.h"

#include "mm_comm_mux.h"
#include "mpi_mux.h"

#include "DemuxCompStream.h"
#include "mm_comm_demux.h"

#include <mpi_demux.h>
#include <mpi_clock.h>
#include <ClockCompPortIndex.h>
#include <memoryAdapter.h>
#include "sc_interface.h"
#include "record_writer.h"

#include "tmessage.h"
#include "tsemaphore.h"
#include <confparser.h>

//#define PASS_VIDEO_DATA
#define CLOCK_CHN_INDEX         (0)
#define DEMUX_CHN_INDEX         (0)
#define MUX_GRP_INDEX           (0)
#define MUX_CHN_INDEX           (0)

typedef struct _OUTSINKINFO_S {
    int mMuxerId;
    MEDIA_FILE_FORMAT_E mOutputFormat;
    int mOutputFd;
    int mFallocateLen;
    BOOL mCallbackOutFlag;
} OUTSINKINFO_S, *PTR_OUTSINKINFO_S;

typedef struct _MUX_CHN_INFO_S {
    OUTSINKINFO_S mSinkInfo;
    MUX_CHN_ATTR_S mMuxChnAttr;
    MUX_CHN mMuxChn;
} MUX_CHN_INFO_S, *PTR_MUX_CHN_INFO_S;

typedef struct Demux2Muxer_Config {
    char srcFile[128];
    char dstFile[128];

    int mMediaFileFmt;
    int mMaxFileDuration;
    int mTestDuration;
} DEMUX2MUXER_CONFIG_S;

typedef struct sample_demux2muxer_s {
    DEMUX2MUXER_CONFIG_S mConfigPara;
    cdx_sem_t mSemExit;
    BOOL mOverFlag;
} SAMPLE_DEMUX2MUXER_S;

static void ShowMediaInfo(DEMUX_MEDIA_INFO_S* pMediaInfo)
{
    alogd("======== MediaInfo ===========");
    alogd("FileSize = %d Duration = %d ms", pMediaInfo->mFileSize, pMediaInfo->mDuration);
    alogd("TypeMap H264(%d) H265(%d) MJPEG(%d) JPEG(%d) AAC(%d) MP3(%d)", PT_H264, PT_H265, PT_MJPEG, PT_JPEG, PT_AAC, PT_MP3);
    // Video Or Jpeg Info
    for (int i = 0; i < pMediaInfo->mVideoNum; i++) {
        //VideoType: PT_JPEG, PT_H264, PT_H265, PT_MJPEG
        alogd("Video: CodecType = %d wWidth = %d wHeight = %d nFrameRate = %d AvgBitsRate = %d MaxBitsRate = %d"
              ,pMediaInfo->mVideoStreamInfo[i].mCodecType
              ,pMediaInfo->mVideoStreamInfo[i].mWidth
              ,pMediaInfo->mVideoStreamInfo[i].mHeight
              ,pMediaInfo->mVideoStreamInfo[i].mFrameRate
              ,pMediaInfo->mVideoStreamInfo[i].mAvgBitsRate
              ,pMediaInfo->mVideoStreamInfo[i].mMaxBitsRate
             );
    }

    // Audio Info
    for (int i = 0; i < pMediaInfo->mAudioNum; i++) {
        alogd("Audio: CodecType = %d nChaninelNum = %d nBitsPerSample = %d nSampleRate = %d AvgBitsRate = %d MaxBitsRate = %d"
              ,pMediaInfo->mAudioStreamInfo[i].mCodecType
              ,pMediaInfo->mAudioStreamInfo[i].mChannelNum
              ,pMediaInfo->mAudioStreamInfo[i].mBitsPerSample
              ,pMediaInfo->mAudioStreamInfo[i].mSampleRate
              ,pMediaInfo->mAudioStreamInfo[i].mAvgBitsRate
              ,pMediaInfo->mAudioStreamInfo[i].mMaxBitsRate
             );
    }

    return;
}


static SAMPLE_DEMUX2MUXER_S *pDemux2MuxerData;

static ERRORTYPE MPPCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    SAMPLE_DEMUX2MUXER_S *pDemux2MuxerData = (SAMPLE_DEMUX2MUXER_S *)cookie;

    if (MOD_ID_MUX == pChn->mModId) {
        switch(event) {
        case MPP_EVENT_RECORD_DONE: {
            int muxerId = *(int*)pEventData;
            alogd("file done, mux_id=%d", muxerId);
            break;
        }
        case MPP_EVENT_NEED_NEXT_FD: {
            int muxerId = *(int*)pEventData;
            alogd("mux need next fd, mux_id=%d", muxerId);
            break;
        }
        case MPP_EVENT_BSFRAME_AVAILABLE: {
            break;
        }
        default:
            break;
        }
    }

    if (MOD_ID_DEMUX == pChn->mModId) {
        switch (event) {
        case MPP_EVENT_NOTIFY_EOF:
            alogd("demux to end of file");
            cdx_sem_up(&pDemux2MuxerData->mSemExit);
            break;

        default:
            break;
        }
    }

    return SUCCESS;
}

int main(int argc, char** argv)
{
    ERRORTYPE ret = 0;

    // 用户配置
    pDemux2MuxerData = (SAMPLE_DEMUX2MUXER_S* )malloc(sizeof(SAMPLE_DEMUX2MUXER_S));
    assert(pDemux2MuxerData != NULL);
    memset(pDemux2MuxerData, 0, sizeof(SAMPLE_DEMUX2MUXER_S));

    strcpy(pDemux2MuxerData->mConfigPara.srcFile, "/mnt/extsd/test_file/test4.mp4");
    strcpy(pDemux2MuxerData->mConfigPara.dstFile, "/mnt/extsd/sample_demux2muxer/output.mp4");
    pDemux2MuxerData->mConfigPara.mMediaFileFmt = MEDIA_FILE_FORMAT_MP4;
    pDemux2MuxerData->mConfigPara.mMaxFileDuration = 500;
    pDemux2MuxerData->mConfigPara.mTestDuration = 500;
    cdx_sem_init(&pDemux2MuxerData->mSemExit, 0);

    // 初始化MPP
    MPP_SYS_CONF_S mSysConf;
    mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&mSysConf);
    AW_MPI_SYS_Init();

    // 创建ClockChn
    CLOCK_CHN mClockChn = 0;
    CLOCK_CHN_ATTR_S mClockChnAttr;
    mClockChnAttr.nWaitMask = 0;
    mClockChnAttr.nWaitMask |= 1<<CLOCK_PORT_INDEX_AUDIO;
    ret = AW_MPI_CLOCK_CreateChn(mClockChn, &mClockChnAttr);
    assert(ret == SUCCESS);
    alogd("创建Clock组件成功");

    // 配置DemuxChn
    int srcFd = open(pDemux2MuxerData->mConfigPara.srcFile, O_RDONLY);
    if (srcFd < 0) {
        aloge("ERROR: cannot open mp4 src file (%s)", pDemux2MuxerData->mConfigPara.srcFile);
        return -1;
    }

    DEMUX_CHN_ATTR_S mDmxChnAttr;
    mDmxChnAttr.mStreamType = STREAMTYPE_LOCALFILE;
    mDmxChnAttr.mSourceType = SOURCETYPE_FD;
    mDmxChnAttr.mSourceUrl  = NULL;
    mDmxChnAttr.mFd         = srcFd;
    mDmxChnAttr.mDemuxDisableTrack = DEMUX_DISABLE_SUBTITLE_TRACK;

    // 创建DemuxChn
    DEMUX_CHN mDmxChn = DEMUX_CHN_INDEX;
    ret = AW_MPI_DEMUX_CreateChn(mDmxChn, &mDmxChnAttr);
    assert(SUCCESS == ret);
    alogd("创建DemuxChn成功");

    // 注册回调函数
    MPPCallbackInfo cbInfo_demux;
    cbInfo_demux.cookie   = (void*)pDemux2MuxerData;
    cbInfo_demux.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
    AW_MPI_DEMUX_RegisterCallback(mDmxChn, &cbInfo_demux);

    // 获取MediaInfo
    DEMUX_MEDIA_INFO_S DemuxMediaInfo;
    ret = AW_MPI_DEMUX_GetMediaInfo(mDmxChn, &DemuxMediaInfo);
    assert(ret == SUCCESS);
    if (  (DemuxMediaInfo.mVideoNum > 0 && DemuxMediaInfo.mVideoIndex >= DemuxMediaInfo.mVideoNum)
          || (DemuxMediaInfo.mAudioNum > 0 && DemuxMediaInfo.mAudioIndex >= DemuxMediaInfo.mAudioNum)
          || (DemuxMediaInfo.mSubtitleNum > 0 && DemuxMediaInfo.mSubtitleIndex >= DemuxMediaInfo.mSubtitleNum)
       ) {
        aloge("fatal error, trackIndex wrong! [%d][%d],[%d][%d],[%d][%d]",
              DemuxMediaInfo.mVideoNum, DemuxMediaInfo.mVideoIndex,
              DemuxMediaInfo.mAudioNum, DemuxMediaInfo.mAudioIndex,
              DemuxMediaInfo.mSubtitleNum, DemuxMediaInfo.mSubtitleIndex);
        return -1;
    }
    assert(DemuxMediaInfo.mVideoNum == 1 && DemuxMediaInfo.mAudioNum == 1);
    ShowMediaInfo(&DemuxMediaInfo);
    alogd("获取MediaInfo成功");

    // 配置MuxChn
    OUTSINKINFO_S sinkInfo = {0};
    sinkInfo.mFallocateLen    = 0;
    sinkInfo.mCallbackOutFlag = FALSE;
    sinkInfo.mOutputFormat    = pDemux2MuxerData->mConfigPara.mMediaFileFmt;
    sinkInfo.mOutputFd        = open(pDemux2MuxerData->mConfigPara.dstFile, O_RDWR | O_CREAT, 0666);
    assert(sinkInfo.mOutputFd > 0);

    MUX_CHN_INFO_S mMuxChnInfo;
    memset(&mMuxChnInfo, 0, sizeof(MUX_CHN_INFO_S));
    mMuxChnInfo.mSinkInfo.mMuxerId           = 0;
    mMuxChnInfo.mSinkInfo.mOutputFormat      = sinkInfo.mOutputFormat;
    mMuxChnInfo.mSinkInfo.mOutputFd          = dup(sinkInfo.mOutputFd);
    mMuxChnInfo.mSinkInfo.mFallocateLen      = sinkInfo.mFallocateLen;
    mMuxChnInfo.mSinkInfo.mCallbackOutFlag   = sinkInfo.mCallbackOutFlag;

    mMuxChnInfo.mMuxChnAttr.mMuxerId         = mMuxChnInfo.mSinkInfo.mMuxerId;
    mMuxChnInfo.mMuxChnAttr.mMediaFileFormat = mMuxChnInfo.mSinkInfo.mOutputFormat;
    mMuxChnInfo.mMuxChnAttr.mMaxFileDuration = pDemux2MuxerData->mConfigPara.mMaxFileDuration * 1000; //s -> ms
    mMuxChnInfo.mMuxChnAttr.mFallocateLen    = mMuxChnInfo.mSinkInfo.mFallocateLen;
    mMuxChnInfo.mMuxChnAttr.mCallbackOutFlag = mMuxChnInfo.mSinkInfo.mCallbackOutFlag;
    mMuxChnInfo.mMuxChnAttr.mFsWriteMode     = FSWRITEMODE_SIMPLECACHE;
    mMuxChnInfo.mMuxChnAttr.mSimpleCacheSize = 4 * 1024;
    mMuxChnInfo.mMuxChn = MM_INVALID_CHN;
    close(sinkInfo.mOutputFd);

    // 创建MuxGrp
    MUX_GRP_ATTR_S mMuxGrpAttr;
    memset(&mMuxGrpAttr, 0, sizeof(MUX_GRP_ATTR_S));
#ifdef PASS_VIDEO_DATA
    mMuxGrpAttr.mVideoEncodeType = DemuxMediaInfo.mVideoStreamInfo[0].mCodecType;
#else
    mMuxGrpAttr.mVideoEncodeType = PT_MAX;
#endif
    mMuxGrpAttr.mWidth           = DemuxMediaInfo.mVideoStreamInfo[0].mWidth;
    mMuxGrpAttr.mHeight          = DemuxMediaInfo.mVideoStreamInfo[0].mHeight;
    mMuxGrpAttr.mVideoFrmRate    = DemuxMediaInfo.mVideoStreamInfo[0].mFrameRate * 1000;

    mMuxGrpAttr.mAudioEncodeType = DemuxMediaInfo.mAudioStreamInfo[0].mCodecType;
    mMuxGrpAttr.mChannels        = DemuxMediaInfo.mAudioStreamInfo[0].mChannelNum;
    mMuxGrpAttr.mBitsPerSample   = DemuxMediaInfo.mAudioStreamInfo[0].mBitsPerSample;
    mMuxGrpAttr.mSamplesPerFrame = 1024;
    mMuxGrpAttr.mSampleRate      = DemuxMediaInfo.mAudioStreamInfo[0].mSampleRate;

    ret = AW_MPI_MUX_CreateGrp(MUX_GRP_INDEX, &mMuxGrpAttr);
    assert(ret == SUCCESS);
    alogd("创建MuxGrp成功");

    // 注册回调函数
    MPPCallbackInfo cbInfo;
    cbInfo.cookie = (void*)pDemux2MuxerData;
    cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
    AW_MPI_MUX_RegisterCallback(MUX_GRP_INDEX, &cbInfo);

    //VencHeaderData H264SpsPpsInfo;
    //AW_MPI_VENC_GetH264SpsPpsInfo(VENC_CHN, &H264SpsPpsInfo);
    //AW_MPI_MUX_SetH264SpsPpsInfo(MUX_GRP_INDEX, &H264SpsPpsInfo);
    AW_MPI_MUX_CreateChn(MUX_GRP_INDEX, 0, &mMuxChnInfo.mMuxChnAttr, mMuxChnInfo.mSinkInfo.mOutputFd);

    // 绑定Demux和Muxer
    if (DemuxMediaInfo.mSubtitleNum > 0) {
        AW_MPI_DEMUX_GetChnAttr(mDmxChn, &mDmxChnAttr);
        mDmxChnAttr.mDemuxDisableTrack |= DEMUX_DISABLE_SUBTITLE_TRACK;
        AW_MPI_DEMUX_SetChnAttr(mDmxChn, &mDmxChnAttr);
    }
#ifndef PASS_VIDEO_DATA
    if (DemuxMediaInfo.mVideoNum > 0) {
        AW_MPI_DEMUX_GetChnAttr(mDmxChn, &mDmxChnAttr);
        mDmxChnAttr.mDemuxDisableTrack |= DEMUX_DISABLE_VIDEO_TRACK;
        AW_MPI_DEMUX_SetChnAttr(mDmxChn, &mDmxChnAttr);
    }
#endif

    MPP_CHN_S ClockChn = {MOD_ID_CLOCK, 0, CLOCK_CHN_INDEX};
    MPP_CHN_S DmxChn = {MOD_ID_DEMUX, 0, DEMUX_CHN_INDEX};
    MPP_CHN_S MuxGrp = {MOD_ID_MUX,   0, MUX_GRP_INDEX};

    ret = AW_MPI_SYS_Bind(&ClockChn, &DmxChn);
    assert(ret == SUCCESS);
    alogd("绑定Clock和Demux成功");

    MppBindControl stBindControl;
    stBindControl.eDomain = COMP_PortDomainAudio;
    ret = AW_MPI_SYS_BindExt(&DmxChn, &MuxGrp, &stBindControl);
    assert(ret == SUCCESS);
    alogd("绑定Demux和Mux音频端口成功");

#ifdef PASS_VIDEO_DATA
    stBindControl.eDomain = COMP_PortDomainVideo;
    ret = AW_MPI_SYS_BindExt(&DmxChn, &MuxGrp, &stBindControl);
    assert(ret == SUCCESS);
    alogd("绑定Demux和Mux视频端口成功");
#endif

    // 启动
    AW_MPI_CLOCK_Start(CLOCK_CHN_INDEX);
    AW_MPI_DEMUX_Start(DEMUX_CHN_INDEX);
    AW_MPI_MUX_StartGrp(MUX_GRP_INDEX);

    // 等待文件读取线程结束
    pDemux2MuxerData->mOverFlag = FALSE;
    if (pDemux2MuxerData->mConfigPara.mTestDuration > 0) {
        //等待退出信号到超时
        cdx_sem_down_timedwait(&pDemux2MuxerData->mSemExit, pDemux2MuxerData->mConfigPara.mTestDuration * 1000);
    } else {
        //等待退出信号到永远
        cdx_sem_down(&pDemux2MuxerData->mSemExit);
    }
    pDemux2MuxerData->mOverFlag = TRUE;

    // 停止工作
    AW_MPI_MUX_StopGrp(MUX_GRP_INDEX);
    AW_MPI_DEMUX_Stop(DEMUX_CHN_INDEX);
    AW_MPI_CLOCK_Stop(CLOCK_CHN_INDEX);

    // 销毁通道
    AW_MPI_MUX_DestroyGrp(MUX_GRP_INDEX);
    AW_MPI_DEMUX_DestroyChn(DEMUX_CHN_INDEX);
    AW_MPI_CLOCK_DestroyChn(CLOCK_CHN_INDEX);

    // 销毁MPP
    AW_MPI_SYS_Exit();

    cdx_sem_deinit(&pDemux2MuxerData->mSemExit);
    free(pDemux2MuxerData);
    pDemux2MuxerData = NULL;

    printf("sample_demux2muxer exit!\n");
    return 0;
}
