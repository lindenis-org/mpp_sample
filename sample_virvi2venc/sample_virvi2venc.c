/******************************************************************************
  Copyright (C), 2001-2017, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : sample_virvi2venc.c
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2017/1/5
  Last Modified :
  Description   : mpp component implement
  Function List :
  History       :
******************************************************************************/

//#define LOG_NDEBUG 0
#define LOG_TAG "sample_virvi2venc"
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

#include "media/mm_comm_vi.h"
#include "media/mpi_vi.h"
#include "media/mpi_isp.h"
#include "media/mpi_venc.h"
#include "media/mpi_sys.h"
#include "mm_common.h"
#include "mm_comm_venc.h"
#include "mm_comm_rc.h"

#include <confparser.h>
#include "sample_virvi2venc.h"
#include "sample_virvi2venc_config.h"

//#define MAX_VIPP_DEV_NUM  2
//#define MAX_VIDEO_NUM         MAX_VIPP_DEV_NUM
//#define MAX_VIR_CHN_NUM   8

VENC_CHN mVeChn;
VI_DEV mViDev;
VI_CHN mViChn;
int AutoTestCount = 0,EncoderCount = 0;

VI2Venc_Cap_S privCap[MAX_VIR_CHN_NUM][MAX_VIR_CHN_NUM];
FILE* OutputFile_Fd;
int hal_vipp_start(VI_DEV ViDev, VI_ATTR_S *pstAttr)
{
    AW_MPI_VI_CreateVipp(ViDev);
    AW_MPI_VI_SetVippAttr(ViDev, pstAttr);
    AW_MPI_VI_EnableVipp(ViDev);
    return 0;
}

int hal_vipp_end(VI_DEV ViDev)
{
    AW_MPI_VI_DisableVipp(ViDev);
    AW_MPI_VI_DestoryVipp(ViDev);
    return 0;
}

int hal_virvi_start(VI_DEV ViDev, VI_CHN ViCh, void *pAttr)
{
    int ret = -1;

    ret = AW_MPI_VI_CreateVirChn(ViDev, ViCh, pAttr);
    if(ret < 0) {
        aloge("Create VI Chn failed,VIDev = %d,VIChn = %d",ViDev,ViCh);
        return ret ;
    }
    ret = AW_MPI_VI_SetVirChnAttr(ViDev, ViCh, pAttr);
    if(ret < 0) {
        aloge("Set VI ChnAttr failed,VIDev = %d,VIChn = %d",ViDev,ViCh);
        return ret ;
    }
    return 0;
}

int hal_virvi_end(VI_DEV ViDev, VI_CHN ViCh)
{
    int ret = -1;
#if 0
    /* better be invoked after AW_MPI_VENC_StopRecvPic */
    ret = AW_MPI_VI_DisableVirChn(ViDev, ViCh);
    if(ret < 0) {
        aloge("Disable VI Chn failed,VIDev = %d,VIChn = %d",ViDev,ViCh);
        return ret ;
    }
#endif
    ret = AW_MPI_VI_DestoryVirChn(ViDev, ViCh);
    if(ret < 0) {
        aloge("Destory VI Chn failed,VIDev = %d,VIChn = %d",ViDev,ViCh);
        return ret ;
    }
    return 0;
}

static int ParseCmdLine(int argc, char **argv, SampleVirvi2VencCmdLineParam *pCmdLinePara)
{
    alogd("sample virvi2venc path:[%s], arg number is [%d]", argv[0], argc);
    int ret = 0;
    int i=1;
    memset(pCmdLinePara, 0, sizeof(SampleVirvi2VencCmdLineParam));
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
            alogd("CmdLine param:\n"
                  "\t-path /home/sample_virvi2venc.conf\n");
            ret = 1;
            break;
        } else {
            alogd("ignore invalid CmdLine param:[%s], type -h to get how to set parameter!", argv[i]);
        }
        i++;
    }
    return ret;
}

static ERRORTYPE loadSampleVirvi2VencConfig(SampleVirvi2VencConfig *pConfig, const char *conf_path)
{
    int ret;
    char *ptr;
    CONFPARSER_S stConfParser;
    ret = createConfParser(conf_path, &stConfParser);
    if(ret < 0) {
        aloge("load conf fail");
        return FAILURE;
    }
    memset(pConfig, 0, sizeof(SampleVirvi2VencConfig));
    ptr = (char*)GetConfParaString(&stConfParser, SAMPLE_Virvi2Venc_Output_File_Path, NULL);
    strncpy(pConfig->OutputFilePath, ptr, MAX_FILE_PATH_SIZE-1);
    pConfig->OutputFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
    pConfig->AutoTestCount = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc_Auto_Test_Count, 0);
    pConfig->EncoderCount = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc_Encoder_Count, 0);
    pConfig->DevNum = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc_Dev_Num, 0);
    pConfig->SrcFrameRate = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc_Src_Frame_Rate, 0);
    pConfig->SrcWidth = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc_Src_Width, 0);
    pConfig->SrcHeight = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc_Src_Height, 0);
    char *pStrPixelFormat = (char*)GetConfParaString(&stConfParser, SAMPLE_Virvi2Venc_Dest_Format, NULL);
    if(!strcmp(pStrPixelFormat, "nv21")) {
        pConfig->DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    } else {
        aloge("fatal error! conf file pic_format must be yuv420sp");
        pConfig->DestPicFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    }
    char *EncoderType = (char*)GetConfParaString(&stConfParser, SAMPLE_Virvi2Venc_Dest_Encoder_Type, NULL);
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
    pConfig->DestWidth = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc_Dest_Width, 0);
    pConfig->DestHeight = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc_Dest_Height, 0);
    pConfig->DestFrameRate = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc_Dest_Frame_Rate, 0);
    pConfig->DestBitRate = GetConfParaInt(&stConfParser, SAMPLE_Virvi2Venc_Dest_Bit_Rate, 0);
    alogd("dev_num=%d, src_width=%d, src_height=%d, src_frame_rate=%d",
          pConfig->DevNum,pConfig->SrcWidth,pConfig->SrcHeight,pConfig->SrcFrameRate);
    alogd("dest_width=%d, dest_height=%d, dest_frame_rate=%d, dest_bit_rate=%d",
          pConfig->DestWidth,pConfig->DestHeight,pConfig->SrcFrameRate,pConfig->DestBitRate);
    destroyConfParser(&stConfParser);
    return SUCCESS;
}

static void *GetEncoderFrameThread(void *pArg)
{
    VI_DEV ViDev;
    VI_CHN ViCh;
    int ret = 0;
    int count = 0;

    VI2Venc_Cap_S *pCap = (VI2Venc_Cap_S *)pArg;
    ViDev = pCap->Dev;
    ViCh = pCap->Chn;
    mViDev = ViDev;
    mViChn = ViCh;
    VENC_STREAM_S VencFrame;
    VENC_PACK_S venc_pack;
    VencFrame.mPackCount = 1;
    VencFrame.mpPack = &venc_pack;
    printf("Cap threadid=0x%lx, ViDev = %d, ViCh = %d\n", pCap->thid, ViDev, ViCh);

    if (mVeChn >= 0 && mViChn >= 0) {
        MPP_CHN_S ViChn = {MOD_ID_VIU, mViDev, mViChn};
        MPP_CHN_S VeChn = {MOD_ID_VENC, 0, mVeChn};
        ret = AW_MPI_SYS_Bind(&ViChn,&VeChn);
        if(ret !=SUCCESS) {
            printf("error!!! vi can not bind venc!!!\n");
            return (void*)FAILURE;
        }
    }
    //printf("start start recv success!\n");
    ret = AW_MPI_VI_EnableVirChn(ViDev, ViCh);
    if (ret != SUCCESS) {
        printf("VI enable error!");
        return (void*)FAILURE;
    }
    ret = AW_MPI_VENC_StartRecvPic(mVeChn);
    if (ret != SUCCESS) {
        printf("VENC Start RecvPic error!");
        return (void*)FAILURE;
    }

    while(count != EncoderCount) {
        count++;
        if((ret = AW_MPI_VENC_GetStream(mVeChn,&VencFrame,4000)) < 0) { //6000(25fps) 4000(30fps)
            printf("get first frmae failed!\n");
            continue;
        } else {
            if(VencFrame.mpPack != NULL && VencFrame.mpPack->mLen0) {
                fwrite(VencFrame.mpPack->mpAddr0,1,VencFrame.mpPack->mLen0,OutputFile_Fd);
            }
            if(VencFrame.mpPack != NULL && VencFrame.mpPack->mLen1) {
                fwrite(VencFrame.mpPack->mpAddr1,1,VencFrame.mpPack->mLen1,OutputFile_Fd);
            }
            ret = AW_MPI_VENC_ReleaseStream(mVeChn,&VencFrame);
            if(ret < 0) {
                printf("falied error,release failed!!!\n");
            }
        }
    }
    return NULL;
}

void Virvi2Venc_HELP()
{
    printf("Run CSI0/CSI1+Venc command: ./sample_virvi2venc -path ./sample_virvi2venc.conf\r\n");
}

int main(int argc, char *argv[])
{
    int ret, count = 0,result = 0;
    int vipp_dev, virvi_chn;
    int isp_dev;

    printf("sample_virvi2venc buile time = %s, %s.\r\n", __DATE__, __TIME__);
    if (argc != 3) {
        Virvi2Venc_HELP();
        exit(0);
    }
    SampleVirvi2VencConfparser stContext;
    /* parse command line param,read sample_virvi2venc.conf */
    if(ParseCmdLine(argc, argv, &stContext.mCmdLinePara) != 0) {
        aloge("fatal error! command line param is wrong, exit!");
        result = -1;
        goto _exit;
    }
    char *pConfigFilePath;
    if(strlen(stContext.mCmdLinePara.mConfigFilePath) > 0) {
        pConfigFilePath = stContext.mCmdLinePara.mConfigFilePath;
    } else {
        pConfigFilePath = DEFAULT_SAMPLE_VIPP2VENC_CONF_PATH;
    }
    /* parse config file. */
    if(loadSampleVirvi2VencConfig(&stContext.mConfigPara, pConfigFilePath) != SUCCESS) {
        aloge("fatal error! no config file or parse conf file fail");
        result = -1;
        goto _exit;
    }
    //open output file
    OutputFile_Fd = fopen(stContext.mConfigPara.OutputFilePath, "wb+");
    if(!OutputFile_Fd) {
        aloge("fatal error! can't open yuv file[%s]", stContext.mConfigPara.OutputFilePath);
        result = -1;
        goto _exit;
    }
    AutoTestCount = stContext.mConfigPara.AutoTestCount;
    EncoderCount = stContext.mConfigPara.EncoderCount;
    while(count != AutoTestCount) {
        printf("======================================.\r\n");
        printf("Auto Test count start: %d. (MaxCount==1000).\r\n", count);
        system("cat /proc/meminfo | grep Committed_AS");
        printf("======================================.\r\n");
        MPP_SYS_CONF_S mSysConf;
        memset(&mSysConf, 0, sizeof(MPP_SYS_CONF_S));
        mSysConf.nAlignWidth = 32;
        AW_MPI_SYS_SetConf(&mSysConf);
        ret = AW_MPI_SYS_Init();
        if (ret < 0) {
            aloge("sys Init failed!");
            return -1;
        }
        VI_ATTR_S stAttr;
        vipp_dev = stContext.mConfigPara.DevNum;
        /* dev:0, chn:0,1,2,3,4...16 */
        /* dev:1, chn:0,1,2,3,4...16 */
        /* dev:2, chn:0,1,2,3,4...16 */
        /* dev:3, chn:0,1,2,3,4...16 */
        /*Set VI Channel Attribute*/
        stAttr.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        stAttr.memtype = V4L2_MEMORY_MMAP;
        stAttr.format.pixelformat = V4L2_PIX_FMT_NV21M;
        stAttr.format.field = V4L2_FIELD_NONE;
        stAttr.format.width = stContext.mConfigPara.SrcWidth;
        stAttr.format.height = stContext.mConfigPara.SrcHeight;
        stAttr.fps = stContext.mConfigPara.SrcFrameRate;
        /* update configuration anyway, do not use current configuration */
        stAttr.use_current_win = 0;
        stAttr.nbufs = 5;
        stAttr.nplanes = 2;

        /* MPP components */
        mVeChn = 0;

        /* venc chn attr */
        int VIFrameRate = stContext.mConfigPara.SrcFrameRate;
        int VencFrameRate = stContext.mConfigPara.DestFrameRate;
        VENC_CHN_ATTR_S mVEncChnAttr;
        memset(&mVEncChnAttr, 0, sizeof(VENC_CHN_ATTR_S));
        SIZE_S wantedVideoSize = {stContext.mConfigPara.DestWidth, stContext.mConfigPara.DestHeight};
        SIZE_S videosize = {stContext.mConfigPara.SrcWidth, stContext.mConfigPara.SrcHeight};
        PAYLOAD_TYPE_E videoCodec = stContext.mConfigPara.EncoderType;
        PIXEL_FORMAT_E wantedPreviewFormat = stContext.mConfigPara.DestPicFormat;
        int wantedFrameRate = stContext.mConfigPara.DestFrameRate;
        mVEncChnAttr.VeAttr.Type = videoCodec;
        mVEncChnAttr.VeAttr.SrcPicWidth = videosize.Width;
        mVEncChnAttr.VeAttr.SrcPicHeight = videosize.Height;
        mVEncChnAttr.VeAttr.Field = VIDEO_FIELD_FRAME;
        mVEncChnAttr.VeAttr.PixelFormat = wantedPreviewFormat;
        int wantedVideoBitRate = stContext.mConfigPara.DestBitRate;
        if(PT_H264 == mVEncChnAttr.VeAttr.Type) {
            mVEncChnAttr.VeAttr.AttrH264e.bByFrame = TRUE;
            mVEncChnAttr.VeAttr.AttrH264e.Profile = 2;
            mVEncChnAttr.VeAttr.AttrH264e.PicWidth = wantedVideoSize.Width;
            mVEncChnAttr.VeAttr.AttrH264e.PicHeight = wantedVideoSize.Height;
            mVEncChnAttr.RcAttr.mRcMode = VENC_RC_MODE_H264CBR;
            mVEncChnAttr.RcAttr.mAttrH264Cbr.mSrcFrmRate = wantedFrameRate;
            if(stContext.mConfigPara.mTimeLapseEnable) {
                mVEncChnAttr.RcAttr.mAttrH264Cbr.fr32DstFrmRate = 1000 + (stContext.mConfigPara.mTimeBetweenFrameCapture<<16);
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
#if 0
        /* has invoked in AW_MPI_SYS_Init() */
        result = VENC_Construct();
        if (result != SUCCESS) {
            printf("VENC Construct error!");
            result = -1;
            goto _exit;
        }
#endif
        hal_vipp_start(vipp_dev, &stAttr);
        // for (virvi_chn = 0; virvi_chn < MAX_VIR_CHN_NUM; virvi_chn++)
        for (virvi_chn = 0; virvi_chn < 1; virvi_chn++) {
            memset(&privCap[vipp_dev][virvi_chn], 0, sizeof(VI2Venc_Cap_S));
            privCap[vipp_dev][virvi_chn].Dev = vipp_dev;
            privCap[vipp_dev][virvi_chn].Chn = virvi_chn;
            privCap[vipp_dev][virvi_chn].s32MilliSec = 5000;  // 2000;
            privCap[vipp_dev][virvi_chn].EncoderType = mVEncChnAttr.VeAttr.Type;
            if (0 == virvi_chn) { /* H264, H265, MJPG, Preview(LCD or HDMI), VDA, ISE, AIE, CVBS */
                /* open isp */
                if (vipp_dev == 0 || vipp_dev == 2) {
                    isp_dev = 1;
                } else if (vipp_dev == 1 || vipp_dev == 3) {
                    isp_dev = 0;
                }
                AW_MPI_ISP_Init();
                AW_MPI_ISP_Run(isp_dev);

                result = hal_virvi_start(vipp_dev, virvi_chn, NULL);
                if(result < 0) {
                    printf("VI start failed!\n");
                    result = -1;
                    goto _exit;
                }
                privCap[vipp_dev][virvi_chn].thid = 0;
                result = AW_MPI_VENC_CreateChn(mVeChn, &mVEncChnAttr);
                if(result < 0) {
                    printf("create venc channel[%d] falied!\n", mVeChn);
                    result = -1;
                    goto _exit;
                }
                VENC_FRAME_RATE_S stFrameRate;
                stFrameRate.SrcFrmRate = VIFrameRate;
                stFrameRate.DstFrmRate = VencFrameRate;
                AW_MPI_VENC_SetFrameRate(mVeChn, &stFrameRate);
                VencHeaderData vencheader;
                OutputFile_Fd = fopen(stContext.mConfigPara.OutputFilePath, "wb+");
                if(PT_H264 == mVEncChnAttr.VeAttr.Type) {
                    AW_MPI_VENC_GetH264SpsPpsInfo(mVeChn, &vencheader);
                    if(vencheader.nLength) {
                        fwrite(vencheader.pBuffer,vencheader.nLength,1,OutputFile_Fd);
                    }
                } else if(PT_H265 == mVEncChnAttr.VeAttr.Type) {
                    AW_MPI_VENC_GetH265SpsPpsInfo(mVeChn, &vencheader);
                    if(vencheader.nLength) {
                        fwrite(vencheader.pBuffer,vencheader.nLength, 1, OutputFile_Fd);
                    }
                }
                result = pthread_create(&privCap[vipp_dev][virvi_chn].thid, NULL, GetEncoderFrameThread, (void *)&privCap[vipp_dev][virvi_chn]);
                if (result < 0) {
                    alogd("pthread_create failed, Dev[%d], Chn[%d].\n", privCap[vipp_dev][virvi_chn].Dev, privCap[vipp_dev][virvi_chn].Chn);
                    continue;
                }
            }
        }
        for (virvi_chn = 0; virvi_chn < 1; virvi_chn++) {
            pthread_join(privCap[vipp_dev][virvi_chn].thid, NULL);
        }

        result = AW_MPI_VENC_StopRecvPic(mVeChn);
        if (result != SUCCESS) {
            printf("VENC Stop Receive Picture error!");
            result = -1;
            goto _exit;
        }
#if 1
        /* better call AW_MPI_VI_DisableVirChn immediately after AW_MPI_VENC_StopRecvPic was invoked */
        ret = AW_MPI_VI_DisableVirChn(vipp_dev, 0);
        if(ret < 0) {
            aloge("Disable VI Chn failed,VIDev = %d,VIChn = %d", vipp_dev, virvi_chn);
            return ret ;
        }
#endif
        result = AW_MPI_VENC_ResetChn(mVeChn);
        if (result != SUCCESS) {
            printf("VENC Reset Chn error!");
            result = -1;
            goto _exit;
        }
        AW_MPI_VENC_DestroyChn(mVeChn);
        if (result != SUCCESS) {
            printf("VENC Destroy Chn error!");
            result = -1;
            goto _exit;
        }
        for (virvi_chn = 0; virvi_chn < 1; virvi_chn++) {
            result = hal_virvi_end(vipp_dev, virvi_chn);
            if(result < 0) {
                printf("VI end failed!\n");
                result = -1;
                goto _exit;
            }
        }

        AW_MPI_ISP_Stop(isp_dev);
        AW_MPI_ISP_Exit();

        hal_vipp_end(vipp_dev);
        /* exit mpp systerm */
        ret = AW_MPI_SYS_Exit();
        if (ret < 0) {
            aloge("sys exit failed!");
            return -1;
        }
        fclose(OutputFile_Fd);
        printf("======================================.\r\n");
        printf("Auto Test count end: %d. (MaxCount==1000).\r\n", count);
        printf("======================================.\r\n");
        count++;
    }
    printf("sample_virvi2venc exit!\n");
    return 0;
_exit:
    return result;
}
