#ifndef _SAMPLE_CVE_BDII_H_
#define _SAMPLE_CVE_BDII_H_

#include <aw_type.h>
#include <media/mm_comm_video.h>

#define MAX_FILE_PATH_SIZE (256)

typedef struct CVE_BDII_CMDLINE_PARAM_S {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];

    int iDev;
    int iPicWidth;
    int iPicHeight;

    char pcPixFmt[32];
    int iFrmRate;
    int iFrmTestCont;

    char pcYUVFileL[MAX_FILE_PATH_SIZE];
    char pcYUVFileR[MAX_FILE_PATH_SIZE];
    char pcOutputFile[MAX_FILE_PATH_SIZE];
} CVE_BDII_CMDLINE_PARAM_S;

typedef struct CVE_BDII_CONF_INFO_S {
    int iDev;
    int iPicWidth;
    int iPicHeight;

    PIXEL_FORMAT_E ePixFmt;
    int iYSize;
    int iUSize;
    int iVSize;

    int iFrmRate;
    int iFrmTestCont;

    AW_HANDLE psBDIIHd;
    char pcYUVFileL[MAX_FILE_PATH_SIZE];
    char pcYUVFileR[MAX_FILE_PATH_SIZE];
    char pcOutputFile[MAX_FILE_PATH_SIZE];

    CVE_BDII_CMDLINE_PARAM_S stCmdlineParam;
} CVE_BDII_CONF_INFO_S;

#define DEFAULT_PIC_WIDTH  384
#define DEFAULT_PIC_HEIGHT 288
#define DEFAULT_FRM_RATE   25
#define DEFAULT_PIX_FMT    MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420
#define DEFAULT_LEFT_YUV_FILE   "./left.yuv"
#define DEFAULT_RIGHT_YUV_FILE  "./right.yuv"
#define DEFAULT_OUPUT_FILE      "./BDIIResult.bin"
#define DEFAULT_TEST_FRAME_COUNT 0

#define CONF_CVE_BDII_YUV_DATA_FILE_L "src_file_l"
#define CONF_CVE_BDII_YUV_DATA_FILE_R "src_file_r"
#define CONF_CVE_BDII_OUTPUT_FILE   "output_file"
#define CONF_CVE_BDII_PIC_WIDTH     "pic_width"
#define CONF_CVE_BDII_PIC_HEIGHT    "pic_height"
#define CONF_CVE_BDII_PIC_FORMAT    "pic_format"
#define CONF_CVE_BDII_FRAME_RATE    "frame_rate"
#define CONF_CVE_BDII_TEST_FRM_CONT "test_frame"

#endif /* end of SAMPLE_CVE_BDII_H */

