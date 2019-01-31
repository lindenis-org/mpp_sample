
#ifndef _SAMPLE_VIRVI2OPENCV2VO_H_
#define _SAMPLE_VIRVI2OPENCV2VO_H_

#include <plat_type.h>
#include <tsemaphore.h>

#define MAX_FILE_PATH_SIZE  (256)
typedef struct awVirVi_PrivCap_S {
	pthread_t thid;
	VI_DEV Dev;
	VI_CHN Chn;
	AW_S32 s32MilliSec;
	VIDEO_FRAME_INFO_S pstFrameInfo;
	int displayColor;
	int movDetSen;
} VirVi_Cap_S;

typedef struct SampleVirVi2OpenCV2VOCmdLineParam {
	char mConfigFilePath[MAX_FILE_PATH_SIZE];
} SampleVirVi2OpenCV2VOCmdLineParam;

typedef struct SampleVirVi2OpenCV2VOConfig {
	int AutoTestCount;
	int GetFrameCount;
	int DevNum;
	int PicWidth;
	int PicHeight;
	int FrameRate;
	__u32 PicFormat; //MM_PIXEL_FORMAT_YUV_PLANAR_420
	int displayColor;
	int movDetSen;
} SampleVirVi2OpenCV2VOConfig;

typedef struct SampleVirVi2OpenCV2VOConfparser {
	SampleVirVi2OpenCV2VOCmdLineParam mCmdLinePara;
	SampleVirVi2OpenCV2VOConfig mConfigPara;
} SampleVirVi2OpenCV2VOConfparser;

#endif  /* _SAMPLE_VIRVI2OPENCV2VO_H_ */

