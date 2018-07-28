
#ifndef _SAMPLE_VIRVI2DFISH2VO_H_
#define _SAMPLE_VIRVI2DFISH2VO_H_

#include <plat_type.h>
#include <tsemaphore.h>

#define MAX_FILE_PATH_SIZE  (256)

typedef struct SampleVirvi2Dfish2VoCmdLineParam {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
} SampleVirvi2Dfish2VoCmdLineParam;

typedef struct SampleVIConfig {
    int DevId;
    int SrcWidth;
    int SrcHeight;
    int SrcFrameRate;
} SampleVIConfig;

typedef struct SampleVOConfig {
    int mFrameRate;
    int mTestDuration;  //unit:s, 0 mean infinite
    int display_width;
    int display_height;
} SampleVOConfig;

typedef struct SampleISEGroupConfig {
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

typedef struct SampleVirvi2Dfish2VoConfig {
    int AutoTestCount;
    SampleVIConfig VIDevConfig;
    SampleISEGroupConfig ISEGroupConfig;
    SampleISEPortConfig ISEPortConfig[4];
    SampleVOConfig  VOChnConfig;
} SampleVirvi2Dfish2VoConfig;

typedef struct SampleVirvi2DfFish2VoConfparser {
    SampleVirvi2Dfish2VoCmdLineParam mCmdLinePara;
    SampleVirvi2Dfish2VoConfig mConfigPara;
} SampleVirvi2Dfish2VoConfparser;

#endif  /* _SAMPLE_VIRVI2DFISH2VO_H_ */

