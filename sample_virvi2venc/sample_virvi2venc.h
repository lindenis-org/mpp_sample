#ifndef _SAMPLE_VIRVI2VENC_H_
#define _SAMPLE_VIRVI2VENC_H_

#include <plat_type.h>
#include <tsemaphore.h>

#define MAX_FILE_PATH_SIZE  (256)
typedef struct awVI2Venc_PrivCap_S {
    pthread_t thid;
    VI_DEV Dev;
    VI_CHN Chn;
    AW_S32 s32MilliSec;
    PAYLOAD_TYPE_E EncoderType;
} VI2Venc_Cap_S;

typedef struct SampleVirvi2VencCmdLineParam {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
} SampleVirvi2VencCmdLineParam;

typedef struct SampleVirvi2VencConfig {
    int AutoTestCount;
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
    char OutputFilePath[MAX_FILE_PATH_SIZE];
} SampleVirvi2VencConfig;

typedef struct SampleVirvi2VencConfparser {
    SampleVirvi2VencCmdLineParam mCmdLinePara;
    SampleVirvi2VencConfig mConfigPara;
} SampleVirvi2VencConfparser;

#endif  /* _SAMPLE_VIRVI2VENC_H_ */

