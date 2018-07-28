
#ifndef _SAMPLE_DFISH_H_
#define _SAMPLE_DFISH_H_

#include <plat_type.h>
#include <tsemaphore.h>

#define MAX_FILE_PATH_SIZE  (256)

typedef struct SampleDfishCmdLineParam {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
} SampleDfishCmdLineParam;

typedef struct SamplePicConfig {
    int PicWidth;
    int PicHeight;
    int PicStride;
    int PicFrameRate;
    float calib_matr[3][3];
    char Pic0_FilePath[MAX_FILE_PATH_SIZE];
    char Pic1_FilePath[MAX_FILE_PATH_SIZE];
} SamplePicConfig;

typedef struct SampleISEGroupConfig {
    int ISEPortNum;
    float Lens_Parameter_P0;
    float Lens_Parameter_Cx0;
    float Lens_Parameter_Cy0;
    float Lens_Parameter_P1;
    float Lens_Parameter_Cx1;
    float Lens_Parameter_Cy1;
    float calib_matr[3][3];
    char OutputFilePath[MAX_FILE_PATH_SIZE];
} SampleISEGroupConfig;

typedef struct SampleISEPortConfig {
    int ISEWidth;
    int ISEHeight;
    int ISEStride;
    int flip_enable;
    int mirror_enable;
} SampleISEPortConfig;

typedef struct SampleDfishConfig {
    int AutoTestCount;
    int Process_Count;
    SamplePicConfig PicConfig;
    SampleISEGroupConfig ISEGroupConfig;
    SampleISEPortConfig ISEPortConfig[4];
} SampleDfishConfig;

typedef struct SampleDfishConfparser {
    SampleDfishCmdLineParam mCmdLinePara;
    SampleDfishConfig mConfigPara;
} SampleDfishConfparser;

#endif  /* _SAMPLE_DFISH_H_ */

