
#ifndef _SAMPLE_FISH_H_
#define _SAMPLE_FISH_H_

#include <plat_type.h>
#include <tsemaphore.h>

#define MAX_FILE_PATH_SIZE  (256)

typedef struct SampleFishCmdLineParam {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
} SampleFishCmdLineParam;

typedef struct SamplePicConfig {
    int PicWidth;
    int PicHeight;
    int PicStride;
    int PicFrameRate;
    char PicFilePath[MAX_FILE_PATH_SIZE];
} SamplePicConfig;

typedef struct SampleISEGroupConfig {
    int ISEPortNum;
    WARP_Type_MO ISE_Dewarp_Mode;
    MOUNT_Type_MO Mount_Mode;
    float Lens_Parameter_P;
    float Lens_Parameter_Cx;
    float Lens_Parameter_Cy;
    int normal_pan;
    int normal_tilt;
    int normal_zoom;
    char OutputFilePath[MAX_FILE_PATH_SIZE];
} SampleISEGroupConfig;

typedef struct SampleISEPortConfig {
    int ISEWidth;
    int ISEHeight;
    int ISEStride;
    int flip_enable;
    int mirror_enable;
} SampleISEPortConfig;

typedef struct SampleFishConfig {
    int AutoTestCount;
    int Process_Count;
    SamplePicConfig PicConfig;
    SampleISEGroupConfig ISEGroupConfig;
    SampleISEPortConfig ISEPortConfig[4];
} SampleFishConfig;

typedef struct SampleFishConfparser {
    SampleFishCmdLineParam mCmdLinePara;
    SampleFishConfig mConfigPara;
} SampleFishConfparser;

#endif  /* _SAMPLE_FISH_H_ */

