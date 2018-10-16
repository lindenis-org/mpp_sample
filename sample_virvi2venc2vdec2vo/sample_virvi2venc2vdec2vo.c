/******************************************************************************
  Copyright (C), 2001-2017, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : sample_virvi2venc2vdec2vo.c
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2017/1/5
  Last Modified :
  Description   : mpp component implement
  Function List :
  History       :
******************************************************************************/

//#define LOG_NDEBUG 0
#define LOG_TAG "sample_virvi2venc2vdec2vo"
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
#include <assert.h>

#include "media/mm_comm_vi.h"
#include "media/mpi_vi.h"
#include "media/mpi_isp.h"
#include "media/mpi_venc.h"
#include "media/mpi_vdec.h"
#include "media/mpi_sys.h"
#include "mm_common.h"
#include "mm_comm_venc.h"
#include "mm_comm_rc.h"
#include "mpi_clock.h"
#include "hwdisplay.h"
#include "sunxi_display2.h"

#include <confparser.h>
#include "sample_virvi2venc2vdec2vo.h"
#include "sample_virvi2venc2vdec2vo_config.h"

#define DEBUG_SAMPLE_SAVE_BIT_STREAM (1)

char *p_header;
int header_length = 0;

static int ParseCmdLine(int argc, char **argv, SampleVirvi2Venc2Vdec2VOCmdLineParam *pCmdLinePara)
{
    alogd("sample virvi2venc path:[%s], arg number is [%d]", argv[0], argc);
    ERRORTYPE ret = 0;
    int i=1;
    memset(pCmdLinePara, 0, sizeof(SampleVirvi2Venc2Vdec2VOCmdLineParam));
    while(i < argc) {
        if(!strcmp(argv[i], "-path")) {
            if(++i >= argc) {
                aloge("fatal error! use -h to learn how to set parameter!!!");
                ret = FAILURE;
                break;
            }
            if(strlen(argv[i]) >= MAX_FILE_PATH_SIZE) {
                aloge("fatal error! file path[%s] too long: [%d]>=[%d]!", argv[i], strlen(argv[i]), MAX_FILE_PATH_SIZE);
            }
            strncpy(pCmdLinePara->mConfigFilePath, argv[i], MAX_FILE_PATH_SIZE-1);
            pCmdLinePara->mConfigFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
        } else if(!strcmp(argv[i], "-h")) {
            alogd("CmdLine param:\n"
                  "\t-path /home/sample_virvi2venc2vdec2vo.conf\n");
            ret = 1;
            break;
        } else {
            alogd("ignore invalid CmdLine param:[%s], type -h to get how to set parameter!", argv[i]);
        }
        i++;
    }
    return SUCCESS;
}

static ERRORTYPE LoadSampleVirvi2Venc2Vdec2VOConfig(SampleVirvi2Venc2Vdec2VOConfig *pConfig, const char *conf_path)
{
    ERRORTYPE ret;
    char *ptr;
    CONFPARSER_S stConfParser;
    ret = createConfParser(conf_path, &stConfParser);
    if(ret < 0) {
        aloge("load conf fail");
        return FAILURE;
    }
    memset(pConfig, 0, sizeof(SampleVirvi2Venc2Vdec2VOConfig));

    pConfig->EncoderCount = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Encoder_Count, 0);
    pConfig->DevNum = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Dev_Num, 0);
    pConfig->SrcFrameRate = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Src_Frame_Rate, 0);
    pConfig->SrcWidth = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Src_Width, 0);
    pConfig->SrcHeight = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Src_Height, 0);
    alogd("dev_num=%d, src_width=%d, src_height=%d, src_frame_rate=%d",
          pConfig->DevNum ,pConfig->SrcWidth ,pConfig->SrcHeight ,pConfig->SrcFrameRate );

    char *EncoderType = (char*)GetConfParaString(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Dest_Encoder_Type, NULL);
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
    pConfig->DestWidth = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Dest_Width, 0);
    pConfig->DestHeight = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Dest_Height, 0);
    pConfig->DestFrameRate = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Dest_Frame_Rate, 0);
    pConfig->DestBitRate= GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Dest_Bit_Rate, 0);
    char *pStrPixelFormat = (char*)GetConfParaString(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Dest_Format, NULL);
    if(!strcmp(pStrPixelFormat, "nv21")) {
        pConfig->DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    } else {
        aloge("fatal error! conf file pic_format must be yuv420sp");
        pConfig->DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }
    alogd("dest_width=%d, dest_height=%d, dest_frame_rate=%d, dest_bit_rate=%d",
          pConfig->DestWidth ,pConfig->DestHeight ,pConfig->DestFrameRate ,pConfig->DestBitRate);

    pConfig->mDisplayWidth  = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Display_Width, 0);
    pConfig->mDisplayHeight = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Display_Height, 0);
    char *pStrDispType = (char*)GetConfParaString(&stConfParser, SAMPLE_Virvi2Venc2Vdec2Vo_Disp_Type, NULL);
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
    alogd("pConfig->mDisplayWidth=[%d], pConfig->mDisplayHeight=[%d], pConfig->mDispSync=[%d], pConfig->mDispType=[%d]",
          pConfig->mDisplayWidth, pConfig->mDisplayHeight, pConfig->mDispSync, pConfig->mDispType);
    destroyConfParser(&stConfParser);

    return SUCCESS;
}

static ERRORTYPE MPPCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    VI2Venc2Vdec2VO_Cap_S *pCap = (VI2Venc2Vdec2VO_Cap_S *)cookie;
    alogd("MPPCallbackWrapper: mModId %d %d",pChn->mModId, event);
    if (pChn->mModId == MOD_ID_VDEC) {
        switch (event) {
        case MPP_EVENT_NOTIFY_EOF:
            alogd("vdec to the end of file");
            if (pCap->mVoChn >= 0) {
                AW_MPI_VO_SetStreamEof(pCap->mVoLayer, pCap->mVoChn, 1);
            }
            //cdx_sem_up(&pCap->mSemExit);
            break;

        default:
            break;
        }
    } else if (pChn->mModId == MOD_ID_VOU) {
        switch (event) {
        case MPP_EVENT_RELEASE_VIDEO_BUFFER: {
            //通知释放buffer
            VENC_STREAM_S *VencFrame = (VENC_STREAM_S *)pEventData;
            alogd("release venc buffer afer vo!!!");
            AW_MPI_VENC_ReleaseStream(pCap->mVenChn, VencFrame);
            break;
        }
        case MPP_EVENT_NOTIFY_EOF:
            alogd("vo to the end of file");
            cdx_sem_up(&pCap->mSemExit);
            break;

        case MPP_EVENT_RENDERING_START:
            alogd("vo start to rendering");
            break;

        default:
            break;
        }
    }

    return SUCCESS;
}

static ERRORTYPE Parse_Config(int argc, char *argv[], VI2Venc2Vdec2VO_Cap_S *pCap)
{
    SampleVirvi2Venc2Vdec2VOCmdLineParam *mCmdLinePara = &pCap->mCmdLinePara;
    SampleVirvi2Venc2Vdec2VOConfig *mConfigPara = &pCap->mConfigPara;

    /* parse command line param,read sample_virvi2venc2vdec2vo.conf */
    if(ParseCmdLine(argc, argv, mCmdLinePara) != 0) {
        aloge("fatal error! command line param is wrong, exit!");
        return FAILURE;
    }
    char *pConfigFilePath;
    if(strlen(mCmdLinePara->mConfigFilePath) > 0) {
        pConfigFilePath = mCmdLinePara->mConfigFilePath;
    } else {
        pConfigFilePath = DEFAULT_SAMPLE_VIPP2VENC2VDEC2VO_CONF_PATH;
    }
    /* parse config file. */
    if(LoadSampleVirvi2Venc2Vdec2VOConfig(mConfigPara, pConfigFilePath) != SUCCESS) {
        aloge("fatal error! no config file or parse conf file fail");
        return FAILURE;
    }

    return SUCCESS;
}

static ERRORTYPE Vi_Component_Init(VI2Venc2Vdec2VO_Cap_S *pCap)
{
    SampleVirvi2Venc2Vdec2VOConfig *mConfigPara = &pCap->mConfigPara;
    VI_ATTR_S stAttr;
    ERRORTYPE ret;

    pCap->mViDev = mConfigPara->DevNum;
    pCap->mViChn = 0;

    /*Set VI Channel Attribute*/
    stAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    stAttr.memtype = V4L2_MEMORY_MMAP;
    stAttr.format.pixelformat = V4L2_PIX_FMT_NV21M;
    stAttr.format.field = V4L2_FIELD_NONE;
    stAttr.format.width = mConfigPara->SrcWidth;
    stAttr.format.height = mConfigPara->SrcHeight;
    stAttr.nbufs = 10;
    stAttr.nplanes = 2;
    stAttr.use_current_win = 0;
    stAttr.fps = mConfigPara->SrcFrameRate;
    stAttr.capturemode = V4L2_MODE_VIDEO;
    stAttr.use_current_win = 0;
    stAttr.wdr_mode = 0;
    ret = AW_MPI_VI_CreateVipp(pCap->mViDev);
    if (ret < 0) {
        aloge("Create vi dev[%d] falied!", pCap->mViDev);
        return ret;
    }
    ret = AW_MPI_VI_SetVippAttr(pCap->mViDev, &stAttr);
    if (ret < 0) {
        aloge("Set vi attr[%d] falied!", pCap->mViDev);
        return ret;
    }
    ret = AW_MPI_VI_EnableVipp(pCap->mViDev);
    if (ret < 0) {
        aloge("Enable vi dev[%d] falied!", pCap->mViDev);
        return ret;
    }

    AW_MPI_ISP_Init();
    if (pCap->mViDev == 0 || pCap->mViDev == 2)
        pCap->mIspChn = 1;
    if (pCap->mViDev == 1 || pCap->mViDev == 3)
        pCap->mIspChn = 0;
    AW_MPI_ISP_Run(pCap->mIspChn); // 3A ini

    ret = AW_MPI_VI_CreateVirChn(pCap->mViDev, pCap->mViChn, NULL);
    if (ret < 0) {
        aloge("Create VI Chn failed,VIDev = %d,VIChn = %d",pCap->mViDev,pCap->mViChn);
        return ret ;
    }
    ret = AW_MPI_VI_SetVirChnAttr(pCap->mViDev, pCap->mViChn, NULL);
    if (ret < 0) {
        aloge("Set VI ChnAttr failed,VIDev = %d,VIChn = %d",pCap->mViDev,pCap->mViChn);
        return ret ;
    }

    return SUCCESS;
}

static ERRORTYPE Vi_Component_Exit(VI2Venc2Vdec2VO_Cap_S *pCap)
{
    ERRORTYPE ret;
    int isp_dev;
#if 0
    /* better be invoked after AW_MPI_VENC_StopRecvPic */
    ret = AW_MPI_VI_DisableVirChn(pCap->mViDev, pCap->mViChn);
    if(ret < 0) {
        aloge("Disable VI Chn failed,VIDev = %d,VIChn = %d",pCap->mViDev,pCap->mViChn);
        return ret ;
    }
#endif
    ret = AW_MPI_VI_DestoryVirChn(pCap->mViDev, pCap->mViChn);
    if(ret < 0) {
        aloge("Destory VI Chn failed,VIDev = %d,VIChn = %d",pCap->mViDev,pCap->mViChn);
        return ret ;
    }

    AW_MPI_ISP_Stop(pCap->mIspChn);
    AW_MPI_ISP_Exit();

    ret = AW_MPI_VI_DisableVipp(pCap->mViDev);
    if(ret < 0) {
        aloge("Disable VI Dev failed,VIDev = %d",pCap->mViDev);
        return ret ;
    }
    ret = AW_MPI_VI_DestoryVipp(pCap->mViDev);
    if(ret < 0) {
        aloge("Destory VI Dev failed,VIDev = %d",pCap->mViDev);
        return ret ;
    }

    return SUCCESS;
}

static ERRORTYPE Venc_Component_Init(VI2Venc2Vdec2VO_Cap_S *pCap)
{
    SampleVirvi2Venc2Vdec2VOConfig *mConfigPara = &pCap->mConfigPara;
    int result;
    pCap->mVenChn = 0;

    /* venc chn attr */
    int VIFrameRate = mConfigPara->SrcFrameRate;
    int VencFrameRate = mConfigPara->DestFrameRate;
    VENC_CHN_ATTR_S mVEncChnAttr;
    memset(&mVEncChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
    SIZE_S wantedVideoSize = {mConfigPara->DestWidth, mConfigPara->DestHeight};
    SIZE_S videosize = {mConfigPara->SrcWidth, mConfigPara->SrcHeight};
    PAYLOAD_TYPE_E videoCodec = mConfigPara->EncoderType;
    PIXEL_FORMAT_E wantedPreviewFormat = mConfigPara->DestPicFormat;
    int wantedFrameRate = mConfigPara->DestFrameRate;
    mVEncChnAttr.VeAttr.Type = videoCodec;
    mVEncChnAttr.VeAttr.SrcPicWidth = videosize.Width;
    mVEncChnAttr.VeAttr.SrcPicHeight = videosize.Height;
    mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
    mVEncChnAttr.VeAttr.PixelFormat = wantedPreviewFormat;
    int wantedVideoBitRate = mConfigPara->DestBitRate;
    if(PT_H264 == mVEncChnAttr.VeAttr.Type) {
        mVEncChnAttr.VeAttr.AttrH264e.bByFrame = TRUE;
        mVEncChnAttr.VeAttr.AttrH264e.Profile = 2;
        mVEncChnAttr.VeAttr.AttrH264e.PicWidth = wantedVideoSize.Width;
        mVEncChnAttr.VeAttr.AttrH264e.PicHeight = wantedVideoSize.Height;
        mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264CBR;
        mVEncChnAttr.RcAttr.mAttrH264Cbr.mSrcFrmRate = wantedFrameRate;
        if(mConfigPara->mTimeLapseEnable) {
            mVEncChnAttr.RcAttr.mAttrH264Cbr.fr32DstFrmRate = 1000 + (mConfigPara->mTimeBetweenFrameCapture<<16);
        } else {
            mVEncChnAttr.RcAttr.mAttrH264Cbr.fr32DstFrmRate = wantedFrameRate;
        }
        mVEncChnAttr.RcAttr.mAttrH264Cbr.mBitRate = wantedVideoBitRate;
    } else if(PT_H265 == mVEncChnAttr.VeAttr.Type) {
        mVEncChnAttr.VeAttr.AttrH265e.mbByFrame = TRUE;
        mVEncChnAttr.VeAttr.AttrH265e.mPicWidth = wantedVideoSize.Width;
        mVEncChnAttr.VeAttr.AttrH265e.mPicHeight = wantedVideoSize.Height;
        mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H265CBR;
        mVEncChnAttr.RcAttr.mAttrH265Cbr.mSrcFrmRate = wantedFrameRate;
        mVEncChnAttr.RcAttr.mAttrH265Cbr.fr32DstFrmRate = wantedFrameRate;
        mVEncChnAttr.RcAttr.mAttrH265Cbr.mBitRate = wantedVideoBitRate;
    } else if(PT_MJPEG == mVEncChnAttr.VeAttr.Type) {
        mVEncChnAttr.VeAttr.AttrMjpeg.mbByFrame = TRUE;
        mVEncChnAttr.VeAttr.AttrMjpeg.mPicWidth= videosize.Width;
        mVEncChnAttr.VeAttr.AttrMjpeg.mPicHeight = videosize.Height;
        mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_MJPEGCBR;
        mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mSrcFrmRate = wantedFrameRate;
        mVEncChnAttr.RcAttr.mAttrMjpegeCbr.fr32DstFrmRate = wantedFrameRate;
        mVEncChnAttr.RcAttr.mAttrMjpegeCbr.mBitRate = wantedVideoBitRate;
    }

    result = AW_MPI_VENC_CreateChn(pCap->mVenChn, &mVEncChnAttr);
    if(result < 0) {
        aloge("create venc channel[%d] falied!", pCap->mVenChn);
        return FAILURE;
    }
    VENC_FRAME_RATE_S stFrameRate;
    stFrameRate.SrcFrmRate = VIFrameRate;
    stFrameRate.DstFrmRate = VencFrameRate;
    AW_MPI_VENC_SetFrameRate(pCap->mVenChn, &stFrameRate);

    VencHeaderData vencheader;
    if(PT_H264 == mVEncChnAttr.VeAttr.Type) {
        AW_MPI_VENC_GetH264SpsPpsInfo(pCap->mVenChn, &vencheader);
    } else if(PT_H265 == mVEncChnAttr.VeAttr.Type) {
        AW_MPI_VENC_GetH265SpsPpsInfo(pCap->mVenChn, &vencheader);
    }
#if 0
    alogd("vencheader len: %d\n",vencheader.nLength);
    unsigned char *p = vencheader.pBuffer;
    for(int i = 0; i < vencheader.nLength;) {
        alogd("%02x %02x %02x %02x %02x %02x %02x %02x %02x",p[i],p[i+1],p[i+2],p[i+3],p[i+4],p[i+5],p[i+6],p[i+7]);
        i = i+8;
    }
#endif
    p_header = (char *)malloc(vencheader.nLength);
    memcpy(p_header, vencheader.pBuffer, vencheader.nLength);
    header_length = vencheader.nLength;

    pCap->EncoderType = mVEncChnAttr.VeAttr.Type;

    return SUCCESS;
}

static ERRORTYPE Venc_Component_Exit(VI2Venc2Vdec2VO_Cap_S *pCap)
{
    int result;

    AW_MPI_VENC_ResetChn(pCap->mVenChn);
    AW_MPI_VENC_DestroyChn(pCap->mVenChn);

    return SUCCESS;
}

static ERRORTYPE createClockChn(VI2Venc2Vdec2VO_Cap_S *pCap)
{
    ERRORTYPE ret;
    BOOL bSuccessFlag = FALSE;

    pCap->mClockChn = 0;
    while (pCap->mClockChn < CLOCK_MAX_CHN_NUM) {
        ret = AW_MPI_CLOCK_CreateChn(pCap->mClockChn, &pCap->mClockChnAttr);
        if (SUCCESS == ret) {
            bSuccessFlag = TRUE;
            alogd("create clock channel[%d] success!", pCap->mClockChn);
            break;
        } else if (ERR_CLOCK_EXIST == ret) {
            alogd("clock channel[%d] is exist, find next!", pCap->mClockChn);
            pCap->mClockChn++;
        } else {
            alogd("create clock channel[%d] ret[0x%x]!", pCap->mClockChn, ret);
            break;
        }
    }

    if (FALSE == bSuccessFlag) {
        pCap->mClockChn = MM_INVALID_CHN;
        aloge("fatal error! create clock channel fail!");
        return FAILURE;
    } else {
        return SUCCESS;
    }
}

static ERRORTYPE ConfigVdecChnAttr(VI2Venc2Vdec2VO_Cap_S *pCap)
{
    memset(&pCap->mVdecChnAttr, 0, sizeof(VDEC_CHN_ATTR_S));
    pCap->mVdecChnAttr.mPicWidth = pCap->mConfigPara.mMaxVdecOutputWidth;
    pCap->mVdecChnAttr.mPicHeight = pCap->mConfigPara.mMaxVdecOutputHeight;
    pCap->mVdecChnAttr.mInitRotation = pCap->mConfigPara.mInitRotation;
    pCap->mVdecChnAttr.mOutputPixelFormat = pCap->mConfigPara.mUserSetPixelFormat;
    pCap->mVdecChnAttr.mType = pCap->EncoderType;
    pCap->mVdecChnAttr.mVdecVideoAttr.mSupportBFrame = 0; //1
    pCap->mVdecChnAttr.mVdecVideoAttr.mMode = VIDEO_MODE_FRAME;

    return SUCCESS;
}

static ERRORTYPE createVdecChn(VI2Venc2Vdec2VO_Cap_S *pCap)
{
    ERRORTYPE ret;
    BOOL nSuccessFlag = FALSE;
    pCap->mVdecChn = 0;

    ConfigVdecChnAttr(pCap);

    while (pCap->mVdecChn < VDEC_MAX_CHN_NUM) {
        ret = AW_MPI_VDEC_CreateChn(pCap->mVdecChn, &pCap->mVdecChnAttr);
        if (SUCCESS == ret) {
            nSuccessFlag = TRUE;
            alogd("create vdec channel[%d] success!", pCap->mVdecChn);
            break;
        } else if (ERR_VDEC_EXIST == ret) {
            alogd("vdec channel[%d] is exist, find next!", pCap->mVdecChn);
            pCap->mVdecChn++;
        } else {
            alogd("create vdec channel[%d] ret[0x%x]!", pCap->mVdecChn, ret);
            break;
        }
    }

    if (FALSE == nSuccessFlag) {
        pCap->mVdecChn = MM_INVALID_CHN;
        aloge("fatal error! create vdec channel fail!");
        return FAILURE;
    } else {
        alogd("add call back");
        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pCap;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VDEC_RegisterCallback(pCap->mVdecChn, &cbInfo);
        return SUCCESS;
    }
    return SUCCESS;
}

static ERRORTYPE createVoChn(VI2Venc2Vdec2VO_Cap_S *pCap)
{
    ERRORTYPE ret;
    BOOL nSuccessFlag = FALSE;
    SampleVirvi2Venc2Vdec2VOConfig *mConfigPara = &pCap->mConfigPara;

    pCap->mVoDev = 0;
    pCap->mUILayer = HLAY(2, 0);

    AW_MPI_VO_Enable(pCap->mVoDev);
    AW_MPI_VO_AddOutsideVideoLayer(pCap->mUILayer);
    AW_MPI_VO_CloseVideoLayer(pCap->mUILayer);//close ui layer.

    //enable vo layer
    int hlay0 = 0;
    while (hlay0 < VO_MAX_LAYER_NUM) {
        if (SUCCESS == AW_MPI_VO_EnableVideoLayer(hlay0)) {
            break;
        }
        hlay0++;
    }

    if (hlay0 >= VO_MAX_LAYER_NUM) {
        aloge("fatal error! enable video layer fail!");
        pCap->mVoLayer = MM_INVALID_DEV;
        AW_MPI_VO_RemoveOutsideVideoLayer(pCap->mUILayer);
        AW_MPI_VO_Disable(pCap->mVoDev);
        return FAILURE;
    }

#if 1
    VO_PUB_ATTR_S spPubAttr;
    AW_MPI_VO_GetPubAttr(pCap->mVoDev, &spPubAttr);
    spPubAttr.enIntfType = mConfigPara->mDispType;
    spPubAttr.enIntfSync = mConfigPara->mDispSync;
    AW_MPI_VO_SetPubAttr(pCap->mVoDev, &spPubAttr);
#endif

    pCap->mVoLayer = hlay0;
    AW_MPI_VO_GetVideoLayerAttr(pCap->mVoLayer, &pCap->mVoLayerAttr);

    pCap->mVoLayerAttr.stDispRect.X = 0;
    pCap->mVoLayerAttr.stDispRect.Y = 0;
    pCap->mVoLayerAttr.stDispRect.Width = pCap->mConfigPara.mDisplayWidth;
    pCap->mVoLayerAttr.stDispRect.Height = pCap->mConfigPara.mDisplayHeight;
    pCap->mVoLayerAttr.enPixFormat = pCap->mConfigPara.mUserSetPixelFormat;
    AW_MPI_VO_SetVideoLayerAttr(pCap->mVoLayer, &pCap->mVoLayerAttr);

    pCap->mVoChn = 0;
    while (pCap->mVoChn < VO_MAX_CHN_NUM) {
        ret = AW_MPI_VO_EnableChn(pCap->mVoLayer, pCap->mVoChn);
        if (SUCCESS == ret) {
            nSuccessFlag = TRUE;
            alogd("create vo channel[%d] success!", pCap->mVoChn);
            break;
        } else if(ERR_VO_CHN_NOT_DISABLE == ret) {
            alogd("vo channel[%d] is exist, find next!", pCap->mVoChn);
            pCap->mVoChn++;
        } else {
            alogd("create vo channel[%d] ret[0x%x]!", pCap->mVoChn, ret);
            break;
        }
    }

    if (FALSE == nSuccessFlag) {
        pCap->mVoChn = MM_INVALID_CHN;
        aloge("fatal error! create vo channel fail!");
        return FAILURE;
    } else {
        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)pCap;
        cbInfo.callback = (MPPCallbackFuncType)&MPPCallbackWrapper;
        AW_MPI_VO_RegisterCallback(pCap->mVoLayer, pCap->mVoChn, &cbInfo);
        AW_MPI_VO_SetChnDispBufNum(pCap->mVoLayer, pCap->mVoChn, 2);
        return SUCCESS;
    }
}

static ERRORTYPE Vdec_Vo_Component_Init(VI2Venc2Vdec2VO_Cap_S *pCap)
{
    ERRORTYPE ret;

    pCap->mConfigPara.mUserSetPixelFormat = pCap->mConfigPara.DestPicFormat;

    pCap->mConfigPara.mMaxVdecOutputWidth = pCap->mConfigPara.DestWidth;
    pCap->mConfigPara.mMaxVdecOutputHeight = pCap->mConfigPara.DestHeight;

    ret = createVdecChn(pCap);
    if (ret != SUCCESS) {
        aloge("create vdec chn fail");
        return FAILURE;
    }

    ret = createVoChn(pCap);
    if (ret == SUCCESS) {
        alogd("bind vdec & vo");
        MPP_CHN_S VdecChn = {MOD_ID_VDEC, 0, pCap->mVdecChn};
        MPP_CHN_S VoChn = {MOD_ID_VOU, pCap->mVoLayer, pCap->mVoChn};

        AW_MPI_SYS_Bind(&VdecChn, &VoChn);
    } else {
        aloge("create vo chn fail");
        return FAILURE;
    }

    pCap->mClockChnAttr.nWaitMask |= 1 << CLOCK_PORT_VIDEO; //becareful this is too important!!!
    ret = createClockChn(pCap);
    if (ret == SUCCESS) {
        alogd("bind clock & vo");
        MPP_CHN_S ClockChn = {MOD_ID_CLOCK, 0, pCap->mClockChn};
        MPP_CHN_S VoChn = {MOD_ID_VOU, pCap->mVoLayer, pCap->mVoChn};

        AW_MPI_SYS_Bind(&ClockChn, &VoChn);
    } else {
        aloge("create clock chn fail");
        return FAILURE;
    }

    return SUCCESS;
}

static ERRORTYPE Vdec_Vo_Component_Exit(VI2Venc2Vdec2VO_Cap_S *pCap)
{
    MPP_CHN_S ClockChn = {MOD_ID_CLOCK, 0, pCap->mClockChn};
    MPP_CHN_S VoChn = {MOD_ID_VOU, pCap->mVoLayer, pCap->mVoChn};
    MPP_CHN_S VdecChn = {MOD_ID_VDEC, 0, pCap->mVdecChn};

    AW_MPI_SYS_UnBind(&ClockChn, &VoChn);
    AW_MPI_SYS_UnBind(&VdecChn, &VoChn);

    AW_MPI_CLOCK_DestroyChn(pCap->mClockChn);
    AW_MPI_VO_DisableChn(pCap->mVoLayer, pCap->mVoChn);
    AW_MPI_VO_DisableVideoLayer(pCap->mVoLayer);
    AW_MPI_VO_RemoveOutsideVideoLayer(pCap->mUILayer);
    AW_MPI_VO_Disable(pCap->mVoDev);
    AW_MPI_VDEC_DestroyChn(pCap->mVdecChn);

    return SUCCESS;
}

static ERRORTYPE Vdec_Vo_Component_Start(VI2Venc2Vdec2VO_Cap_S *pCap)
{
    ERRORTYPE ret;

    alogd("start stream");
    AW_MPI_CLOCK_Start(pCap->mClockChn);
    AW_MPI_VDEC_StartRecvStream(pCap->mVdecChn);
    AW_MPI_VO_StartChn(pCap->mVoLayer, pCap->mVoChn);

    return SUCCESS;
}

static ERRORTYPE Vdec_Vo_Component_Stop(VI2Venc2Vdec2VO_Cap_S *pCap)
{
    ERRORTYPE ret;

    alogd("stop vo chn");
    AW_MPI_VO_StopChn(pCap->mVoLayer,pCap->mVoChn);
    AW_MPI_VDEC_StopRecvStream(pCap->mVdecChn);
    AW_MPI_CLOCK_Stop(pCap->mClockChn);

    return SUCCESS;
}

static void my_itoa(char* str1, int n)
{
    int a = n;
    char str[8];
    memset(str,0,8);
    sprintf(str, "%d", a);
    for (int i = 0; i < 7; i++) {
        if (str[i] == 0)
            str[i + 1] = 0;
    }
    strcpy(str1,str);
    return;
}

static void *GetEncoderFrameThread(void *pArg)
{
    VI2Venc2Vdec2VO_Cap_S *pCap = (VI2Venc2Vdec2VO_Cap_S *)pArg;
    ERRORTYPE ret = 0;
    int count = 0;

    VENC_STREAM_S VencFrame;
    VENC_PACK_S venc_pack;
    VencFrame.mPackCount = 1;
    VencFrame.mpPack = &venc_pack;

    unsigned int uPhyAddr;
    void *pVirtAddr;
    int nFrameSize = 4 * 1024 * 1024;
    AW_MPI_SYS_MmzAlloc_Cached(&uPhyAddr, &pVirtAddr, nFrameSize);

    alogd("Cap threadid=0x%lx, pCap->mViDev = %d,pCap->mViChn = %d", pCap->thid, pCap->mViDev,pCap->mViChn);

    MPP_CHN_S ViChn = {MOD_ID_VIU, pCap->mViDev, pCap->mViChn};
    MPP_CHN_S VenChn = {MOD_ID_VENC, 0, pCap->mVenChn};

    ret = AW_MPI_SYS_Bind(&ViChn,&VenChn);
    if(ret !=SUCCESS) {
        aloge("error!!! vi can not bind venc!!!");
        return (void*)FAILURE;
    }

    ret = AW_MPI_VI_EnableVirChn(pCap->mViDev,pCap->mViChn);
    if (ret != SUCCESS) {
        aloge("VI enable error!");
        return (void*)FAILURE;
    }
    ret = AW_MPI_VENC_StartRecvPic(pCap->mVenChn);
    if (ret != SUCCESS) {
        aloge("VENC Start RecvPic error!");
        return (void*)FAILURE;
    }

    VDEC_STREAM_S VdecFrame;
    memset(&VdecFrame, 0, sizeof(VdecFrame));
    while (count != pCap->mConfigPara.EncoderCount) {
        count++;
        if ((ret = AW_MPI_VENC_GetStream(pCap->mVenChn,&VencFrame, 4000)) < 0) { //6000(25fps) 4000(30fps)
            aloge("get first frame failed!");
            continue;
        } else {
            if (p_header != NULL ) {
                VdecFrame.pAddr = (unsigned char*)p_header;
                VdecFrame.mLen = header_length;
                VdecFrame.mbEndOfFrame = TRUE;
                VdecFrame.mPTS = VencFrame.mpPack->mPTS;
#if DEBUG_SAMPLE_SAVE_BIT_STREAM
                if (header_length > 0 && pCap->fd_out) {
                    fwrite(VdecFrame.pAddr, 1, VdecFrame.mLen, pCap->fd_out);
                    if (pCap->fd_out_len) {
                        char str[8];
                        my_itoa(str, VdecFrame.mLen);
                        int ret = fwrite(str, 1, sizeof(str), pCap->fd_out_len);
                    }
                }
#endif
                ret = AW_MPI_VDEC_SendStream(pCap->mVdecChn, &VdecFrame, 100);
                if(ret != SUCCESS) {
                    aloge("send stream with 100ms timeout fail?!");
                }
                free(p_header);
                p_header = NULL;
            }

#if DEBUG_SAMPLE_SAVE_BIT_STREAM
            if (VencFrame.mpPack->mLen0 > 0 && pCap->fd_out) {
                fwrite(VencFrame.mpPack->mpAddr0, 1, VencFrame.mpPack->mLen0, pCap->fd_out);
                if (pCap->fd_out_len) {
                    char str[8];
                    my_itoa(str,VencFrame.mpPack->mLen0);
                    int ret = fwrite(str, 1, 8, pCap->fd_out_len);
                }
            }
            if (VencFrame.mpPack->mLen1 > 0 && pCap->fd_out) {
                fwrite(VencFrame.mpPack->mpAddr1, 1, VencFrame.mpPack->mLen1, pCap->fd_out);
                if (pCap->fd_out_len) {
                    char str[8];
                    my_itoa(str,VencFrame.mpPack->mLen1);
                    int ret = fwrite(str, 1, 8, pCap->fd_out_len);
                }
            }
#endif
            int offset = 0;
            memcpy(pVirtAddr + offset, VencFrame.mpPack->mpAddr0, VencFrame.mpPack->mLen0);
            offset += VencFrame.mpPack->mLen0;
            memcpy(pVirtAddr + offset, VencFrame.mpPack->mpAddr1, VencFrame.mpPack->mLen1);

            offset += VencFrame.mpPack->mLen1;
            VdecFrame.pAddr = pVirtAddr;
            VdecFrame.mLen = offset;
            VdecFrame.mbEndOfFrame = TRUE;
            VdecFrame.mPTS = VencFrame.mpPack->mPTS;

            ret = AW_MPI_VDEC_SendStream(pCap->mVdecChn, &VdecFrame, 100);
            if (ret != SUCCESS) {
                aloge("fatal error! send stream with 100ms timeout fail");
            }

            AW_MPI_VENC_ReleaseStream(pCap->mVenChn, &VencFrame);
        }
    }

    AW_MPI_SYS_MmzFree(uPhyAddr, pVirtAddr);
    return NULL;
}

void Virvi2Venc2Vdec2Vo_HELP()
{
    alogw("Run command: ./sample_virvi2venc2vdec2vo -path ./sample_virvi2venc2vdec2vo.conf");
}

int main(int argc, char *argv[])
{
    ERRORTYPE result;
    VI2Venc2Vdec2VO_Cap_S *privCap;

    alogd("sample_virvi2venc2vdec2vo buile time = %s, %s", __DATE__, __TIME__);
    if (argc != 3) {
        Virvi2Venc2Vdec2Vo_HELP();
        exit(0);
    }

    privCap = (VI2Venc2Vdec2VO_Cap_S* )malloc(sizeof(VI2Venc2Vdec2VO_Cap_S));
    if (privCap == NULL) {
        aloge("malloc struct fail");
        return FAILURE;
    }
    memset(privCap, 0, sizeof(VI2Venc2Vdec2VO_Cap_S));

    //parse config
    Parse_Config(argc, argv, privCap);

#if DEBUG_SAMPLE_SAVE_BIT_STREAM
    privCap->fd_out = fopen("stream.bin", "wb");
    if (privCap->fd_out == NULL) {
        aloge("ERROR: cannot create out file");
        goto err_sys_init;
    }
    privCap->fd_out_len = fopen("stream.len", "wb");
    if (privCap->fd_out_len == NULL) {
        aloge("ERROR: cannot create out file");
        goto err_sys_init;
    }
#endif

    MPP_SYS_CONF_S mSysConf;
    memset(&mSysConf, 0, sizeof(MPP_SYS_CONF_S));
    mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&mSysConf);
    if (AW_MPI_SYS_Init() != SUCCESS) {
        aloge("sys Init failed!");
        goto err_sys_init;
    }

    //vi
    if (Vi_Component_Init(privCap) != SUCCESS) {
        aloge("prepare failed");
        goto err_vi_init;
    }

    //venc
    if (Venc_Component_Init(privCap) != SUCCESS) {
        aloge("prepare failed");
        goto err_venc_init;
    }

    //vdec + vo
    if (Vdec_Vo_Component_Init(privCap) != SUCCESS) {
        aloge("prepare failed");
        goto err_vo_init;
    }
    if (Vdec_Vo_Component_Start(privCap) != SUCCESS) {
        aloge("start play fail");
        goto err_vo_start;
    }

    privCap->thid = 0;
    result = pthread_create(&privCap->thid, NULL, GetEncoderFrameThread, (void *)privCap);
    if (result < 0) {
        aloge("pthread_create failed, Dev[%d], Chn[%d].", privCap->mViDev, privCap->mViChn);
        goto err_pth_creat;
    }

    pthread_join(privCap->thid, NULL);

    result = AW_MPI_VENC_StopRecvPic(privCap->mVenChn);
    if (result != SUCCESS) {
        aloge("VI Stop Cap Picture error!");
    }
    result = AW_MPI_VI_DisableVirChn(privCap->mViDev, privCap->mViChn);
    if (result != SUCCESS) {
        aloge("VENC Stop Receive Picture error!");
    }
err_pth_creat:
    if (Vdec_Vo_Component_Stop(privCap) != SUCCESS) {
        aloge("vdec and vo component stop failed");
    }
err_vo_start:
    //vdec + vo
    if (Vdec_Vo_Component_Exit(privCap) != SUCCESS) {
        aloge("vdec and vo component exit failed");
    }
err_vo_init:
    if (Venc_Component_Exit(privCap) != SUCCESS) {
        aloge("venc component exit failed");
    }
err_venc_init:
    if (Vi_Component_Exit(privCap) != SUCCESS) {
        aloge("vi component exit failed");
    }
err_vi_init:
    /* exit mpp systerm */
    if (AW_MPI_SYS_Exit() != SUCCESS) {
        aloge("sys exit failed!");
    }
#if DEBUG_SAMPLE_SAVE_BIT_STREAM
    if (privCap->fd_out) {
        fclose(privCap->fd_out);
        privCap->fd_out = NULL;
    }
    if (privCap->fd_out_len) {
        fclose(privCap->fd_out_len);
        privCap->fd_out_len = NULL;
    }
#endif
err_sys_init:
    free(privCap);
    alogd("sample_virvi2venc2vdec2vo exit!");
    return result;
}
