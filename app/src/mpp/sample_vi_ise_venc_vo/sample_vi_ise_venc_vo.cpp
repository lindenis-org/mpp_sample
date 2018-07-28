/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file sample_vi_ise_venc_vo.cpp
 * @brief 该目录是对mpp中的 VI+ISE+VENC+VO 通路sample
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
#if 0
typedef enum tag_SAMPLE_ISE_MODE_E {
    ISE_ONE_FISH_180 = 0,
    ISE_ONE_FISH_360,
    ISE_ONE_FISH_PTZ_4CH,
    ISE_ONE_FISH_PTZ_2CH,

    ISE_TWO_FISH_360,

    ISE_TWO_ISE_90,
    ISE_TWO_ISE_100,
    ISE_TWO_ISE_110,

    ISE_MODE_BOTTON,
} SAMPLE_ISE_MODE_E;
#endif

/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
struct vi_venc_param {
    pthread_t       thd_id;
    int             venc_chn;
    int             run_flag;
    PAYLOAD_TYPE_E  venc_type;
    MediaStream    *stream;
};


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static struct vi_venc_param g_param_0;
static struct vi_venc_param g_param_1;
static struct vi_venc_param g_param_2;
static struct vi_venc_param g_param_3;
static struct vi_venc_param g_param_4;

static MediaStream *g_stream_0 = NULL;
static MediaStream *g_stream_1 = NULL;
static MediaStream *g_stream_2 = NULL;
static MediaStream *g_stream_3 = NULL;
static MediaStream *g_stream_4 = NULL;

static unsigned int   g_rc_mode   = 1;          /* 0:CBR 1:VBR 2:FIXQp 3:ABR 4:QPMAP */
static unsigned int   g_profile   = 1;          /* 0: baseline  1:MP  2:HP  3: SVC-T [0,3] */
static PAYLOAD_TYPE_E g_venc_type = PT_H264;    /* PT_H264/PT_H265/PT_MJPEG */
static ROTATE_E       g_rotate    = ROTATE_NONE;

static MPP_MENU_ISE_CFG_E  g_ise_menu = MENU_ISE_FISH_360_MODE_4096x1024;
static ISE_MODE_CFG_E      g_ise_cfg  = ISE_FISH_360_MODE_2048_TO_4096x1024;


static MENU_INODE g_menu_top[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"ISP  control",   NULL,   NULL, NULL},
    {(char *)"VI   control",   NULL,   NULL, NULL},
    {(char *)"VENC control",   NULL,   NULL, NULL},
    {(char *)"VO   control",   NULL,   NULL, NULL},
    {(char *)"Quite vi+ise+(venc+rtsp)+vo sample",  ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};

static MENU_INODE g_menu_guide[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"Set ISE mode          (default:one fish 360)", mpp_menu_ise_mode_select, &g_ise_menu, NULL},
    {(char *)"Set VENC Payload Type (default:H264)",  mpp_menu_venc_payload_type_set, &g_venc_type, NULL},
    {(char *)"Set VENC RC Mode      (default:VBR)",   mpp_menu_venc_rc_mode_set,      &g_rc_mode,   NULL},
    {(char *)"Set VENC Profile      (default:MP)",    mpp_menu_venc_profile_set,      &g_profile,   NULL},
    {(char *)"Set VENC rotate       (default:rotate_0)",    mpp_menu_venc_rotate_set, &g_rotate,    NULL},
    {(char *)"Save confige and run this sample",      ExitCurrentMenuLevel, NULL, NULL},
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
    switch(g_ise_menu) {
    case MENU_ISE_FISH_180_MODE_2048x2048:
        g_ise_cfg = ISE_FISH_180_MODE_2048_TO_2048x2048;
        break;

    case MENU_ISE_FISH_360_MODE_4096x1024:
        g_ise_cfg = ISE_FISH_360_MODE_2048_TO_4096x1024;
        break;

    case MENU_ISE_FISH_BOTTOM_4PTZ_MODE_1024x1024:
        g_ise_cfg = ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024;
        break;

    case MENU_ISE_TWO_FISH_360_MODE_3840x1920:
        g_ise_cfg = ISE_TWO_FISH_360_MODE_1920_TO_3840x1920;
        break;

    case MENU_ISE_TWO_ISE_90_MODE_3840x1080:
        g_ise_cfg = ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080;
        break;

    default:
        ERR_PRT("Input g_ise_menu:%d error!\n", g_ise_menu);
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

static int vi_create(ISE_MODE_CFG_E ise_cfg)
{
    int ret    = 0;
    int vi_chn = 0;
    VI_ATTR_S vi_attr;

    switch (ise_cfg) {
    case ISE_FISH_180_MODE_2048_TO_1024x1024:
    case ISE_FISH_180_MODE_2048_TO_2048x2048:
    case ISE_FISH_360_MODE_2048_TO_4096x1024:
    case ISE_FISH_360_MODE_2048_TO_2048x512: {
        /**  create vi dev 1  src 2048x2048 for ISE **/
        ret = mpp_comm_vi_get_attr(VI_2048x2048_30FPS, &vi_attr);
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
        /**  create vi dev 3  src 1024x1024 for (VENC + VO) **/
        ret = mpp_comm_vi_get_attr(VI_1024x1024_30FPS, &vi_attr);
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

        AW_MPI_ISP_Init();
        AW_MPI_ISP_Run(ISP_DEV_0);
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        /**  create vi dev 1  src 2048x2048 for Fish PTZ **/
        ret = mpp_comm_vi_get_attr(VI_2048x2048_30FPS, &vi_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_get_attr fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_vi_dev_create(VI_DEV_1, &vi_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_dev_create fail! ret:%d  vi_dev:%d\n", ret, VI_DEV_1);
            return -1;
        }
        for (vi_chn = 0; vi_chn < 4; vi_chn++) {
            ret = mpp_comm_vi_chn_create(VI_DEV_1, vi_chn);
            if (ret) {
                ERR_PRT("Do mpp_comm_vi_chn_create fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_1, vi_chn);
                return -1;
            }
        }

        /**  create vi dev 3  src 1024x1024 for (VENC + VO) **/
        ret = mpp_comm_vi_get_attr(VI_1024x1024_30FPS, &vi_attr);
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

        AW_MPI_ISP_Init();
        AW_MPI_ISP_Run(ISP_DEV_0);
    }
    break;

    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920: {
        ret = mpp_comm_vi_get_attr(VI_1920x1920_25FPS, &vi_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_get_attr fail! ret:%d \n", ret);
            return -1;
        }

        ret = mpp_comm_vi_dev_create(VI_DEV_0, &vi_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_dev_create fail! ret:%d  vi_dev:%d\n", ret, VI_DEV_0);
            return -1;
        }
        ret = mpp_comm_vi_chn_create(VI_DEV_0, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_chn_create fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_1, vi_chn);
            return -1;
        }

        ret = mpp_comm_vi_dev_create(VI_DEV_1, &vi_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_dev_create fail! ret:%d  vi_dev:%d\n", ret, VI_DEV_1);
            return -1;
        }
        ret = mpp_comm_vi_chn_create(VI_DEV_1, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_chn_create fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_1, vi_chn);
            return -1;
        }

        AW_MPI_ISP_Init();
        AW_MPI_ISP_Run(ISP_DEV_0);
        AW_MPI_ISP_Run(ISP_DEV_1);
    }
    break;

    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
        ret = mpp_comm_vi_get_attr(VI_1080P_25FPS, &vi_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_get_attr fail! ret:%d \n", ret);
            return -1;
        }

        ret = mpp_comm_vi_dev_create(VI_DEV_0, &vi_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_dev_create fail! ret:%d  vi_dev:%d\n", ret, VI_DEV_0);
            return -1;
        }
        ret = mpp_comm_vi_chn_create(VI_DEV_0, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_chn_create fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_1, vi_chn);
            return -1;
        }

        ret = mpp_comm_vi_dev_create(VI_DEV_1, &vi_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_dev_create fail! ret:%d  vi_dev:%d\n", ret, VI_DEV_1);
            return -1;
        }
        ret = mpp_comm_vi_chn_create(VI_DEV_1, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_chn_create fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_1, vi_chn);
            return -1;
        }

        AW_MPI_ISP_Init();
        AW_MPI_ISP_Run(ISP_DEV_0);
        AW_MPI_ISP_Run(ISP_DEV_1);
    }
    break;

    default:
        ERR_PRT("Input ise_cfg:%d don't supoort!\n", ise_cfg);
        return -1;
        break;
    }

    return 0;
}


static int vi_destroy(ISE_MODE_CFG_E ise_cfg)
{
    int ret    = 0;
    int vi_chn = 0;

    switch (ise_cfg) {
    case ISE_FISH_180_MODE_2048_TO_1024x1024:
    case ISE_FISH_180_MODE_2048_TO_2048x2048:
    case ISE_FISH_360_MODE_2048_TO_4096x1024:
    case ISE_FISH_360_MODE_2048_TO_2048x512: {
        ret = mpp_comm_vi_chn_destroy(VI_DEV_1, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_chn_destroy fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_1, VI_CHN_0);
        }
        ret = mpp_comm_vi_chn_destroy(VI_DEV_3, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_chn_destroy fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_3, VI_CHN_0);
        }
        ret = mpp_comm_vi_chn_destroy(VI_DEV_3, VI_CHN_1);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_chn_destroy fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_3, VI_CHN_1);
        }

        ret = mpp_comm_vi_dev_destroy(VI_DEV_1);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_dev_destroy fail! ret:%d  vi_dev:%d \n", ret, VI_DEV_1);
        }
        ret = mpp_comm_vi_dev_destroy(VI_DEV_3);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_dev_destroy fail! ret:%d  vi_dev:%d \n", ret, VI_DEV_3);
        }

        AW_MPI_ISP_Stop(ISP_DEV_0);
        AW_MPI_ISP_Exit();
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        /**  destroy vi dev 2  **/
        for (vi_chn = 0; vi_chn < 4; vi_chn++) {
            ret = mpp_comm_vi_chn_destroy(VI_DEV_1, vi_chn);
            if (ret) {
                ERR_PRT("Do mpp_comm_vi_chn_destroy fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_1, vi_chn);
            }
        }

        ret = mpp_comm_vi_chn_destroy(VI_DEV_3, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_chn_destroy fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_3, VI_CHN_0);
        }
        ret = mpp_comm_vi_chn_destroy(VI_DEV_3, VI_CHN_1);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_chn_destroy fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_3, VI_CHN_1);
        }
        ret = mpp_comm_vi_dev_destroy(VI_DEV_1);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_dev_destroy fail! ret:%d  vi_dev:%d \n", ret, VI_DEV_1);
        }
        ret = mpp_comm_vi_dev_destroy(VI_DEV_3);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_dev_destroy fail! ret:%d  vi_dev:%d \n", ret, VI_DEV_3);
        }

        AW_MPI_ISP_Stop(ISP_DEV_0);
        AW_MPI_ISP_Exit();
    }
    break;

    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920:
    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
        ret = mpp_comm_vi_chn_destroy(VI_DEV_0, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_chn_destroy fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_0, VI_CHN_0);
        }
        ret = mpp_comm_vi_chn_destroy(VI_DEV_1, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_chn_destroy fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_1, VI_CHN_0);
        }

        ret = mpp_comm_vi_dev_destroy(VI_DEV_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_dev_destroy fail! ret:%d  vi_dev:%d \n", ret, VI_DEV_0);
        }
        ret = mpp_comm_vi_dev_destroy(VI_DEV_1);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_dev_destroy fail! ret:%d  vi_dev:%d \n", ret, VI_DEV_1);
        }

        AW_MPI_ISP_Stop(ISP_DEV_0);
        AW_MPI_ISP_Stop(ISP_DEV_1);
        AW_MPI_ISP_Exit();
    }
    break;

    default:
        ERR_PRT("Input ise_cfg:%d don't supoort!\n", ise_cfg);
        break;
    }

    return 0;
}


/* ise_cfg: output  */
static int ise_create(ISE_MODE_CFG_E ise_cfg)
{
    int ret = 0;
    int ise_grp = 0;
    ISE_CHN_ATTR_S chn_attr;

    switch(ise_cfg) {
    case ISE_FISH_180_MODE_2048_TO_1024x1024:
    case ISE_FISH_180_MODE_2048_TO_2048x2048:
    case ISE_FISH_360_MODE_2048_TO_4096x1024:
    case ISE_FISH_360_MODE_2048_TO_2048x512: {
        ret = mpp_comm_ise_grp_create(ISE_GRP_0, ISEMODE_ONE_FISHEYE);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_grp_create fail! ret:%d  ise_grp:%d \n", ret, ISE_GRP_0);
            return -1;
        }
        /* ISE_FISH_180_MODE_2048_TO_2048x2048  or  ISE_FISH_180_MODE_2048_TO_1024x1024 */
        ret = mpp_comm_ise_get_cfg(ise_cfg, 1, &chn_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_get_cfg fail! ret:%d  g_ise_cfg:%d \n", ret, g_ise_cfg);
            return -1;
        }
        ret = mpp_comm_ise_chn_create(ISE_GRP_0, ISE_CHN_0, &chn_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_chn_create fail! ret:%d  ise_grp:%d  ise_chn:%d\n", ret, ISE_GRP_0, ISE_CHN_0);
            return -1;
        }
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        /* Very fish_PTZ mode, have one grp and two out_chn: one for venc, and other for vo display. */
        for (ise_grp = 0; ise_grp < 4; ise_grp++) {
            switch(ise_grp) {
            case 0:
                ise_cfg = ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024;
                break;
            case 1:
                ise_cfg = ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024;
                break;
            case 2:
                ise_cfg = ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024;
                break;
            case 3:
                ise_cfg = ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024;
                break;
            }
            ret = mpp_comm_ise_grp_create(ise_grp, ISEMODE_ONE_FISHEYE);
            if (ret) {
                ERR_PRT("Do mpp_comm_ise_grp_create fail! ret:%d  ise_grp:%d \n", ret, ISE_GRP_0);
                return -1;
            }
            ret = mpp_comm_ise_get_cfg(ise_cfg, 1, &chn_attr);
            if (ret) {
                ERR_PRT("Do mpp_comm_ise_get_cfg fail! ret:%d  g_ise_cfg:%d \n", ret, g_ise_cfg);
                return -1;
            }
            ret = mpp_comm_ise_chn_create(ise_grp, ISE_CHN_0, &chn_attr);
            if (ret) {
                ERR_PRT("Do mpp_comm_ise_chn_create fail! ret:%d  ise_grp:%d  ise_chn:%d\n", ret, ise_grp, ISE_CHN_0);
                return -1;
            }
        }
    }
    break;

    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920: {
        ret = mpp_comm_ise_grp_create(ISE_GRP_0, ISEMODE_TWO_FISHEYE);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_grp_create fail! ret:%d  ise_grp:%d \n", ret, ISE_GRP_0);
            return -1;
        }
        ret = mpp_comm_ise_get_cfg(ise_cfg, 2, &chn_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_get_cfg fail! ret:%d  g_ise_cfg:%d \n", ret, g_ise_cfg);
            return -1;
        }
        ret = mpp_comm_ise_chn_create(ISE_GRP_0, ISE_CHN_0, &chn_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_chn_create fail! ret:%d  ise_grp:%d  ise_chn:%d\n", ret, ISE_GRP_0, ISE_CHN_0);
            return -1;
        }

        ret = mpp_comm_ise_chn_create(ISE_GRP_0, ISE_CHN_1, &chn_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_chn_create fail! ret:%d  ise_grp:%d  ise_chn:%d\n", ret, ISE_GRP_0, ISE_CHN_1);
            return -1;
        }
    }
    break;

    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
        ret = mpp_comm_ise_grp_create(ISE_GRP_0, ISEMODE_TWO_ISE);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_grp_create fail! ret:%d  ise_grp:%d \n", ret, ISE_GRP_0);
            return -1;
        }
        ret = mpp_comm_ise_get_cfg(ise_cfg, 2, &chn_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_get_cfg fail! ret:%d  g_ise_cfg:%d \n", ret, g_ise_cfg);
            return -1;
        }
        ret = mpp_comm_ise_chn_create(ISE_GRP_0, ISE_CHN_0, &chn_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_chn_create fail! ret:%d  ise_grp:%d  ise_chn:%d\n", ret, ISE_GRP_0, ISE_CHN_0);
            return -1;
        }
        ret = mpp_comm_ise_chn_create(ISE_GRP_0, ISE_CHN_1, &chn_attr);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_chn_create fail! ret:%d  ise_grp:%d  ise_chn:%d\n", ret, ISE_GRP_0, ISE_CHN_1);
            return -1;
        }
    }
    break;

    default:
        ERR_PRT("Input ise_cfg:%d don't supoort!\n", ise_cfg);
        return -1;
        break;
    }

    return 0;
}

static int ise_destroy(ISE_MODE_CFG_E ise_cfg)
{
    int ret = 0;
    int ise_grp = 0;

    switch(ise_cfg) {
    case ISE_FISH_180_MODE_2048_TO_1024x1024:
    case ISE_FISH_180_MODE_2048_TO_2048x2048:
    case ISE_FISH_360_MODE_2048_TO_4096x1024:
    case ISE_FISH_360_MODE_2048_TO_2048x512: {
        ret = mpp_comm_ise_chn_destroy(ISE_GRP_0, ISE_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_chn_destroy fail! ret:%d  ise_grp:%d  ise_chn:%d\n", ret, ISE_GRP_0, ISE_CHN_0);
        }

        ret = mpp_comm_ise_grp_destroy(ISE_GRP_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_grp_destroy fail! ret:%d  ise_grp:%d \n", ret, ISE_GRP_0);
        }
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        for (ise_grp = 0; ise_grp < 4; ise_grp++) {
            ret = mpp_comm_ise_chn_destroy(ise_grp, ISE_CHN_0);
            if (ret) {
                ERR_PRT("Do mpp_comm_ise_chn_destroy fail! ret:%d  ise_grp:%d  ise_chn:%d\n", ret, ise_grp, ISE_CHN_0);
            }
            ret = mpp_comm_ise_grp_destroy(ise_grp);
            if (ret) {
                ERR_PRT("Do mpp_comm_ise_grp_destroy fail! ret:%d  ise_grp:%d \n", ret, ise_grp);
            }
        }
    }
    break;

    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920:
    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
        ret = mpp_comm_ise_chn_destroy(ISE_GRP_0, ISE_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_chn_destroy fail! ret:%d  ise_grp:%d  ise_chn:%d\n", ret, ISE_GRP_0, ISE_CHN_0);
        }
        ret = mpp_comm_ise_chn_destroy(ISE_GRP_0, ISE_CHN_1);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_chn_destroy fail! ret:%d  ise_grp:%d  ise_chn:%d\n", ret, ISE_GRP_0, ISE_CHN_1);
        }

        ret = mpp_comm_ise_grp_destroy(ISE_GRP_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_grp_destroy fail! ret:%d  ise_grp:%d \n", ret, ISE_GRP_0);
        }
    }
    break;

    default:
        ERR_PRT("Input ise_cfg:%d don't supoort!\n", ise_cfg);
        break;
    }

    return 0;
}


static int venc_create(ISE_MODE_CFG_E ise_cfg)
{
    int ret = 0;
    int venc_chn = 0;
    VENC_CFG_S   venc_cfg = {0};

    switch(ise_cfg) {
    case ISE_FISH_180_MODE_2048_TO_2048x2048: {
        ret = mpp_comm_venc_get_cfg(VENC_2048x2048_TO_2048x2048_10M_30FPS, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_venc_create(VENC_CHN_0, g_venc_type, g_rc_mode, g_profile, g_rotate, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_create fail! ret:%d \n", ret);
            return -1;
        }

        ret = mpp_comm_venc_get_cfg(VENC_1024x1024_TO_1024x1024_4M_30FPS, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_venc_create(VENC_CHN_1, g_venc_type, g_rc_mode, g_profile, g_rotate, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_create fail! ret:%d \n", ret);
            return -1;
        }
    }
    break;

    case ISE_FISH_360_MODE_2048_TO_4096x1024: {
        ret = mpp_comm_venc_get_cfg(VENC_4096x1024_TO_4096x1024_10M_30FPS, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_venc_create(VENC_CHN_0, g_venc_type, g_rc_mode, g_profile, g_rotate, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_create fail! ret:%d \n", ret);
            return -1;
        }

        ret = mpp_comm_venc_get_cfg(VENC_1024x1024_TO_1024x1024_4M_30FPS, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_venc_create(VENC_CHN_1, g_venc_type, g_rc_mode, g_profile, g_rotate, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_create fail! ret:%d \n", ret);
            return -1;
        }
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        ret = mpp_comm_venc_get_cfg(VENC_1024x1024_TO_1024x1024_4M_30FPS, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
            return -1;
        }
        for (venc_chn = 0; venc_chn < 5; venc_chn++) {
            ret = mpp_comm_venc_create(venc_chn, g_venc_type, g_rc_mode, g_profile, g_rotate, &venc_cfg);
            if (ret) {
                ERR_PRT("Do mpp_comm_venc_create fail! venc_chn:%d ret:%d \n", venc_chn, ret);
                return -1;
            }
        }
    }
    break;

    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920: {
        ret = mpp_comm_venc_get_cfg(VENC_3840x1920_TO_3840x1920_8M_15FPS, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_venc_create(VENC_CHN_0, g_venc_type, g_rc_mode, g_profile, g_rotate, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_create fail! ret:%d \n", ret);
            return -1;
        }

        ret = mpp_comm_venc_get_cfg(VENC_1280x640_TO_1280x640_4M_25FPS, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_venc_create(VENC_CHN_1, g_venc_type, g_rc_mode, g_profile, g_rotate, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_create fail! ret:%d \n", ret);
            return -1;
        }
    }
    break;

    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
        ret = mpp_comm_venc_get_cfg(VENC_3840x1080_TO_3840x1080_8M_25FPS, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_venc_create(VENC_CHN_0, g_venc_type, g_rc_mode, g_profile, g_rotate, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_create fail! ret:%d \n", ret);
            return -1;
        }

        ret = mpp_comm_venc_get_cfg(VENC_1920x540_TO_1920x540_4M_25FPS, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_venc_create(VENC_CHN_1, g_venc_type, g_rc_mode, g_profile, g_rotate, &venc_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_create fail! ret:%d \n", ret);
            return -1;
        }
    }
    break;

    default:
        ERR_PRT("Input ise_cfg:%d don't supoort!\n", ise_cfg);
        break;
    }

    /* Register Venc call back function */
    MPPCallbackInfo venc_cb;
    venc_cb.cookie   = NULL;
    venc_cb.callback = (MPPCallbackFuncType)&MPPCallbackFunc;
    AW_MPI_VENC_RegisterCallback(VENC_CHN_0, &venc_cb);

    return 0;
}

static int venc_destroy(ISE_MODE_CFG_E ise_cfg)
{
    int ret = 0;
    int venc_chn = 0;

    switch(ise_cfg) {
    case ISE_FISH_180_MODE_2048_TO_1024x1024:
    case ISE_FISH_180_MODE_2048_TO_2048x2048:
    case ISE_FISH_360_MODE_2048_TO_4096x1024:
    case ISE_FISH_360_MODE_2048_TO_2048x512:
    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920:
    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
        ret = mpp_comm_venc_destroy(VENC_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_destroy fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
        }
        ret = mpp_comm_venc_destroy(VENC_CHN_1);
        if (ret) {
            ERR_PRT("Do mpp_comm_venc_destroy fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_1);
        }
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        for (venc_chn = 0; venc_chn < 5; venc_chn++) {
            ret = mpp_comm_venc_destroy(venc_chn);
            if (ret) {
                ERR_PRT("Do mpp_comm_venc_destroy fail! ret:%d  venc_chn:%d \n", ret, venc_chn);
            }
        }
    }
    break;

    default:
        ERR_PRT("Input ise_cfg:%d don't supoort!\n", ise_cfg);
        break;
    }

    return 0;
}


static int vo_create(ISE_MODE_CFG_E ise_cfg)
{
    int ret = 0;
    VO_DEV_TYPE_E vo_type;
    VO_DEV_CFG_S  vo_cfg;
    VO_CHN_CFG_S  vo_chn_cfg;

    vo_type           = VO_DEV_LCD;
    vo_cfg.res_width  = 720;
    vo_cfg.res_height = 1280;
    ret = mpp_comm_vo_dev_create(vo_type, &vo_cfg);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_dev_create fail! ret:%d  vo_type:%d\n", ret, vo_type);
        return -1;
    }

    switch(ise_cfg) {
    case ISE_FISH_180_MODE_2048_TO_1024x1024:
    case ISE_FISH_180_MODE_2048_TO_2048x2048:
    case ISE_FISH_360_MODE_2048_TO_4096x1024:
    case ISE_FISH_360_MODE_2048_TO_2048x512: {
        vo_chn_cfg.top        = 0;
        vo_chn_cfg.left       = 0;
        vo_chn_cfg.width      = 720;
        vo_chn_cfg.height     = 1280;
        vo_chn_cfg.vo_buf_num = 0;
        ret = mpp_comm_vo_chn_create(VO_CHN_0, &vo_chn_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_vo_chn_create fail! ret:%d  vo_chn:%d\n", ret, VO_CHN_0);
            return -1;
        }
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        vo_chn_cfg.top        = 0;
        vo_chn_cfg.left       = 0;
        vo_chn_cfg.width      = 720;
        vo_chn_cfg.height     = 1280;
        vo_chn_cfg.vo_buf_num = 0;
        ret = mpp_comm_vo_chn_create(VO_CHN_0, &vo_chn_cfg);
        if (ret) {
            ERR_PRT("Do mpp_comm_vo_chn_create fail! ret:%d  vo_chn:%d\n", ret, VO_CHN_0);
            return -1;
        }
    }
    break;

    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920:
    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
    }
    break;

    default:
        ERR_PRT("Input ise_cfg:%d don't supoort!\n", ise_cfg);
        break;
    }

    return 0;
}


static int vo_destroy(ISE_MODE_CFG_E ise_cfg)
{
    int ret = 0;

    switch(ise_cfg) {
    case ISE_FISH_180_MODE_2048_TO_1024x1024:
    case ISE_FISH_180_MODE_2048_TO_2048x2048:
    case ISE_FISH_360_MODE_2048_TO_4096x1024:
    case ISE_FISH_360_MODE_2048_TO_2048x512: {
        ret = mpp_comm_vo_chn_destroy(VO_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vo_chn_destroy fail! ret:%d  vo_chn:%d\n", ret, VO_CHN_0);
        }

        ret = mpp_comm_vo_dev_destroy(VO_DEV_LCD);
        if (ret) {
            ERR_PRT("Do mpp_comm_vo_dev_destroy fail! ret:%d  vo_type:%d\n", ret, VO_DEV_LCD);
        }
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        ret = mpp_comm_vo_chn_destroy(VO_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vo_chn_destroy fail! ret:%d  vo_chn:%d\n", ret, VO_CHN_0);
        }

        ret = mpp_comm_vo_dev_destroy(VO_DEV_LCD);
        if (ret) {
            ERR_PRT("Do mpp_comm_vo_dev_destroy fail! ret:%d  vo_type:%d\n", ret, VO_DEV_LCD);
        }
    }
    break;

    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920:
    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
    }
    break;

    default:
        ERR_PRT("Input ise_cfg:%d don't supoort!\n", ise_cfg);
        break;
    }

    return 0;
}


static int components_bind(ISE_MODE_CFG_E ise_cfg)
{
    int ret = 0;
    int vi_chn = 0, venc_chn = 0, ise_grp = 0, ise_chn = 0;

    switch(ise_cfg) {
    case ISE_FISH_180_MODE_2048_TO_1024x1024:
    case ISE_FISH_180_MODE_2048_TO_2048x2048:
    case ISE_FISH_360_MODE_2048_TO_4096x1024:
    case ISE_FISH_360_MODE_2048_TO_2048x512: {
        ret = mpp_comm_vi_bind_ise(VI_DEV_1, VI_CHN_0, ISE_GRP_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_bind_ise fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_ise_bind_venc(ISE_GRP_0, ISE_CHN_0, VENC_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_bind_venc fail! ret:%d \n", ret);
            return -1;
        }

        ret = mpp_comm_vi_bind_venc(VI_DEV_3, VI_CHN_0, VENC_CHN_1);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_bind_venc fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_vi_bind_vo(VI_DEV_3, VI_CHN_1, VO_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_bind_vo fail! ret:%d \n", ret);
            return -1;
        }
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        for (vi_chn = 0, venc_chn = 0, ise_grp = 0, ise_chn = 0;
             vi_chn < 4;
             vi_chn++, venc_chn++, ise_grp++) {
            ret = mpp_comm_vi_bind_ise(VI_DEV_1, vi_chn, ise_grp);
            if (ret) {
                ERR_PRT("Do mpp_comm_vi_bind_ise fail! ret:%d \n", ret);
                return -1;
            }

            ret = mpp_comm_ise_bind_venc(ise_grp, ise_chn, venc_chn);
            if (ret) {
                ERR_PRT("Do mpp_comm_ise_bind_venc fail! ret:%d \n", ret);
                return -1;
            }
        }

        ret = mpp_comm_vi_bind_venc(VI_DEV_3, VI_CHN_0, VENC_CHN_4);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_bind_venc fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_vi_bind_vo(VI_DEV_3, VI_CHN_1, VO_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_bind_vo fail! ret:%d \n", ret);
            return -1;
        }
    }
    break;

    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920:
    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
        ret = mpp_comm_vi_bind_ise(VI_DEV_0, VI_CHN_0, ISE_GRP_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_bind_ise fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_vi_bind_ise(VI_DEV_1, VI_CHN_0, ISE_GRP_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_bind_ise fail! ret:%d \n", ret);
            return -1;
        }

        ret = mpp_comm_ise_bind_venc(ISE_GRP_0, ISE_CHN_0, VENC_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_bind_venc fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_ise_bind_venc(ISE_GRP_0, ISE_CHN_1, VENC_CHN_1);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_bind_venc fail! ret:%d \n", ret);
            return -1;
        }
    }
    break;

    default:
        ERR_PRT("Input ise_cfg:%d don't supoort!\n", ise_cfg);
        break;
    }

    return 0;
}


static int components_unbind(ISE_MODE_CFG_E ise_cfg)
{
    int ret = 0;
    int vi_chn = 0, venc_chn = 0, ise_grp = 0, ise_chn = 0;

    switch(ise_cfg) {
    case ISE_FISH_180_MODE_2048_TO_1024x1024:
    case ISE_FISH_180_MODE_2048_TO_2048x2048:
    case ISE_FISH_360_MODE_2048_TO_4096x1024:
    case ISE_FISH_360_MODE_2048_TO_2048x512: {
        ret = mpp_comm_vi_unbind_ise(VI_DEV_1, VI_CHN_0, ISE_GRP_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_unbind_ise fail! ret:%d \n", ret);
        }
        ret = mpp_comm_ise_unbind_venc(ISE_GRP_0, ISE_CHN_0, VENC_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_unbind_venc fail! ret:%d \n", ret);
        }
        ret = mpp_comm_vi_unbind_venc(VI_DEV_3, VI_CHN_0, VENC_CHN_1);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_unbind_venc fail! ret:%d \n", ret);
        }
        ret = mpp_comm_vi_unbind_vo(VI_DEV_3, VI_CHN_1, VO_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_unbind_vo fail! ret:%d \n", ret);
        }
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        for (vi_chn = 0, venc_chn = 0, ise_grp = 0, ise_chn = 0;
             vi_chn < 4;
             vi_chn++, venc_chn++, ise_grp++) {
            ret = mpp_comm_ise_unbind_venc(ise_grp, ise_chn, venc_chn);
            if (ret) {
                ERR_PRT("Do mpp_comm_ise_unbind_venc fail! ret:%d \n", ret);
                return -1;
            }

            ret = mpp_comm_vi_unbind_ise(VI_DEV_1, vi_chn, ise_grp);
            if (ret) {
                ERR_PRT("Do mpp_comm_vi_unbind_ise fail! ret:%d \n", ret);
                return -1;
            }
        }
        ret = mpp_comm_vi_unbind_venc(VI_DEV_3, VI_CHN_0, VENC_CHN_4);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_unbind_venc fail! ret:%d \n", ret);
        }
        ret = mpp_comm_vi_unbind_vo(VI_DEV_3, VI_CHN_1, VO_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_unbind_vo fail! ret:%d \n", ret);
        }
    }
    break;

    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920:
    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
        ret = mpp_comm_ise_unbind_venc(ISE_GRP_0, ISE_CHN_0, VENC_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_unbind_venc fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_ise_unbind_venc(ISE_GRP_0, ISE_CHN_1, VENC_CHN_1);
        if (ret) {
            ERR_PRT("Do mpp_comm_ise_unbind_venc fail! ret:%d \n", ret);
            return -1;
        }

        ret = mpp_comm_vi_unbind_ise(VI_DEV_0, VI_CHN_0, ISE_GRP_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_unbind_ise fail! ret:%d \n", ret);
            return -1;
        }
        ret = mpp_comm_vi_unbind_ise(VI_DEV_1, VI_CHN_0, ISE_GRP_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vi_unbind_ise fail! ret:%d \n", ret);
            return -1;
        }
    }
    break;

    default:
        ERR_PRT("Input ise_cfg:%d don't supoort!\n", ise_cfg);
        break;
    }

    return 0;
}


static int components_start(ISE_MODE_CFG_E ise_cfg)
{
    int ret = 0;
    int vi_chn = 0, venc_chn = 0, ise_grp = 0;

    switch(ise_cfg) {
    case ISE_FISH_180_MODE_2048_TO_1024x1024:
    case ISE_FISH_180_MODE_2048_TO_2048x2048:
    case ISE_FISH_360_MODE_2048_TO_4096x1024:
    case ISE_FISH_360_MODE_2048_TO_2048x512: {
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

        ret = AW_MPI_ISE_Start(ISE_GRP_0);
        if (ret) {
            ERR_PRT("Do AW_MPI_ISE_Start fail! ret:%d\n", ret);
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

        ret = mpp_comm_vo_chn_start(VO_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vo_chn_start vo_chn:%d fail! ret:%d\n", VO_CHN_0, ret);
            return -1;
        }
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        for (vi_chn = 0, venc_chn = 0, ise_grp = 0; ise_grp < 4; vi_chn++, venc_chn++, ise_grp++) {
            ret = AW_MPI_VI_EnableVirChn(VI_DEV_1, vi_chn);
            if (ret) {
                ERR_PRT("Do AW_MPI_VI_EnableVirChn fail! ret:%d\n", ret);
                return -1;
            }

            ret = AW_MPI_ISE_Start(ise_grp);
            if (ret) {
                ERR_PRT("Do AW_MPI_ISE_Start fail! ret:%d\n", ret);
                return -1;
            }

            ret = AW_MPI_VENC_StartRecvPic(venc_chn);
            if (ret) {
                ERR_PRT("Do AW_MPI_VENC_StartRecvPic fail! ret:%d\n", ret);
                return -1;
            }
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

        ret = AW_MPI_VENC_StartRecvPic(VENC_CHN_4);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_StartRecvPic fail! ret:%d\n", ret);
            return -1;
        }

        ret = mpp_comm_vo_chn_start(VO_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vo_chn_start vo_chn:%d fail! ret:%d\n", VO_CHN_0, ret);
            return -1;
        }
    }
    break;

    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920:
    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
        ret = AW_MPI_VI_EnableVirChn(VI_DEV_0, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_EnableVirChn fail! ret:%d\n", ret);
            return -1;
        }
        ret = AW_MPI_VI_EnableVirChn(VI_DEV_1, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_EnableVirChn fail! ret:%d\n", ret);
            return -1;
        }

        ret = AW_MPI_ISE_Start(ISE_GRP_0);
        if (ret) {
            ERR_PRT("Do AW_MPI_ISE_Start fail! ret:%d\n", ret);
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
    }
    break;

    default:
        ERR_PRT("Input ise_cfg:%d don't supoort!\n", ise_cfg);
        break;
    }

    return 0;
}


static int components_stop(ISE_MODE_CFG_E ise_cfg)
{
    int ret = 0;
    int vi_chn = 0, venc_chn = 0, ise_grp = 0;

    switch(ise_cfg) {
    case ISE_FISH_180_MODE_2048_TO_1024x1024:
    case ISE_FISH_180_MODE_2048_TO_2048x2048:
    case ISE_FISH_360_MODE_2048_TO_4096x1024:
    case ISE_FISH_360_MODE_2048_TO_2048x512: {
        ret = mpp_comm_vo_chn_stop(VO_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vo_chn_stop vo_chn:%d fail! ret:%d\n", VO_CHN_0, ret);
        }

        ret = AW_MPI_VENC_StopRecvPic(VENC_CHN_0);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_StopRecvPic fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
        }
        ret = AW_MPI_VENC_StopRecvPic(VENC_CHN_1);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_StopRecvPic fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_1);
        }

        ret = AW_MPI_ISE_Stop(ISE_GRP_0);
        if (ret) {
            ERR_PRT("Do AW_MPI_ISE_Stop fail! ret:%d\n", ret);
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
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        for (vi_chn = 0, venc_chn = 0, ise_grp = 0; ise_grp < 4; vi_chn++, venc_chn++, ise_grp++) {
            ret = AW_MPI_ISE_Stop(ise_grp);
            if (ret) {
                ERR_PRT("Do AW_MPI_ISE_Stop fail! ret:%d\n", ret);
                return -1;
            }

            ret = AW_MPI_VENC_StopRecvPic(venc_chn);
            if (ret) {
                ERR_PRT("Do AW_MPI_VENC_StopRecvPic fail! ret:%d\n", ret);
                return -1;
            }

            ret = AW_MPI_VI_DisableVirChn(VI_DEV_1, vi_chn);
            if (ret) {
                ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
            }
        }

        ret = mpp_comm_vo_chn_stop(VO_CHN_0);
        if (ret) {
            ERR_PRT("Do mpp_comm_vo_chn_stop vo_chn:%d fail! ret:%d\n", VO_CHN_0, ret);
        }
        ret = AW_MPI_VENC_StopRecvPic(VENC_CHN_4);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_StopRecvPic fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_4);
        }
        ret = AW_MPI_VI_DisableVirChn(VI_DEV_3, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
        }
        ret = AW_MPI_VI_DisableVirChn(VI_DEV_3, VI_CHN_1);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
        }

    }
    break;

    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920:
    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
        ret = AW_MPI_ISE_Stop(ISE_GRP_0);
        if (ret) {
            ERR_PRT("Do AW_MPI_ISE_Stop fail! ret:%d\n", ret);
        }
        ret = AW_MPI_VENC_StopRecvPic(VENC_CHN_0);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_StopRecvPic fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
        }
        ret = AW_MPI_VENC_StopRecvPic(VENC_CHN_1);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_StopRecvPic fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_1);
        }

        ret = AW_MPI_VI_DisableVirChn(VI_DEV_0, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
        }
        ret = AW_MPI_VI_DisableVirChn(VI_DEV_1, VI_CHN_0);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
        }
    }
    break;

    default:
        ERR_PRT("Input ise_cfg:%d don't supoort!\n", ise_cfg);
        break;
    }

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
    g_stream_2 = rtsp->createMediaStream("ch2", attr);
    g_stream_3 = rtsp->createMediaStream("ch3", attr);
    g_stream_4 = rtsp->createMediaStream("ch4", attr);
    rtsp->runWithNewThread();

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

    if (ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024 == g_ise_cfg ||
        ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024 == g_ise_cfg ||
        ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024 == g_ise_cfg ||
        ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024 == g_ise_cfg) {
        g_param_2.stream    = g_stream_2;
        g_param_2.run_flag  = 1;
        g_param_2.venc_chn  = VENC_CHN_2;
        g_param_2.venc_type = g_venc_type;
        ret = pthread_create(&g_param_2.thd_id, NULL, SendStreamByRtsp, &g_param_2);
        if (ret) {
            ERR_PRT("Do pthread_create fail! ret:%d\n", ret);
        }

        g_param_3.stream    = g_stream_3;
        g_param_3.run_flag  = 1;
        g_param_3.venc_chn  = VENC_CHN_3;
        g_param_3.venc_type = g_venc_type;
        ret = pthread_create(&g_param_3.thd_id, NULL, SendStreamByRtsp, &g_param_3);
        if (ret) {
            ERR_PRT("Do pthread_create fail! ret:%d\n", ret);
        }

        g_param_4.stream    = g_stream_4;
        g_param_4.run_flag  = 1;
        g_param_4.venc_chn  = VENC_CHN_4;
        g_param_4.venc_type = g_venc_type;
        ret = pthread_create(&g_param_4.thd_id, NULL, SendStreamByRtsp, &g_param_4);
        if (ret) {
            ERR_PRT("Do pthread_create fail! ret:%d\n", ret);
        }

    }

    return 0;
}


static void rtsp_stop(TinyServer *rtsp)
{
    g_param_0.run_flag = 0;
    g_param_1.run_flag = 0;
    g_param_2.run_flag = 0;
    g_param_3.run_flag = 0;
    g_param_4.run_flag = 0;

    usleep(10*1000);

    rtsp->stop();
    pthread_join(g_param_0.thd_id, 0);
    delete g_param_0.stream;

    pthread_join(g_param_1.thd_id, 0);
    delete g_param_1.stream;

    if (ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024 == g_ise_cfg ||
        ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024 == g_ise_cfg ||
        ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024 == g_ise_cfg ||
        ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024 == g_ise_cfg) {
        pthread_join(g_param_2.thd_id, 0);
        delete g_param_2.stream;

        pthread_join(g_param_3.thd_id, 0);
        delete g_param_3.stream;

        pthread_join(g_param_4.thd_id, 0);
        delete g_param_4.stream;
    }
}


#ifdef SAMPLE_MODE
int main(int argc, char **argv)
#else
int SampleViIseVencVo(void *pData, char *pTitle)
#endif
{
    int ret = 0;

    printf("\033[2J");
    printf("\n\nDo sample vi+ise+(venc+rtsp)+vo. default:one_fish_360 out venc:4096x1024@30fps H264/VBR/MainProfile\n");
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
        goto _exit_0;
    }

    /* Step 2. Get vi config to create 2048x2048 vi channel */
    ret = vi_create(g_ise_cfg);
    if (ret) {
        ERR_PRT("Do vi_create fail! ret:%d\n", ret);
        goto _exit_1;
    }

    /* Step 3. Create ise modo fish, two_fish, or two_ise */
    ret = ise_create(g_ise_cfg);
    if (ret) {
        ERR_PRT("Do ise_create fail! ret:%d\n", ret);
        goto _exit_2;
    }

    /* Step 4. Get venc config to create 2048x2048 venc channel */
    ret = venc_create(g_ise_cfg);
    if (ret) {
        ERR_PRT("Do venc_create fail! ret:%d\n", ret);
        goto _exit_3;
    }

    /* Step 4. Create vo chnl */
    ret = vo_create(g_ise_cfg);
    if (ret) {
        ERR_PRT("Do vo_create fail! ret:%d\n", ret);
        goto _exit_4;
    }

    /* Setup 5. Bind vi + ise + venc and vi + (venc+vo) */
    ret = components_bind(g_ise_cfg);
    if (ret) {
        ERR_PRT("Do components_bind fail! ret:%d\n", ret);
        goto _exit_5;
    }

    /* Setup 6. Start vi, ise, venc, vo */
    ret = components_start(g_ise_cfg);
    if (ret) {
        ERR_PRT("Do components_start fail! ret:%d\n", ret);
        goto _exit_6;
    }

    /* Setup 7. Create thread, and get stream send to rtsp */
    rtsp_start(rtsp);

    /* Setup 8. Do this block menu control function */
    MenuCtrl();

    rtsp_stop(rtsp);
    delete rtsp;

_exit_6:
    components_stop(g_ise_cfg);

_exit_5:
    components_unbind(g_ise_cfg);

_exit_4:
    vo_destroy(g_ise_cfg);

_exit_3:
    venc_destroy(g_ise_cfg);

_exit_2:
    ise_destroy(g_ise_cfg);

_exit_1:
    vi_destroy(g_ise_cfg);

_exit_0:
    ret = mpp_comm_sys_exit();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_exit fail! ret:%d \n", ret);
    }

    return ret;
}

