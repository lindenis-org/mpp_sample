/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_vi_venc_mux_vo.cpp
 * @brief 该目录是对mpp中的 VI+VENC+MUX+VO 通路sample
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-05-08
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

//#define RECORDE_PATH_DIR   "/home"
//#define RECORDE_FILE_MP4_0 "/home/ch0_0.mp4"
//#define RECORDE_FILE_MP4_1 "/home/ch0_1.mp4"

#define RECORDE_PATH_DIR   "/tmp"
#define RECORDE_FILE_MP4_0 "/tmp/ch0_0.mp4"
#define RECORDE_FILE_MP4_1 "/tmp/ch0_1.mp4"


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static MPPCallbackInfo g_mux_cb;
static MUX_CB_PARAM_S  g_mux_param;

static MPPCallbackInfo g_mux_cb_1;
static MUX_CB_PARAM_S  g_mux_param_1;

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
    {(char *)"Quite vi+(venc+mux)+vo sample",  ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};

static MENU_INODE g_menu_guide[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"Set VI+VENC scene     (Resolution,    default:4K@25fps+720P@25fps)",  mpp_menu_venc_scene_choose,     &g_vi_ve_cfg, NULL},
    {(char *)"Set VENC Payload Type (H264/H265,     default:H264)",                 mpp_menu_venc_payload_type_set, &g_venc_type, NULL},
    {(char *)"Set VENC RC Mode      (CBR/VBR/FIXQP, default:CBR)",                  mpp_menu_venc_rc_mode_set,      &g_rc_mode,   NULL},
    {(char *)"Set VENC Profile      (BL/MP/HP,      default:Main_Profile)",         mpp_menu_venc_profile_set,      &g_profile,   NULL},
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
    vo_type           = VO_DEV_LCD;
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


static int mux_create(void)
{
    int ret = 0;
    VENC_CFG_S      venc_cfg  = {0};
    MUX_GRP_ATTR_S  grp_attr;

    /*  Greate mux grp_0 channel */
    ret = mpp_comm_venc_get_cfg(g_venc_type_0, &venc_cfg);
    if (ret) {
        ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
        return -1;
    }

    ret = mpp_comm_mux_video_full_cfg(g_venc_type, venc_cfg.dst_width, venc_cfg.dst_height, venc_cfg.dst_fps, &grp_attr);
    if (ret) {
        ERR_PRT("Do mpp_comm_mux_video_full_cfg fail! ret:0x%x \n", ret);
        return -1;
    }
    memset(g_mux_param.file_dir, 0, sizeof(g_mux_param.file_dir));
    strcpy(g_mux_param.file_dir, RECORDE_PATH_DIR);
    g_mux_param.venc_id  = VENC_CHN_0;
    g_mux_param.grp_id   = MUX_GRP_0;
    g_mux_param.chn_id   = MUX_CHN_0;
    g_mux_param.file_cnt = 1;
    g_mux_cb.cookie      = &g_mux_param;
    g_mux_cb.callback    = mpp_comm_mux_callbcak;
    ret = mpp_comm_mux_grp_create(MUX_GRP_0, &grp_attr, &g_mux_cb);
    if (ret) {
        ERR_PRT("Do mpp_comm_mux_grp_create fail! ret:0x%x \n", ret);
        return -1;
    }

    /*  Greate  mux grp_1 channel */
    ret = mpp_comm_venc_get_cfg(g_venc_type_1, &venc_cfg);
    if (ret) {
        ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
        return -1;
    }

    ret = mpp_comm_mux_video_full_cfg(g_venc_type, venc_cfg.dst_width, venc_cfg.dst_height, venc_cfg.dst_fps, &grp_attr);
    if (ret) {
        ERR_PRT("Do mpp_comm_mux_video_full_cfg fail! ret:0x%x \n", ret);
        return -1;
    }
    memset(g_mux_param_1.file_dir, 0, sizeof(g_mux_param_1.file_dir));
    strcpy(g_mux_param_1.file_dir, RECORDE_PATH_DIR);
    g_mux_param_1.venc_id  = VENC_CHN_1;
    g_mux_param_1.grp_id   = MUX_GRP_1;
    g_mux_param_1.chn_id   = MUX_CHN_0;
    g_mux_param_1.file_cnt = 1;
    g_mux_cb_1.cookie      = &g_mux_param_1;
    g_mux_cb_1.callback    = mpp_comm_mux_callbcak;
    ret = mpp_comm_mux_grp_create(MUX_GRP_1, &grp_attr, &g_mux_cb_1);
    if (ret) {
        ERR_PRT("Do mpp_comm_mux_grp_create fail! ret:0x%x \n", ret);
        return -1;
    }

    /*  Set muxer sps/pps info */
    if (g_venc_type == PT_H264) {
        VencHeaderData H264SpsPpsInfo;
        ret = AW_MPI_VENC_GetH264SpsPpsInfo(VENC_CHN_0, &H264SpsPpsInfo);
        if (SUCCESS == ret) {
            AW_MPI_MUX_SetH264SpsPpsInfo(VENC_CHN_0, &H264SpsPpsInfo);
        } else {
            ERR_PRT("Do AW_MPI_VENC_GetH264SpsPpsInfo fail! ret:0x%x \n", ret);
        }

        ret = AW_MPI_VENC_GetH264SpsPpsInfo(VENC_CHN_1, &H264SpsPpsInfo);
        if (SUCCESS == ret) {
            AW_MPI_MUX_SetH264SpsPpsInfo(VENC_CHN_1, &H264SpsPpsInfo);
        } else {
            ERR_PRT("Do AW_MPI_VENC_GetH264SpsPpsInfo fail! ret:0x%x \n", ret);
        }
    } else if(g_venc_type == PT_H265) {
        VencHeaderData H265SpsPpsInfo;
        ret = AW_MPI_VENC_GetH265SpsPpsInfo(VENC_CHN_0, &H265SpsPpsInfo);
        if (SUCCESS == ret) {
            AW_MPI_MUX_SetH265SpsPpsInfo(VENC_CHN_0, &H265SpsPpsInfo);
        } else {
            ERR_PRT("Do AW_MPI_VENC_GetH265SpsPpsInfo fail! ret:0x%x \n", ret);
        }
        ret = AW_MPI_VENC_GetH265SpsPpsInfo(VENC_CHN_1, &H265SpsPpsInfo);
        if (SUCCESS == ret) {
            AW_MPI_MUX_SetH265SpsPpsInfo(VENC_CHN_1, &H265SpsPpsInfo);
        } else {
            ERR_PRT("Do AW_MPI_VENC_GetH265SpsPpsInfo fail! ret:0x%x \n", ret);
        }
    }

    ret = mpp_comm_mux_chn_create(MUX_GRP_0, MUX_CHN_0, MEDIA_FILE_FORMAT_MP4, RECORDE_FILE_MP4_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_mux_grp_create fail! ret:0x%x \n", ret);
        return -1;
    }
    ret = mpp_comm_mux_chn_create(MUX_GRP_1, MUX_CHN_0, MEDIA_FILE_FORMAT_MP4, RECORDE_FILE_MP4_1);
    if (ret) {
        ERR_PRT("Do mpp_comm_mux_grp_create fail! ret:0x%x \n", ret);
        return -1;
    }

    return 0;
}


static int mux_destroy(void)
{
    int ret = 0;

    ret = mpp_comm_mux_chn_destroy(MUX_GRP_0, MUX_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_mux_chn_destroy fail! ret:%d  mux_grp:%d  mux_chn:%d\n", ret, MUX_GRP_0, MUX_CHN_0);
    }
    ret = mpp_comm_mux_grp_destroy(MUX_GRP_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_mux_grp_destroy fail! ret:%d  mux_grp:%d\n", ret, MUX_GRP_0);
    }

    ret = mpp_comm_mux_chn_destroy(MUX_GRP_1, MUX_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_mux_chn_destroy fail! ret:%d  mux_grp:%d  mux_chn:%d\n", ret, MUX_GRP_1, MUX_CHN_0);
    }
    ret = mpp_comm_mux_grp_destroy(MUX_GRP_1);
    if (ret) {
        ERR_PRT("Do mpp_comm_mux_grp_destroy fail! ret:%d  mux_grp:%d\n", ret, MUX_GRP_1);
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

    ret = mpp_comm_venc_bind_mux(VENC_CHN_0, MUX_GRP_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_venc_bind_mux fail! ret:%d \n", ret);
        return -1;
    }
    ret = mpp_comm_venc_bind_mux(VENC_CHN_1, MUX_GRP_1);
    if (ret) {
        ERR_PRT("Do mpp_comm_venc_bind_mux fail! ret:%d \n", ret);
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

    ret = mpp_comm_venc_unbind_mux(VENC_CHN_0, MUX_GRP_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_venc_unbind_mux fail! ret:%d\n", ret);
    }
    ret = mpp_comm_venc_unbind_mux(VENC_CHN_1, MUX_GRP_1);
    if (ret) {
        ERR_PRT("Do mpp_comm_venc_unbind_mux fail! ret:%d\n", ret);
    }

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
        return -1;
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

    ret = AW_MPI_MUX_StartGrp(MUX_GRP_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_MUX_StartGrp fail! ret:%d\n", ret);
        return -1;
    }
    ret = AW_MPI_MUX_StartGrp(MUX_GRP_1);
    if (ret) {
        ERR_PRT("Do AW_MPI_MUX_StartGrp fail! ret:%d\n", ret);
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

    ret = AW_MPI_MUX_StopGrp(MUX_GRP_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_MUX_StopGrp fail! ret:0x%x\n", ret);
    }
    ret = AW_MPI_MUX_StopGrp(MUX_GRP_1);
    if (ret) {
        ERR_PRT("Do AW_MPI_MUX_StopGrp fail! ret:0x%x\n", ret);
    }

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
int SampleViVencMuxVO(void *pData, char *pTitle)
#endif
{
    int ret = 0;

    printf("\033[2J");
    printf("\n\n\nDo sample vi+(venc+mux)+vo. default:VI_4K@30fps+VENC(4K@30fps+VGA@30fps) H264/CBR/MainProfile\n");
    RunMenuCtrl(g_menu_guide);

    ParseMenuParam();

    /* Step 1. Init mpp system */
    ret = mpp_comm_sys_init();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_init fail! ret:%d \n", ret);
        return -1;
    }

    /* Step 2. Get vi config to create 1080P/720P vi channel */
    ret = vi_create();
    if (ret < 0) {
        ERR_PRT("Do vi_create fail! ret:%d\n", ret);
        goto _exit_1;
    }

    /* Step 3. Get venc config to create 1080P/720P venc channel */
    ret = venc_create();
    if (ret < 0) {
        ERR_PRT("Do venc_create fail! ret:%d\n", ret);
        goto _exit_2;
    }

    /* Setup 4. Create vo device , layer channel,  and Clock device */
    ret = vo_create();
    if (ret < 0) {
        ERR_PRT("Do vo_create fail! ret:%d \n", ret);
        goto _exit_3;
    }

    /* Setup 5. Create MUX Group and Channel */
    ret = mux_create();
    if (ret < 0) {
        ERR_PRT("Do mux_create fail! ret:%d \n", ret);
        goto _exit_4;
    }

    /* Setup 6. Bind vi_0 to venc_0  and vi_1 to venc_1 */
    ret = components_bind();
    if (ret < 0) {
        ERR_PRT("Do components_bind fail! ret:%d \n", ret);
        goto _exit_5;
    }

    /* Setup 7. Start vi and venc */
    ret = components_start();
    if (ret < 0) {
        ERR_PRT("Do components_start fail! ret:%d \n", ret);
        goto _exit_6;
    }

    /* Setup 7. Do this block menu control function */
    MenuCtrl();

_exit_6:
    components_stop();

_exit_5:
    components_unbind();

_exit_4:
    mux_destroy();

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

