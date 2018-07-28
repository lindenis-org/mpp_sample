
#ifndef _SAMPLE_VIRVI2ISE2VO_H_
#define _SAMPLE_VIRVI2ISE2VO_H_

#include <plat_type.h>
#include <tsemaphore.h>

#define MAX_FILE_PATH_SIZE  (256)

typedef struct SampleVirvi2ISE2VoCmdLineParam {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
} SampleVirvi2ISE2VoCmdLineParam;

typedef struct SampleVIConfig {
    int DevId;
    int SrcWidth;
    int SrcHeight;
    int SrcFrameRate;
} SampleVIConfig;

typedef struct SampleVOConfig {
    int mTestDuration;  //unit:s, 0 mean infinite
    int display_width;
    int display_height;
} SampleVOConfig;

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

typedef struct SampleVirvi2ISE2VoConfig {
    int AutoTestCount;
    SampleVIConfig VIDevConfig;
    SampleISEGroupConfig ISEGroupConfig;
    SampleISEPortConfig ISEPortConfig[4];
    int VencChnNum;
    int EncoderCount;
    PAYLOAD_TYPE_E EncoderType;
    PIXEL_FORMAT_E DestPicFormat; //PIXEL_FORMAT_YUV_PLANAR_420
    SampleVOConfig VOChnConfig;
} SampleVirvi2ISE2VoConfig;

typedef struct SampleVirvi2ISE2VoConfparser {
    SampleVirvi2ISE2VoCmdLineParam mCmdLinePara;
    SampleVirvi2ISE2VoConfig mConfigPara;
} SampleVirvi2ISE2VoConfparser;

#endif  /* _SAMPLE_VIRVI2ISE2VO_H_ */

