#define LOG_TAG "sample_vi2MOD"
#include <utils/plat_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <confparser.h>

#define TIME_TEST
#ifdef TIME_TEST
#include "SystemBase.h"
#endif

#include "aw_ai_cve_base_type.h"
#include "aw_ai_common_type.h"
#include "aw_ai_cve_dtca_interface.h"
#include "CVEMiddleInterface.h"
#include "mm_comm_video.h"
#include <mpi_videoformat_conversion.h>
#include "media/mpi_sys.h"
#include "media/mpi_vi.h"
#include "media/mpi_isp.h"

#include "sample_vi2MOD.h"

/* use ion or malloc to alloc memory
 * attention: must use ion
 */
#if 0
#define USE_ION
#else
#define USE_MALLOC
#endif

static int g_iThreadExitFlag = 0;

static void SigHandle(int iArg)
{
    g_iThreadExitFlag = 1;
}

/******************************************************************************
* function : read and process YUV image
******************************************************************************/
void *DTCAProcessThread(void *pArg)
{
    int iRet;
    CVE_DTCA_CONF_INFO_S *pstConfInfo = (CVE_DTCA_CONF_INFO_S *)pArg;

    AW_IMAGE_S stImage;

    FILE *pOutFd;
    char pcOutPath[256];
    char pcTmp[1024];
    strncpy(pcOutPath, pstConfInfo->pcOutputFile, sizeof(pcOutPath));
    pOutFd = fopen(pcOutPath, "wb");
    if (NULL == pOutFd) {
        aloge("open output file %s failed!!\n", pcOutPath);
        iRet = -1;
        goto open_result_file_err;
    }

    int iFindTargetNum = 0;
    int iFrameIndex   = 0;
    AW_STATUS_E eProcessRet;
    AW_U32 iTimeStamp  = 0;
    AW_AI_CVE_DTCA_RESULT_S *pstDTCAResult = &pstConfInfo->stResult;
    VIDEO_FRAME_INFO_S stFrame;

    while (1) {
        if (g_iThreadExitFlag) {
            g_iThreadExitFlag = 0;
            break;
        }
        if (iFrameIndex >= pstConfInfo->iFrmTestCont && pstConfInfo->iFrmTestCont != 0) {
            alogd("end of test frame count number[%d]!!\n", iFrameIndex);
            break;
        }

        iRet = AW_MPI_VI_GetFrame(pstConfInfo->iDev, pstConfInfo->iChn, &stFrame, 50);
        if (iRet != 0) {
            aloge("get one frame failed!!\n");
            continue;
        }

        /* set image attribution */
        stImage.mPixelFormat = stFrame.VFrame.mPixelFormat;
        stImage.mWidth       = stFrame.VFrame.mWidth;
        stImage.mHeight      = stFrame.VFrame.mHeight;
        stImage.mStride[0]   = stFrame.VFrame.mWidth;
        stImage.mpVirAddr[0] = stFrame.VFrame.mpVirAddr[0];
        stImage.mpVirAddr[1] = stFrame.VFrame.mpVirAddr[1];
        stImage.mpVirAddr[2] = stFrame.VFrame.mpVirAddr[2];
        iTimeStamp = stFrame.VFrame.mpts / 1000;

#ifdef TIME_TEST
        int64_t nStartTm = CDX_GetSysTimeUsMonotonic();//start time
#endif
        eProcessRet = AW_AI_CVE_DTCA_Process(pstConfInfo->psDTCAHd, &stImage, iTimeStamp, pstDTCAResult);
#ifdef TIME_TEST
        int64_t nEndTm = CDX_GetSysTimeUsMonotonic();//end time
        alogd("frame[%d] handle time[%lld]", iFrameIndex, nEndTm-nStartTm);
#endif
        if (eProcessRet == AW_STATUS_OK) {
            if (pstDTCAResult->stTargetSet.s32TargetNum > 0) {
                iRet = snprintf(pcTmp, sizeof(pcTmp), "=============frame %d==============\n", iFrameIndex);
                pcTmp[1023] = '\0';
                fwrite(pcTmp, 1, iRet, pOutFd);
            }

            for (int i = 0; i < pstDTCAResult->stTargetSet.s32TargetNum; i++) {
                printf("\033[33m");
                printf("=================================Get one target============================\n");
                printf("the frame index is [%d]\n", iFrameIndex);
                printf("the target ID is %u\n", pstDTCAResult->stTargetSet.astTargets[i].u32ID);
                printf("the target type is %u\n", pstDTCAResult->stTargetSet.astTargets[i].u32Type);
                printf("the target point is [%d, %d]\n", pstDTCAResult->stTargetSet.astTargets[i].stPoint.s16X,
                       pstDTCAResult->stTargetSet.astTargets[i].stPoint.s16Y);
                printf("the target rect is [%d, %d, %d, %d]\n",
                       pstDTCAResult->stTargetSet.astTargets[i].stRect.s16Left,
                       pstDTCAResult->stTargetSet.astTargets[i].stRect.s16Top,
                       pstDTCAResult->stTargetSet.astTargets[i].stRect.s16Right,
                       pstDTCAResult->stTargetSet.astTargets[i].stRect.s16Bottom);
                printf("the target area is %d\n", pstDTCAResult->stTargetSet.astTargets[i].s32Area);
                printf("the target speed is %f\n", pstDTCAResult->stTargetSet.astTargets[i].fSpeed);
                printf("===========================================================================\n");
                printf("\033[0m");
                fflush(stdout); // flush stdout, resume console's output format
                iFindTargetNum ++;

                iRet = snprintf(pcTmp, sizeof(pcTmp),\
                                "the target ID is %u\n"\
                                "the target type is %u\n"\
                                "the target point is [%d, %d]\n"\
                                "the target rect is [%d, %d, %d, %d]\n"\
                                "the target area is %d\n"\
                                "the target speed is %f\n\n",\
                                pstDTCAResult->stTargetSet.astTargets[i].u32ID, pstDTCAResult->stTargetSet.astTargets[i].u32Type,\
                                pstDTCAResult->stTargetSet.astTargets[i].stPoint.s16X, pstDTCAResult->stTargetSet.astTargets[i].stPoint.s16Y,\
                                pstDTCAResult->stTargetSet.astTargets[i].stRect.s16Left, pstDTCAResult->stTargetSet.astTargets[i].stRect.s16Top,\
                                pstDTCAResult->stTargetSet.astTargets[i].stRect.s16Right, pstDTCAResult->stTargetSet.astTargets[i].stRect.s16Bottom,\
                                pstDTCAResult->stTargetSet.astTargets[i].s32Area, pstDTCAResult->stTargetSet.astTargets[i].fSpeed);
                pcTmp[1023] = '\0';
                fwrite(pcTmp, 1, iRet, pOutFd);
            }
        }

        AW_MPI_VI_ReleaseFrame(pstConfInfo->iDev, pstConfInfo->iChn, &stFrame);
//        iTimeStamp += 1000 / pstConfInfo->iFrmRate;
        iFrameIndex ++;
    }

    if (0 == iFindTargetNum) {
        printf("\033[31m");
        printf("==================================\n");
        printf("can not find the a target\n");
        printf("==================================\n");
        printf("\033[0m");
        fflush(stdout);
    }

    iRet = 0;
    fclose(pOutFd);
open_result_file_err:
    pthread_exit(&iRet);
}

static ERRORTYPE InitVippDevice(CVE_DTCA_CONF_INFO_S *pConfInfo)
{
    int iRet = SUCCESS;

    if (pConfInfo->iDev > 3 || pConfInfo->iDev < 0) {
        aloge("wrong device number[%d]!\n", pConfInfo->iDev);
        return -1;
    }

    MPP_SYS_CONF_S mSysConf;
    mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&mSysConf);
    iRet = AW_MPI_SYS_Init();
    if (iRet < 0) {
        aloge("sys Init failed!");
        goto sys_init_err;
    }

    VI_ATTR_S stAttr;
    /* dev:0, chn:0,1,2,3 */
    /* dev:1, chn:0,1,2,3 */
    /* dev:2, chn:0,1,2,3 */
    /* dev:3, chn:0,1,2,3 */
    /*Set VI Channel Attribute*/
    stAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    stAttr.memtype = V4L2_MEMORY_MMAP;
    stAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(pConfInfo->ePixFmt);
    stAttr.format.field = V4L2_FIELD_NONE;
    stAttr.format.width = pConfInfo->iPicWidth;
    stAttr.format.height = pConfInfo->iPicHeight;
    stAttr.nbufs = 5;
    stAttr.nplanes = 2;
    stAttr.fps = pConfInfo->iFrmRate;
    stAttr.capturemode = V4L2_MODE_VIDEO;     // V4L2_MODE_VIDEO; V4L2_MODE_IMAGE; V4L2_MODE_PREVIEW
    stAttr.use_current_win = 0;               // always update the attribution
    AW_MPI_VI_CreateVipp(pConfInfo->iDev);
    AW_MPI_VI_SetVippAttr(pConfInfo->iDev, &stAttr); // AW_MPI_VI_GetVippAttr(ViDev, pstAttr);
    AW_MPI_VI_EnableVipp(pConfInfo->iDev);

    alogd("src_file=%s, pic_width=%d, pic_height=%d, pic_format=%d, frame_rate=%d, vi_dev=%d\n", pConfInfo->pcYUVFile,\
          pConfInfo->iPicWidth, pConfInfo->iPicHeight, pConfInfo->ePixFmt, \
          pConfInfo->iFrmRate, pConfInfo->iDev);

    return SUCCESS;
sys_init_err:
    return iRet;
}

static ERRORTYPE UnInitVippDevice(CVE_DTCA_CONF_INFO_S *pConfInfo)
{
    AW_MPI_VI_DisableVipp(pConfInfo->iDev);
    AW_MPI_VI_DestoryVipp(pConfInfo->iDev);
    AW_MPI_SYS_Exit();

    return SUCCESS;
}

static ERRORTYPE InitDTCAModule(CVE_DTCA_CONF_INFO_S *pConfInfo)
{
    int iRet = 0;

    AW_HANDLE pDTCAHd;
    AW_AI_CVE_DTCA_PARAM stDTCAParam;
    AW_AI_CVE_CLBR_PARAM stCLBRParam;

    memset(&pConfInfo->stResult, 0, sizeof(AW_AI_CVE_ALGO_RESULT_S));
    memset(&stDTCAParam, 0, sizeof(AW_AI_CVE_DTCA_PARAM));
    memset(&stCLBRParam, 0, sizeof(AW_AI_CVE_CLBR_PARAM));
    stDTCAParam.cParamInout.s32Width     = pConfInfo->iPicWidth;
    stDTCAParam.cParamInout.s32Height    = pConfInfo->iPicHeight;
    stDTCAParam.cParamInout.u32FrameTime = 1000 / pConfInfo->iFrmRate;

    printf("pConfInfo->iPicWidth = %d\n", pConfInfo->iPicWidth);
    printf("pConfInfo->iPicHeight = %d\n", pConfInfo->iPicHeight);

    AW_AI_CVE_DTCA_ParseParamFile("./DTCAParam/params_cvedtca.xml", &stDTCAParam.cParamAlgo, NULL);
    AW_AI_CVE_CLBR_ParseParamFile("./DTCAParam/params_cveclbr.xml", &stCLBRParam, NULL);

    /* initialize DTCA module */
    pDTCAHd = AW_AI_CVE_DTCA_Init(&stDTCAParam, &stCLBRParam);
    if (NULL == pDTCAHd) {
        aloge("AW_AI_CVE_DTCA_Init failed!!\n");
        iRet = -1;
        goto dtca_init_err;
    }
    pConfInfo->psDTCAHd = pDTCAHd;

    AW_CHAR pcDTCAVer[256];
    AW_AI_CVE_DTCA_GetAlgoVersion(pcDTCAVer);
    alogd("\n DTCA Version : %s \n", pcDTCAVer);

    return 0;
    AW_AI_CVE_DTCA_UnInit(pDTCAHd, &pConfInfo->stResult);
dtca_init_err:
    return iRet;
}

static ERRORTYPE UnInitDTCAModule(CVE_DTCA_CONF_INFO_S *pConfInfo)
{
    AW_AI_CVE_DTCA_UnInit(pConfInfo->psDTCAHd, &pConfInfo->stResult);
    return 0;
}

static ERRORTYPE InitVirviComp(VI_DEV iViDev, VI_CHN iViCh, void *pAttr)
{
    int iRet = SUCCESS;

    iRet = AW_MPI_VI_CreateVirChn(iViDev, iViCh, pAttr);
    if(iRet < 0) {
        aloge("Create VI Chn failed,VIDev = %d,VIChn = %d", iViDev, iViCh);
        goto create_chn_err;
    }
    iRet = AW_MPI_VI_SetVirChnAttr(iViDev, iViCh, pAttr);
    if(iRet < 0) {
        aloge("Set VI ChnAttr failed,VIDev = %d,VIChn = %d", iViDev, iViCh);
        goto set_attr_err;
    }
    iRet = AW_MPI_VI_EnableVirChn(iViDev, iViCh);
    if(iRet < 0) {
        aloge("VI Enable VirChn failed,VIDev = %d,VIChn = %d", iViDev, iViCh);
        goto enable_chn_err;
    }

    return SUCCESS;
    AW_MPI_VI_DestoryVirChn(iViDev, iViCh);
create_chn_err:
set_attr_err:
    AW_MPI_VI_DisableVirChn(iViDev, iViCh);
enable_chn_err:
    return iRet;
}

static ERRORTYPE UnInitVirviComp(VI_DEV iViDev, VI_CHN iViCh)
{
    AW_MPI_VI_DisableVirChn(iViDev, iViCh);
    AW_MPI_VI_DestoryVirChn(iViDev, iViCh);

    return SUCCESS;
}

static ERRORTYPE LoadConfigPara(CVE_DTCA_CONF_INFO_S *pConfInfo, char *pcConfPath)
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
        pConfInfo->iDev       = DEFAULT_DEVICE;
        pConfInfo->iFrmRate   = DEFAULT_FRM_RATE;
        pConfInfo->iFrmTestCont = DEFAULT_TEST_FRAME_COUNT;
        strcpy(pConfInfo->pcYUVFile, DEFAULT_YUV_FILE);
        strcpy(pConfInfo->pcOutputFile, DEFAULT_OUPUT_FILE);

        goto create_conf_err;
    }

    pConfInfo->iPicWidth  = GetConfParaInt(&mConf, CONF_CVE_DTCA_PIC_WIDTH, 0);
    pConfInfo->iPicHeight = GetConfParaInt(&mConf, CONF_CVE_DTCA_PIC_HEIGHT, 0);
    pConfInfo->iFrmRate   = GetConfParaInt(&mConf, CONF_CVE_DTCA_FRAME_RATE, 0);
    pConfInfo->iDev       = GetConfParaInt(&mConf, CONF_CVE_DTCA_DEVICE, 0);
    pConfInfo->iFrmTestCont = GetConfParaInt(&mConf, CONF_CVE_DTCA_TEST_FRM_CONT, 0);

    pcTmpPtr = (char *)GetConfParaString(&mConf, CONF_CVE_DTCA_YUV_DATA_FILE, NULL);
    if (NULL != pcTmpPtr) {
        strncpy(pConfInfo->pcYUVFile, pcTmpPtr, sizeof(pConfInfo->pcYUVFile));
    }
    pcTmpPtr = (char *)GetConfParaString(&mConf, CONF_CVE_DTCA_OUTPUT_FILE, NULL);
    if (NULL != pcTmpPtr) {
        strncpy(pConfInfo->pcOutputFile, pcTmpPtr, sizeof(pConfInfo->pcOutputFile));
    }

    pConfInfo->ePixFmt    = DEFAULT_PIX_FMT;
    pcTmpPtr = (char *)GetConfParaString(&mConf, CONF_CVE_DTCA_PIC_FORMAT, NULL);

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
    if (pConfInfo->stCmdlineParam.iDev != -1) {
        pConfInfo->iDev = pConfInfo->stCmdlineParam.iDev;
    }
    if (pConfInfo->stCmdlineParam.iFrmTestCont != -1) {
        pConfInfo->iFrmTestCont = pConfInfo->stCmdlineParam.iFrmTestCont;
    }

    if (pConfInfo->stCmdlineParam.pcYUVFile[0] != 0) {
        strncpy(pConfInfo->pcYUVFile, pConfInfo->stCmdlineParam.pcYUVFile, sizeof(pConfInfo->pcYUVFile));
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

    alogd("src_file=%s, output_file=%s, pic_width=%d, pic_height=%d, frame_rate=%d ", \
          pConfInfo->pcYUVFile, pConfInfo->pcOutputFile, pConfInfo->iPicWidth, pConfInfo->iPicHeight, pConfInfo->iFrmRate);
    printf("pic_format=%d, device=%d, frm_test_cont=%d\n", pConfInfo->ePixFmt, pConfInfo->iDev, pConfInfo->iFrmTestCont);

    destroyConfParser(&mConf);
    return SUCCESS;
}

static struct option pstLongOptions[] = {
    {"help",      no_argument,       0, 'h'},
    {"path",      required_argument, 0, 'p'},
    {"width",     required_argument, 0, 'x'},
    {"height",    required_argument, 0, 'y'},
    {"framerate", required_argument, 0, 'f'},
    {"testcnt",   required_argument, 0, 't'},
    {"pic",       required_argument, 0, 'm'},
    {"format",    required_argument, 0, 'e'},
    {"device",    required_argument, 0, 'd'},
    {"output",    required_argument, 0, 'o'},
    {0,           0,                 0,  0 }
};

static char pcHelpInfo[] =
    "\033[33m"
    "exec [-h|--help] [-p|--path]\n"
    "   <-h|--help>:  print to the help information\n"
    "   <-p|--path>       <args>: point to the configuration file path\n"
    "   <-x|--width>      <args>: set video picture width\n"
    "   <-y|--height>     <args>: set video picture height\n"
    "   <-f|--framerate>  <args>: set the video frame rate\n"
    "   <-t|--testcnt>    <args>: set the test frame count number\n"
    "   <-m|--pic>        <args>: point to the yuv file\n"
    "   <-e|--format>     <args>: set the video format\n"
    "   <-d|--device>     <args>: set the device number\n"
    "   <-o|--output>     <args>: output result to this file\n"
    "\033[0m\n";

static int ParseCmdLine(int argc, char **argv, CVE_DTCA_CMDLINE_PARAM_S *pCmdLinePara)
{
    int mRet;
    int iOptIndex = 0;

    memset(pCmdLinePara, -1, sizeof(CVE_DTCA_CMDLINE_PARAM_S));
    pCmdLinePara->pcYUVFile[0] = 0;
    pCmdLinePara->mConfigFilePath[0] = 0;
    pCmdLinePara->pcPixFmt[0]  = 0;
    pCmdLinePara->pcOutputFile[0] = 0;
    while (1) {
        mRet = getopt_long(argc, argv, ":p:hx:y:f:t:m:e:d:o:", pstLongOptions, &iOptIndex);
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
        case 'm':
            alogd("yuv file is [%s]\n", optarg);
            strncpy(pCmdLinePara->pcYUVFile, optarg, sizeof(pCmdLinePara->pcYUVFile));
            break;
        case 'e':
            alogd("format is [%s]\n", optarg);
            strncpy(pCmdLinePara->pcPixFmt, optarg, sizeof(pCmdLinePara->pcPixFmt));
            break;
        case 'd':
            alogd("device is [%d]\n", atoi(optarg));
            pCmdLinePara->iDev = atoi(optarg);
            break;
        case 'f':
            alogd("frame rate is [%d]\n", atoi(optarg));
            pCmdLinePara->iFrmRate = atoi(optarg);
            break;
        case 't':
            alogd("test frame count is [%d]\n", atoi(optarg));
            pCmdLinePara->iFrmTestCont = atoi(optarg);
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
* FileName: main.c
* Version:  V1.0
* function    : main()
* Description : Vehicle License Plate Recognition(DTCA) Lib Sample
******************************************************************************/
int main(int argc, char *argv[])
{
    int iRet;

    alogd("sample_vi2MOD running!\n");
    CVE_DTCA_CONF_INFO_S stDTCAConfInfo;
    memset(&stDTCAConfInfo, 0, sizeof(CVE_DTCA_CONF_INFO_S));

    iRet = ParseCmdLine(argc, argv, &stDTCAConfInfo.stCmdlineParam);
    if (iRet) {
//        alogd("parse cmdline error, may use default configuration.\n");
        return -1;
    }

    /* always return SUCCESS */
    LoadConfigPara(&stDTCAConfInfo, stDTCAConfInfo.stCmdlineParam.mConfigFilePath);

    /* initialize video input device */
    iRet = InitVippDevice(&stDTCAConfInfo);
    if (iRet < 0) {
        aloge("InitVippDevice failed!!\n");
        goto init_vipp_err;
    }

#define ISP_RUN 1
#if ISP_RUN
    /* open isp, improve the video's quality */
    int iIspDev = 0;

    if (stDTCAConfInfo.iDev == 0 || stDTCAConfInfo.iDev == 2) {
        iIspDev = 1;
    } else if (stDTCAConfInfo.iDev == 1 || stDTCAConfInfo.iDev == 3) {
        iIspDev = 0;
    }
    AW_MPI_ISP_Init();
    AW_MPI_ISP_Run(iIspDev);
#endif

    int iYSize;
    int iUSize;
    int iVSize;

    iYSize = stDTCAConfInfo.iPicWidth * stDTCAConfInfo.iPicHeight;
    switch (stDTCAConfInfo.ePixFmt) {
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
    stDTCAConfInfo.iYSize = iYSize;
    stDTCAConfInfo.iUSize = iUSize;
    stDTCAConfInfo.iVSize = iVSize;

    /******************************************
     * create DTCA module
     *****************************************/
    iRet = InitDTCAModule(&stDTCAConfInfo);
    if (iRet < 0) {
        aloge("InitDTCAModule failed!!\n");
        iRet = -1;
        goto init_dtca_module_err;
    }

    /* create a virvi component, use virtual channel 0 */
    int iViDev = stDTCAConfInfo.iDev;
    int iViChn = 0;
    iRet = InitVirviComp(iViDev, iViChn, NULL);
    if (iRet < 0) {
        aloge("InitVirviComp failed!!\n");
        goto init_virvi_comp_err;
    }

    /* create video capture thread and process video pictures */
    pthread_t tid;

    signal(SIGINT, SigHandle);
    g_iThreadExitFlag = 0;

    stDTCAConfInfo.iChn = iViChn;
    iRet = pthread_create(&tid, NULL, DTCAProcessThread, &stDTCAConfInfo);
    if (iRet != 0) {
        aloge("create thread failed!!\n");
        iRet = -1;
        goto create_thread_err;
    }

    pthread_join(tid, NULL);
create_thread_err:
    UnInitVirviComp(iViDev, iViChn);
init_virvi_comp_err:
    UnInitDTCAModule(&stDTCAConfInfo);
unknow_format:
#if ISP_RUN
    AW_MPI_ISP_Stop(iIspDev);
    AW_MPI_ISP_Exit();
#endif

init_dtca_module_err:
    UnInitVippDevice(&stDTCAConfInfo);
init_vipp_err:
    if (0 == iRet) {
        printf("sample_vi2MOD exit!\n");
    }
    return iRet;
}


