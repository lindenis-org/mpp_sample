
#ifndef _SAMPLE_VIPP2DFISH2VENC_H_
#define _SAMPLE_VIRVI2DFISH2VENC_H_

#include <plat_type.h>
#include <tsemaphore.h>

#define MAX_FILE_PATH_SIZE  (256)

typedef struct SampleVirvi2Dfish2VencCmdLineParam {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
} SampleVirvi2Dfish2VencCmdLineParam;

typedef struct SampleVIConfig {
    int DevId;
    int SrcWidth;
    int SrcHeight;
    int SrcFrameRate;
} SampleVIConfig;

typedef struct SampleVencConfig {
    int mTimeLapseEnable;
    int mTimeBetweenFrameCapture;
    int DestWidth;
    int DestHeight;
    int DestFrameRate;
    int DestBitRate;
    char OutputFilePath[MAX_FILE_PATH_SIZE];
} SampleVencConfig;

typedef struct SampleISEGroupConfig {
    int ISEPortNum;
    float Lens_Parameter_P0;
    int Lens_Parameter_Cx0;
    int Lens_Parameter_Cy0;
    float Lens_Parameter_P1;
    int Lens_Parameter_Cx1;
    int Lens_Parameter_Cy1;
    float calib_matr[3][3];
} SampleISEGroupConfig;

typedef struct SampleISEPortConfig {
    int ISEWidth;
    int ISEHeight;
    int ISEStride;
    int flip_enable;
    int mirror_enable;
} SampleISEPortConfig;

typedef struct SampleVirvi2Dfish2VencConfig {
    int AutoTestCount;
    SampleVIConfig VIDevConfig;
    SampleISEGroupConfig ISEGroupConfig;
    SampleISEPortConfig ISEPortConfig[4];
    int VencChnNum;
    PAYLOAD_TYPE_E EncoderType;
    int EncoderCount;
    PIXEL_FORMAT_E DestPicFormat; //PIXEL_FORMAT_YUV_PLANAR_420
    SampleVencConfig VencChnConfig[4];
} SampleVirvi2Dfish2VencConfig;

typedef struct SampleVirvi2Dfish2VencConfparser {
    SampleVirvi2Dfish2VencCmdLineParam mCmdLinePara;
    SampleVirvi2Dfish2VencConfig mConfigPara;
} SampleVirvi2Dfish2VencConfparser;

#endif  /* _SAMPLE_VIRVI2DFISH2VENC_H_ */

