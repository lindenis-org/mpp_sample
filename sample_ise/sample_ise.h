
#ifndef _SAMPLE_ISE_H_
#define _SAMPLE_ISE_H_

#include <plat_type.h>
#include <tsemaphore.h>

#define MAX_FILE_PATH_SIZE  (256)

typedef struct SampleISECmdLineParam {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
} SampleISECmdLineParam;

typedef struct SamplePicConfig {
    int PicWidth;
    int PicHeight;
    int PicStride;
    int PicFrameRate;
    char Pic0_FilePath[MAX_FILE_PATH_SIZE];
    char Pic1_FilePath[MAX_FILE_PATH_SIZE];
} SamplePicConfig;

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
    char    OutputFilePath[MAX_FILE_PATH_SIZE];
} SampleISEGroupConfig;

typedef struct SampleISEPortConfig {
    int ISEWidth;
    int ISEHeight;
    int ISEStride;
    int flip_enable;
    int mirror_enable;
} SampleISEPortConfig;

typedef struct SampleISEConfig {
    int AutoTestCount;
    int Process_Count;
    SamplePicConfig PicConfig;
    SampleISEGroupConfig ISEGroupConfig;
    SampleISEPortConfig ISEPortConfig[4];
} SampleISEConfig;

typedef struct SampleISEConfparser {
    SampleISECmdLineParam mCmdLinePara;
    SampleISEConfig mConfigPara;
} SampleISEConfparser;

#endif  /* _SAMPLE_ISE_H_ */

