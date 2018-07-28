//#define LOG_NDEBUG 0
#define LOG_TAG "sample_region"
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
#include "sample_region_config.h"
#include "sample_region.h"
#include "mpi_isp.h"
#include <BITMAP_S.h>
#include "media/mpi_venc.h"
#include "mm_comm_venc.h"


#define HAVE_H264

static SampleRegionContext *pSampleRegionContext = NULL;
static FILE* OutputFile_Fd;

int initSampleRegionContext(SampleRegionContext *pContext)
{
    memset(pContext, 0, sizeof(SampleRegionContext));
    pContext->mUILayer = HLAY(2, 0);
    cdx_sem_init(&pContext->mSemExit, 0);
    return 0;
}

int destroySampleRegionContext(SampleRegionContext *pContext)
{
    cdx_sem_deinit(&pContext->mSemExit);
    return 0;
}

static int ParseCmdLine(int argc, char **argv, SampleRegionCmdLineParam *pCmdLinePara)
{
    //alogd("sample_region input path is : %s", argv[0]);
    int ret = 0;
    int i = 1;
    memset(pCmdLinePara, 0, sizeof(SampleRegionCmdLineParam));
    while(i < argc) {
        if(!strcmp(argv[i], "-path")) {
            if((++i) >= argc) {
                aloge("fatal error!");
                ret = -1;
                break;
            }
            if(strlen(argv[i]) >= MAX_FILE_PATH_SIZE) {
                aloge("fatal error!");
            }
            strncpy(pCmdLinePara->mConfigFilePath, argv[i], strlen(argv[i]));
            pCmdLinePara->mConfigFilePath[strlen(argv[i])] = '\0';
        } else if(!strcmp(argv[i], "-h")) {
            printf("CmdLine param example:\n"
                   "\t run -path /home/sample_vi2vo.conf\n");
            ret = 1;
            break;
        } else {
            printf("CmdLine param example:\n"                "\t run -path /home/sample_vi2vo.conf\n");
        }
        ++i;
    }
    return ret;
}


static ERRORTYPE loadSampleRegionConfig(SampleRegionConfig *pConfig, const char *conf_path)
{
    int ret;
    char *ptr;

    if(NULL == conf_path) {
        alogd("user not set config file. use default test parameter!");
        pConfig->mCaptureWidth = 1920;
        pConfig->mCaptureHeight = 1080;
        pConfig->mPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        pConfig->mFrameRate = 30;
        pConfig->mTestDuration = 0;

        pConfig->overlay_x = 860;
        pConfig->overlay_y = 540;
        pConfig->overlay_w = 100;
        pConfig->overlay_h = 100;

        pConfig->cover_x = 100;
        pConfig->cover_y = 100;
        pConfig->cover_w = 400;
        pConfig->cover_h = 400;

        pConfig->add_venc_channel = FALSE;
        pConfig->encoder_count = 5000;
        pConfig->bit_rate = 8388608;
        pConfig->EncoderType = PT_H264;
        //pConfig->OutputFilePath = "test.h264";
        strcpy(pConfig->OutputFilePath, "test.h264");

        return SUCCESS;
    }
    CONFPARSER_S stConfParser;
    ret = createConfParser(conf_path, &stConfParser);
    if(ret < 0) {
        aloge("load conf fail");
        return FAILURE;
    }

    memset(pConfig, 0, sizeof(SampleRegionConfig));

    pConfig->mCaptureWidth = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_CAPTURE_WIDTH, 1920);
    pConfig->mCaptureHeight = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_CAPTURE_HEIGHT, 1080);

    char *pStrPixelFormat = (char*)GetConfParaString(&stConfParser, SAMPLE_REGION_KEY_PIC_FORMAT, NULL);
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
    pConfig->mFrameRate = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_FRAME_RATE, 30);
    pConfig->mTestDuration = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_TEST_DURATION, 0);

    pConfig->overlay_x = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_OVERLAY_X, 0);
    pConfig->overlay_y = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_OVERLAY_Y, 0);
    pConfig->overlay_w = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_OVERLAY_W, 0);
    pConfig->overlay_h = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_OVERLAY_H, 0);

    pConfig->cover_x = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_COVER_X, 0);
    pConfig->cover_y = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_COVER_Y, 0);
    pConfig->cover_w = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_COVER_W, 0);
    pConfig->cover_h = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_COVER_H, 0);

    char *AddVenc = (char*)GetConfParaString(&stConfParser, SAMPLE_REGION_KEY_ADD_VENC_CHANNEL, NULL);
    if(!strcmp(AddVenc, "yes")) {
        pConfig->add_venc_channel = TRUE;
    } else {
        pConfig->add_venc_channel = FALSE;
    }
    pConfig->encoder_count = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_ENCODER_COUNT, 5000);
    pConfig->bit_rate = GetConfParaInt(&stConfParser, SAMPLE_REGION_KEY_BIT_RATE, 0);
    char *EncoderType = (char*)GetConfParaString(&stConfParser, SAMPLE_REGION_KEY_ENCODERTYPE, NULL);
    if(!strcmp(EncoderType, "H.264")) {
        pConfig->EncoderType = PT_H264;
    } else if(!strcmp(EncoderType, "H.265")) {
        pConfig->EncoderType = PT_H265;
    } else if(!strcmp(EncoderType, "MJPEG")) {
        pConfig->EncoderType = PT_MJPEG;
    } else {
        alogw("unsupported venc type:%p,encoder type turn to H.264!",EncoderType);
        pConfig->EncoderType = PT_H264;
    }
    char *pStr = (char *)GetConfParaString(&stConfParser, SAMPLE_REGION_KEY_OUTPUT_FILE_PATH, NULL);
    strncpy(pConfig->OutputFilePath, pStr, MAX_FILE_PATH_SIZE-1);
    pConfig->OutputFilePath[MAX_FILE_PATH_SIZE-1] = '\0';


    destroyConfParser(&stConfParser);
    return SUCCESS;
}

void handle_exit(int signo)
{
    alogd("user want to exit!");
    if(pSampleRegionContext != NULL) {
        cdx_sem_up(&pSampleRegionContext->mSemExit);
    }
}


static ERRORTYPE SampleRegion_VOCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    ERRORTYPE ret = SUCCESS;
    SampleRegionContext *pContext = ( SampleRegionContext*)cookie;
    if(MOD_ID_VOU == pChn->mModId) {
        if(pChn->mChnId != pContext->mVOChn) {
            aloge("fatal error! VO chnId[%d]!=[%d]", pChn->mChnId, pContext->mVOChn);
        }
        switch(event) {
        case MPP_EVENT_RELEASE_VIDEO_BUFFER: {
            aloge("fatal error! sample_vi2vo use binding mode!");
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

static ERRORTYPE SampleRegion_CLOCKCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    alogw("clock channel[%d] has some event[0x%x]", pChn->mChnId, event);
    return SUCCESS;
}

static void *GetEncoderFrameThread(void * pArg)
{
    //*pSampleRegionContext
    int count = 0;
    VENC_STREAM_S VencFrame;
    VENC_PACK_S venc_pack;
    VencFrame.mPackCount = 1;
    VencFrame.mpPack = &venc_pack;

    int eRet =  AW_MPI_VI_CreateVirChn(pSampleRegionContext->mVIDev, pSampleRegionContext->mVIChn + 1, NULL);
    if(eRet != SUCCESS) {
        aloge("error:AW_MPI_VI_EnableVipp failed");
    }

    MPP_CHN_S ViChn = {MOD_ID_VIU, pSampleRegionContext->mVIDev, pSampleRegionContext->mVIChn + 1};
    MPP_CHN_S VeChn = {MOD_ID_VENC, 0, pSampleRegionContext->mVEChn};
    AW_MPI_SYS_Bind(&ViChn,&VeChn);

    eRet = AW_MPI_VI_EnableVirChn(pSampleRegionContext->mVIDev, pSampleRegionContext->mVIChn + 1);
    if(eRet != SUCCESS) {
        aloge("error:enablecirchn failed");
    }

    AW_MPI_VENC_StartRecvPic(pSampleRegionContext->mVEChn);

    //save the video
    while(count <  pSampleRegionContext->mConfigPara.encoder_count) {

        count++;
        if(AW_MPI_VENC_GetStream(pSampleRegionContext->mVEChn, &VencFrame, 4000) < 0) {
            aloge("get first frame failed!\n");
            continue;
        } else {
            if(VencFrame.mpPack != NULL && VencFrame.mpPack->mLen0) {
                fwrite(VencFrame.mpPack->mpAddr0, 1, VencFrame.mpPack->mLen0, OutputFile_Fd);
            }
            if(VencFrame.mpPack != NULL && VencFrame.mpPack->mLen1) {
                fwrite(VencFrame.mpPack->mpAddr1, 1, VencFrame.mpPack->mLen1, OutputFile_Fd);
            }

            AW_MPI_VENC_ReleaseStream(pSampleRegionContext->mVEChn, &VencFrame);

        }
    }

    AW_MPI_VENC_StopRecvPic(pSampleRegionContext->mVEChn);
    AW_MPI_VI_DisableVirChn(pSampleRegionContext->mVIDev, pSampleRegionContext->mVIChn + 1);

    AW_MPI_VENC_ResetChn(pSampleRegionContext->mVEChn);
    AW_MPI_VENC_DestroyChn(pSampleRegionContext->mVEChn);
    AW_MPI_VI_DestoryVirChn(pSampleRegionContext->mVIDev, pSampleRegionContext->mVIChn + 1);

    return NULL;
}


int
main(int argc, char **argv)
{
    int result = 0;
    BOOL AddVencChannel = FALSE;
    ERRORTYPE eRet = SUCCESS;

    SampleRegionContext stContext;
    initSampleRegionContext(&stContext);
    pSampleRegionContext = &stContext;

    if(ParseCmdLine(argc, argv, &stContext.mCmdLinePara) != 0) {
        result = -1;
        goto _exit;

    }
    char *pConfigFilePath = NULL;
    if(strlen(stContext.mCmdLinePara.mConfigFilePath) > 0) {
        pConfigFilePath = stContext.mCmdLinePara.mConfigFilePath;
    }
    if(loadSampleRegionConfig(&stContext.mConfigPara, pConfigFilePath) != SUCCESS) {
        aloge("fatal error! no config file or parse conf file fail");
        result = -1;
        goto _exit;
    }

    alogw("=============================================>");
    AddVencChannel = stContext.mConfigPara.add_venc_channel;
    if(AddVencChannel) {
        alogw("Had add the VencChanel, have two channel:VI_To_VO and VI_To_Venc");
    } else {
        alogw("Only have the VI_To_VO Channel");
    }

    alogw("=============================================>");

    if (signal(SIGINT, handle_exit) == SIG_ERR) {
        perror("error:can't catch SIGSEGV");
    }

    memset(&stContext.mSysConf, 0, sizeof(MPP_SYS_CONF_S));
    stContext.mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&stContext.mSysConf);
    AW_MPI_SYS_Init();


    stContext.mVIDev = 3;
    stContext.mVIChn = 0;
    eRet = AW_MPI_VI_CreateVipp(stContext.mVIDev);
    if(eRet != SUCCESS) {
        aloge("error:AW_MPI_VI_CreateVipp failed");
    }

    VI_ATTR_S attr;
    eRet = AW_MPI_VI_GetVippAttr(stContext.mVIDev, &attr);
    if(eRet != SUCCESS) {
        aloge("error:AW_MPI_VI_GetVippAttr failed");
    }
    memset(&attr, 0, sizeof(VI_ATTR_S));
    attr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    attr.memtype = V4L2_MEMORY_MMAP;
    attr.format.pixelformat = map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(stContext.mConfigPara.mPicFormat);
    attr.format.field = V4L2_FIELD_NONE;
    attr.format.width = stContext.mConfigPara.mCaptureWidth;
    attr.format.height = stContext.mConfigPara.mCaptureHeight;
    attr.nbufs = 10;
    attr .nplanes = 2;
    attr.fps = 30;
    eRet = AW_MPI_VI_SetVippAttr(stContext.mVIDev, &attr);
    if(eRet != SUCCESS) {
        aloge("error:AW_MPI_SetVippAttr failed");
    }

    AW_MPI_ISP_SetMirror(0, 0);
    eRet = AW_MPI_VI_EnableVipp(stContext.mVIDev);
    if(eRet != SUCCESS) {
        aloge("error:AW_MPI_VI_EnableVipp failed");
    }

    AW_MPI_ISP_Init();
    AW_MPI_ISP_Run(0);

    eRet = AW_MPI_VI_CreateVirChn(stContext.mVIDev, stContext.mVIChn, NULL);
    if(eRet != SUCCESS) {
        aloge("error:AW_MPI_VI_CreateVirChn failed ");
    }


    //enable vo dev
    stContext.mVoDev = 0;
    AW_MPI_VO_Enable(stContext.mVoDev);
    AW_MPI_VO_AddOutsideVideoLayer(stContext.mUILayer);
    AW_MPI_VO_CloseVideoLayer(stContext.mUILayer);

    //enable vo layer
    int hlay0 = 0;
    while(hlay0 < VO_MAX_LAYER_NUM) {
        if(SUCCESS == AW_MPI_VO_EnableVideoLayer(hlay0)) {
            break;
        }
        ++hlay0;
    }
    if(hlay0 == VO_MAX_LAYER_NUM) {
        aloge("error: enable video layer failed");
    }
    stContext.mVoLayer = hlay0;
    AW_MPI_VO_GetVideoLayerAttr(stContext.mVoLayer, &stContext.mLayerAttr);
    stContext.mLayerAttr.stDispRect.X = 0;
    stContext.mLayerAttr.stDispRect.Y = 0;
    stContext.mLayerAttr.stDispRect.Width = 640;
    stContext.mLayerAttr.stDispRect.Height = 480;
    AW_MPI_VO_SetVideoLayerAttr(stContext.mVoLayer, &stContext.mLayerAttr);

    //creat vo channel and clock channel
    BOOL bSuccessFlag = FALSE;
    stContext.mVOChn = 0;
    while(stContext.mVOChn < VO_MAX_CHN_NUM) {
        eRet = AW_MPI_VO_EnableChn(stContext.mVoLayer, stContext.mVOChn);
        if(eRet == SUCCESS) {
            bSuccessFlag = TRUE;
            break;
        } else if(eRet == ERR_VO_CHN_NOT_DISABLE) {
            alogd("vo channel[%d] is exist, find next", stContext.mVOChn);
            stContext.mVOChn++;
        } else {
            aloge("error:create vo channel[%d] ret[0x%x]", stContext.mVOChn, eRet);
        }
    }
    if(FALSE == bSuccessFlag) {
        stContext.mVOChn = MM_INVALID_CHN;
        aloge("error: create vo channel failed");
    }


    MPPCallbackInfo cbInfo;
    cbInfo.cookie = (void *)&stContext;
    cbInfo.callback = (MPPCallbackFuncType)&SampleRegion_VOCallbackWrapper;
    AW_MPI_VO_RegisterCallback(stContext.mVoLayer, stContext.mVOChn, &cbInfo);
    AW_MPI_VO_SetChnDispBufNum(stContext.mVoLayer, stContext.mVOChn, 0);


    //creat the venc
    {
        stContext.mVEChn = 0;
        int VIFrameRate = stContext.mConfigPara.mFrameRate;
        int VencFrameRate = stContext.mConfigPara.mFrameRate;
        VENC_CHN_ATTR_S mVEncChnAttr;
        memset(&mVEncChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
        SIZE_S wantedVideoSize = {stContext.mConfigPara.mCaptureWidth, stContext.mConfigPara.mCaptureHeight};
        SIZE_S videosize = {stContext.mConfigPara.mCaptureWidth, stContext.mConfigPara.mCaptureHeight};
        PAYLOAD_TYPE_E videoCodec = stContext.mConfigPara.EncoderType;
        PIXEL_FORMAT_E wantedPreviewFormat = stContext.mConfigPara.mPicFormat;
        int wantedFrameRate = stContext.mConfigPara.mFrameRate;
        mVEncChnAttr.VeAttr.Type = videoCodec;
        mVEncChnAttr.VeAttr.SrcPicWidth = videosize.Width;
        mVEncChnAttr.VeAttr.SrcPicHeight = videosize.Height;
        mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
        mVEncChnAttr.VeAttr.PixelFormat = wantedPreviewFormat;
        int wantedVideoBitRate = stContext.mConfigPara.bit_rate;
        if(PT_H264 == mVEncChnAttr.VeAttr.Type) {
            mVEncChnAttr.VeAttr.AttrH264e.bByFrame = TRUE;
            mVEncChnAttr.VeAttr.AttrH264e.Profile = 2;
            mVEncChnAttr.VeAttr.AttrH264e.PicWidth = wantedVideoSize.Width;
            mVEncChnAttr.VeAttr.AttrH264e.PicHeight = wantedVideoSize.Height;
            mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264CBR;
            mVEncChnAttr.RcAttr.mAttrH264Cbr.mSrcFrmRate = wantedFrameRate;
            mVEncChnAttr.RcAttr.mAttrH264Cbr.fr32DstFrmRate = wantedFrameRate;
            mVEncChnAttr.RcAttr.mAttrH264Cbr.mBitRate = wantedVideoBitRate;

        } else if(PT_H265 == mVEncChnAttr.VeAttr.Type) {
            mVEncChnAttr.VeAttr.AttrH265e.mbByFrame = TRUE;
            mVEncChnAttr.VeAttr.AttrH265e.mPicWidth = wantedVideoSize.Width;
            mVEncChnAttr.VeAttr.AttrH265e.mPicHeight = wantedVideoSize.Height;
            mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265CBR;
            mVEncChnAttr.RcAttr.mAttrH265Cbr.mSrcFrmRate = wantedFrameRate;
            mVEncChnAttr.RcAttr.mAttrH265Cbr.fr32DstFrmRate = wantedFrameRate;
            mVEncChnAttr.RcAttr.mAttrH265Cbr.mBitRate = wantedVideoBitRate;
        } else if(PT_MJPEG== mVEncChnAttr.VeAttr.Type) {
            mVEncChnAttr.VeAttr.AttrMjpeg.mbByFrame = TRUE;
            mVEncChnAttr.VeAttr.AttrMjpeg.mPicWidth = wantedVideoSize.Width;
            mVEncChnAttr.VeAttr.AttrMjpeg.mPicHeight = wantedVideoSize.Height;
            mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mSrcFrmRate = wantedFrameRate;
            mVEncChnAttr.RcAttr.mAttrMjpegeCbr.fr32DstFrmRate = wantedFrameRate;
            mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mBitRate = wantedVideoBitRate;
        }

        eRet =  AW_MPI_VENC_CreateChn(stContext.mVEChn, &mVEncChnAttr);
        if(eRet != SUCCESS) {
            aloge("error:AW_MPI_VENC_CreateChn failed");
            goto _exit;

        }

        VENC_FRAME_RATE_S stFrameRate;
        stFrameRate.SrcFrmRate = VIFrameRate;
        stFrameRate.DstFrmRate = VencFrameRate;
        AW_MPI_VENC_SetFrameRate(stContext.mVEChn, &stFrameRate);

        VencHeaderData vencheader;
        OutputFile_Fd = fopen(stContext.mConfigPara.OutputFilePath,"wb+");
        if(!OutputFile_Fd) {
            aloge("error:can not open the file");
            goto _exit;
        }
        if(PT_H264 == mVEncChnAttr.VeAttr.Type) {
            AW_MPI_VENC_GetH264SpsPpsInfo(stContext.mVEChn, &vencheader);
            if(vencheader.nLength) {
                fwrite(vencheader.pBuffer, vencheader.nLength, 1, OutputFile_Fd);
            }
        } else if(PT_H265 == mVEncChnAttr.VeAttr.Type) {
            AW_MPI_VENC_GetH265SpsPpsInfo(stContext.mVEChn, &vencheader);
            if(vencheader.nLength) {
                fwrite(vencheader.pBuffer, vencheader.nLength, 1, OutputFile_Fd);
            }
        }

    }

    //bind clock,vo, viChn
    MPP_CHN_S VOChn = {MOD_ID_VOU, stContext.mVoLayer, stContext.mVOChn};
    MPP_CHN_S VIChn = {MOD_ID_VIU, stContext.mVIDev, stContext.mVIChn};
    MPP_CHN_S VeChn = {MOD_ID_VENC, 0, stContext.mVEChn};

    AW_MPI_SYS_Bind(&VIChn, &VOChn);

    //start vo, clock, vi_channel andvvenc
    eRet = AW_MPI_VI_EnableVirChn(stContext.mVIDev, stContext.mVIChn);
    if(eRet != SUCCESS) {
        aloge("error:enablecirchn failed");
    }
    AW_MPI_VO_StartChn(stContext.mVoLayer, stContext.mVOChn);

    if(AddVencChannel) {
        pthread_t pthread_id;
        if(pthread_create(&pthread_id , NULL, GetEncoderFrameThread, NULL) < 0) {
            aloge("error: the pthread can not be created");
        }
        alogw("the pthread of venc had created");
    }

    //test the region
    if(stContext.mConfigPara.overlay_h != 0 && stContext.mConfigPara.overlay_w != 0) {
        RGN_ATTR_S stRegion;
        BITMAP_S stBmp;
        int nSize = 0;
        RGN_CHN_ATTR_S stRgnChnAttr ;

        memset(&stRegion, 0, sizeof(RGN_ATTR_S));
        stRegion.enType = OVERLAY_RGN;
        stRegion.unAttr.stOverlay.mPixelFmt = MM_PIXEL_FORMAT_RGB_8888;
        //stRegion.unAttr.stOverlay.mSize = {(unsigned int)stContext.mConfigPara.overlay_w, (unsigned int)stContext.mConfigPara.overlay_h};
        stRegion.unAttr.stOverlay.mSize.Width = (unsigned int)stContext.mConfigPara.overlay_w;
        stRegion.unAttr.stOverlay.mSize.Height = (unsigned int)stContext.mConfigPara.overlay_h;
        stContext.mOverlayHandle = 0;
        AW_MPI_RGN_Create(stContext.mOverlayHandle, &stRegion);

        memset(&stBmp, 0, sizeof(BITMAP_S));
        stBmp.mPixelFormat = stRegion.unAttr.stOverlay.mPixelFmt;
        stBmp.mWidth = stRegion.unAttr.stOverlay.mSize.Width;
        stBmp.mHeight = stRegion.unAttr.stOverlay.mSize.Height;
        nSize = BITMAP_S_GetdataSize(&stBmp);
        stBmp.mpData = malloc(nSize);
        if(NULL == stBmp.mpData) {
            aloge("fatal error! malloc fail!");
        }
        memset(stBmp.mpData, 0xCC, nSize);
        AW_MPI_RGN_SetBitMap(stContext.mOverlayHandle, &stBmp);
        free(stBmp.mpData);

        stRgnChnAttr.bShow = TRUE;
        stRgnChnAttr.enType = stRegion.enType;
        //stRgnChnAttr.unChnAttr.stOverlayChn.stPoint = {stContext.mConfigPara.overlay_x, stContext.mConfigPara.overlay_y};
        stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.X = stContext.mConfigPara.overlay_x;
        stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.Y = stContext.mConfigPara.overlay_y;
        stRgnChnAttr.unChnAttr.stOverlayChn.mLayer = 0;
        stRgnChnAttr.unChnAttr.stOverlayChn.mFgAlpha = 0x5C; // global alpha mode value for ARGB1555
        AW_MPI_RGN_AttachToChn(stContext.mOverlayHandle,  &VIChn, &stRgnChnAttr);

        //add to venchn
        memset(&stRegion, 0, sizeof(RGN_ATTR_S));
        stRegion.enType = OVERLAY_RGN;
        stRegion.unAttr.stOverlay.mPixelFmt = MM_PIXEL_FORMAT_RGB_8888;
        //stRegion.unAttr.stOverlay.mSize = {(unsigned int)stContext.mConfigPara.overlay_w, (unsigned int)stContext.mConfigPara.overlay_h};
        stRegion.unAttr.stOverlay.mSize.Width = 16;
        stRegion.unAttr.stOverlay.mSize.Height = 16;
        //stContext.mOverlayHandle += 1;
        AW_MPI_RGN_Create(stContext.mOverlayHandle + 1, &stRegion);

        memset(&stBmp, 0, sizeof(BITMAP_S));
        stBmp.mPixelFormat = stRegion.unAttr.stOverlay.mPixelFmt;
        stBmp.mWidth = stRegion.unAttr.stOverlay.mSize.Width;
        stBmp.mHeight = stRegion.unAttr.stOverlay.mSize.Height;
        nSize = BITMAP_S_GetdataSize(&stBmp);
        stBmp.mpData = malloc(nSize);
        if(NULL == stBmp.mpData) {
            aloge("fatal error! malloc fail!");
        }
        memset(stBmp.mpData, 0xCC, nSize);
        AW_MPI_RGN_SetBitMap(stContext.mOverlayHandle + 1, &stBmp);
        free(stBmp.mpData);

        stRgnChnAttr.bShow = TRUE;
        stRgnChnAttr.enType = stRegion.enType;
        //stRgnChnAttr.unChnAttr.stOverlayChn.stPoint = {stContext.mConfigPara.overlay_x, stContext.mConfigPara.overlay_y};
        stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.X = 320;
        stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.Y = 320;
        stRgnChnAttr.unChnAttr.stOverlayChn.mLayer = 0;
        stRgnChnAttr.unChnAttr.stOverlayChn.mFgAlpha = 0x5C; // global alpha mode value for ARGB1555
        AW_MPI_RGN_AttachToChn(stContext.mOverlayHandle + 1,  &VeChn, &stRgnChnAttr);

    }

    if(stContext.mConfigPara.cover_h != 0 && stContext.mConfigPara.cover_w != 0) {
        //add to vichn
        RGN_ATTR_S stRegion;
        RGN_CHN_ATTR_S stRgnChnAttr;

        memset(&stRegion, 0, sizeof(RGN_ATTR_S));
        stRegion.enType = COVER_RGN;
        stContext.mCoverHandle = 10;
        AW_MPI_RGN_Create(stContext.mCoverHandle, &stRegion);

        stRgnChnAttr.bShow = TRUE;
        stRgnChnAttr.enType = stRegion.enType;
        stRgnChnAttr.unChnAttr.stCoverChn.enCoverType = AREA_RECT;
        //stRgnChnAttr.unChnAttr.stCoverChn.stRect = {stContext.mConfigPara.cover_x, stContext.mConfigPara.cover_y, (unsigned int)stContext.mConfigPara.cover_w, (unsigned int)stContext.mConfigPara.cover_h};
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.X = stContext.mConfigPara.cover_x;
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.Y = stContext.mConfigPara.cover_y;
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.Width = stContext.mConfigPara.cover_w;
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.Height= stContext.mConfigPara.cover_h;
        stRgnChnAttr.unChnAttr.stCoverChn.mColor = 0;
        stRgnChnAttr.unChnAttr.stCoverChn.mLayer = 0;
        AW_MPI_RGN_AttachToChn(stContext.mCoverHandle,  &VIChn, &stRgnChnAttr);

        //add to venc
        memset(&stRegion, 0, sizeof(RGN_ATTR_S));
        stRegion.enType = COVER_RGN;
        // stContext.mCoverHandle += 1;
        AW_MPI_RGN_Create(stContext.mCoverHandle + 1, &stRegion);

        stRgnChnAttr.bShow = TRUE;
        stRgnChnAttr.enType = stRegion.enType;
        stRgnChnAttr.unChnAttr.stCoverChn.enCoverType = AREA_RECT;
        //stRgnChnAttr.unChnAttr.stCoverChn.stRect = {stContext.mConfigPara.cover_x, stContext.mConfigPara.cover_y, (unsigned int)stContext.mConfigPara.cover_w, (unsigned int)stContext.mConfigPara.cover_h};
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.X = 16;
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.Y = 16;
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.Width = 480;
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.Height= 480;
        stRgnChnAttr.unChnAttr.stCoverChn.mColor = 255;
        stRgnChnAttr.unChnAttr.stCoverChn.mLayer = 0;
        AW_MPI_RGN_AttachToChn(stContext.mCoverHandle + 1,  &VeChn, &stRgnChnAttr);

    }

    sleep(5);
    alogw("now, let us change the displayattr");
    {
        RGN_CHN_ATTR_S stRgnChnAttr;
        //change the overlay
        AW_MPI_RGN_GetDisplayAttr(stContext.mOverlayHandle,  &VIChn, &stRgnChnAttr);
        stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.X += 50;
        stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.Y = 0;
        AW_MPI_RGN_SetDisplayAttr(stContext.mOverlayHandle, &VIChn, &stRgnChnAttr);

        AW_MPI_RGN_GetDisplayAttr(stContext.mOverlayHandle + 1,  &VeChn, &stRgnChnAttr);
        stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.X = 400;
        stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.Y = 400;
        AW_MPI_RGN_SetDisplayAttr(stContext.mOverlayHandle + 1, &VeChn, &stRgnChnAttr);

        //change the cover
        AW_MPI_RGN_GetDisplayAttr(stContext.mCoverHandle, &VIChn, &stRgnChnAttr);
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.X = 0;
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.Y = 0;
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.Width += 50;
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.Height += 50;
        AW_MPI_RGN_SetDisplayAttr(stContext.mCoverHandle, &VIChn, &stRgnChnAttr);

        AW_MPI_RGN_GetDisplayAttr(stContext.mCoverHandle + 1, &VeChn, &stRgnChnAttr);
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.X = 800;
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.Y = 800;
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.Width += 16;
        stRgnChnAttr.unChnAttr.stCoverChn.stRect.Height += 16;
        AW_MPI_RGN_SetDisplayAttr(stContext.mCoverHandle + 1, &VeChn, &stRgnChnAttr);

    }

    if(stContext.mConfigPara.mTestDuration > 0) {
        cdx_sem_down_timedwait(&stContext.mSemExit, stContext.mConfigPara.mTestDuration * 1000);
    } else {
        cdx_sem_down(&stContext.mSemExit);
    }

    AW_MPI_RGN_DetachFromChn(stContext.mOverlayHandle, &VIChn);
    AW_MPI_RGN_Destroy(stContext.mOverlayHandle);

    AW_MPI_RGN_DetachFromChn(stContext.mCoverHandle, &VIChn);
    AW_MPI_RGN_Destroy(stContext.mCoverHandle);

    AW_MPI_RGN_DetachFromChn(stContext.mOverlayHandle + 1, &VeChn);
    AW_MPI_RGN_Destroy(stContext.mOverlayHandle + 1);

    AW_MPI_RGN_DetachFromChn(stContext.mCoverHandle + 1, &VeChn);
    AW_MPI_RGN_Destroy(stContext.mCoverHandle + 1);

_exit:
    AW_MPI_ISP_Stop(0);
    AW_MPI_ISP_Exit();

    //stop vo channel, clock channel, vi channel,
    AW_MPI_VENC_StopRecvPic(stContext.mVEChn);
    AW_MPI_VO_StopChn(stContext.mVoLayer, stContext.mVOChn);
    AW_MPI_VI_DisableVirChn(stContext.mVIDev, stContext.mVIChn);
    AW_MPI_VI_DisableVirChn(stContext.mVIDev, stContext.mVIChn + 1);
    AW_MPI_VENC_ResetChn(stContext.mVEChn);
    AW_MPI_VO_DisableChn(stContext.mVoLayer, stContext.mVOChn);

    stContext.mVOChn = MM_INVALID_CHN;
    stContext.mClockChn = MM_INVALID_CHN;
    AW_MPI_VENC_DestroyChn(stContext.mVEChn);
    AW_MPI_VI_DestoryVirChn(stContext.mVIDev, stContext.mVIChn);
    AW_MPI_VI_DestoryVirChn(stContext.mVIDev, stContext.mVIChn + 1);
    AW_MPI_VI_DisableVipp(stContext.mVIDev);
    AW_MPI_VI_DestoryVipp(stContext.mVIDev);
    stContext.mVIDev = MM_INVALID_DEV;
    stContext.mVIChn = MM_INVALID_CHN;
    stContext.mVEChn = MM_INVALID_CHN;

    //disable vo layer
    AW_MPI_VO_DisableVideoLayer(stContext.mVoLayer);
    stContext.mVoLayer = -1;
    AW_MPI_VO_RemoveOutsideVideoLayer(stContext.mUILayer);
    //disalbe vo dev
    AW_MPI_VO_Disable(stContext.mVoDev);
    stContext.mVoDev = -1;
    //exit mpp system
    AW_MPI_SYS_Exit();
    fclose(OutputFile_Fd);

    destroySampleRegionContext(&stContext);
    if (result == 0) {
        printf("sample_region exit!\n");
    }
    return result;
}


