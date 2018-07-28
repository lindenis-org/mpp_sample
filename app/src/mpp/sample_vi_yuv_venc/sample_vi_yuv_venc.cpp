/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file sample_vi_yuv_venc.cpp
 * @brief 该目录是对mpp中的 VI+YUV+VENC 通路sample
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-04-08
 */

/************************************************************************************************/
/*                                      Include Files                                           */
/************************************************************************************************/

#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "menu.h"
#include "mpp_comm.h"
#include "mpp_menu.h"

#include "aw_ai_eve_type.h"
#include "aw_ai_eve_event_interface.h"

#include "rtsp/MediaStream.h"
#include "rtsp/TinyServer.h"


/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
struct vi_venc_param {
    pthread_t       thd_id;
    pthread_t       proc_id;
    int             vi_dev;
    int             vi_chn;
    int             venc_chn;
    int             run_flag;
    PAYLOAD_TYPE_E  venc_type;
    MediaStream    *stream;
};


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/

/*** eve parameter ***/
static int g_eve_run        = 1;
static int g_facedet_run    = 1;
static int g_eve_start_proc = 0;
static int g_eve_dmaflag    = 0;
static int g_eve_ready      = 0;

static pthread_t        g_eve_proc_id;
static pthread_t        g_facedet_proc_id;
static pthread_mutex_t  g_eve_lock = PTHREAD_MUTEX_INITIALIZER;
static AW_HANDLE        g_eve_event = AW_NULL;
static AW_IMAGE_S       g_eve_image;
static AW_AI_EVE_EVENT_RESULT_S        g_eve_result    = {0};
static AW_AI_EVE_EVENT_FACEDET_PARAM_S g_facedet_param = {0};

static struct vi_venc_param g_param_0;
static struct vi_venc_param g_param_1;

static MediaStream *g_stream_0 = NULL;
static MediaStream *g_stream_1 = NULL;

static unsigned int   g_rc_mode   = 0;          /* 0:CBR 1:VBR 2:FIXQp 3:ABR 4:QPMAP */
static unsigned int   g_profile   = 1;          /* 0: baseline  1:MP  2:HP  3: SVC-T [0,3] */
static PAYLOAD_TYPE_E g_venc_type = PT_H264;    /* PT_H264/PT_H265/PT_MJPEG */
static ROTATE_E       g_rotate    = ROTATE_NONE;

static MPP_COM_VI_TYPE_E g_vi_type_0 = VI_4K_25FPS;
static MPP_COM_VI_TYPE_E g_vi_type_1 = VI_720P_25FPS;

static VENC_CFG_TYPE_E g_venc_type_0 = VENC_4K_TO_4K_18M_25FPS;
static VENC_CFG_TYPE_E g_venc_type_1 = VENC_720P_TO_720P_4M_25FPS;

static MPP_MENU_VI_VENC_CFG_E  g_vi_ve_cfg = VI_1080P_30FPS_VE_1080P_30FPS_VE_720P_30FPS;

static MENU_INODE g_menu_top[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"ISP  control",   NULL,   NULL, NULL},
    {(char *)"VI   control",   NULL,   NULL, NULL},
    {(char *)"VENC control",   NULL,   NULL, NULL},
    {(char *)"Quite vi+(venc+rtsp) sample",  ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/

static int timeval_subtract(struct timeval* result, struct timeval* x, struct timeval* y)
{
    if (x->tv_sec > y->tv_sec)
        return -1;

    if ((x->tv_sec == y->tv_sec) && (x->tv_usec > y->tv_usec))
        return -1;

    result->tv_sec  = (y->tv_sec  - x->tv_sec);
    result->tv_usec = (y->tv_usec - x->tv_usec);

    if (result->tv_usec < 0) {
        result->tv_sec--;
        result->tv_usec += 1000000;
    }

    return 0;
}

static void dmaCallBackFunc(void* pUsr)
{
#ifdef TEST_PRINT
    printf("dmaCallBackFunc\n");
#endif
    g_eve_dmaflag = 1;
}

static void MenuCtrl(void)
{
    int  ret = 0;

    ret = mpp_menu_isp_get(&g_menu_top[0].subMenu);
    if (ret) {
        ERR_PRT("Do mpp_menu_isp_get fail! ret:%d\n", ret);
    }

    ret = mpp_menu_vi_get(&g_menu_top[1].subMenu);
    if (ret) {
        ERR_PRT("Do mpp_menu_vi_get fail! ret:%d\n", ret);
    }

    ret = mpp_menu_venc_get(&g_menu_top[2].subMenu);
    if (ret) {
        ERR_PRT("Do mpp_menu_venc_get fail! ret:%d\n", ret);
    }

    RunMenuCtrl(g_menu_top);
}


static ERRORTYPE MPPCallbackFunc(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    return SUCCESS;
}


static void *SendStreamByRtsp(void *param)
{
    int            ret = 0;
    unsigned char *buf = NULL;
    unsigned int   len = 0;
    uint64_t       pts = 0;
    int            frame_type;
    VencHeaderData        head_info   = {0};
    struct vi_venc_param *local_param = (struct vi_venc_param *)param;

    while (local_param->run_flag) {
        buf        = NULL;
        len        = 0;
        frame_type = -1;

        ret = mpp_comm_venc_get_stream(local_param->venc_chn, local_param->venc_type, -1, &buf, &len, &pts, &frame_type, &head_info);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_get_stream fail! ret:%d\n", ret);
            continue;
        }

        if (1 == frame_type) {
            /* Get I frame */
            local_param->stream->appendVideoData(head_info.pBuffer, head_info.nLength, pts, MediaStream::FRAME_DATA_TYPE_HEADER);
            local_param->stream->appendVideoData(buf, len, pts, MediaStream::FRAME_DATA_TYPE_I);
        } else {
            local_param->stream->appendVideoData(buf, len, pts, MediaStream::FRAME_DATA_TYPE_P);
        }
    }

    DB_PRT("Out this function ... ... \n");
    return NULL;
}


static void *get_yuv_encode_proc(void *param)
{
    int      ret = 0, i = 0;
    int      sx  = 0, sy = 0, ex = 0, ey = 0;
    uint64_t curPts = 0;
    VIDEO_FRAME_INFO_S       video_frame = {0};
    struct vi_venc_param *local_param = (struct vi_venc_param *)param;

    while (local_param->run_flag) {
        memset(&video_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = AW_MPI_VI_GetFrame(local_param->vi_dev, local_param->vi_chn, &video_frame, 100);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_GetFrame fail! ret:0x%x\n", ret);
            continue;
        }

        /*printf("mId:%d  mWidth:%d  mHeight:%d  mOffsetLeft:%d mOffsetTop:%d  mOffsetRight:%d mOffsetBottom:%d  mField:%d  mVideoFormat:%d  mCompressMode:%d mpts:%ld  \n",
            video_frame.mId, video_frame.VFrame.mWidth, video_frame.VFrame.mHeight,
            video_frame.VFrame.mOffsetLeft, video_frame.VFrame.mOffsetTop,
            video_frame.VFrame.mOffsetRight, video_frame.VFrame.mOffsetBottom,
            video_frame.VFrame.mField, video_frame.VFrame.mVideoFormat,
            video_frame.VFrame.mCompressMode, video_frame.VFrame.mpts);*/

        pthread_mutex_lock(&g_eve_lock);
        if (g_eve_ready) {
            //g_eve_ready = 0;
            for (i = 0; i < g_eve_result.sTarget.s32TargetNum; i++) {
                sx = (video_frame.VFrame.mWidth  * g_eve_result.sTarget.astTargets[i].stRect.s16Left) / 640;
                sy = (video_frame.VFrame.mHeight * g_eve_result.sTarget.astTargets[i].stRect.s16Top)  / 360;
                ex = (video_frame.VFrame.mWidth  * g_eve_result.sTarget.astTargets[i].stRect.s16Right)  / 640;
                ey = (video_frame.VFrame.mHeight * g_eve_result.sTarget.astTargets[i].stRect.s16Bottom) / 360;
                draw_rectangle_nv21((unsigned char *)video_frame.VFrame.mpVirAddr[0], (unsigned char *)video_frame.VFrame.mpVirAddr[1],
                                    video_frame.VFrame.mWidth, video_frame.VFrame.mHeight, 3,
                                    sx, sy, ex, ey);
                //DB_PRT("====TargetNum:%d  (%d-%d) (%d-%d)\n", eve_result.sTarget.s32TargetNum, sx, sy, ex, ey);
            }
        }
        pthread_mutex_unlock(&g_eve_lock);

        video_frame.VFrame.mOffsetLeft   = 0;
        video_frame.VFrame.mOffsetTop    = 0;
        video_frame.VFrame.mOffsetRight  = video_frame.VFrame.mOffsetLeft + video_frame.VFrame.mWidth;
        video_frame.VFrame.mOffsetBottom = video_frame.VFrame.mOffsetTop + video_frame.VFrame.mHeight;
        video_frame.VFrame.mField        = VIDEO_FIELD_FRAME;
        video_frame.VFrame.mVideoFormat	 = VIDEO_FORMAT_LINEAR;
        video_frame.VFrame.mCompressMode = COMPRESS_MODE_NONE;
        video_frame.VFrame.mpts = curPts;
        curPts += (1*1000*1000) / 25;

        ret = AW_MPI_VENC_SendFrame(local_param->venc_chn, &video_frame, 70);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_SendFrame fail! ret:0x%x\n", ret);
        }

        ret = AW_MPI_VI_ReleaseFrame(local_param->vi_dev, local_param->vi_chn, &video_frame);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_ReleaseFrame fail! ret:0x%x\n", ret);
        }
    }

    DB_PRT("Out this function ... ... \n");
    return NULL;
}


static void *get_yuv_eve_proc(void *param)
{
    int ret = 0;
    VI_DEV ViDev = VI_DEV_3;
    VI_CHN ViCh  = VI_CHN_0;
    VIDEO_FRAME_INFO_S  video_frame = {0};
    struct timeval dmastarttime, dmaendtime, dmadifftime;

    DB_PRT("Do this function ... ... \n");

    while (g_eve_run) {
        memset(&video_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = AW_MPI_VI_GetFrame(ViDev, ViCh, &video_frame, 70);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_GetFrame fail! ret:0x%x\n", ret);
            continue;
        }

        //pthread_mutex_lock(&pCap->lock);
        if(g_eve_start_proc == 0) {
            g_eve_image.mPhyAddr[0]  = (AW_U8 *)video_frame.VFrame.mPhyAddr[0];
            g_eve_image.mPhyAddr[1]  = (AW_U8 *)video_frame.VFrame.mPhyAddr[1];
            g_eve_image.mWidth       = video_frame.VFrame.mWidth;
            g_eve_image.mHeight      = video_frame.VFrame.mHeight;
            g_eve_image.mStride[0]   = video_frame.VFrame.mStride[0];
            g_eve_image.mStride[1]   = video_frame.VFrame.mStride[1];
            g_eve_image.mStride[2]   = video_frame.VFrame.mStride[2];
            g_eve_image.mPixelFormat = video_frame.VFrame.mPixelFormat;
            memcpy(g_eve_image.mpVirAddr[0], (AW_U8 *)video_frame.VFrame.mpVirAddr[0],
                   video_frame.VFrame.mWidth*video_frame.VFrame.mHeight);
            memcpy(g_eve_image.mpVirAddr[1], (AW_U8 *)video_frame.VFrame.mpVirAddr[1],
                   video_frame.VFrame.mWidth*video_frame.VFrame.mHeight/2);

            AW_AI_EVE_Event_SetEveSourceAddress(g_eve_event, (void *)g_eve_image.mPhyAddr[0]);
            g_eve_start_proc = 1;
            //pthread_mutex_unlock(&pCap->lock);
            gettimeofday(&dmastarttime, 0);
            while(1) {
                if(g_eve_dmaflag == 1) {
                    g_eve_dmaflag = 0;
#ifdef TEST_PRINT
                    printf("finish dma callback!\n");
#endif
                    break;
                } else {
                    gettimeofday(&dmaendtime, 0);
                    timeval_subtract(&dmadifftime, &dmastarttime, &dmaendtime);
                    if(dmadifftime.tv_usec/1000.f > 20) { //³¬Ê±Ç¿ÖÆÍË³ö
#ifdef TEST_PRINT
                        printf("dma wait too long, exit current dma callback!\n");
#endif
                        break;
                    }
                    //printf("wait finish dma callback!\n");
                    usleep(1000);
                }
            }
        }

#ifdef TEST_PRINT
        printf("GetCSIFrameThread############################################################\n");
        i++;
        if (i % 25 == 0) {
            time_t now;
            struct tm *timenow;
            time(&now);
            timenow = localtime(&now);
            printf("Cap threadid=0x%lx,ViDev=%d,VirVi=%d; local time is %s\r\n", pCap->thid, ViDev, ViCh, asctime(timenow));
        }
#endif

        AW_MPI_VI_ReleaseFrame(ViDev, ViCh, &video_frame);
    }
    return NULL;
}


static void *eve_facedet_proc(void *param)
{
    int i = 0;
    int iTimeGap = 0;
    uint64_t    curPts = 0;
    AW_STATUS_E status;
    AW_AI_EVE_EVENT_RESULT_S eve_result  = {0};
#ifdef TEST_TIME
    struct timeval starttime, endtime, difftime;
#endif

    while (g_facedet_run) {
        if(g_eve_start_proc) {
#ifdef TEST_TIME
            gettimeofday(&starttime, 0);
#endif
#ifdef TEST_PRINT
            printf("############################################################\n");
#endif
            curPts += (1*1000*1000) / 25;
            status = AW_AI_EVE_Event_Process(g_eve_event, &g_eve_image, curPts, &eve_result);

            pthread_mutex_lock(&g_eve_lock);
            if (AW_STATUS_ERROR != status && eve_result.sTarget.s32TargetNum > 0) {
                g_eve_ready = 1;
                memcpy(&g_eve_result, &eve_result, sizeof(eve_result));
            } else {
                g_eve_ready = 0;
            }
            pthread_mutex_unlock(&g_eve_lock);

            if(eve_result.sTarget.s32TargetNum > 0) {
                //printf("total face = %d\n",eve_result.sTarget.s32TargetNum);
                for(i = 0; i < eve_result.sTarget.s32TargetNum; i++) {
                    //printf("total face = %d, face id = %i, x = %d, y = %d, w = %d, h = %d\n",
                    //		   pCap->faceres.sTarget.s32TargetNum,
                    //	       i, pCap->faceres.sTarget.astTargets[i].stRect.s16Left,
                    //	       pCap->faceres.sTarget.astTargets[i].stRect.s16Top,
                    //	       pCap->faceres.sTarget.astTargets[i].stRect.s16Right - pCap->faceres.sTarget.astTargets[i].stRect.s16Left + 1,
                    //	       pCap->faceres.sTarget.astTargets[i].stRect.s16Bottom - pCap->faceres.sTarget.astTargets[i].stRect.s16Top + 1);
                }
            }

#ifdef TEST_TIME
            gettimeofday(&endtime, 0);
            timeval_subtract(&difftime, &starttime, &endtime);
            iTimeGap = difftime.tv_usec/1000.f;
            if(iTimeGap > 100) {
                printf("AW_AI_EVE_Event_Process cost time = %f ms\n", difftime.tv_usec/1000.f);
            }
#endif
            if(AW_STATUS_ERROR == status) {
                printf("AW_AI_EVE_Event_Process failure!, errorcode = %d\n", AW_AI_EVE_Event_GetLastError(g_eve_event));
                break;
            }
            g_eve_start_proc = 0;
        } else {
            usleep(1 * 1000);
        }
    }

    return NULL;
}


static int CreateRtspServer(TinyServer **rtsp)
{
    int ret  = 0;
    int port = 8554;
    char ip[64] = {0};
    std::string ip_addr;

    ret = get_net_dev_ip("eth0", ip);
    if (ret) {
        ERR_PRT("Do get_net_dev_ip fail! ret:%d\n", ret);
        return -1;
    }
    DB_PRT("This dev eth0 ip:%s \n", ip);

    ip_addr = ip;
    *rtsp = TinyServer::createServer(ip_addr, port);

    DB_PRT("============================================================\n");
    DB_PRT("   rtsp://%s:%d/ch0~n  \n", ip_addr.c_str(), port);
    DB_PRT("============================================================\n");

    return 0;
}


static int rtsp_start(TinyServer *rtsp)
{
    int ret = 0;

    MediaStream::MediaStreamAttr attr;
    if (PT_H264 == g_venc_type) {
        attr.videoType  = MediaStream::MediaStreamAttr::VIDEO_TYPE_H264;
    } else if (PT_H265 == g_venc_type) {
        attr.videoType  = MediaStream::MediaStreamAttr::VIDEO_TYPE_H265;
    }
    attr.audioType  = MediaStream::MediaStreamAttr::AUDIO_TYPE_AAC;
    attr.streamType = MediaStream::MediaStreamAttr::STREAM_TYPE_UNICAST;

    g_stream_0 = rtsp->createMediaStream("ch0", attr);
    g_stream_1 = rtsp->createMediaStream("ch1", attr);
    rtsp->runWithNewThread();

    g_param_0.stream    = g_stream_0;
    g_param_0.run_flag  = 1;
    g_param_0.vi_dev    = VI_DEV_1;
    g_param_0.vi_chn    = VI_CHN_0;
    g_param_0.venc_chn  = VENC_CHN_0;
    g_param_0.venc_type = g_venc_type;
    ret = pthread_create(&g_param_0.thd_id, NULL, SendStreamByRtsp, &g_param_0);
    if (ret) {
        ERR_PRT("Do pthread_create fail! ret:%d\n", ret);
    }

    return 0;
}


static void rtsp_stop(TinyServer *rtsp)
{
    g_param_0.run_flag = 0;
    g_param_1.run_flag = 0;

    usleep(10*1000);

    rtsp->stop();
    pthread_join(g_param_0.thd_id, 0);
    delete g_param_0.stream;
}


static int vi_create(void)
{
    int ret = 0;
    VI_ATTR_S vi_attr;

    /**  create vi dev 1  src 1080P **/
    ret = mpp_comm_vi_get_attr(g_vi_type_0, &vi_attr);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_get_attr fail! ret:%d \n", ret);
        return -1;
    }
    vi_attr.fps = 25,
            ret = mpp_comm_vi_dev_create(VI_DEV_1, &vi_attr);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_dev_create fail! ret:%d  vi_dev:%d\n", ret, VI_DEV_1);
        return -1;
    }
    ret = mpp_comm_vi_chn_create(VI_DEV_1, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_chn_create fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_1, VI_CHN_0);
        return -1;
    }

    /**  create vi dev 3  src VGA **/
    ret = mpp_comm_vi_get_attr(g_vi_type_1, &vi_attr);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_get_attr fail! ret:%d \n", ret);
        return -1;
    }
    vi_attr.format.width  = 640,
                   vi_attr.format.height = 360,
                                  vi_attr.fps           = 25,
                                          ret = mpp_comm_vi_dev_create(VI_DEV_3, &vi_attr);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_dev_create fail! ret:%d  vi_dev:%d\n", ret, VI_DEV_3);
        return -1;
    }
    ret = mpp_comm_vi_chn_create(VI_DEV_3, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_chn_create fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_3, VI_CHN_0);
        return -1;
    }

    /*  Run isp server */
    AW_MPI_ISP_Init();
    AW_MPI_ISP_Run(ISP_DEV_0);

    return 0;
}


static int vi_destroy(void)
{
    int ret = 0;

    /*  Stop isp server */
    AW_MPI_ISP_Stop(ISP_DEV_0);
    AW_MPI_ISP_Exit();

    /**  destroy vi dev 1  **/
    ret = mpp_comm_vi_chn_destroy(VI_DEV_1, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_chn_destroy fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_1, VI_CHN_0);
    }
    ret = mpp_comm_vi_dev_destroy(VI_DEV_1);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_dev_destroy fail! ret:%d  vi_dev:%d \n", ret, VI_DEV_1);
    }

    /**  destroy vi dev 3  **/
    ret = mpp_comm_vi_chn_destroy(VI_DEV_3, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_chn_destroy fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_3, VI_CHN_0);
    }
    ret = mpp_comm_vi_dev_destroy(VI_DEV_3);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_dev_destroy fail! ret:%d  vi_dev:%d \n", ret, VI_DEV_3);
    }

    return 0;
}


static int venc_create(void)
{
    int ret = 0;

    VENC_CFG_S venc_cfg  = {0};

    ret = mpp_comm_venc_get_cfg(g_venc_type_0, &venc_cfg);
    if (ret) {
        ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
        return -1;
    }
    ret = mpp_comm_venc_create(VENC_CHN_0, g_venc_type, g_rc_mode, g_profile, g_rotate, &venc_cfg);
    if (ret) {
        ERR_PRT("Do mpp_comm_venc_create fail! ret:%d \n", ret);
        return -1;
    }

    /* Register Venc call back function */
    MPPCallbackInfo venc_cb;
    venc_cb.cookie   = NULL;
    venc_cb.callback = (MPPCallbackFuncType)&MPPCallbackFunc;
    AW_MPI_VENC_RegisterCallback(VENC_CHN_0, &venc_cb);

    return 0;
}

static int venc_destroy(void)
{
    int ret = 0;

    ret = mpp_comm_venc_destroy(VENC_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_venc_destroy fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
    }
    return 0;
}


static int eve_create(void)
{
    int ret = 0;
    int i = 0, mT = 0, iTemp = 0;
    FILE *fileevecfg = NULL;
    VI_ATTR_S        vi_attr;
    AW_STATUS_E      status   = AW_STATUS_ERROR;
    AW_AI_EVE_CTRL_S sEVECtrl = {0};

    /**  create vi dev 3  src VGA **/
    ret = mpp_comm_vi_get_attr(g_vi_type_1, &vi_attr);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_get_attr fail! ret:%d \n", ret);
        return -1;
    }
    vi_attr.format.width  = 640,
                   vi_attr.format.height = 360,
                                  vi_attr.fps           = 25,

                                          sEVECtrl.addrInputType  = AW_AI_EVE_ADDR_INPUT_PHY;//ÊäÈëÎïÀíµØÖ·
    sEVECtrl.scale_factor   = 1;
    sEVECtrl.mScanStageNo   = 10;
    sEVECtrl.yStep          = 3;
    sEVECtrl.xStep0         = 1;
    sEVECtrl.xStep1         = 4;
    sEVECtrl.mMidRltStageNo = 10;
    sEVECtrl.mMidRltNum     = 0;
    sEVECtrl.mRltNum        = AW_AI_EVE_MAX_RESULT_NUM;
    sEVECtrl.rltType        = AW_AI_EVE_RLT_OUTPUT_DETAIL;

    sEVECtrl.mDmaOut.s16Width              = vi_attr.format.width;
    sEVECtrl.mDmaOut.s16Height             = vi_attr.format.height;
    sEVECtrl.mPyramidLowestLayel.s16Width  = vi_attr.format.width;
    sEVECtrl.mPyramidLowestLayel.s16Height = vi_attr.format.height;
    sEVECtrl.dmaSrcSize.s16Width           = vi_attr.format.width;
    sEVECtrl.dmaSrcSize.s16Height          = vi_attr.format.height;
    sEVECtrl.dmaDesSize.s16Width           = vi_attr.format.width;
    sEVECtrl.dmaDesSize.s16Height          = vi_attr.format.height;
    sEVECtrl.dmaRoi.s16X                   = 0;
    sEVECtrl.dmaRoi.s16Y                   = 0;
    sEVECtrl.dmaRoi.s16Width               = sEVECtrl.dmaDesSize.s16Width;
    sEVECtrl.dmaRoi.s16Height              = sEVECtrl.dmaDesSize.s16Height;
    AW_U8 *awKeyNew = "1111111111111111";
    fileevecfg = fopen("/usr/share/eve.conf", "r");
    if(fileevecfg == NULL) {
        ERR_PRT("EVE cfg file is not exist! So use defualt config param.\n");
        sEVECtrl.classifierNum = 8;
        sEVECtrl.classifierPath[0].path = (AW_S8*)"/usr/share/classifier/frontface.ld";
        sEVECtrl.classifierPath[0].key = awKeyNew;
        sEVECtrl.classifierPath[1].path = (AW_S8*)"/usr/share/classifier/fullprofleftface.ld";
        sEVECtrl.classifierPath[1].key = awKeyNew;
        sEVECtrl.classifierPath[2].path = (AW_S8*)"/usr/share/classifier/fullprofrightface.ld";
        sEVECtrl.classifierPath[2].key = awKeyNew;
        sEVECtrl.classifierPath[3].path = (AW_S8*)"/usr/share/classifier/halfdownface.ld";
        sEVECtrl.classifierPath[3].key = awKeyNew;
        sEVECtrl.classifierPath[4].path = (AW_S8*)"/usr/share/classifier/profileface.ld";
        sEVECtrl.classifierPath[4].key = awKeyNew;
        sEVECtrl.classifierPath[5].path = (AW_S8*)"/usr/share/classifier/rotleftface.ld";
        sEVECtrl.classifierPath[5].key = awKeyNew;
        sEVECtrl.classifierPath[6].path = (AW_S8*)"/usr/share/classifier/rotrightface.ld";
        sEVECtrl.classifierPath[6].key = awKeyNew;
        sEVECtrl.classifierPath[7].path = (AW_S8*)"/usr/share/classifier/smallface.ld";
        sEVECtrl.classifierPath[7].key = awKeyNew;
        g_facedet_param.s32ClassifyFlag   = 0; //close
    } else {
        fscanf(fileevecfg, "%d", &sEVECtrl.classifierNum);
        DB_PRT("face clasifier num = %d\n", sEVECtrl.classifierNum);
        for(i = 0; i < sEVECtrl.classifierNum; i++) {
            sEVECtrl.classifierPath[i].path = (char *)malloc(256);
            fscanf(fileevecfg, "%s",  sEVECtrl.classifierPath[i].path);
            DB_PRT("classifier : %s\n", sEVECtrl.classifierPath[i].path);
            sEVECtrl.classifierPath[i].key = awKeyNew;
        }
        fscanf(fileevecfg, "%d\n", &iTemp);
        sEVECtrl.scale_factor = iTemp;
        fscanf(fileevecfg, "%d\n", &iTemp);
        sEVECtrl.mScanStageNo = iTemp;
        fscanf(fileevecfg, "%d\n", &iTemp);
        sEVECtrl.yStep = iTemp;
        fscanf(fileevecfg, "%d\n", &iTemp);
        sEVECtrl.xStep0 = iTemp;
        fscanf(fileevecfg, "%d\n", &iTemp);
        sEVECtrl.xStep1 = iTemp;
        fscanf(fileevecfg, "%d\n", &mT);
        fscanf(fileevecfg, "%d\n", &iTemp);
        sEVECtrl.mMidRltNum = iTemp;
        fscanf(fileevecfg, "%d\n", &iTemp);
        sEVECtrl.mMidRltStageNo = iTemp;
        fscanf(fileevecfg, "%d\n", &iTemp);
        g_facedet_param.s32ClassifyFlag = iTemp;

        DB_PRT("scale_factor = %d, mScanStageNo = %d, yStep = %d, xStep0 = %d, xStep1 = %d, MergerT = %d, mMidRltNum = %d, mMidRltStageNo = %d\n",
               sEVECtrl.scale_factor, sEVECtrl.mScanStageNo,sEVECtrl.yStep,sEVECtrl.xStep0,
               sEVECtrl.xStep1, mT, sEVECtrl.mMidRltNum, sEVECtrl.mMidRltStageNo);
    }

    sEVECtrl.dmaCallBackFunc   = &dmaCallBackFunc;
    //sEVECtrl.dma_pUsr = (void *)&privCap[vipp_dev][virvi_chn];

    g_eve_event = AW_AI_EVE_Event_Init(&sEVECtrl);
    if (AW_NULL == g_eve_event) {
        ERR_PRT("Do AW_AI_EVE_Event_Init fail!\n");
        return -1;
    }

    g_eve_image.mWidth       = vi_attr.format.width;
    g_eve_image.mHeight      = vi_attr.format.height;
    g_eve_image.mPixelFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    g_eve_image.mpVirAddr[0] = (AW_PVOID)malloc(vi_attr.format.width * vi_attr.format.height);
    g_eve_image.mpVirAddr[1] = (AW_PVOID)malloc(vi_attr.format.width * vi_attr.format.height / 2);
    g_eve_image.mpVirAddr[2] = 0;
    g_eve_image.mPhyAddr[0]  = 0;
    g_eve_image.mPhyAddr[1]  = 0;
    g_eve_image.mPhyAddr[2]  = 0;
    g_eve_image.mStride[0]   = vi_attr.format.width;
    g_eve_image.mStride[1]   = vi_attr.format.width;
    g_eve_image.mStride[2]   = 0;

    g_facedet_param.sRoiSet.s32RoiNum = 1;
    g_facedet_param.sRoiSet.sID[0] = 1;
    g_facedet_param.sRoiSet.sRoi[0].s16Left      = 5;
    g_facedet_param.sRoiSet.sRoi[0].s16Top       = 5;
    g_facedet_param.sRoiSet.sRoi[0].s16Right     = vi_attr.format.width  - 5;
    g_facedet_param.sRoiSet.sRoi[0].s16Bottom    = vi_attr.format.height - 5;
    //g_facedet_param.s32ClassifyFlag   = 0; //close
    g_facedet_param.s32MinFaceSize    = 20;
    g_facedet_param.s32OverLapCoeff   = 20;
    g_facedet_param.s32MergeThreshold = 3;
    g_facedet_param.s32MaxFaceNum = 128;
    g_facedet_param.s8Cfgfile         = "/usr/share/classifier/face_classifier_24X24.cfg";
    g_facedet_param.s8Weightfile      = "/usr/share/classifier/face_classifier_24X24.cweights";

    g_eve_start_proc = 0;
    g_eve_dmaflag    = 0;

    AW_AI_EVE_Event_SetEveDMAExecuteMode(g_eve_event, AW_AI_EVE_DMA_EXECUTE_SYNC);
    status = AW_AI_EVE_Event_SetEventParam(g_eve_event, AW_AI_EVE_EVENT_FACEDETECT, (void *)&g_facedet_param);
    if (AW_STATUS_OK != status) {
        ERR_PRT("Do AW_AI_EVE_Event_SetEventParam fail! status:%d \n", status);
        return -1;
    }

    return 0;
}


static int eve_destroy(void)
{
    int ret = 0;

    //ret = mpp_comm_venc_destroy(VENC_CHN_0);
    if (ret) {
        //ERR_PRT("Do mpp_comm_venc_destroy fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
    }
    return 0;
}


static int components_start(void)
{
    int ret = 0;

    ret = AW_MPI_VI_EnableVirChn(VI_DEV_1, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_EnableVirChn fail! ret:%d\n", ret);
        return -1;
    }
    ret = AW_MPI_VI_EnableVirChn(VI_DEV_3, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_EnableVirChn fail! ret:%d\n", ret);
        return -1;
    }

    ret = AW_MPI_VENC_StartRecvPic(VENC_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VENC_StartRecvPic fail! ret:%d\n", ret);
        return -1;
    }

    g_param_0.run_flag  = 1;
    g_param_0.vi_dev    = VI_DEV_1;
    g_param_0.vi_chn    = VI_CHN_0;
    ret = pthread_create(&g_param_0.proc_id, NULL, get_yuv_encode_proc, &g_param_0);
    if (ret) {
        ERR_PRT("Do pthread_create fail! ret:%d\n", ret);
    }

    ret = pthread_create(&g_eve_proc_id, NULL, get_yuv_eve_proc, NULL);
    if (ret) {
        ERR_PRT("Do pthread_create fail! ret:%d\n", ret);
    }


    ret = pthread_create(&g_facedet_proc_id, NULL, eve_facedet_proc, NULL);
    if (ret) {
        ERR_PRT("Do pthread_create fail! ret:%d\n", ret);
    }

    return 0;
}


static int components_stop(void)
{
    int ret = 0;

    ret = AW_MPI_VENC_StopRecvPic(VENC_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VENC_StopRecvPic fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
    }

    ret = AW_MPI_VI_DisableVirChn(VI_DEV_1, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
    }
    ret = AW_MPI_VI_DisableVirChn(VI_DEV_3, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
    }

    g_param_0.run_flag = 0;
    pthread_join(g_param_0.proc_id, 0);

    return 0;
}


#ifdef SAMPLE_MODE
int main(int argc, char **argv)
#else
int SampleViYuvVenc(void *pData, char *pTitle)
#endif
{
    int ret = 0;

    printf("\033[2J");
    printf("\n\n\nDo sample vi->yuv+venc+rtsp. default:VI_1080P@30fps+VENC_1080P@30fps H264/CBR/MainProfile\n");

    g_vi_type_0   = VI_1080P_30FPS;
    g_vi_type_1   = VI_VGA_30FPS;
    g_venc_type_0 = VENC_1080P_TO_1080P_8M_30FPS;

    /* Setup 0. Create rtsp */
    TinyServer *rtsp;
    ret = CreateRtspServer(&rtsp);
    if (ret) {
        ERR_PRT("Do CreateRtspServer fail! ret:%d \n", ret);
        return -1;
    }

    /* Step 1. Init mpp system */
    ret = mpp_comm_sys_init();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_init fail! ret:%d \n", ret);
        return -1;
    }

    /* Step 2. Get vi config to create 1080P/720P vi channel */
    ret = vi_create();
    if (ret) {
        ERR_PRT("Do vi_create fail! ret:%d\n", ret);
        goto _exit_1;
    }

    /* Step 3. Get venc config to create 1080P/720P venc channel */
    ret = venc_create();
    if (ret) {
        ERR_PRT("Do venc_create fail! ret:%d\n", ret);
        goto _exit_2;
    }

    /* Step 4. Get venc config to create 1080P/720P venc channel */
    ret = eve_create();
    if (ret) {
        ERR_PRT("Do eve_create fail! ret:%d\n", ret);
        goto _exit_3;
    }

    /* Setup 5. Start vi and venc */
    ret = components_start();
    if (ret) {
        ERR_PRT("Do components_start fail! ret:%d\n", ret);
        goto _exit_3;
    }

    /* Setup 6. Create thread, and get stream send to rtsp */
    rtsp_start(rtsp);

    /* Setup 7. Do this block menu control function */
    //MenuCtrl();
    while(1) sleep(1);

    rtsp_stop(rtsp);
    delete rtsp;

_exit_3:
    components_stop();

_exit_2:
    venc_destroy();

_exit_1:
    vi_destroy();

    ret = mpp_comm_sys_exit();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_exit fail! ret:%d \n", ret);
    }

    return ret;
}

