#define LOG_TAG "sample_vi2EVE"
#include <utils/plat_log.h>

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include "aw_ai_common_type.h"
//#include "aw_ai_eve_event_interface.h"
#include <mpi_videoformat_conversion.h>
#include "media/mm_comm_vi.h"
#include "media/mpi_sys.h"
#include "media/mpi_vi.h"
#include "media/mpi_isp.h"

#include <confparser.h>
#include "sample_vi2EVE.h"

#define TIME_TEST
#ifdef TIME_TEST
#include "SystemBase.h"
#endif

typedef struct SAMPLE_EVE_TEST_DATA_S {
    int iViDev;
    int iViChn;

    AW_HANDLE pHandle;
} SAMPLE_EVE_TEST_DATA_S ;

static void DmaCallBackFunc(void* pUsr)
{
#ifdef TEST_PRINT
    printf("DmaCallBackFunc\n");
#endif
}

static int g_iThreadExitFlag = 0;

static void SigHandle(int iArg)
{
    g_iThreadExitFlag = 1;
}

static void *EVEProcessThread(void *pArg)
{
    int iRet;
    EVE_CONF_INFO_S *pstConfInfo = (EVE_CONF_INFO_S *)pArg;

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
    AW_AI_EVE_EVENT_RESULT_S *pstEVEResult = &pstConfInfo->stResult;
    VIDEO_FRAME_INFO_S stFrame;

    while (1) {
        if (g_iThreadExitFlag) {
            g_iThreadExitFlag = 0;
            goto thread_exit;
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
        stImage.mPhyAddr[0]  = stFrame.VFrame.mPhyAddr[0];
        stImage.mPhyAddr[1]  = stFrame.VFrame.mPhyAddr[1];
        stImage.mPhyAddr[2]  = stFrame.VFrame.mPhyAddr[2];
        iTimeStamp = stFrame.VFrame.mpts;

        AW_STATUS_E eProcessRet;
#ifdef TIME_TEST
        int64_t nStartTm = CDX_GetSysTimeUsMonotonic();//start time
#endif
        AW_AI_EVE_Event_SetEveSourceAddress(pstConfInfo->psEVEHd, (void *)stImage.mPhyAddr[0]);
        eProcessRet = AW_AI_EVE_Event_Process(pstConfInfo->psEVEHd, &stImage, iTimeStamp, pstEVEResult);
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
        } else {
            aloge("********************wrong********************\n");
        }

        AW_MPI_VI_ReleaseFrame(pstConfInfo->iDev, pstConfInfo->iChn, &stFrame);
        iFrameIndex++;
    }

thread_exit:
    if (0 == iFindTargetNum) {
        printf("\033[31m");
        printf("==================================\n");
        printf("can not find the a target\n");
        printf("==================================\n");
        printf("\033[0m");
        fflush(stdout); // flush stdout, resume console's output format
    }

    iRet = 0;
    fclose(pOutFd);
open_result_file_err:
    pthread_exit(&iRet);
}

static ERRORTYPE InitVippDevice(EVE_CONF_INFO_S *pConfInfo)
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

static ERRORTYPE UnInitVippDevice(EVE_CONF_INFO_S *pConfInfo)
{
    AW_MPI_VI_DisableVipp(pConfInfo->iDev);
    AW_MPI_VI_DestoryVipp(pConfInfo->iDev);
    AW_MPI_SYS_Exit();

    return SUCCESS;
}

static ERRORTYPE InitEVEModule(EVE_CONF_INFO_S *pConfInfo)
{
    int iRet = 0;

    /* initilize EVE module */
    AW_AI_EVE_CTRL_S sEVECtrl;
    AW_HANDLE pEVEHd;

//    sEVECtrl.addrInputType = AW_AI_EVE_ADDR_INPUT_VIR; /* for malloc, we use virtual address */
    sEVECtrl.addrInputType = AW_AI_EVE_ADDR_INPUT_PHY;
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
    stFaceDetParam.sRoiSet.sRoi[0].s16Top    = 0;
    stFaceDetParam.sRoiSet.sRoi[0].s16Bottom = pConfInfo->iPicHeight;
    stFaceDetParam.sRoiSet.sRoi[0].s16Left   = 0;
    stFaceDetParam.sRoiSet.sRoi[0].s16Right  = pConfInfo->iPicWidth;
    stFaceDetParam.s32ClassifyFlag = 0; //close
    stFaceDetParam.s32MinFaceSize  = 20;
    stFaceDetParam.s32OverLapCoeff = 20;
    stFaceDetParam.s32MaxFaceNum = 128;
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

static ERRORTYPE UnInitEVEModule(AW_HANDLE pHandle)
{
    AW_AI_EVE_Event_UnInit(pHandle);
    return SUCCESS;
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

int main(int argc, char *argv[])
{
    int iRet;

    alogd("sample_vi2EVE running!\n");
    EVE_CONF_INFO_S stEVEConfInfo;
    memset(&stEVEConfInfo, 0, sizeof(EVE_CONF_INFO_S));

    iRet = ParseCmdLine(argc, argv, &stEVEConfInfo.stCmdlineParam);
    if (iRet < 0) {
//        aloge("parse cmdline error.\n");
        return -1;
    }

    /* will always return SUCCESS */
    LoadConfigPara(&stEVEConfInfo, stEVEConfInfo.stCmdlineParam.mConfigFilePath);

    /* initialize video input device */
    iRet = InitVippDevice(&stEVEConfInfo);
    if (iRet < 0) {
        aloge("InitVippDevice failed!!\n");
        goto init_vipp_err;
    }
#if 1
#define ISP_RUN 1
#if ISP_RUN
    /* open isp, improve the video's quality */
    int iIspDev = 0;

    if (stEVEConfInfo.iDev == 0 || stEVEConfInfo.iDev == 2) {
        iIspDev = 1;
    } else if (stEVEConfInfo.iDev == 1 || stEVEConfInfo.iDev == 3) {
        iIspDev = 0;
    }
    AW_MPI_ISP_Init();
    AW_MPI_ISP_Run(iIspDev);
#endif

    iRet = InitEVEModule(&stEVEConfInfo);
    if (iRet < 0) {
        aloge("InitEVEModule failed!!\n");
        goto init_eve_module_err;
    }

    /* create a virvi component, use virtual channel 0 */
    int iViDev = stEVEConfInfo.iDev;
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

    stEVEConfInfo.iChn = iViChn;
    iRet = pthread_create(&tid, NULL, EVEProcessThread, &stEVEConfInfo);
    if (iRet != 0) {
        aloge("create thread failed!!\n");
        iRet = -1;
        goto create_thread_err;
    }

    pthread_join(tid, NULL);
create_thread_err:
    UnInitVirviComp(iViDev, iViChn);
init_virvi_comp_err:
    UnInitEVEModule(stEVEConfInfo.psEVEHd);
init_eve_module_err:
#if ISP_RUN
    AW_MPI_ISP_Stop(iIspDev);
    AW_MPI_ISP_Exit();
#endif
#endif
    UnInitVippDevice(&stEVEConfInfo);
init_vipp_err:
    if (0 == iRet) {
        printf("sample_vi2EVE exit!\n");
    }
    return iRet;
}
