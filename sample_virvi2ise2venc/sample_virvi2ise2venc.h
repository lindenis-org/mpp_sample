
#ifndef _SAMPLE_VIRVI2ISE2VENC_H_
#define _SAMPLE_VIRVI2ISE2VENC_H_

#include <plat_type.h>
#include <tsemaphore.h>

#define MAX_FILE_PATH_SIZE  (256)

typedef struct SampleVirvi2ISE2VencCmdLineParam {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
} SampleVirvi2ISE2VencCmdLineParam;

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
    int     ISEPortNum;
    int     ncam;
    int     stre_en;
    float   stre_coeff;
    int     offset_r2l;
    int     ov;
    float   pano_fov;
    float   t_angle;
    float   hfov;
    float   wfov;
    float   wfov_rev;
    double  calib_matr[3][3];
    double  calib_matr_cv[3][3];
    double  distort[8];
    float   Lens_Parameter_P0;
    float   Lens_Parameter_P1;
} SampleISEGroupConfig;

typedef struct SampleISEPortConfig {
    int ISEWidth;
    int ISEHeight;
    int ISEStride;
    int flip_enable;
    int mirror_enable;
} SampleISEPortConfig;

typedef struct SampleVirvi2ISE2VencConfig {
    int AutoTestCount;
    SampleVIConfig VIDevConfig;
    SampleISEGroupConfig ISEGroupConfig;
    SampleISEPortConfig ISEPortConfig[4];
    int VencChnNum;
    int EncoderCount;
    PAYLOAD_TYPE_E EncoderType;
    PIXEL_FORMAT_E DestPicFormat; //PIXEL_FORMAT_YUV_PLANAR_420
    SampleVencConfig VencChnConfig[4];
} SampleVirvi2ISE2VencConfig;

typedef struct SampleVirvi2ISE2VencConfparser {
    SampleVirvi2ISE2VencCmdLineParam mCmdLinePara;
    SampleVirvi2ISE2VencConfig mConfigPara;
} SampleVirvi2ISE2VencConfparser;

#endif  /* _SAMPLE_VIRVI2ISE2VENC_H_ */

