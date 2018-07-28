#define LOG_TAG "sample_cve_BDII"
#include <utils/plat_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include "media/mpi_sys.h"
#include <confparser.h>
#include <ion_memmanager.h>
#include "aw_ai_cve_base_type.h"
#include "aw_ai_cve_bdii_interface.h"

#include "sample_cve_BDII.h"

#define TIME_TEST
#ifdef TIME_TEST
#include "SystemBase.h"
#endif

#define HAVE_CVE
#ifdef HAVE_CVE
#include "types.h"
#include "CVEMiddleInterface.h"
#endif

/******************************************************************************
* function : read and process YUV image
******************************************************************************/
int BDIIProcessImage(AW_IMAGE_S *pstImageL, AW_IMAGE_S *pstImageR, CVE_BDII_CONF_INFO_S *pstConfInfo, AW_AI_CVE_BDII_RULT_S *pstResult)
{
    int iRet;
    int iFdL, iFdR;

    iFdL = open(pstConfInfo->pcYUVFileL, O_RDONLY);
    if (iFdL < 0) {
        aloge("open %s file failed!!\n", pstConfInfo->pcYUVFileL);
        iRet = -1;
        goto open_file_l_err;
    }
    iFdR = open(pstConfInfo->pcYUVFileR, O_RDONLY);
    if (iFdR < 0) {
        aloge("open %s file failed!!\n", pstConfInfo->pcYUVFileR);
        iRet = -1;
        goto open_file_r_err;
    }

    struct stat stFstatL;
    iRet = fstat(iFdL, &stFstatL);
    if (iRet < 0) {
        aloge("get file status failed!!\n");
        goto file_stat_l_err;
    }
    struct stat stFstatR;
    iRet = fstat(iFdR, &stFstatR);
    if (iRet < 0) {
        aloge("get file status failed!!\n");
        goto file_stat_r_err;
    }

    char *pcFileMemStartL, *pcFileMemStartR;
    char *pcFileMemSeekL, *pcFileMemSeekR;
    pcFileMemStartL = mmap(NULL, stFstatL.st_size, PROT_READ, MAP_SHARED, iFdL, 0);
    if (NULL == pcFileMemStartL) {
        aloge("mmap file memory failed!!\n");
        goto file_map_l_err;
    }
    pcFileMemStartR = mmap(NULL, stFstatR.st_size, PROT_READ, MAP_SHARED, iFdR, 0);
    if (NULL == pcFileMemStartR) {
        aloge("mmap file memory failed!!\n");
        goto file_map_r_err;
    }

    pcFileMemSeekL = pcFileMemStartL;
    pcFileMemSeekR = pcFileMemStartR;

    FILE *pOutFd;
    char pcOutPath[256];
    strncpy(pcOutPath, pstConfInfo->pcOutputFile, sizeof(pcOutPath));
    pOutFd = fopen(pcOutPath, "wb");
    if (NULL == pOutFd) {
        aloge("open output file failed!!\n");
        iRet = -1;
        goto open_result_file_err;
    }

    int iFrmIndex = 0;
    while (1) {
        if (iFrmIndex >= pstConfInfo->iFrmTestCont && pstConfInfo->iFrmTestCont != 0) {
            alogd("end of test frame count number[%d]!!\n", iFrmIndex);
            break;
        }
        if (pcFileMemSeekL >= pcFileMemStartL + stFstatL.st_size) {
            alogd("end of file!!\n");
            break;
        }
        if (pcFileMemSeekR >= pcFileMemStartR + stFstatR.st_size) {
            alogd("end of file!!\n");
            break;
        }
        memcpy(pstImageL->mpVirAddr[0], pcFileMemSeekL, pstConfInfo->iYSize);
        pcFileMemSeekL += pstConfInfo->iYSize + pstConfInfo->iUSize + pstConfInfo->iVSize;
        iRet = ion_flushCache(pstImageL->mpVirAddr[0], pstConfInfo->iYSize);
        memcpy(pstImageR->mpVirAddr[0], pcFileMemSeekR, pstConfInfo->iYSize);
        pcFileMemSeekR += pstConfInfo->iYSize + pstConfInfo->iUSize + pstConfInfo->iVSize;
        iRet = ion_flushCache(pstImageR->mpVirAddr[0], pstConfInfo->iYSize);

        AW_STATUS_E eProcessRet;
        alogd("begin to process yuv image file, process frame index is [%d]\n", iFrmIndex);
#ifdef TIME_TEST
        int64_t nStartTm = CDX_GetSysTimeUsMonotonic();//start time
#endif
        eProcessRet = AW_AI_CVE_BDII_Process(pstConfInfo->psBDIIHd, pstImageL, pstImageR, pstResult);
        if (eProcessRet == AW_STATUS_OK) {
            fwrite(pstResult->as16DeepImg, 1, pstConfInfo->iYSize * sizeof(AW_S16), pOutFd);
        }
#ifdef TIME_TEST
        int64_t nEndTm = CDX_GetSysTimeUsMonotonic();//end time
        alogd("frame[%d] handle time[%lld]", iFrmIndex, nEndTm - nStartTm);
#endif
        iFrmIndex ++;
    }

    iRet = 0;

    fclose(pOutFd);
open_result_file_err:
    munmap(pcFileMemStartR, stFstatR.st_size);
file_map_r_err:
    munmap(pcFileMemStartL, stFstatL.st_size);
file_map_l_err:
file_stat_r_err:
file_stat_l_err:
    close(iFdR);
open_file_r_err:
    close(iFdL);
open_file_l_err:
    return iRet;
}

static ERRORTYPE LoadConfigPara(CVE_BDII_CONF_INFO_S *pConfInfo, char *pcConfPath)
{
    int iRet;
    char *pcTmpPtr = NULL;
    CONFPARSER_S mConf;

    iRet = createConfParser(pcConfPath, &mConf);
    if (iRet < 0) {
        aloge("load conf fail, use default configuration\n");
        pConfInfo->iPicWidth  = DEFAULT_PIC_WIDTH;
        pConfInfo->iPicHeight = DEFAULT_PIC_HEIGHT;
        pConfInfo->ePixFmt    = DEFAULT_PIX_FMT;
        pConfInfo->iFrmTestCont = DEFAULT_TEST_FRAME_COUNT;
        strcpy(pConfInfo->pcYUVFileL, DEFAULT_LEFT_YUV_FILE);
        strcpy(pConfInfo->pcYUVFileR, DEFAULT_RIGHT_YUV_FILE);
        strcpy(pConfInfo->pcOutputFile, DEFAULT_OUPUT_FILE);

        goto create_conf_err;
    }

    pConfInfo->iPicWidth  = GetConfParaInt(&mConf, CONF_CVE_BDII_PIC_WIDTH, 0);
    pConfInfo->iPicHeight = GetConfParaInt(&mConf, CONF_CVE_BDII_PIC_HEIGHT, 0);
    pConfInfo->iFrmRate   = GetConfParaInt(&mConf, CONF_CVE_BDII_FRAME_RATE, 0);
    pConfInfo->iFrmTestCont = GetConfParaInt(&mConf, CONF_CVE_BDII_TEST_FRM_CONT, 0);

    pcTmpPtr = (char *)GetConfParaString(&mConf, CONF_CVE_BDII_YUV_DATA_FILE_L, NULL);
    if (NULL != pcTmpPtr) {
        strncpy(pConfInfo->pcYUVFileL, pcTmpPtr, sizeof(pConfInfo->pcYUVFileL));
    }
    pcTmpPtr = (char *)GetConfParaString(&mConf, CONF_CVE_BDII_YUV_DATA_FILE_R, NULL);
    if (NULL != pcTmpPtr) {
        strncpy(pConfInfo->pcYUVFileR, pcTmpPtr, sizeof(pConfInfo->pcYUVFileR));
    }
    pcTmpPtr = (char *)GetConfParaString(&mConf, CONF_CVE_BDII_OUTPUT_FILE, NULL);
    if (NULL != pcTmpPtr) {
        strncpy(pConfInfo->pcOutputFile, pcTmpPtr, sizeof(pConfInfo->pcOutputFile));
    }

    pConfInfo->ePixFmt    = DEFAULT_PIX_FMT;
    pcTmpPtr = (char *)GetConfParaString(&mConf, CONF_CVE_BDII_PIC_FORMAT, NULL);

create_conf_err:
    if (pConfInfo->stCmdlineParam.iPicWidth != -1) {
        pConfInfo->iPicWidth = pConfInfo->stCmdlineParam.iPicWidth;
    }
    if (pConfInfo->stCmdlineParam.iPicHeight != -1) {
        pConfInfo->iPicHeight = pConfInfo->stCmdlineParam.iPicHeight;
    }
    if (pConfInfo->stCmdlineParam.iFrmRate != -1) {
        pConfInfo->iFrmRate = pConfInfo->stCmdlineParam.iFrmRate;
    }
    if (pConfInfo->stCmdlineParam.iFrmTestCont != -1) {
        pConfInfo->iFrmTestCont = pConfInfo->stCmdlineParam.iFrmTestCont;
    }

    if (pConfInfo->stCmdlineParam.pcYUVFileL[0] != 0) {
        strncpy(pConfInfo->pcYUVFileL, pConfInfo->stCmdlineParam.pcYUVFileL, sizeof(pConfInfo->pcYUVFileL));
    }
    if (pConfInfo->stCmdlineParam.pcYUVFileR[0] != 0) {
        strncpy(pConfInfo->pcYUVFileR, pConfInfo->stCmdlineParam.pcYUVFileR, sizeof(pConfInfo->pcYUVFileR));
    }
    if (pConfInfo->stCmdlineParam.pcOutputFile[0] != 0) {
        strncpy(pConfInfo->pcOutputFile, pConfInfo->stCmdlineParam.pcOutputFile, sizeof(pConfInfo->pcOutputFile));
    }

    if (pConfInfo->stCmdlineParam.pcPixFmt[0] != 0) {
        pcTmpPtr = (char *)pConfInfo->stCmdlineParam.pcPixFmt;
    }
    if (NULL != pcTmpPtr) {
        if (!strcmp(pcTmpPtr, "yu12")) {
            pConfInfo->ePixFmt = MM_PIXEL_FORMAT_YUV_PLANAR_420;
        } else if (!strcmp(pcTmpPtr, "yv12")) {
            pConfInfo->ePixFmt = MM_PIXEL_FORMAT_YVU_PLANAR_420;
        } else if (!strcmp(pcTmpPtr, "nv21")) {
            pConfInfo->ePixFmt = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        } else if (!strcmp(pcTmpPtr, "nv12")) {
            pConfInfo->ePixFmt = MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        } else if (!strcmp(pcTmpPtr, "yuv422p")) {
            pConfInfo->ePixFmt = MM_PIXEL_FORMAT_YUV_PLANAR_422;
        } else if (!strcmp(pcTmpPtr, "yuv422sp")) {
            pConfInfo->ePixFmt = MM_PIXEL_FORMAT_YUV_SEMIPLANAR_422;
        } else {
            aloge("not support the pixfmt, use default configuration 'MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420'!!");
        }
    }

    alogd("src_file_l=%s, src_file_r=%s, output_file=%s, pic_width=%d, pic_height=%d, pic_format=%d, frm_test_cont=%d\n",\
          pConfInfo->pcYUVFileL, pConfInfo->pcYUVFileR, pConfInfo->pcOutputFile,\
          pConfInfo->iPicWidth, pConfInfo->iPicHeight, pConfInfo->ePixFmt, pConfInfo->iFrmTestCont);

    destroyConfParser(&mConf);
    return SUCCESS;
}

static struct option pstLongOptions[] = {
    {"help",    no_argument,       0, 'h'},
    {"path",    required_argument, 0, 'p'},
    {"width",   required_argument, 0, 'x'},
    {"height",  required_argument, 0, 'y'},
    {"testcnt", required_argument, 0, 't'},
    {"left",    required_argument, 0, 'l'},
    {"right",   required_argument, 0, 'r'},
    {"output",  required_argument, 0, 'o'},
    {0,         0,                 0,  0 }
};

static char pcHelpInfo[] =
    "\033[33m"
    "exec [-h|--help] [-p|--path]\n"
    "   <-h|--help>: print to the help information\n"
    "   <-p|--path>   <args>: point to the configuration file path\n"
    "   <-x|--width>  <args>: set video picture width\n"
    "   <-y|--height> <args>: set video picture height\n"
    "   <-e|--format> <args>: set the video format\n"
    "   <-t|--testcnt><args>: set the test frame count number\n"
    "   <-l|--left>   <args>: point to the left yuv image file\n"
    "   <-r|--right>  <args>: point to the right yuv image file\n"
    "   <-o|--output> <args>: output result to this file\n"
    "\033[0m\n";

static int ParseCmdLine(int argc, char **argv, CVE_BDII_CMDLINE_PARAM_S *pCmdLinePara)
{
    int mRet;
    int iOptIndex = 0;

    memset(pCmdLinePara, -1, sizeof(CVE_BDII_CMDLINE_PARAM_S));
    pCmdLinePara->pcYUVFileL[0] = 0;
    pCmdLinePara->pcYUVFileR[0] = 0;
    pCmdLinePara->mConfigFilePath[0] = 0;
    pCmdLinePara->pcOutputFile[0] = 0;
    pCmdLinePara->pcPixFmt[0]  = 0;
    while (1) {
        mRet = getopt_long(argc, argv, ":p:hx:y:e:t:l:r:o:", pstLongOptions, &iOptIndex);
        if (mRet == -1) {
            break;
        }

        switch (mRet) {
        case 'h':
            printf("%s", pcHelpInfo);
            goto print_help_exit;
            break;
            /* let the "sampleXXX -path sampleXXX.conf" command to be compatible with
             * "sampleXXX -p sampleXXX.conf"
             */
        case 'p':
            if (strcmp("ath", optarg) == 0) {
                if (NULL == argv[optind]) {
                    printf("%s", pcHelpInfo);
                    goto opt_need_arg;
                }
                alogd("path is [%s]\n", argv[optind]);
                strncpy(pCmdLinePara->mConfigFilePath, argv[optind], sizeof(pCmdLinePara->mConfigFilePath));
            } else {
                alogd("path is [%s]\n", optarg);
                strncpy(pCmdLinePara->mConfigFilePath, optarg, sizeof(pCmdLinePara->mConfigFilePath));
            }
            break;
        case 'x':
            alogd("width is [%d]\n", atoi(optarg));
            pCmdLinePara->iPicWidth = atoi(optarg);
            break;
        case 'y':
            alogd("height is [%d]\n", atoi(optarg));
            pCmdLinePara->iPicHeight = atoi(optarg);
            break;
        case 'e':
            alogd("format is [%s]\n", optarg);
            strncpy(pCmdLinePara->pcPixFmt, optarg, sizeof(pCmdLinePara->pcPixFmt));
            break;
        case 't':
            alogd("test frame count is [%d]\n", atoi(optarg));
            pCmdLinePara->iFrmTestCont = atoi(optarg);
            break;
        case 'l':
            alogd("left yuv file is [%s]\n", optarg);
            strncpy(pCmdLinePara->pcYUVFileL, optarg, sizeof(pCmdLinePara->pcYUVFileL));
            break;
        case 'r':
            alogd("right yuv file is [%s]\n", optarg);
            strncpy(pCmdLinePara->pcYUVFileR, optarg, sizeof(pCmdLinePara->pcYUVFileR));
            break;
        case 'o':
            alogd("output file is [%s]\n", optarg);
            strncpy(pCmdLinePara->pcOutputFile, optarg, sizeof(pCmdLinePara->pcOutputFile));
            break;
        case ':':
            aloge("option \"%s\" need <arg>\n", argv[optind - 1]);
            goto opt_need_arg;
            break;
        case '?':
            if (optind > 2) {
                break;
            }
            aloge("unknow option \"%s\"\n", argv[optind - 1]);
            printf("%s", pcHelpInfo);
            goto unknow_option;
            break;
        default:
            printf("?? why getopt_long returned character code 0%o ??\n", mRet);
            break;
        }
    }

    return 0;
opt_need_arg:
unknow_option:
print_help_exit:
    return -1;
}

/******************************************************************************
* Author:
* FileName: main.cpp
* Version:  V1.0
* function    : main()
* Description : Binocular depth information image(BDII) Lib Sample
******************************************************************************/
int main(int argc, char *argv[])
{
    int iRet;

    alogd("sample_cve_BDII running!\n");
    CVE_BDII_CONF_INFO_S stBDIIConfInfo;
    memset(&stBDIIConfInfo, 0, sizeof(CVE_BDII_CONF_INFO_S));

    iRet = ParseCmdLine(argc, argv, &stBDIIConfInfo.stCmdlineParam);
    if (iRet) {
//        aloge("parse cmdline error.\n");
        return -1;
    }

    /* will always return SUCCESS */
    LoadConfigPara(&stBDIIConfInfo, stBDIIConfInfo.stCmdlineParam.mConfigFilePath);

    MPP_SYS_CONF_S mSysConf;
    memset(&mSysConf,0,sizeof(MPP_SYS_CONF_S));
    mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&mSysConf);
    iRet = AW_MPI_SYS_Init();
    if (iRet < 0) {
        aloge("sys init failed");
        return -1;
    }

    /* create and initialize BDII module */
    AW_HANDLE pBDIIHd;
    pBDIIHd = AW_AI_CVE_BDII_Create();
    if (NULL == pBDIIHd) {
        aloge("AW_AI_CVE_BDII_Create failed!!\n");
        iRet = -1;
        goto bdii_create_err;
    }
    stBDIIConfInfo.psBDIIHd = pBDIIHd;

    AW_AI_CVE_BDII_INIT_PARA_S stBDIIParam;                 // set BDII algorithm init para
    memset(&stBDIIParam,0,sizeof(stBDIIParam));

    /* ftzero:映射范围[16-128],默认31  */
    stBDIIParam.u8ftzero = 31;
    /* maxDisparity:视差最大值[16~128]，默认64 */
    stBDIIParam.u8maxDisparity = 64;
    /* SADWindowSize:统计窗口尺寸[5~21]，默认7，要求奇数值 */
    stBDIIParam.u8SADWindowSize = 7;
    /* textureThreshold:纹理阈值参数[1~128]，默认10 */
    stBDIIParam.u8textureThreshold = 10;
    /* uniquenessRatio:纹理阈值参数[1~50]，默认15 */
    stBDIIParam.u8uniquenessRatio = 15;
    /* disp12MaxDiff:验证可容许最大偏移[1~10]，默认1 */
    stBDIIParam.u8disp12MaxDiff = 1;
    iRet = AW_AI_CVE_BDII_Init(pBDIIHd, &stBDIIParam);            // init BDII algorithm
    if (AW_STATUS_OK != iRet) {
        aloge("AW_AI_CVE_BDII_Init failed\n");
        iRet = -1;
        goto bdii_init_err;
    }

    /* alloc memory */
    int iYSize;
    int iUSize;
    int iVSize;

    iYSize = stBDIIConfInfo.iPicWidth * stBDIIConfInfo.iPicHeight;
    switch (stBDIIConfInfo.ePixFmt) {
    case MM_PIXEL_FORMAT_YUV_PLANAR_420:
    case MM_PIXEL_FORMAT_YVU_PLANAR_420:
        iUSize = iVSize = iYSize / 4;
        break;
    case MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420:
    case MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420:
        iUSize = iYSize / 2;
        iVSize = 0;
        break;
    case MM_PIXEL_FORMAT_YUV_PLANAR_422:
        iUSize = iVSize = iYSize / 2;
        break;
    case MM_PIXEL_FORMAT_YUV_SEMIPLANAR_422:
        iUSize = iYSize;
        iVSize = 0;
        break;
    default:
        aloge("unsupported format!!\n");
        goto unknow_format;
    }
    stBDIIConfInfo.iYSize = iYSize;
    stBDIIConfInfo.iUSize = iUSize;
    stBDIIConfInfo.iVSize = iVSize;

    iRet = ion_memOpen();
    if (iRet < 0) {
        aloge("ion_memOpen failed!!\n");
        goto ion_open_err;
    }

    AW_IMAGE_S  stImageL;
    AW_IMAGE_S  stImageR;

    memset(&stImageL, 0, sizeof(AW_IMAGE_S));
    memset(&stImageR, 0, sizeof(AW_IMAGE_S));

    stImageL.mWidth       = stBDIIConfInfo.iPicWidth;
    stImageL.mHeight      = stBDIIConfInfo.iPicHeight;
    stImageL.mStride[0]   = stBDIIConfInfo.iPicWidth;
    /* alloc memory use ION */
    stImageL.mpVirAddr[0] = ion_allocMem(iYSize);
    if (NULL == stImageL.mpVirAddr[0]) {
        aloge("ion_allocMem failed!!\n");
        iRet = -1;
        goto ion_alloc_l_err;
    }
    /* fluse cache */
    iRet = ion_flushCache(stImageL.mpVirAddr[0], iYSize);

    stImageR.mWidth       = stBDIIConfInfo.iPicWidth;
    stImageR.mHeight      = stBDIIConfInfo.iPicHeight;
    stImageR.mStride[0]   = stBDIIConfInfo.iPicWidth;
    /* alloc memory use ION */
    stImageR.mpVirAddr[0] = ion_allocMem(iYSize);
    if (NULL == stImageR.mpVirAddr[0]) {
        aloge("ion_allocMem failed!!\n");
        iRet = -1;
        goto ion_alloc_r_err;
    }
    /* fluse cache */
    iRet = ion_flushCache(stImageR.mpVirAddr[0], iYSize);

    /******************************************
    step  3: BDII algo process
    ******************************************/
    AW_AI_CVE_BDII_RULT_S stBDIIResult;

    memset(&stBDIIResult, 0, sizeof(AW_AI_CVE_BDII_RULT_S));
    stBDIIResult.as16DeepImg = (AW_S16 *)ion_allocMem(iYSize * sizeof(AW_S16));
    stBDIIResult.as32CostImg = (AW_S32 *)ion_allocMem(iYSize * sizeof(AW_S32));
    if (NULL == stBDIIResult.as16DeepImg || NULL == stBDIIResult.as32CostImg) {
        aloge("ion_allocMem failed!!\n");
        iRet = -1;
        goto ion_alloc_result_err;
    }

    iRet = BDIIProcessImage(&stImageL, &stImageR, &stBDIIConfInfo, &stBDIIResult);
    if (iRet < 0) {
        aloge("reading and process image failure!!\n");
        goto read_img_err;
    }

read_img_err:
    ion_freeMem(stBDIIResult.as16DeepImg);
    ion_freeMem(stBDIIResult.as32CostImg);
    stBDIIResult.as16DeepImg = NULL;
    stBDIIResult.as32CostImg = NULL;
ion_alloc_result_err:
    ion_freeMem(stImageR.mpVirAddr[0]);
    stImageR.mpVirAddr[0] = NULL;
ion_alloc_r_err:
    ion_freeMem(stImageL.mpVirAddr[0]);
    stImageL.mpVirAddr[0] = NULL;
ion_alloc_l_err:
    ion_memClose();
ion_open_err:
unknow_format:
bdii_init_err:
    AW_AI_CVE_BDII_Release(pBDIIHd);
bdii_create_err:
    if (0 == iRet) {
        printf("sample_cve_BDII exit!\n");
    }
    return iRet;
}





