#define LOG_TAG "sample_vi2VLPR"
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
#include "aw_ai_cve_vlpr_interface.h"
#include "CVEMiddleInterface.h"
#include "mm_comm_video.h"
#include <mpi_videoformat_conversion.h>
#include "media/mpi_sys.h"
#include "media/mpi_vi.h"
#include "media/mpi_isp.h"

#include "sample_vi2VLPR.h"

/* use ion or malloc to alloc memory
 * attention: must use ion
 */
#if 0
#define USE_ION
#else
#define USE_MALLOC
#endif
#define CVE_TEST_STRIDE_NUM 8
static int g_iThreadExitFlag = 0;

static void SigHandle(int iArg)
{
    g_iThreadExitFlag = 1;
}

/******************************************************************************
* function : read and process YUV image
******************************************************************************/
void *VLPRProcessThread(void *pArg)
{
    int iRet;
    CVE_VLPR_CONF_INFO_S *pstConfInfo = (CVE_VLPR_CONF_INFO_S *)pArg;

    AW_IMAGE_S stImage;
    struct CveIonAddr stCVEAddr;
    /* alloc memory use ION */
    /* why use CVE_Malloc, because the library method will
     * use CVE_MemFluseCache to flush stImage.mpVirAddr[0]
     * if not use CVE_Malloc, the flush failed error will be occured.
     */
    iRet = CVE_Malloc(&stCVEAddr, pstConfInfo->iYSize);
    int stride = (stImage.mWidth + CVE_TEST_STRIDE_NUM -1)/CVE_TEST_STRIDE_NUM * CVE_TEST_STRIDE_NUM;
    for (int ii = 0; ii< stImage.mWidth; ii++ ) {
        memcpy(stCVEAddr.mmu_Addr + ii*stride,stCVEAddr.mmu_Addr + stImage.mWidth*ii,
               stImage.mWidth);
    }
    stImage.mpVirAddr[0] = stCVEAddr.mmu_Addr;
    stImage.mPhyAddr[0]  = (unsigned int)(stCVEAddr.phy_Addr);
    stImage.mStride[0] = stride;
    FILE *pOutFd;
    char pcOutPath[256];
    char pcTmp[1024];
    strncpy(pcOutPath, pstConfInfo->pcOutputFile, sizeof(pcOutPath));
    pOutFd = fopen(pcOutPath, "wb+");
    if (NULL == pOutFd) {
        aloge("open output file %s failed!!\n", pcOutPath);
        iRet = -1;
        goto open_result_file_err;
    }

    int iFindPlateNum = 0;
    int iFrameIndex   = 0;
    AW_STATUS_E eProcessRet;
    AW_U32 iTimeStamp = 0;
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
        memcpy(stImage.mpVirAddr[0], stFrame.VFrame.mpVirAddr[0], pstConfInfo->iYSize);
        stImage.mpVirAddr[1] = stFrame.VFrame.mpVirAddr[1];
        stImage.mpVirAddr[2] = stFrame.VFrame.mpVirAddr[2];
        iTimeStamp = stFrame.VFrame.mpts / 1000;

        AW_AI_CVE_VLPR_PROC_PARA_S  stParams;
        AW_AI_CVE_VLPR_RULT_S       stOutput;
        memset(&stOutput,0,sizeof(AW_AI_CVE_VLPR_RULT_S));
        memset(&stParams,0,sizeof(stParams));

        /* set image plate recognize zone */
        stParams.s32MaxPlateWidth = 300;
        stParams.s32MinPlateWidth = 70;
        stParams.stRecRect.s16Left = 0;
        stParams.stRecRect.s16Top =  0;
        stParams.stRecRect.s16Right = stImage.mWidth - 1;
        stParams.stRecRect.s16Bottom = stImage.mHeight - 1;

#ifdef TIME_TEST
        int64_t nStartTm = CDX_GetSysTimeUsMonotonic();//start time
#endif
        eProcessRet = AW_AI_CVE_VLPR_Process(pstConfInfo->psVLPRHd, &stImage, &stParams, &stOutput);
#ifdef TIME_TEST
        int64_t nEndTm = CDX_GetSysTimeUsMonotonic();//end time
        alogd("frame[%d] handle time[%lld]", iFrameIndex, nEndTm-nStartTm);
#endif
        if (eProcessRet == AW_STATUS_OK) {
            if (stOutput.s32NumOutput > 0) {
                iRet = snprintf(pcTmp, sizeof(pcTmp), "=============frame %d==============\n", iFrameIndex);
                pcTmp[1023] = '\0';
                fwrite(pcTmp, 1, iRet, pOutFd);
            }

            for (int i = 0; i < stOutput.s32NumOutput; i++) {
                printf("\033[33m");
                printf("===========Find one plate=========\n");
                printf("the plate num is %s\n", stOutput.astOutput[i].as8PlateNum);
                printf("the plate color is %d\n", stOutput.astOutput[i].ePlateColor);
                printf("the vehicle color is %d\n", stOutput.astOutput[i].eVehicleColor);
                printf("the plate type is %d\n", stOutput.astOutput[i].ePlateType);
                printf("==================================\n");
                printf("\033[0m");
                fflush(stdout);
                iFindPlateNum ++;

                iRet = snprintf(pcTmp, sizeof(pcTmp),\
                                "the plate num is %s\n"\
                                "the plate color is %d\n"\
                                "the vehicle color is %d\n"\
                                "the plate type is %d\n\n", stOutput.astOutput[i].as8PlateNum, stOutput.astOutput[i].ePlateColor,\
                                stOutput.astOutput[i].eVehicleColor, stOutput.astOutput[i].ePlateType);
                pcTmp[1023] = '\0';
                fwrite(pcTmp, 1, iRet, pOutFd);
            }
        }

        AW_MPI_VI_ReleaseFrame(pstConfInfo->iDev, pstConfInfo->iChn, &stFrame);
        iFrameIndex ++;
    }

    if (0 == iFindPlateNum) {
        printf("\033[31m");
        printf("==================================\n");
        printf("can not find the a plate\n");
        printf("==================================\n");
        printf("\033[0m");
        fflush(stdout);
    }

    iRet = 0;
    fclose(pOutFd);
open_result_file_err:
    CVE_Free(&stCVEAddr);
    pthread_exit(&iRet);
}

static ERRORTYPE InitVippDevice(CVE_VLPR_CONF_INFO_S *pConfInfo)
{
    int iRet = SUCCESS;

    if (pConfInfo->iDev > 3 || pConfInfo->iDev < 0) {
        aloge("wrong device number[%d]!\n", pConfInfo->iDev);
        return -1;
    }

    MPP_SYS_CONF_S mSysConf;
    memset(&mSysConf, 0, sizeof(mSysConf));
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

    alogd("pic_width=%d, pic_height=%d, pic_format=%d, frame_rate=%d, vi_dev=%d\n",
          pConfInfo->iPicWidth, pConfInfo->iPicHeight, pConfInfo->ePixFmt, \
          pConfInfo->iFrmRate, pConfInfo->iDev);

    return SUCCESS;
sys_init_err:
    return iRet;
}

static ERRORTYPE UnInitVippDevice(CVE_VLPR_CONF_INFO_S *pConfInfo)
{
    AW_MPI_VI_DisableVipp(pConfInfo->iDev);
    AW_MPI_VI_DestoryVipp(pConfInfo->iDev);
    AW_MPI_SYS_Exit();

    return SUCCESS;
}

static ERRORTYPE InitVLPRModule(CVE_VLPR_CONF_INFO_S *pConfInfo)
{
    int iRet = 0;

    AW_HANDLE pVLPRHd;
    AW_AI_CVE_VLPR_INIT_PARA_S stVLPRParam;                // set VLPR algorithm init para
    AW_U8 au8Version[64];

    memset(&stVLPRParam, 0, sizeof(stVLPRParam));

    pVLPRHd = AW_AI_CVE_VLPR_Create();                     //create VLPR algorithm  engine
    if (pVLPRHd == NULL) {
        aloge("\n VLPR_Create Failured!!! \n");
        iRet = -1;
        goto vlpr_create_err;
    }
    pConfInfo->psVLPRHd = pVLPRHd;

    stVLPRParam.s32MaxPlateNum = 5;
    stVLPRParam.s32PlateConfidThrld = 10;
    stVLPRParam.astCharType[0] = AW_AI_CVE_HANZI_CHINA;
    stVLPRParam.astCharType[1] = AW_AI_CVE_CHARTYPE_ONLYALPHABET;
    stVLPRParam.astCharType[2] = AW_AI_CVE_CHARTYPE_CHARACTER;
    stVLPRParam.astCharType[3] = AW_AI_CVE_CHARTYPE_CHARACTER;
    stVLPRParam.astCharType[4] = AW_AI_CVE_CHARTYPE_CHARACTER;
    stVLPRParam.astCharType[5] = AW_AI_CVE_CHARTYPE_CHARACTER;
    stVLPRParam.astCharType[6] = AW_AI_CVE_CHARTYPE_CHARACTER;
    stVLPRParam.s32PlateTypeSp = 0;                         // 0:all plate; example:stVLPRParam.s32PlateTypeSp = AW_AI_CVE_PLATE_YELLOW|AW_AI_CVE_PLATE_BLUE; Recognize blue plate and yellow plate
    stVLPRParam.ps8ModelPath = "./Binary";                  // Model files' path
    iRet = AW_AI_CVE_VLPR_Init(pVLPRHd, &stVLPRParam);      // init VLPR algorithm
    if (AW_STATUS_ERROR == iRet) {
        aloge("AW_AI_CVE_VLPR_Init failed!!\n");
        iRet = -1;
        goto vlpr_init_err;
    }

    AW_AI_CVE_VLPR_GetVersion(pVLPRHd, au8Version);
    alogd("\n Vlpr Version : %s \n",au8Version);

    return 0;
vlpr_init_err:
    AW_AI_CVE_VLPR_Release(pVLPRHd);
vlpr_create_err:
    return iRet;
}

static ERRORTYPE UnInitVLPRModule(CVE_VLPR_CONF_INFO_S *pConfInfo)
{
    AW_AI_CVE_VLPR_Release(pConfInfo->psVLPRHd);
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

static ERRORTYPE LoadConfigPara(CVE_VLPR_CONF_INFO_S *pConfInfo, char *pcConfPath)
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
        strcpy(pConfInfo->pcOutputFile, DEFAULT_OUPUT_FILE);
        goto create_conf_err;
    }

    pConfInfo->iPicWidth  = GetConfParaInt(&mConf, CONF_CVE_VLPR_PIC_WIDTH, 0);
    pConfInfo->iPicHeight = GetConfParaInt(&mConf, CONF_CVE_VLPR_PIC_HEIGHT, 0);
    pConfInfo->iFrmRate   = GetConfParaInt(&mConf, CONF_CVE_VLPR_FRAME_RATE, 0);
    pConfInfo->iDev       = GetConfParaInt(&mConf, CONF_CVE_VLPR_DEVICE, 0);
    pConfInfo->iFrmTestCont = GetConfParaInt(&mConf, CONF_CVE_VLPR_TEST_FRM_CONT, 0);

    pcTmpPtr = (char *)GetConfParaString(&mConf, CONF_CVE_VLPR_OUTPUT_FILE, NULL);
    if (NULL != pcTmpPtr) {
        strncpy(pConfInfo->pcOutputFile, pcTmpPtr, sizeof(pConfInfo->pcOutputFile));
    }

    pConfInfo->ePixFmt    = DEFAULT_PIX_FMT;
    pcTmpPtr = (char *)GetConfParaString(&mConf, CONF_CVE_VLPR_PIC_FORMAT, NULL);

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

    alogd("output_file=%s, pic_width=%d, pic_height=%d, frame_rate=%d ", \
          pConfInfo->pcOutputFile, pConfInfo->iPicWidth, pConfInfo->iPicHeight, pConfInfo->iFrmRate);
    printf("pic_format=%d, device=%d, frm_test_cont=%d\n", pConfInfo->ePixFmt, pConfInfo->iDev, pConfInfo->iFrmTestCont);

    destroyConfParser(&mConf);
    return SUCCESS;
}

static struct option pstLongOptions[] = {
    {"help",      no_argument,       0, 'h'},
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
    "   <-e|--format>     <args>: set the video format\n"
    "   <-d|--device>     <args>: set the device number\n"
    "   <-o|--output>     <args>: output result to this file\n"
    "\033[0m\n";

static int ParseCmdLine(int argc, char **argv, CVE_VLPR_CMDLINE_PARAM_S *pCmdLinePara)
{
    int mRet;
    int iOptIndex = 0;

    memset(pCmdLinePara, -1, sizeof(CVE_VLPR_CMDLINE_PARAM_S));
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
* Description : Vehicle License Plate Recognition(VLPR) Lib Sample
******************************************************************************/
int main(int argc, char *argv[])
{
    int iRet;

    alogd("sample_vi2VLPR running!\n");
    CVE_VLPR_CONF_INFO_S stVLPRConfInfo;
    memset(&stVLPRConfInfo, 0, sizeof(CVE_VLPR_CONF_INFO_S));

    iRet = ParseCmdLine(argc, argv, &stVLPRConfInfo.stCmdlineParam);
    if (iRet) {
        aloge("fatal error! command line param is wrong, exit!");
        return -1;
    }

    /* always return SUCCESS */
    LoadConfigPara(&stVLPRConfInfo, stVLPRConfInfo.stCmdlineParam.mConfigFilePath);

    /* initialize video input device */
    iRet = InitVippDevice(&stVLPRConfInfo);
    if (iRet < 0) {
        aloge("InitVippDevice failed!!\n");
        goto init_vipp_err;
    }

#define ISP_RUN 1
#if ISP_RUN
    /* open isp, improve the video's quality */
    int iIspDev = 0;

    if (stVLPRConfInfo.iDev == 0 || stVLPRConfInfo.iDev == 2) {
        iIspDev = 1;
    } else if (stVLPRConfInfo.iDev == 1 || stVLPRConfInfo.iDev == 3) {
        iIspDev = 0;
    }
    AW_MPI_ISP_Init();
    AW_MPI_ISP_Run(iIspDev);
#endif

    int iYSize;
    int iUSize;
    int iVSize;

    iYSize = stVLPRConfInfo.iPicWidth * stVLPRConfInfo.iPicHeight;
    switch (stVLPRConfInfo.ePixFmt) {
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
    stVLPRConfInfo.iYSize = iYSize;
    stVLPRConfInfo.iUSize = iUSize;
    stVLPRConfInfo.iVSize = iVSize;

    /******************************************
     * create VLPR module
     *****************************************/
    iRet = InitVLPRModule(&stVLPRConfInfo);
    if (iRet < 0) {
        aloge("InitVLPRModule failed!!\n");
        iRet = -1;
        goto init_vlpr_module_err;
    }

    /* create a virvi component, use virtual channel 0 */
    int iViDev = stVLPRConfInfo.iDev;
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

    stVLPRConfInfo.iChn = iViChn;
    iRet = pthread_create(&tid, NULL, VLPRProcessThread, &stVLPRConfInfo);
    if (iRet != 0) {
        aloge("create thread failed!!\n");
        iRet = -1;
        goto create_thread_err;
    }

    pthread_join(tid, NULL);
create_thread_err:
    UnInitVirviComp(iViDev, iViChn);
init_virvi_comp_err:
    UnInitVLPRModule(&stVLPRConfInfo);
unknow_format:
#if ISP_RUN
    AW_MPI_ISP_Stop(iIspDev);
    AW_MPI_ISP_Exit();
#endif

init_vlpr_module_err:
    UnInitVippDevice(&stVLPRConfInfo);
init_vipp_err:
    if (0 == iRet) {
        printf("sample_vi2VLPR exit!\n");
    }
    return iRet;
}

