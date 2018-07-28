#define LOG_TAG "sample_EVE"
#include <utils/plat_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include "media/mpi_sys.h"
#include <confparser.h>
#include <ion_memmanager.h>

#define TIME_TEST
#ifdef TIME_TEST
#include "SystemBase.h"
#endif

#include <confparser.h>
#include <ion_memmanager.h>
#include "CVEMiddleInterface.h"
//#include "aw_ai_eve_event_interface.h"

#include "sample_EVE.h"

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

static void DmaCallBackFunc(void* pUsr)
{
#ifdef TEST_PRINT
    printf("DmaCallBackFunc\n");
#endif
}

/******************************************************************************
* function : read and process YUV image
******************************************************************************/
int EVEProcessImage(AW_IMAGE_S *pstImage, EVE_CONF_INFO_S *pstConfInfo)
{
    int iRet;
    int iFd;

    /* open and map file memory */
    iFd = open(pstConfInfo->pcYUVFile, O_RDONLY);
    if (iFd < 0) {
        aloge("open %s file failed!!\n", pstConfInfo->pcYUVFile);
        iRet = -1;
        goto open_file_err;
    }

    struct stat stFstat;
    iRet = fstat(iFd, &stFstat);
    if (iRet < 0) {
        aloge("get file status failed!!\n");
        goto file_stat_err;
    }

    char *pcFileMemStart;
    char *pcFileMemSeek;
    pcFileMemStart = mmap(NULL, stFstat.st_size, PROT_READ, MAP_SHARED, iFd, 0);
    if (NULL == pcFileMemStart) {
        aloge("mmap file memory failed!!\n");
        goto file_map_err;
    }

    pcFileMemSeek = pcFileMemStart;

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
    int iFrameIndex    = 0;
    AW_U32 iTimeStamp  = 0;
    AW_AI_EVE_EVENT_RESULT_S *pstEVEResult = &pstConfInfo->stResult;
    while (1) {
        if (g_iThreadExitFlag) {
            g_iThreadExitFlag = 0;
            break;
        }

        if (iFrameIndex >= pstConfInfo->iFrmTestCont && pstConfInfo->iFrmTestCont != 0) {
            alogd("end of test frame count number[%d]!!\n", iFrameIndex);
            break;
        }
        if (pcFileMemSeek >= pcFileMemStart + stFstat.st_size) {
            alogd("end of file!!\n");
            break;
        }
        memcpy(pstImage->mpVirAddr[0], pcFileMemSeek, pstConfInfo->iYSize);
        pcFileMemSeek += pstConfInfo->iYSize;
        memcpy(pstImage->mpVirAddr[1], pcFileMemSeek, pstConfInfo->iUSize);
        pcFileMemSeek += pstConfInfo->iUSize;
        memcpy(pstImage->mpVirAddr[2], pcFileMemSeek, pstConfInfo->iVSize);
        pcFileMemSeek += pstConfInfo->iVSize;
#ifdef USE_ION
        /* use CVE_MAlloc, do not need ion_flushCache */
//      iRet = ion_flushCache(pstImage->mpVirAddr[0], pstConfInfo->iYSize);
        iRet = ion_flushCache(pstImage->mpVirAddr[1], pstConfInfo->iUSize);
        iRet = pstConfInfo->iVSize ? ion_flushCache(pstImage->mpVirAddr[2], pstConfInfo->iVSize) : 0;
#endif
        AW_STATUS_E eProcessRet;
#ifdef TIME_TEST
        int64_t nStartTm = CDX_GetSysTimeUsMonotonic();//start time
#endif
        AW_AI_EVE_Event_SetEveSourceAddress(pstConfInfo->psEVEHd, (void *)pstImage->mpVirAddr[0]);
        eProcessRet = AW_AI_EVE_Event_Process(pstConfInfo->psEVEHd, pstImage, iTimeStamp, pstEVEResult);
#ifdef TIME_TEST
        int64_t nEndTm = CDX_GetSysTimeUsMonotonic();//end time
        alogd("frame[%d] handle time[%lld]", iFrameIndex, nEndTm-nStartTm);
#endif
        if (eProcessRet == AW_STATUS_OK) {
            if (pstEVEResult->sTarget.s32TargetNum > 0) {
                printf("\033[32m");
                printf(">>>>>>>find [%d] faces in this frame<<<<<<<\n", pstEVEResult->sTarget.s32TargetNum);
                printf("\033[0m");
                fflush(stdout); // flush stdout, resume console's output format
                iRet = snprintf(pcTmp, sizeof(pcTmp), "=============find [%d] faces in frame [%d]=============\n",\
                                pstEVEResult->sTarget.s32TargetNum, iFrameIndex);
                pcTmp[1023] = '\0';
                fwrite(pcTmp, 1, iRet, pOutFd);
            }

            for (int i = 0; i < pstEVEResult->sTarget.s32TargetNum; i++) {
                printf("\033[33m");
                printf("=================================Get one target============================\n");
                printf("the frame index is [%d]\n", iFrameIndex);
                printf("the target ID is %u\n", pstEVEResult->sTarget.astTargets[i].u32ID);
                printf("the target type is %u\n", pstEVEResult->sTarget.astTargets[i].u32Type);
                printf("the target point is [%d, %d]\n", pstEVEResult->sTarget.astTargets[i].stPoint.s16X,
                       pstEVEResult->sTarget.astTargets[i].stPoint.s16Y);
                printf("the target rect is [%d, %d, %d, %d]\n",
                       pstEVEResult->sTarget.astTargets[i].stRect.s16Left,
                       pstEVEResult->sTarget.astTargets[i].stRect.s16Top,
                       pstEVEResult->sTarget.astTargets[i].stRect.s16Right,
                       pstEVEResult->sTarget.astTargets[i].stRect.s16Bottom);
                printf("===========================================================================\n");
                printf("\033[0m");
                fflush(stdout); // flush stdout, resume console's output format
                iFindTargetNum ++;

                iRet = snprintf(pcTmp, sizeof(pcTmp),\
                                "the frame index is [%d]\n"\
                                "the target type is %u\n"\
                                "the target point is [%d, %d]\n"\
                                "the target rect is [%d, %d, %d, %d]\n\n",\
                                pstEVEResult->sTarget.astTargets[i].u32ID, pstEVEResult->sTarget.astTargets[i].u32Type,\
                                pstEVEResult->sTarget.astTargets[i].stPoint.s16X, pstEVEResult->sTarget.astTargets[i].stPoint.s16Y,\
                                pstEVEResult->sTarget.astTargets[i].stRect.s16Left, pstEVEResult->sTarget.astTargets[i].stRect.s16Top,\
                                pstEVEResult->sTarget.astTargets[i].stRect.s16Right, pstEVEResult->sTarget.astTargets[i].stRect.s16Bottom);
                pcTmp[1023] = '\0';
                fwrite(pcTmp, 1, iRet, pOutFd);
            }
        }

        iTimeStamp += 1000 / pstConfInfo->iFrmRate;
        iFrameIndex++;
    }

    if (0 == iFindTargetNum) {
        printf("\033[31m");
        printf("==================================\n");
        printf("can not find the a target\n");
        printf("==================================\n");
        printf("\033[0m");
    }

    iRet = 0;
    fclose(pOutFd);
open_result_file_err:
    munmap(pcFileMemStart, stFstat.st_size);
file_map_err:
file_stat_err:
    close(iFd);
open_file_err:
    return iRet;
}

static ERRORTYPE InitEVEModule(EVE_CONF_INFO_S *pConfInfo)
{
    int iRet = 0;

    /* initilize EVE module */
    AW_AI_EVE_CTRL_S sEVECtrl;
    AW_HANDLE pEVEHd;

    sEVECtrl.addrInputType = AW_AI_EVE_ADDR_INPUT_VIR; /* for malloc, we use virtual address */
//    sEVECtrl.addrInputType = AW_AI_EVE_ADDR_INPUT_PHY;
//    sEVECtrl.classifierNum = 5;
    sEVECtrl.scale_factor = 1;
    sEVECtrl.mScanStageNo = 10;
    sEVECtrl.yStep = 4;
    sEVECtrl.xStep0 =1;
    sEVECtrl.xStep1 =3;
    sEVECtrl.mRltNum = AW_AI_EVE_MAX_RESULT_NUM;
    sEVECtrl.mMidRltNum = 0;
    sEVECtrl.mMidRltStageNo = 10;
    sEVECtrl.rltType = AW_AI_EVE_RLT_OUTPUT_DETAIL;

    sEVECtrl.mDmaOut.s16Width  = pConfInfo->iPicWidth;
    sEVECtrl.mDmaOut.s16Height = pConfInfo->iPicHeight;
    sEVECtrl.mPyramidLowestLayel.s16Width = pConfInfo->iPicWidth;
    sEVECtrl.mPyramidLowestLayel.s16Height = pConfInfo->iPicHeight;
    sEVECtrl.dmaSrcSize.s16Width = pConfInfo->iPicWidth;
    sEVECtrl.dmaSrcSize.s16Height = pConfInfo->iPicHeight;
    sEVECtrl.dmaDesSize.s16Width = sEVECtrl.dmaSrcSize.s16Width;
    sEVECtrl.dmaDesSize.s16Height = sEVECtrl.dmaSrcSize.s16Height;
    sEVECtrl.dmaRoi.s16X = 0;
    sEVECtrl.dmaRoi.s16Y = 0;
    sEVECtrl.dmaRoi.s16Width = sEVECtrl.dmaDesSize.s16Width;
    sEVECtrl.dmaRoi.s16Height = sEVECtrl.dmaDesSize.s16Height;
#if 1
    AW_S8 *awKeyNew = "1111111111111111"; //key
    sEVECtrl.classifierNum = 8;
    sEVECtrl.classifierPath[0].path = (AW_S8*)"./classifier/frontface.ld";
    sEVECtrl.classifierPath[0].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[1].path = (AW_S8*)"./classifier/fullprofleftface.ld";
    sEVECtrl.classifierPath[1].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[2].path = (AW_S8*)"./classifier/fullprofrightface.ld";
    sEVECtrl.classifierPath[2].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[3].path = (AW_S8*)"./classifier/halfdownface.ld";
    sEVECtrl.classifierPath[3].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[4].path = (AW_S8*)"./classifier/profileface.ld";
    sEVECtrl.classifierPath[4].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[5].path = (AW_S8*)"./classifier/rotleftface.ld";
    sEVECtrl.classifierPath[5].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[6].path = (AW_S8*)"./classifier/rotrightface.ld";
    sEVECtrl.classifierPath[6].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[7].path = (AW_S8*)"./classifier/smallface.ld";
    sEVECtrl.classifierPath[7].key  = (AW_U8*)awKeyNew;
#else
    sEVECtrl.classifierNum = 5;
    sEVECtrl.classifierPath[0] = "./classifier/frontfacenew.ld";
    sEVECtrl.classifierPath[1] = "./classifier/halfdown.ld";
    sEVECtrl.classifierPath[2] = "./classifier/datarotnew1.ld";
    sEVECtrl.classifierPath[3] = "./classifier/faceprofileclip.ld";
    sEVECtrl.classifierPath[4] = "./classifier/fullprofileold.ld";
#endif
    sEVECtrl.dmaCallBackFunc = &DmaCallBackFunc;
    sEVECtrl.dma_pUsr = pConfInfo;
    pEVEHd = AW_AI_EVE_Event_Init(&sEVECtrl);
    if(AW_NULL == pEVEHd) {
        aloge("AW_AI_EVE_Event_Init failure!\n");
        iRet = -1;
        goto eve_init_err;
    }
    pConfInfo->psEVEHd = pEVEHd; /* store EVE handle into custom struct */

    AW_AI_EVE_Event_SetEveDMAExecuteMode(pEVEHd, AW_AI_EVE_DMA_EXECUTE_SYNC);

    /* set face event parameter */
    AW_AI_EVE_EVENT_FACEDET_PARAM_S stFaceDetParam;
    stFaceDetParam.sRoiSet.s32RoiNum = 1;
    stFaceDetParam.sRoiSet.sID[0] = 1;
    stFaceDetParam.sRoiSet.sRoi[0].s16Top  = 0;
    stFaceDetParam.sRoiSet.sRoi[0].s16Bottom  = pConfInfo->iPicHeight;
    stFaceDetParam.sRoiSet.sRoi[0].s16Left = 0;
    stFaceDetParam.sRoiSet.sRoi[0].s16Right = pConfInfo->iPicWidth;
    stFaceDetParam.s32ClassifyFlag = 0; //close
    stFaceDetParam.s32MaxFaceNum = 128;
    stFaceDetParam.s32MinFaceSize  = 20;
    stFaceDetParam.s32OverLapCoeff = 20;
    stFaceDetParam.s32MergeThreshold = 3;
    stFaceDetParam.s8Cfgfile = AW_NULL;
    stFaceDetParam.s8Weightfile = AW_NULL;

    iRet = AW_AI_EVE_Event_SetEventParam(pEVEHd, AW_AI_EVE_EVENT_FACEDETECT, &stFaceDetParam);
    if(AW_STATUS_ERROR == iRet) {
        aloge("AW_AI_EVE_Event_SetEventParam failure!, errorcode = %d\n", AW_AI_EVE_Event_GetLastError(pEVEHd));
        iRet = -1;
        goto eve_set_event_err;
    }

    /* get and print EVE version */
    AW_S8 cVersion[128];

    AW_AI_EVE_Event_GetAlgoVersion(cVersion);
    alogd("EVE version is %s!\n", cVersion);

    return 0;
eve_set_event_err:
    AW_AI_EVE_Event_UnInit(pEVEHd);
eve_init_err:
    return iRet;
}

static ERRORTYPE UnInitEVEModule(EVE_CONF_INFO_S *pConfInfo)
{
    AW_AI_EVE_Event_UnInit(pConfInfo->psEVEHd);
    return SUCCESS;
}

static ERRORTYPE LoadConfigPara(EVE_CONF_INFO_S *pConfInfo, char *pcConfPath)
{
    int iRet;
    char *pcTmpPtr = NULL;
    CONFPARSER_S mConf;

    /* try to open configuration file, or will use default configuration */
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

    pConfInfo->iPicWidth  = GetConfParaInt(&mConf, CONF_EVE_PIC_WIDTH, 0);
    pConfInfo->iPicHeight = GetConfParaInt(&mConf, CONF_EVE_PIC_HEIGHT, 0);
    pConfInfo->iFrmRate   = GetConfParaInt(&mConf, CONF_EVE_FRAME_RATE, 0);
    pConfInfo->iDev       = GetConfParaInt(&mConf, CONF_EVE_DEVICE, 0);
    pConfInfo->iFrmTestCont = GetConfParaInt(&mConf, CONF_EVE_TEST_FRM_CONT, 0);

    pcTmpPtr = (char *)GetConfParaString(&mConf, CONF_EVE_YUV_DATA_FILE, NULL);
    if (NULL != pcTmpPtr) {
        strncpy(pConfInfo->pcYUVFile, pcTmpPtr, sizeof(pConfInfo->pcYUVFile));
    }
    pcTmpPtr = (char *)GetConfParaString(&mConf, CONF_EVE_OUTPUT_FILE, NULL);
    if (NULL != pcTmpPtr) {
        strncpy(pConfInfo->pcOutputFile, pcTmpPtr, sizeof(pConfInfo->pcOutputFile));
    }

    pConfInfo->ePixFmt    = DEFAULT_PIX_FMT;
    pcTmpPtr = (char *)GetConfParaString(&mConf, CONF_EVE_PIC_FORMAT, NULL);

    /* check if user has set the configuration with commond line */
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
    {"help",        no_argument,       0, 'h'},
    {"path",        required_argument, 0, 'p'},
    {"width",       required_argument, 0, 'x'},
    {"height",      required_argument, 0, 'y'},
    {"framerate",   required_argument, 0, 'f'},
    {"testcnt",     required_argument, 0, 't'},
    {"pic",         required_argument, 0, 'm'},
    {"format",      required_argument, 0, 'e'},
    {"device",      required_argument, 0, 'd'},
    {"output",      required_argument, 0, 'o'},
    {0,             0,                 0,  0 }
};

static char pcHelpInfo[] =
    "\033[33m"
    "exec [-h|--help] [-p|--path]\n"
    "   <-h|--help>: print to the help information\n"
    "   <-p|--path>       <args>: point to the configuration file path\n"
    "   <-x|--width>      <args>: set video picture width\n"
    "   <-y|--height>     <args>: set video picture height\n"
    "   <-f|--framerate>  <args>: set the video frame rate\n"
    "   <-t|--testcnt>    <args>: set the test frame count number\n"
    "   <-m|--pic>        <args>: point to the yuv file\n"
    "   <-e|--format>     <args>: set the video format\n"
    "   <-o|--output>     <args>: output result to this file\n"
    "\033[0m\n";

static int ParseCmdLine(int argc, char **argv, EVE_CMDLINE_PARAM_S *pCmdLinePara)
{
    int mRet;
    int iOptIndex = 0;

    memset(pCmdLinePara, -1, sizeof(EVE_CMDLINE_PARAM_S));
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
* Description : Vehicle License Plate Recognition(EVE) Lib Sample
******************************************************************************/
int main(int argc, char *argv[])
{
    int iRet;

    alogd("sample_EVE running!\n");
    EVE_CONF_INFO_S stEVEConfInfo;
    memset(&stEVEConfInfo, 0, sizeof(EVE_CONF_INFO_S));

    iRet = ParseCmdLine(argc, argv, &stEVEConfInfo.stCmdlineParam);
    if (iRet) {
//        alogd("parse cmdline error, may use default configuration.\n");
        return -1;
    }

    /* always return SUCCESS */
    LoadConfigPara(&stEVEConfInfo, stEVEConfInfo.stCmdlineParam.mConfigFilePath);


    MPP_SYS_CONF_S mSysConf;
    memset(&mSysConf,0,sizeof(MPP_SYS_CONF_S));
    mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&mSysConf);
    iRet = AW_MPI_SYS_Init();
    if (iRet < 0) {
        aloge("sys init failed");
        return -1;
    }

    /******************************************
     * step 1 : create EVE
     *****************************************/
    iRet = InitEVEModule(&stEVEConfInfo);
    if (iRet < 0) {
        aloge("InitEVEModule failed!!\n");
        iRet = -1;
        goto init_hcnt_module_err;
    }

    /******************************************
    step 2 : alloc memory and Read Image
    ******************************************/
    int iYSize;
    int iUSize;
    int iVSize;

    iYSize = stEVEConfInfo.iPicWidth * stEVEConfInfo.iPicHeight;
    switch (stEVEConfInfo.ePixFmt) {
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
    stEVEConfInfo.iYSize = iYSize;
    stEVEConfInfo.iUSize = iUSize;
    stEVEConfInfo.iVSize = iVSize;

    printf("iYSize = %d\n", iYSize);
    printf("iUSize = %d\n", iUSize);
    printf("iVSize = %d\n", iVSize);

    AW_IMAGE_S  stImage;
    struct CveIonAddr stCVEAddr;

    stImage.mPixelFormat = stEVEConfInfo.ePixFmt;
    stImage.mWidth       = stEVEConfInfo.iPicWidth;
    stImage.mHeight      = stEVEConfInfo.iPicHeight;
    stImage.mStride[0]   = stEVEConfInfo.iPicWidth;

#ifdef USE_ION
    iRet = ion_memOpen();
    if (iRet < 0) {
        aloge("ion_memOpen failed!!\n");
        goto ion_open_err;
    }

    /* alloc memory use ION */
    stImage.mpVirAddr[0] = ion_allocMem(iYSize);
    stImage.mpVirAddr[1] = ion_allocMem(iUSize);
    stImage.mpVirAddr[2] = iVSize ? ion_allocMem(iVSize) : NULL;
    if (NULL == stImage.mpVirAddr[1]) {
        aloge("ion_allocMem failed!!\n");
        iRet = -1;
        goto ion_alloc_err;
    }
    stImage.mPhyAddr[0]  = ion_getMemPhyAddr(stImage.mpVirAddr[0]);
    stImage.mPhyAddr[1]  = ion_getMemPhyAddr(stImage.mpVirAddr[1]);
    stImage.mPhyAddr[2]  = iVSize ? ion_getMemPhyAddr(stImage.mpVirAddr[2]) : 0;
    if (0 == stImage.mPhyAddr[1]) {
        aloge("ion_getMemPhyAddr failed!!\n");
        iRet = -1;
        goto ion_phy_err;
    }

    /* fluse cache */
    /* mpVirAddr[0] use CVE_MAlloc, do not need ion_flushCache */
//  iRet = ion_flushCache(stImage.mpVirAddr[0], iYSize);
    iRet = ion_flushCache(stImage.mpVirAddr[1], iUSize);
    iRet = iVSize ? ion_flushCache(stImage.mpVirAddr[2], iVSize) : 0;
#else
    stImage.mpVirAddr[0] = malloc(iYSize);
    stImage.mpVirAddr[1] = malloc(iUSize);
    stImage.mpVirAddr[2] = iVSize ? malloc(iVSize) : NULL;
    if (NULL == stImage.mpVirAddr[1]) {
        aloge("malloc failed!!\n");
        iRet = -1;
        goto malloc_err;
    }
#endif

    signal(SIGINT, SigHandle);
    /******************************************
    step 3 : EVE algo process
    ******************************************/
    iRet = EVEProcessImage(&stImage, &stEVEConfInfo);
    if (iRet < 0) {
        aloge("reading and process image failure!!\n");
        goto read_img_err;
    }

    iRet = 0;
read_img_err:
#ifdef USE_ION
ion_phy_err:
ion_alloc_err:
    iVSize = iVSize ? ion_freeMem(stImage.mpVirAddr[2]) : 0;
    ion_freeMem(stImage.mpVirAddr[1]);
    ion_freeMem(stImage.mpVirAddr[0]);
    stImage.mpVirAddr[2] = NULL;
    stImage.mpVirAddr[1] = NULL;
    stImage.mpVirAddr[0] = NULL;
    ion_memClose();
ion_open_err:
#else
malloc_err:
    free(stImage.mpVirAddr[2]);
    free(stImage.mpVirAddr[1]);
    free(stImage.mpVirAddr[0]);
    stImage.mpVirAddr[2] = NULL;
    stImage.mpVirAddr[1] = NULL;
    stImage.mpVirAddr[0] = NULL;
#endif
unknow_format:
    UnInitEVEModule(&stEVEConfInfo);
init_hcnt_module_err:
    if (0 == iRet) {
        printf("sample_EVE exit!\n");
    }
    return iRet;
}

