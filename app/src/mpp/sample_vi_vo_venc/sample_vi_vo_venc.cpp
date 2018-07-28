/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_vi_venc.cpp
 * @brief 该目录是对mpp中的 VI+(VENC+VO) 通路sample
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

#include "rtsp/MediaStream.h"
#include "rtsp/TinyServer.h"


/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
//#define ENABLE_VO_CLOCK
#define ENABLE_SELECT_SEND


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
struct vi_venc_vo_param {
    pthread_t       thd_id;
    int             venc_chn;
    int             run_flag;
    PAYLOAD_TYPE_E  venc_type;
    MediaStream    *stream;
};


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static int g_send_flag = 1;
static pthread_t g_thd_id;

static struct vi_venc_vo_param g_param_0;
static struct vi_venc_vo_param g_param_1;

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

static MPP_MENU_VI_VENC_CFG_E  g_vi_ve_cfg = VI_4K_25FPS_VE_4K_25FPS_VE_720P_25FPS;

static MENU_INODE g_menu_top[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"ISP  control",   NULL,   NULL, NULL},
    {(char *)"VI   control",   NULL,   NULL, NULL},
    {(char *)"VENC control",   NULL,   NULL, NULL},
    {(char *)"VO   control",   NULL,   NULL, NULL},
    {(char *)"Quite vi+(venc+rtsp)+vo sample",  ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};

static MENU_INODE g_menu_guide[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"Set VI+VENC scene     (Resolution,    default:4K@25fps+720P@25fps)",  mpp_menu_venc_scene_choose,     &g_vi_ve_cfg, NULL},
    {(char *)"Set VENC Payload Type (H264/H265,     default:H264)",                 mpp_menu_venc_payload_type_set, &g_venc_type, NULL},
    {(char *)"Set VENC RC Mode      (CBR/VBR/FIXQP, default:CBR)",                  mpp_menu_venc_rc_mode_set,      &g_rc_mode,   NULL},
    {(char *)"Set VENC Profile      (BL/MP/HP,      default:Main Profile)",         mpp_menu_venc_profile_set,      &g_profile,   NULL},
    {(char *)"Set VENC rotate       (0/90/180/270,  default:rotate_0)",             mpp_menu_venc_rotate_set,       &g_rotate,    NULL},
    {(char *)"Save confige and run this sample",   ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/

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

    ret = mpp_menu_vo_get(&g_menu_top[3].subMenu);
    if (ret) {
        ERR_PRT("Do mpp_menu_vo_get fail! ret:%d\n", ret);
    }

    RunMenuCtrl(g_menu_top);
}


static int ParseMenuParam(void)
{
    switch(g_vi_ve_cfg) {
    case VI_4K_30FPS_VE_4K_30FPS_VE_VGA_30FPS:
        g_vi_type_0   = VI_4K_30FPS;
        g_vi_type_1   = VI_VGA_30FPS;
        g_venc_type_0 = VENC_4K_TO_4K_20M_30FPS;
        g_venc_type_1 = VENC_VGA_TO_VGA_1M_30FPS;
        break;

    case VI_4K_25FPS_VE_4K_25FPS_VE_720P_25FPS:
        g_vi_type_0   = VI_4K_25FPS;
        g_vi_type_1   = VI_720P_25FPS;
        g_venc_type_0 = VENC_4K_TO_4K_18M_25FPS;
        g_venc_type_1 = VENC_720P_TO_720P_4M_25FPS;
        break;

    case VI_2K_30FPS_VE_2K_30FPS_VE_720P_30FPS:
        g_vi_type_0   = VI_2K_30FPS;
        g_vi_type_1   = VI_720P_30FPS;
        g_venc_type_0 = VENC_2K_TO_2K_16M_30FPS;
        g_venc_type_1 = VENC_720P_TO_720P_4M_30FPS;
        break;

    case VI_1080P_30FPS_VE_1080P_30FPS_VE_720P_30FPS:
        g_vi_type_0   = VI_1080P_30FPS;
        g_vi_type_1   = VI_720P_30FPS;
        g_venc_type_0 = VENC_1080P_TO_1080P_8M_30FPS;
        g_venc_type_1 = VENC_720P_TO_720P_4M_30FPS;
        break;

    case VI_2880x2160_30FPS_VE_2880x2160_30FPS_VE_1080P_30FPS:
        g_vi_type_0   = VI_2880x2160_30FPS;
        g_vi_type_1   = VI_1080P_30FPS;
        g_venc_type_0 = VENC_2880x2160_TO_2880x2160_12M_30FPS;
        g_venc_type_1 = VENC_1080P_TO_1080P_8M_30FPS;
        break;

    case VI_2592x1944_30FPS_VE_2592x1944_30FPS_VE_1080P_30FPS:
        g_vi_type_0   = VI_2592x1944_30FPS;
        g_vi_type_1   = VI_1080P_30FPS;
        g_venc_type_0 = VENC_2592x1944_TO_2592x1944_10M_30FPS;
        g_venc_type_1 = VENC_1080P_TO_1080P_8M_30FPS;
        break;

    default:
        ERR_PRT("Input g_vi_ve_cfg:%d error!\n", g_vi_ve_cfg);
        return -1;
        break;
    }

    return 0;
}


static ERRORTYPE MPPCallbackFunc(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    DB_PRT(" pChn:%d  event:%d \n", pChn->mModId, (int)event);

    return SUCCESS;
}

static void *SendStreamByRtsp(void *param)
{
    int            ret = 0;
    unsigned char *buf = NULL;
    unsigned int   len = 0;
    uint64_t       pts = 0;
    int            frame_type;
    VencHeaderData head_info;
    struct vi_venc_vo_param *local_param = (struct vi_venc_vo_param *)param;

    while (local_param->run_flag) {
        buf        = NULL;
        len        = 0;
        frame_type = -1;
        ret = mpp_comm_venc_get_stream(local_param->venc_chn, local_param->venc_type, -1, &buf, &len, &pts, &frame_type, &head_info);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_get_stream fail! ret:%d\n", ret);
            continue;
        }
        //DB_PRT("get stream  buf:%p  len:%d  frame_type:%d \n", buf, len, frame_type);

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

static void *SelectStreamSendByRtsp(void *param)
{
    int ret = 0, chn_cnt = 0;
    int time_out = 0;
    unsigned char *buf = NULL;
    unsigned int   len = 0;
    uint64_t       pts = 0;
    int            frame_type;
    VencHeaderData head_info;

    int hand_fd[VENC_MAX_CHN_NUM] = {0};
    handle_set rd_fds, bak_fds;
    PAYLOAD_TYPE_E venc_type[VENC_MAX_CHN_NUM];
    VENC_CHN_ATTR_S venc_attr;
    MediaStream *p_stream[VENC_MAX_CHN_NUM] = {NULL};
    MediaStream *rtsp_stream = NULL;

    AW_MPI_SYS_HANDLE_ZERO(&rd_fds);
    AW_MPI_SYS_HANDLE_ZERO(&bak_fds);

    hand_fd[0] = AW_MPI_VENC_GetHandle(VENC_CHN_0);
    AW_MPI_SYS_HANDLE_SET(hand_fd[0], &bak_fds);
    hand_fd[1] = AW_MPI_VENC_GetHandle(VENC_CHN_1);
    AW_MPI_SYS_HANDLE_SET(hand_fd[1], &bak_fds);

    AW_MPI_VENC_GetChnAttr(VENC_CHN_0, &venc_attr);
    venc_type[0] = venc_attr.VeAttr.Type;
    AW_MPI_VENC_GetChnAttr(VENC_CHN_1, &venc_attr);
    venc_type[1] = venc_attr.VeAttr.Type;

    p_stream[0] = g_stream_0;
    p_stream[1] = g_stream_1;
    p_stream[2] = NULL;

    time_out = 100; //100ms time out
    chn_cnt  = 2;

    while (g_send_flag) {
        rd_fds = bak_fds;

        ret = AW_MPI_SYS_HANDLE_Select(&rd_fds, time_out);
        if (ret == 0) {
            DB_PRT("Do AW_MPI_SYS_HANDLE_Select time_out:%d!\n", time_out);
            usleep(10*1000);
            continue;
        } else if (ret < 0) {
            ERR_PRT("Do AW_MPI_SYS_HANDLE_Select error! ret:%d  errno[%d] errinfo[%s]\n",
                    ret, errno, strerror(errno));
            continue;
        } else {
            for (int i = 0; i < chn_cnt; i++) {
                if (AW_MPI_SYS_HANDLE_ISSET(hand_fd[i], &rd_fds)) {
                    buf        = NULL;
                    len        = 0;
                    frame_type = -1;

                    ret = mpp_comm_venc_get_stream(i, venc_type[i], 70, &buf, &len, &pts, &frame_type, &head_info);
                    if (ret) {
                        ERR_PRT("Do mpp_comm_venc_get_stream fail! ret:%d\n", ret);
                        continue;
                    }
                    //DB_PRT("get stream  chn:%d  buf:%p  len:%d  frame_type:%d \n", i, buf, len, frame_type);
                    if (NULL == p_stream) {
                        continue;
                    }

                    rtsp_stream = p_stream[i];
                    if (1 == frame_type) {
                        /* Get I frame */
                        rtsp_stream->appendVideoData(head_info.pBuffer, head_info.nLength, pts, MediaStream::FRAME_DATA_TYPE_HEADER);
                        rtsp_stream->appendVideoData(buf, len, pts, MediaStream::FRAME_DATA_TYPE_I);
                    } else {
                        rtsp_stream->appendVideoData(buf, len, pts, MediaStream::FRAME_DATA_TYPE_P);
                    }
                }
            }
        }
    }

    DB_PRT("Out this function ... ... \n");
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

#ifdef ENABLE_SELECT_SEND
    ret = pthread_create(&g_thd_id, NULL, SelectStreamSendByRtsp, NULL);
    if (ret) {
        ERR_PRT("Do pthread_create SelectStreamSendByRtsp fail! ret:%d  errno[%d] errinfo[%s]\n",
                ret, errno, strerror(errno));
    }
#else
    g_param_0.stream    = g_stream_0;
    g_param_0.run_flag  = 1;
    g_param_0.venc_chn  = VENC_CHN_0;
    g_param_0.venc_type = g_venc_type;
    ret = pthread_create(&g_param_0.thd_id, NULL, SendStreamByRtsp, &g_param_0);
    if (ret) {
        ERR_PRT("Do pthread_create fail! ret:%d\n", ret);
    }

    g_param_1.stream    = g_stream_1;
    g_param_1.run_flag  = 1;
    g_param_1.venc_chn  = VENC_CHN_1;
    g_param_1.venc_type = g_venc_type;
    ret = pthread_create(&g_param_1.thd_id, NULL, SendStreamByRtsp, &g_param_1);
    if (ret) {
        ERR_PRT("Do pthread_create fail! ret:%d\n", ret);
    }
#endif

    return 0;
}


static void rtsp_stop(TinyServer *rtsp)
{
#ifdef ENABLE_SELECT_SEND
    g_send_flag = 0;
    usleep(10*1000);

    rtsp->stop();
    pthread_join(g_thd_id, 0);
    delete g_param_0.stream;
    delete g_param_1.stream;
#else
    g_param_0.run_flag = 0;
    g_param_1.run_flag = 0;
    usleep(10*1000);

    rtsp->stop();
    pthread_join(g_param_0.thd_id, 0);
    delete g_param_0.stream;

    pthread_join(g_param_1.thd_id, 0);
    delete g_param_1.stream;
#endif
}


static int vi_create(void)
{
    int ret = 0;
    VI_ATTR_S vi_attr;

    /**  create vi dev 2  src 1080P **/
    ret = mpp_comm_vi_get_attr(g_vi_type_0, &vi_attr);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_get_attr fail! ret:%d \n", ret);
        return -1;
    }
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

    /**  create vi dev 3  src 720P **/
    ret = mpp_comm_vi_get_attr(g_vi_type_1, &vi_attr);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_get_attr fail! ret:%d \n", ret);
        return -1;
    }
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
    ret = mpp_comm_vi_chn_create(VI_DEV_3, VI_CHN_1);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_chn_create fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_3, VI_CHN_1);
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

    /**  destroy vi dev 2  **/
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
    ret = mpp_comm_vi_chn_destroy(VI_DEV_3, VI_CHN_1);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_chn_destroy fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_1, VI_CHN_1);
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

    ret = mpp_comm_venc_get_cfg(g_venc_type_1, &venc_cfg);
    if (ret) {
        ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
        return -1;
    }
    ret = mpp_comm_venc_create(VENC_CHN_1, g_venc_type, g_rc_mode, g_profile, g_rotate, &venc_cfg);
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
    ret = mpp_comm_venc_destroy(VENC_CHN_1);
    if (ret) {
        ERR_PRT("Do mpp_comm_venc_destroy fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_1);
    }

    return 0;
}


static int vo_create(void)
{
    int ret = 0;
    VO_DEV_TYPE_E vo_type;
    VO_DEV_CFG_S  vo_cfg;
    vo_type           = VO_DEV_HDMI;
    vo_cfg.res_width  = 1920;
    vo_cfg.res_height = 1080;
    ret = mpp_comm_vo_dev_create(vo_type, &vo_cfg);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_dev_create fail! ret:%d  vo_type:%d\n", ret, vo_type);
        return -1;
    }

    VO_CHN_CFG_S vo_chn_cfg;
    vo_chn_cfg.top        = 0;
    vo_chn_cfg.left       = 0;
    vo_chn_cfg.width      = 1920;
    vo_chn_cfg.height     = 1080;
    vo_chn_cfg.vo_buf_num = 0;
    ret = mpp_comm_vo_chn_create(VO_CHN_0, &vo_chn_cfg);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_chn_create fail! ret:%d  vo_chn:%d\n", ret, VO_CHN_0);
        return -1;
    }

#ifdef ENABLE_VO_CLOCK
    /* Create Clock for vo mode */
    ret = mpp_comm_vo_clock_create(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_clock_create fail! ret:%d  clock_chn:%d\n", ret, CLOCK_CHN_0);
        return -1;
    }
#endif

    return 0;
}

static int vo_destroy(void)
{
    int ret = 0;

#ifdef ENABLE_VO_CLOCK
    ret = mpp_comm_vo_clock_destroy(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_clock_destroy fail! ret:%d  clock_chn:%d\n", ret, CLOCK_CHN_0);
    }
#endif

    ret = mpp_comm_vo_chn_destroy(VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_chn_destroy fail! ret:%d  vo_chn:%d\n", ret, VO_CHN_0);
    }

    ret = mpp_comm_vo_dev_destroy(VO_DEV_LCD);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_dev_destroy fail! ret:%d  vo_type:%d\n", ret, VO_DEV_LCD);
    }

    return 0;
}


static int components_bind(void)
{
    int ret = 0;

    ret = mpp_comm_vi_bind_venc(VI_DEV_1, VI_CHN_0, VENC_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_bind_venc fail! ret:%d \n", ret);
        return -1;
    }
    ret = mpp_comm_vi_bind_venc(VI_DEV_3, VI_CHN_0, VENC_CHN_1);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_bind_venc fail! ret:%d \n", ret);
        return -1;
    }

#ifdef ENABLE_VO_CLOCK
    ret = mpp_comm_vo_bind_clock(CLOCK_CHN_0, VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_bind_clock fail! ret:%d clock_chn:%d vo_chn:%d\n", ret, CLOCK_CHN_0, VO_CHN_0);
        return -1;
    }
#endif

    ret = mpp_comm_vi_bind_vo(VI_DEV_3, VI_CHN_1, VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_bind_vo fail! ret:%d \n", ret);
        return -1;
    }

    return 0;
}


static int components_unbind(void)
{
    int ret = 0;

    ret = mpp_comm_vi_unbind_venc(VI_DEV_1, VI_CHN_0, VENC_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_unbind_venc fail! ret:%d\n", ret);
    }
    ret = mpp_comm_vi_unbind_venc(VI_DEV_3, VI_CHN_0, VENC_CHN_1);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_unbind_venc fail! ret:%d\n", ret);
    }

    ret = mpp_comm_vi_unbind_vo(VI_DEV_3, VI_CHN_1, VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_unbind_vo fail! ret:%d\n", ret);
    }

#ifdef ENABLE_VO_CLOCK
    ret = mpp_comm_vo_unbind_clock(CLOCK_CHN_0, VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_unbind_clock fail! ret:%d clock_chn:%d vo_chn:%d\n", ret, CLOCK_CHN_0, VO_CHN_0);
    }
#endif

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
    ret = AW_MPI_VI_EnableVirChn(VI_DEV_3, VI_CHN_1);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_EnableVirChn fail! ret:%d\n", ret);
        return -1;
    }

    ret = AW_MPI_VENC_StartRecvPic(VENC_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VENC_StartRecvPic fail! ret:%d\n", ret);
        return -1;
    }
    ret = AW_MPI_VENC_StartRecvPic(VENC_CHN_1);
    if (ret) {
        ERR_PRT("Do AW_MPI_VENC_StartRecvPic fail! ret:%d\n", ret);
        return -1;
    }

#ifdef ENABLE_VO_CLOCK
    ret = AW_MPI_CLOCK_Start(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_CLOCK_Start fail! clock_chn:%d  ret:%d\n", CLOCK_CHN_0, ret);
        return -1;
    }
#endif

    ret = mpp_comm_vo_chn_start(VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_chn_start vo_chn:%d fail! ret:%d\n", VO_CHN_0, ret);
        return -1;
    }

    return 0;
}


static int components_stop(void)
{
    int ret = 0;

    ret = mpp_comm_vo_chn_stop(VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_chn_stop vo_chn:%d fail! ret:%d\n", VO_CHN_0, ret);
    }

#ifdef ENABLE_VO_CLOCK
    ret = AW_MPI_CLOCK_Stop(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_CLOCK_Stop fail! clock_chn:%d  ret:%d\n", CLOCK_CHN_0, ret);
    }
#endif

    ret = AW_MPI_VENC_StopRecvPic(VENC_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VENC_StopRecvPic fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
    }
    ret = AW_MPI_VENC_StopRecvPic(VENC_CHN_1);
    if (ret) {
        ERR_PRT("Do AW_MPI_VENC_StopRecvPic fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_1);
    }

    ret = AW_MPI_VI_DisableVirChn(VI_DEV_1, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
    }
    ret = AW_MPI_VI_DisableVirChn(VI_DEV_3, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
    }
    ret = AW_MPI_VI_DisableVirChn(VI_DEV_3, VI_CHN_1);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
    }

    return 0;
}


#ifdef SAMPLE_MODE
int main(int argc, char **argv)
#else
int SampleViVoVenc(void *pData, char *pTitle)
#endif
{
    int ret = 0;

    printf("\033[2J");
    printf("\n\n\nDo sample vi+(venc+rtsp)+vo. default:VI_4K@25fps+VENC(4K@25fps+720P@25fps) H264/CBR/MainProfile\n");
    RunMenuCtrl(g_menu_guide);

    ParseMenuParam();

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

    /* Step 4. Create vo device and layer */
    ret = vo_create();
    if (ret) {
        ERR_PRT("Do vo_create fail! ret:%d \n", ret);
        goto _exit_3;
    }

    /* Step 5. Bind clock to vo,  and vi to vo, vi to venc */
    ret = components_bind();
    if (ret) {
        ERR_PRT("Do components_bind fail! ret:%d \n", ret);
        goto _exit_4;
    }

    /* Setup 6. Start  vi, venc, clock, vo */
    ret = components_start();
    if (ret) {
        ERR_PRT("Do components_start fail! ret:%d \n", ret);
        goto _exit_5;
    }

    /* Setup 7. Create thread, and get stream send to rtsp */
    rtsp_start(rtsp);

    /* Setup 7. Do this block menu control function */
    MenuCtrl();

    rtsp_stop(rtsp);
    delete rtsp;

_exit_5:
    components_stop();

_exit_4:
    components_unbind();

_exit_3:
    vo_destroy();

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

