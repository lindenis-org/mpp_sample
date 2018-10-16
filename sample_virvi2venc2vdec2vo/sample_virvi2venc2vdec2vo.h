#ifndef _SAMPLE_Virvi2Venc2Vdec2VO_H_
#define _SAMPLE_Virvi2Venc2Vdec2VO_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <pthread.h>

#include "mm_comm_sys.h"
#include "mpi_sys.h"

#include "DemuxCompStream.h"
#include "mm_comm_demux.h"
#include "mpi_demux.h"

#include "mm_comm_vdec.h"
#include "mpi_vdec.h"

#include "mm_comm_vo.h"
#include "mpi_vo.h"
#include <tsemaphore.h>

#define MAX_FILE_PATH_SIZE	(256)

typedef enum {
    STATE_PREPARED = 0,
    STATE_PAUSE,
    STATE_PLAY,
    STATE_STOP,
} STATE_E;

enum CLOCK_COMP_PORT_INDEX {
    CLOCK_PORT_AUDIO = 0, //to audio render
    CLOCK_PORT_VIDEO = 1, //to video render
    CLOCK_PORT_DEMUX = 2, //to demux
    CLOCK_PORT_VDEC	 = 3, //to vdec
};

typedef struct SampleVirvi2Venc2Vdec2VOCmdLineParam {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
} SampleVirvi2Venc2Vdec2VOCmdLineParam;

typedef struct SampleVirvi2Venc2Vdec2VOConfig {
    int EncoderCount;
    int DevNum;
    int SrcWidth;
    int SrcHeight;
    int SrcFrameRate;
    PAYLOAD_TYPE_E EncoderType;
    int mTimeLapseEnable;
    int mTimeBetweenFrameCapture;
    int DestWidth;
    int DestHeight;
    int DestFrameRate;
    int DestBitRate;
    PIXEL_FORMAT_E DestPicFormat; //MM_PIXEL_FORMAT_YUV_PLANAR_420

    //vdec
    int mMaxVdecOutputWidth;
    int mMaxVdecOutputHeight;
    int mInitRotation;
    int mUserSetPixelFormat;
    int mCodecType;

    //vo
    int mDisplayWidth;
    int mDisplayHeight;
    VO_INTF_TYPE_E mDispType;
    VO_INTF_SYNC_E mDispSync;
} SampleVirvi2Venc2Vdec2VOConfig;

typedef struct awVI2Venc2Vdec2VO_PrivCap_S {
    SampleVirvi2Venc2Vdec2VOCmdLineParam mCmdLinePara;
    SampleVirvi2Venc2Vdec2VOConfig mConfigPara;

    MPP_SYS_CONF_S mSysConf;

    cdx_sem_t mSemExit;

    pthread_t thid;

    VI_DEV mViDev;
    VI_CHN mViChn;

    int mIspChn;

    VENC_CHN mVenChn;
    PAYLOAD_TYPE_E EncoderType;

    VDEC_CHN mVdecChn;
    VDEC_CHN_ATTR_S mVdecChnAttr;

    int mUILayer;

    VO_DEV mVoDev;
    VO_LAYER mVoLayer;
    VO_CHN mVoChn;
    VO_VIDEO_LAYER_ATTR_S mVoLayerAttr;
    VO_CHN_ATTR_S mVoChnAttr;

    CLOCK_CHN mClockChn;
    CLOCK_CHN_ATTR_S mClockChnAttr;

    BOOL mOverFlag;

    FILE * fd_out;
    FILE * fd_out_len;
} VI2Venc2Vdec2VO_Cap_S;

#endif	/* _SAMPLE_Virvi2Venc2Vdec2VO_H_ */

