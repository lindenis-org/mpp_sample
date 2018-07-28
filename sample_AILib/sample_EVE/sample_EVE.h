#ifndef _SAMPLE_EVE_H_
#define _SAMPLE_EVE_H_

#include <aw_type.h>
#include <media/mm_comm_video.h>

#include "aw_ai_eve_event_interface.h"

#define MAX_FILE_PATH_SIZE (256)

typedef struct EVE_CMDLINE_PARAM_S {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
    int iDev;
    int iPicWidth;
    int iPicHeight;

    char pcPixFmt[32];
    int iFrmRate;
    int iFrmTestCont;

    char pcYUVFile[MAX_FILE_PATH_SIZE];
    char pcOutputFile[MAX_FILE_PATH_SIZE];
} EVE_CMDLINE_PARAM_S;

typedef struct EVE_CONF_INFO_S {
    int iDev;
    int iChn;
    int iPicWidth;
    int iPicHeight;

    PIXEL_FORMAT_E ePixFmt;
    int iYSize;
    int iUSize;
    int iVSize;

    int iFrmRate;
    int iFrmTestCont;

    AW_HANDLE psEVEHd;
    AW_AI_EVE_EVENT_RESULT_S stResult;

    char pcYUVFile[MAX_FILE_PATH_SIZE];
    char pcOutputFile[MAX_FILE_PATH_SIZE];

    EVE_CMDLINE_PARAM_S stCmdlineParam;
} EVE_CONF_INFO_S;

#define DEFAULT_DEVICE     3
#define DEFAULT_PIC_WIDTH  640
#define DEFAULT_PIC_HEIGHT 360
#define DEFAULT_FRM_RATE   25
#define DEFAULT_PIX_FMT    MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420
#define DEFAULT_YUV_FILE   "./face.yuv"
#define DEFAULT_OUPUT_FILE "./EVEResult.bin"
#define DEFAULT_TEST_FRAME_COUNT 0

#define CONF_EVE_DEVICE        "dev"
#define CONF_EVE_YUV_DATA_FILE "src_file"
#define CONF_EVE_PIC_WIDTH     "pic_width"
#define CONF_EVE_PIC_HEIGHT    "pic_height"
#define CONF_EVE_PIC_FORMAT    "pic_format"
#define CONF_EVE_FRAME_RATE    "frame_rate"
#define CONF_EVE_OUTPUT_FILE   "output_file"
#define CONF_EVE_TEST_FRM_CONT "test_frame"

#endif /* end of SAMPLE_EVE_H */

