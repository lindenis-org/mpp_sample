#ifndef _SAMPLE_CVE_VLPR_H_
#define _SAMPLE_CVE_VLPR_H_

#include <aw_type.h>
#include <media/mm_comm_video.h>

#define MAX_FILE_PATH_SIZE (256)

typedef struct CVE_VLPR_CMDLINE_PARAM_S {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
    int iPicWidth;
    int iPicHeight;

    char pcPixFmt[32];
    int iFrmTestCont;

    char pcYUVFile[MAX_FILE_PATH_SIZE];
    char pcOutputFile[MAX_FILE_PATH_SIZE];
} CVE_VLPR_CMDLINE_PARAM_S;

typedef struct CVE_VLPR_CONF_INFO_S {
    int iPicWidth;
    int iPicHeight;

    PIXEL_FORMAT_E ePixFmt;
    int iYSize;
    int iUSize;
    int iVSize;

    int iFrmTestCont;

    AW_HANDLE psVLPRHd;
    char pcYUVFile[MAX_FILE_PATH_SIZE];
    char pcOutputFile[MAX_FILE_PATH_SIZE];

    CVE_VLPR_CMDLINE_PARAM_S stCmdlineParam;
} CVE_VLPR_CONF_INFO_S;

#define DEFAULT_PIC_WIDTH  1280
#define DEFAULT_PIC_HEIGHT 720
#define DEFAULT_PIX_FMT    MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420
#define DEFAULT_YUV_FILE   "./plate.yuv"
#define DEFAULT_OUPUT_FILE "./VLPRResult.bin"
#define DEFAULT_TEST_FRAME_COUNT 0

#define CONF_CVE_VLPR_YUV_DATA_FILE "src_file"
#define CONF_CVE_VLPR_PIC_WIDTH     "pic_width"
#define CONF_CVE_VLPR_PIC_HEIGHT    "pic_height"
#define CONF_CVE_VLPR_PIC_FORMAT    "pic_format"
#define CONF_CVE_VLPR_OUTPUT_FILE   "output_file"
#define CONF_CVE_VLPR_TEST_FRM_CONT "test_frame"

#endif /* end of SAMPLE_CVE_VLPR_H */

