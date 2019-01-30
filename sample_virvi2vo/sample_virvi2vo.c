//#define LOG_NDEBUG 0
#define LOG_TAG "sample_virvi2vo"
#include <utils/plat_log.h>

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "media/mm_comm_vi.h"
#include "media/mpi_vi.h"
#include "vo/hwdisplay.h"
#include "log/log_wrapper.h"

#include <ClockCompPortIndex.h>
#include <mpi_videoformat_conversion.h>
#include <confparser.h>
#include "sample_virvi2vo_config.h"
#include "sample_virvi2vo.h"
#include "mpi_isp.h"

SampleVIRVI2VOContext *pSampleVIRVI2VOContext = NULL;

int initSampleVIRVI2VOContext(SampleVIRVI2VOContext *pContext)
{
    memset(pContext, 0, sizeof(SampleVIRVI2VOContext));
    //pContext->mUILayer = HLAY(2, 0);
    cdx_sem_init(&pContext->mSemExit, 0);
    return 0;
}

int destroySampleVIRVI2VOContext(SampleVIRVI2VOContext *pContext)
{
    cdx_sem_deinit(&pContext->mSemExit);
    return 0;
}

static ERRORTYPE SampleVIRVI2VO_VOCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    ERRORTYPE ret = SUCCESS;
    SampleVIRVI2VOContext *pContext = (SampleVIRVI2VOContext*)cookie;
    if(MOD_ID_VOU == pChn->mModId) {
        if(pChn->mChnId != pContext->mVOChn) {
            aloge("fatal error! VO chnId[%d]!=[%d]", pChn->mChnId, pContext->mVOChn);
        }
        switch(event) {
        case MPP_EVENT_RELEASE_VIDEO_BUFFER: {
            aloge("fatal error! sample_virvi2vo use binding mode!");
            break;
        }
        case MPP_EVENT_SET_VIDEO_SIZE: {
            SIZE_S *pDisplaySize = (SIZE_S*)pEventData;
            alogd("vo report video display size[%dx%d]", pDisplaySize->Width, pDisplaySize->Height);
            break;
        }
        case MPP_EVENT_RENDERING_START: {
            alogd("vo report rendering start");
            break;
        }
        default: {
            //postEventFromNative(this, event, 0, 0, pEventData);
            aloge("fatal error! unknown event[0x%x] from channel[0x%x][0x%x][0x%x]!", event, pChn->mModId, pChn->mDevId, pChn->mChnId);
            ret = ERR_VO_ILLEGAL_PARAM;
            break;
        }
        }
    } else {
        aloge("fatal error! why modId[0x%x]?", pChn->mModId);
        ret = FAILURE;
    }
    return ret;
}

static ERRORTYPE SampleVIRVI2VO_CLOCKCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    alogw("clock channel[%d] has some event[0x%x]", pChn->mChnId, event);
    return SUCCESS;
}

static int ParseCmdLine(int argc, char **argv, SampleVIRVI2VOCmdLineParam *pCmdLinePara)
{
    alogd("sample_virvi2vo path:[%s], arg number is [%d]", argv[0], argc);
    int ret = 0;
    int i=1;
    memset(pCmdLinePara, 0, sizeof(SampleVIRVI2VOCmdLineParam));
    while(i < argc) {
        if(!strcmp(argv[i], "-path")) {
            if(++i >= argc) {
                aloge("fatal error! use -h to learn how to set parameter!!!");
                ret = -1;
                break;
            }
            if(strlen(argv[i]) >= MAX_FILE_PATH_SIZE) {
                aloge("fatal error! file path[%s] too long: [%d]>=[%d]!", argv[i], strlen(argv[i]), MAX_FILE_PATH_SIZE);
            }
            strncpy(pCmdLinePara->mConfigFilePath, argv[i], MAX_FILE_PATH_SIZE-1);
            pCmdLinePara->mConfigFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
        } else if(!strcmp(argv[i], "-h")) {
            printf("CmdLine param example:\n"
                   "\t run -path /home/sample_virvi2vo.conf\n");
            ret = 1;
            break;
        } else {
            alogd("ignore invalid CmdLine param:[%s], type -h to get how to set parameter!", argv[i]);
        }
        i++;
    }
    return ret;
}

static ERRORTYPE loadSampleVIRVI2VOConfig(SampleVIRVI2VOConfig *pConfig, const char *conf_path)
{
    int ret;
    char *ptr;
    if(NULL == conf_path) {
        alogd("user not set config file. use default test parameter!");
        pConfig->mDevNum = 3;
        pConfig->mCaptureWidth = 1920;
        pConfig->mCaptureHeight = 1080;
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        pConfig->mFrameRate = 30;
        pConfig->mTestDuration = 0;
        return SUCCESS;
    }
    CONFPARSER_S stConfParser;
    ret = createConfParser(conf_path, &stConfParser);
    if(ret < 0) {
        aloge("load conf fail");
        return FAILURE;
    }
    memset(pConfig, 0, sizeof(SampleVIRVI2VOConfig));
    pConfig->mDevNum        = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_DEVICE_NUMBER, 0);
    pConfig->mCaptureWidth  = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_CAPTURE_WIDTH, 0);
    pConfig->mCaptureHeight = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_CAPTURE_HEIGHT, 0);
    pConfig->mDisplayWidth  = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_WIDTH, 0);
    pConfig->mDisplayHeight = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_DISPLAY_HEIGHT, 0);
    char *pStrPixelFormat = (char*)GetConfParaString(&stConfParser, SAMPLE_VIRVI2VO_KEY_PIC_FORMAT, NULL);
    if(!strcmp(pStrPixelFormat, "yu12")) {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YUV_PLANAR_420;
    } else if(!strcmp(pStrPixelFormat, "yv12")) {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_PLANAR_420;
    } else if(!strcmp(pStrPixelFormat, "nv21")) {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    } else if(!strcmp(pStrPixelFormat, "nv12")) {
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    } else {
        aloge("fatal error! conf file pic_format is [%s]?", pStrPixelFormat);
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }
    char *pStrDispType = (char*)GetConfParaString(&stConfParser, SAMPLE_VIRVI2VO_KEY_DISP_TYPE, NULL);
    if (!strcmp(pStrDispType, "hdmi")) {
        pConfig->mDispType = VO_INTF_HDMI;
        if (pConfig->mDisplayWidth > 1920)
            pConfig->mDispSync = VO_OUTPUT_3840x2160_30;
        else if (pConfig->mDisplayWidth > 1280)
            pConfig->mDispSync = VO_OUTPUT_1080P30;
        else
            pConfig->mDispSync = VO_OUTPUT_720P60;
    } else if (!strcmp(pStrDispType, "lcd")) {
        pConfig->mDispType = VO_INTF_LCD;
        pConfig->mDispSync = VO_OUTPUT_NTSC;
    } else if (!strcmp(pStrDispType, "cvbs")) {
        pConfig->mDispType = VO_INTF_CVBS;
        pConfig->mDispSync = VO_OUTPUT_NTSC;
    }

    pConfig->mFrameRate = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_FRAME_RATE, 0);
    pConfig->mTestDuration = GetConfParaInt(&stConfParser, SAMPLE_VIRVI2VO_KEY_TEST_DURATION, 0);

    alogd("pConfig->mCaptureWidth=[%d], pConfig->mCaptureHeight=[%d], pConfig->mDevNum=[%d]",
          pConfig->mCaptureWidth, pConfig->mCaptureHeight, pConfig->mDevNum);
    alogd("pConfig->mDisplayWidth=[%d], pConfig->mDisplayHeight=[%d], pConfig->mDispSync=[%d], pConfig->mDispType=[%d]",
          pConfig->mDisplayWidth, pConfig->mDisplayHeight, pConfig->mDispSync, pConfig->mDispType);
    alogd("pConfig->mFrameRate=[%d], pConfig->mTestDuration=[%d]",
          pConfig->mFrameRate, pConfig->mTestDuration);

    destroyConfParser(&stConfParser);
    return SUCCESS;
}
typedef struct {
    int ae_mode;
    int ae_expOffset;
    int ae_exposure;
    int gain;
    int awb_mode;
    int awb_coltmp;
    int brightness;
    int contrast;
    int saturation;
    int hue;
    unsigned int maxexp;
    unsigned int minexp;
    int sharpness;
} Test_IspParam_S;

Test_IspParam_S param = {0};
void handle_exit(int signo)
{
    alogd("user want to exit!");
    FILE *isp_fp = NULL;
    isp_fp = fopen("./isp_config.txt","w");
    if(isp_fp == NULL) {
        printf("open isp config file error!\n");
    } else {
        fprintf(isp_fp, "ae_mode=%d\n", param.ae_mode);
        fprintf(isp_fp, "ae_expOffset=%d\n", param.ae_expOffset);
        fprintf(isp_fp, "ae_exposure=%d\n", param.ae_exposure);
        fprintf(isp_fp, "gain=%d\n", param.gain);
        fprintf(isp_fp, "awb_mode=%d\n", param.awb_mode);
        fprintf(isp_fp, "awb_coltmp=%d\n", param.awb_coltmp);
        fprintf(isp_fp, "brightness=%d\n", param.brightness);
        fprintf(isp_fp, "contrast=%d\n", param.contrast);
        fprintf(isp_fp, "saturation=%d\n", param.saturation);
        fprintf(isp_fp, "hue=%d\n", param.hue);
        fprintf(isp_fp, "maxexp=%d\n", param.maxexp);
        fprintf(isp_fp, "minexp=%d\n", param.minexp);
        fprintf(isp_fp, "sharpness=%d\n", param.sharpness);

        fclose(isp_fp);
    }

    if(NULL != pSampleVIRVI2VOContext) {
        cdx_sem_up(&pSampleVIRVI2VOContext->mSemExit);
    }
}




void print_menu(void)
{
    printf("\n---------------------\n");
    printf("Please input the num to config ISp \n");
    printf("\t num 1 is the AE_mode\n\t num 2 is the AE_expOffset\n\t num 3 is the AE_exposure\n\t num 4 is the gain\n\t num 5 is AWB_MODE\n\t num 6 is GetColorTemp\n\t num 7 is AE Max Exp\n\t num 8 is AE Min Exp\n\t num 9 is HUE\n\t num 10 is the Brightness\n\t num 11 is the contrast\n\t num 12 is sattuation\n\t num 13 is Sharpness\n\t num 99 is quit\n");
    printf("\n---------------------\n");

}

int main(int argc __attribute__((__unused__)), char *argv[] __attribute__((__unused__)))
{
    int result = 0;
    int iIspDev;
    //Test_IspParam_S param = {0};
    FILE *fp = fopen("isp_congfig.txt", "r");
    if (NULL != fp) {
        //遍历参数文件
        char text[1024];
        while (fgets(text, 1024, fp)) {
            //解析参数
            char *chr = strchr(text, '=');
            if (NULL == chr)
                continue;

            *chr++ = '\0';
            if (0 == strcmp(text, "ae_mode"))
                param.ae_mode = atoi(chr);
            else if (0 == strcmp(text, "ae_expOffset"))
                param.ae_expOffset = atoi(chr);
            else if (0 == strcmp(text, "ae_exposure"))
                param.ae_exposure = atoi(chr);
            else if (0 == strcmp(text, "gain"))
                param.gain = atoi(chr);
            else if (0 == strcmp(text, "awb_mode"))
                param.awb_mode = atoi(chr);
            else if (0 == strcmp(text, "awb_coltmp"))
                param.awb_coltmp = atoi(chr);
            else if (0 == strcmp(text, "brightness"))
                param.brightness = atoi(chr);
            else if (0 == strcmp(text, "contrast"))
                param.contrast = atoi(chr);
            else if (0 == strcmp(text, "saturation"))
                param.saturation = atoi(chr);
            else if (0 == strcmp(text, "maxexp"))
                param.maxexp = atoi(chr);
            else if (0 == strcmp(text, "minexp"))
                param.minexp = atoi(chr);
            else if (0 == strcmp(text, "hue"))
                param.hue = atoi(chr);
            else if (0 == strcmp(text, "sharpness"))
                param.sharpness = atoi(chr);
        }
        fclose(fp);
    }



    SampleVIRVI2VOContext stContext;

    printf("sample_virvi2vo running!\n");
    initSampleVIRVI2VOContext(&stContext);
    pSampleVIRVI2VOContext = &stContext;

    /* parse command line param */
    if(ParseCmdLine(argc, argv, &stContext.mCmdLinePara) != 0) {
        //aloge("fatal error! command line param is wrong, exit!");
        result = -1;
        goto _exit;
    }
    char *pConfigFilePath;
    if(strlen(stContext.mCmdLinePara.mConfigFilePath) > 0) {
        pConfigFilePath = stContext.mCmdLinePara.mConfigFilePath;
    } else {
        pConfigFilePath = NULL;
    }

    /* parse config file. */
    if(loadSampleVIRVI2VOConfig(&stContext.mConfigPara, pConfigFilePath) != SUCCESS) {
        aloge("fatal error! no config file or parse conf file fail");
        result = -1;
        goto _exit;
    }

    /* register process function for SIGINT, to exit program. */
    if (signal(SIGINT, handle_exit) == SIG_ERR)
        perror("can't catch SIGSEGV");

    /* init mpp system */
    memset(&stContext.mSysConf, 0, sizeof(MPP_SYS_CONF_S));
    stContext.mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&stContext.mSysConf);
    result = AW_MPI_SYS_Init();
    if (result) {
        aloge("fatal error! AW_MPI_SYS_Init failed");
        goto sys_init_err;
    }

    /* create vi channel */
    stContext.mVIDev = stContext.mConfigPara.mDevNum;
    stContext.mVIChn = 0;
    alogd("Vipp dev[%d] vir_chn[%d]", stContext.mVIDev, stContext.mVIChn);
    ERRORTYPE eRet = AW_MPI_VI_CreateVipp(stContext.mVIDev);
    if (eRet != SUCCESS) {
        aloge("fatal error! AW_MPI_VI CreateVipp failed");
        result = eRet;
        goto vi_create_vipp_err;
    }
    VI_ATTR_S stAttr;
    eRet = AW_MPI_VI_GetVippAttr(stContext.mVIDev, &stAttr);
    if (eRet != SUCCESS) {
        aloge("fatal error! AW_MPI_VI GetVippAttr failed");
    }
    memset(&stAttr, 0, sizeof(VI_ATTR_S));
    stAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    stAttr.memtype = V4L2_MEMORY_MMAP;
    stAttr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(stContext.mConfigPara.mPicFormat);
    stAttr.format.field = V4L2_FIELD_NONE;
    // stAttr.format.colorspace = V4L2_COLORSPACE_JPEG;
    stAttr.format.width = stContext.mConfigPara.mCaptureWidth;
    stAttr.format.height = stContext.mConfigPara.mCaptureHeight;
    stAttr.nbufs = 10;
    stAttr.nplanes = 2;
    /* do not use current param, if set to 1, all this configuration will
     * not be used.
     */
    stAttr.use_current_win = 0;
    stAttr.fps = stContext.mConfigPara.mFrameRate;
    eRet = AW_MPI_VI_SetVippAttr(stContext.mVIDev, &stAttr);
    if (eRet != SUCCESS) {
        aloge("fatal error! AW_MPI_VI SetVippAttr failed");
    }
    eRet = AW_MPI_VI_EnableVipp(stContext.mVIDev);
    if(eRet != SUCCESS) {
        aloge("fatal error! enableVipp fail!");
        result = eRet;
        goto vi_enable_vipp_err;
    }
#define ISP_RUN 1
#if ISP_RUN
    /* open isp */
    if (stContext.mVIDev == 0 || stContext.mVIDev == 2) {
        iIspDev = 1;
    } else if (stContext.mVIDev == 1 || stContext.mVIDev == 3) {
        iIspDev = 0;
    }
    AW_MPI_ISP_Init();
    AW_MPI_ISP_Run(iIspDev);
    AW_MPI_VI_SetVippMirror(stContext.mVIDev,0);//0,1
    AW_MPI_VI_SetVippFlip(stContext.mVIDev,0);//0,1
#endif
    eRet = AW_MPI_VI_CreateVirChn(stContext.mVIDev, stContext.mVIChn, NULL);
    if(eRet != SUCCESS) {
        aloge("fatal error! createVirChn[%d] fail!", stContext.mVIChn);
    }
    /* enable vo dev */
    stContext.mVoDev = 0;
    AW_MPI_VO_Enable(stContext.mVoDev);
    //AW_MPI_VO_AddOutsideVideoLayer(stContext.mUILayer);
    //AW_MPI_VO_CloseVideoLayer(stContext.mUILayer); /* close ui layer. */
    /* enable vo layer */
    int hlay0 = 0;
    while(hlay0 < VO_MAX_LAYER_NUM) {
        if(SUCCESS == AW_MPI_VO_EnableVideoLayer(hlay0)) {
            break;
        }
        hlay0++;
    }
    if(hlay0 >= VO_MAX_LAYER_NUM) {
        aloge("fatal error! enable video layer fail!");
    }

    VO_PUB_ATTR_S spPubAttr;
    AW_MPI_VO_GetPubAttr(stContext.mVoDev, &spPubAttr);
    spPubAttr.enIntfType = stContext.mConfigPara.mDispType;
    spPubAttr.enIntfSync = stContext.mConfigPara.mDispSync;
    AW_MPI_VO_SetPubAttr(stContext.mVoDev, &spPubAttr);

    stContext.mVoLayer = hlay0;
    AW_MPI_VO_SetVideoLayerPriority(stContext.mVoLayer, 11);
    AW_MPI_VO_GetVideoLayerAttr(stContext.mVoLayer, &stContext.mLayerAttr);
    stContext.mLayerAttr.stDispRect.X = 0;
    stContext.mLayerAttr.stDispRect.Y = 0;
    stContext.mLayerAttr.stDispRect.Width = stContext.mConfigPara.mDisplayWidth;
    stContext.mLayerAttr.stDispRect.Height = stContext.mConfigPara.mDisplayHeight;
    AW_MPI_VO_SetVideoLayerAttr(stContext.mVoLayer, &stContext.mLayerAttr);

    /* create vo channel and clock channel.
    (because frame information has 'pts', there is no need clock channel now)
    */
    BOOL bSuccessFlag = FALSE;
    stContext.mVOChn = 0;
    while(stContext.mVOChn < VO_MAX_CHN_NUM) {
        eRet = AW_MPI_VO_EnableChn(stContext.mVoLayer, stContext.mVOChn);
        if(SUCCESS == eRet) {
            bSuccessFlag = TRUE;
            alogd("create vo channel[%d] success!", stContext.mVOChn);
            break;
        } else if(ERR_VO_CHN_NOT_DISABLE == eRet) {
            alogd("vo channel[%d] is exist, find next!", stContext.mVOChn);
            stContext.mVOChn++;
        } else {
            aloge("fatal error! create vo channel[%d] ret[0x%x]!", stContext.mVOChn, eRet);
            break;
        }
    }
    if(FALSE == bSuccessFlag) {
        stContext.mVOChn = MM_INVALID_CHN;
        aloge("fatal error! create vo channel fail!");
        result = -1;
        goto vo_create_chn_err;
    }
    MPPCallbackInfo cbInfo;
    cbInfo.cookie = (void*)&stContext;
    cbInfo.callback = (MPPCallbackFuncType)&SampleVIRVI2VO_VOCallbackWrapper;
    AW_MPI_VO_RegisterCallback(stContext.mVoLayer, stContext.mVOChn, &cbInfo);
    AW_MPI_VO_SetChnDispBufNum(stContext.mVoLayer, stContext.mVOChn, 2);

    /* bind clock,vo, viChn
    (because frame information has 'pts', there is no need to bind clock channel now)
    */
    MPP_CHN_S VOChn = {MOD_ID_VOU, stContext.mVoLayer, stContext.mVOChn};
    MPP_CHN_S VIChn = {MOD_ID_VIU, stContext.mVIDev, stContext.mVIChn};
    AW_MPI_SYS_Bind(&VIChn, &VOChn);

    /* start vo, vi_channel. */
    eRet = AW_MPI_VI_EnableVirChn(stContext.mVIDev, stContext.mVIChn);
    if(eRet != SUCCESS) {
        aloge("fatal error! enableVirChn fail!");
        result = eRet;
        goto vi_enable_virchn_err;
    }
    AW_MPI_VO_StartChn(stContext.mVoLayer, stContext.mVOChn);

#if 0
    char str[256]  = {0};
    char val[16] = {0};
    int num = 0, vl = 0;

    printf("\033[33m");
    printf("===========ISP test=========\n");
    printf("input <99> & ctrl+c to exit!\n");
    printf("input <1-13> to test\n");
    printf("============================\n");
    printf("\033[0m");
    while (1) {
        print_menu();
        memset(str, 0, sizeof(str));
        memset(val,0,sizeof(val));
        fgets(str, 256, stdin);
        num = atoi(str);
        if (99 == num) {
            printf("break test.\n");
            printf("enter ctrl+c to exit this program.\n");
            break;
        }

        switch (num) {
        case 1://///////
            AW_MPI_ISP_AE_GetMode(iIspDev,&vl);
            printf("\n---------------AE set mode---the current AE mode = %d\n",vl);
            fgets(val,16,stdin);
            param.ae_mode = atoi(val);
            AW_MPI_ISP_AE_SetMode(iIspDev,param.ae_mode);//0 ,1---ok
            AW_MPI_ISP_AE_GetMode(iIspDev,&vl);
            printf("AE mode: after setting  value = %d\r\n", vl);
            break;
        case 2:
            AW_MPI_ISP_AE_GetExposureBias(iIspDev,&vl);
            printf("\n---------------AE set expOffset---the current AE expOffset bias = %d\n",vl);
            fgets(val,16,stdin);
            param.ae_expOffset = atoi(val);
            AW_MPI_ISP_AE_SetExposureBias(iIspDev,param.ae_expOffset);//0~8---ok
            AW_MPI_ISP_AE_GetExposureBias(iIspDev,&vl);
            printf("AE exposure bias: after setting  value = %d\r\n", vl);
            break;
        case 3://////
            AW_MPI_ISP_AE_GetExposure(iIspDev,&vl);
            printf("\n---------------AE set exposure---the current AE exposure = %d\n",vl);
            fgets(val,16,stdin);
            param.ae_exposure = atoi(val);
            AW_MPI_ISP_AE_SetExposure(iIspDev,param.ae_exposure);//
            AW_MPI_ISP_AE_GetExposure(iIspDev,&vl);//
            printf("AE exposure: after setting value = %d\r\n", vl);
            break;
        case 4://///////
            AW_MPI_ISP_AE_GetGain(iIspDev,&vl);
            printf("\n---------------AE set gain---the current AE gain  = %d\n",vl);
            fgets(val,16,stdin);
            param.gain = atoi(val);
            AW_MPI_ISP_AE_SetGain(iIspDev,param.gain );//256
            AW_MPI_ISP_AE_GetGain(iIspDev,&vl);//256
            printf("AE gain: after setting value = %d\r\n", vl);
            break;
        case 5:
            AW_MPI_ISP_AWB_GetMode(iIspDev,&vl);
            printf("\n---------------AWB  mode---the current AWB mode  = %d\n",vl);
            fgets(val,16,stdin);
            param.awb_mode = atoi(val);
            AW_MPI_ISP_AWB_SetMode(iIspDev,param.awb_mode);//0,1---ok
            AW_MPI_ISP_AWB_GetMode(iIspDev,&vl);//0,1
            printf("AWB mode: after setting value = %d\r\n", vl);
            break;
        case 6:
            AW_MPI_ISP_AWB_GetColorTemp(iIspDev,&vl);
            printf("\n---------------AWB  clolor temperature---the current AWB clolor temperature  = %d\n",vl);
            fgets(val,16,stdin);
            param.awb_coltmp = atoi(val);
            AW_MPI_ISP_AWB_SetColorTemp(iIspDev,param.awb_coltmp);//0,1,2,3,4,5,6,7, ---ok
            AW_MPI_ISP_AWB_GetColorTemp(iIspDev,&vl);//0,1,2,3,4,5,6,7,
            printf("AWB clolor temperature: after setting value = %d\r\n", vl);
            break;
        case 7:
            AW_MPI_ISP_GetAeMinMaxExp(iIspDev, &param.minexp, &param.maxexp);
            printf("\n---------------AE Max Exp---the current AE Max Exp  = %d\n",param.maxexp);
            fgets(val,16,stdin);
            param.maxexp = atoi(val);
            AW_MPI_ISP_SetAeMinMaxExp(iIspDev, param.minexp, param.maxexp);//
            AW_MPI_ISP_GetAeMinMaxExp(iIspDev, &param.minexp, &param.maxexp);//
            printf("AeMaxExp: after setting MAx value = %d.\r\n", param.maxexp);
            break;
        case 8:
            AW_MPI_ISP_GetAeMinMaxExp(iIspDev, &param.minexp, &param.maxexp);
            printf("\n---------------AE Min Exp---the current AE Min Exp  = %d\n",param.minexp);
            fgets(val,16,stdin);
            param.minexp = atoi(val);
            AW_MPI_ISP_SetAeMinMaxExp(iIspDev, param.minexp, param.maxexp);//
            AW_MPI_ISP_GetAeMinMaxExp(iIspDev, &param.minexp, &param.maxexp);//
            printf("AeMinExp: after setting MIN value = %d.\r\n", param.minexp);
            break;
        case 9:
            AW_MPI_ISP_GetHue(iIspDev,&vl);
            printf("\n---------------HUE  VALUE---the current HUE  VALUE  = %d\n",vl);
            fgets(val,16,stdin);
            param.hue = atoi(val);
            AW_MPI_ISP_SetHue(iIspDev,param.hue);//
            AW_MPI_ISP_GetHue(iIspDev,&vl);//
            printf("HUE: after setting value = %d\r\n", vl);
            break;
        case 10:
            AW_MPI_ISP_GetBrightness(iIspDev,&vl);
            printf("\n---------------brightness  VALUE---the current brightness  value  = %d\n",vl);
            fgets(val,16,stdin);
            param.brightness = atoi(val);
            AW_MPI_ISP_SetBrightness(iIspDev,param.brightness);//0~256
            AW_MPI_ISP_GetBrightness(iIspDev,&vl);//0~256
            printf("brightness: after setting value = %d\r\n", vl);
            break;
        case 11:
            AW_MPI_ISP_GetContrast(iIspDev,&vl);
            printf("\n---------------contrast  VALUE---the current contrast  value  = %d\n",vl);
            fgets(val,16,stdin);
            param.contrast = atoi(val);
            AW_MPI_ISP_SetContrast(iIspDev,param.contrast);//0~256
            AW_MPI_ISP_GetContrast(iIspDev,&vl);//0~256
            printf("contrast: after setting value = %d\r\n", vl);
            break;
        case 12:
            AW_MPI_ISP_GetSaturation(iIspDev,&vl);
            printf("\n---------------saturation  VALUE---the current saturation  value  = %d\n",vl);
            fgets(val,16,stdin);
            param.saturation = atoi(val);
            AW_MPI_ISP_SetSaturation(iIspDev,param.saturation);//0~256
            AW_MPI_ISP_GetSaturation(iIspDev,&vl);//0~256
            printf("saturation: after setting value = %d\r\n", vl);
            break;
        case 13:
            AW_MPI_ISP_GetSharpness(iIspDev,&vl);
            printf("\n---------------sharpness  VALUE---the current sharpness  value  = %d\n",vl);
            fgets(val,16,stdin);
            param.sharpness = atoi(val);
            AW_MPI_ISP_SetSharpness(iIspDev,param.sharpness);//0~256
            AW_MPI_ISP_GetSharpness(iIspDev,&vl);//0~256
            printf("sharpness: after setting value = %d\r\n", vl);
            break;
        default:
            printf("intput error.\r\n");
            break;
        }
    }

#endif

    if(stContext.mConfigPara.mTestDuration > 0) {
        cdx_sem_down_timedwait(&stContext.mSemExit, stContext.mConfigPara.mTestDuration*1000);
    } else {
        cdx_sem_down(&stContext.mSemExit);
    }
    /* stop vo channel, vi channel */
    AW_MPI_VO_StopChn(stContext.mVoLayer, stContext.mVOChn);
    AW_MPI_VI_DisableVirChn(stContext.mVIDev, stContext.mVIChn);

vi_enable_virchn_err:
    AW_MPI_VO_DisableChn(stContext.mVoLayer, stContext.mVOChn);
    stContext.mVOChn = MM_INVALID_CHN;

vo_create_chn_err:
    AW_MPI_VI_DestoryVirChn(stContext.mVIDev, stContext.mVIChn);

#if ISP_RUN
    AW_MPI_ISP_Stop(iIspDev);
    AW_MPI_ISP_Exit();
#endif
    AW_MPI_VI_DisableVipp(stContext.mVIDev);
    AW_MPI_VI_DestoryVipp(stContext.mVIDev);
    stContext.mVIDev = MM_INVALID_DEV;
    stContext.mVIChn = MM_INVALID_CHN;

    /* disable vo layer */
    AW_MPI_VO_DisableVideoLayer(stContext.mVoLayer);
    stContext.mVoLayer = -1;
    //AW_MPI_VO_RemoveOutsideVideoLayer(stContext.mUILayer);
    /* disalbe vo dev */
    AW_MPI_VO_Disable(stContext.mVoDev);
    stContext.mVoDev = -1;

vi_enable_vipp_err:
vi_create_vipp_err:
    /* exit mpp system */
    AW_MPI_SYS_Exit();
sys_init_err:
_exit:
    destroySampleVIRVI2VOContext(&stContext);
    if (result == 0) {
        printf("sample_virvi2vo exit!\n");
    }

    return result;
}

