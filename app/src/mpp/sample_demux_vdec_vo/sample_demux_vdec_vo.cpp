/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file sample_demux_vdec_vo.cpp
 * @brief 该目录是对mpp中的 DEMUX+VDEC+VO 通路sample
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-05-10
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


/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
//#define SRC_FILE_NAME "/tmp/venc_1080p_30fps_h264.mp4"
//#define SRC_FILE_NAME "/tmp/venc_720p_25fps_h264.mp4"
#define SRC_FILE_NAME "/tmp/venc_4k_25fps_h264.mp4"


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static MPPCallbackInfo  g_cb_info;
static DEMUX_INFO_S     g_demux_info;
static DEMUX_CB_PARAM_S g_demux_param;
static ROTATE_E         g_rotate = ROTATE_NONE;

static MENU_INODE g_menu_top[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"VDEC control",   NULL,   NULL, NULL},
    {(char *)"VO   control",   NULL,   NULL, NULL},
    {(char *)"Quite demux+vdec+vo sample",  ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};

static MENU_INODE g_menu_guide[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"Set VDEC rotate  (default:rotate_0)",  mpp_menu_vdec_rotate_set,  &g_rotate,  NULL},
    {(char *)"Save confige and run this sample",     ExitCurrentMenuLevel, NULL, NULL},
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

    ret = mpp_menu_vdec_get(&g_menu_top[0].subMenu);
    if (ret) {
        ERR_PRT("Do mpp_menu_vdec_get fail! ret:%d\n", ret);
    }

    ret = mpp_menu_vo_get(&g_menu_top[1].subMenu);
    if (ret) {
        ERR_PRT("Do mpp_menu_vo_get fail! ret:%d\n", ret);
    }

    RunMenuCtrl(g_menu_top);
}


static int vo_create(void)
{
    int ret = 0;
    VO_DEV_TYPE_E vo_type;
    VO_DEV_CFG_S  vo_cfg;
    vo_type           = VO_DEV_LCD;
    vo_cfg.res_width  = 720;
    vo_cfg.res_height = 1280;
    ret = mpp_comm_vo_dev_create(vo_type, &vo_cfg);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_dev_create fail! ret:%d  vo_type:%d\n", ret, vo_type);
        return -1;
    }

    VO_CHN_CFG_S vo_chn_cfg;
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

    /* Create Clock for vo mode */
    ret = mpp_comm_vo_clock_create(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_clock_create fail! ret:%d  clock_chn:%d\n", ret, CLOCK_CHN_0);
        return -1;
    }

    return 0;
}

static int vo_destroy(void)
{
    int ret = 0;

    ret = mpp_comm_vo_clock_destroy(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_clock_destroy fail! ret:%d  clock_chn:%d\n", ret, CLOCK_CHN_0);
    }

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

    ret = mpp_comm_demux_bind_vdec(DEMUX_CHN_0, VDEC_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_demux_bind_vdec fail! ret:%d demux_chn:%d vdec_chn:%d\n", ret, DEMUX_CHN_0, VDEC_CHN_0);
        return -1;
    }

    ret = mpp_comm_vdec_bind_vo(VDEC_CHN_0, VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vdec_bind_vo fail! ret:%d vdec_chn:%d vo_chn:%d\n", ret, VDEC_CHN_0, VO_CHN_0);
        return -1;
    }

    ret = mpp_comm_demux_bind_clock(CLOCK_CHN_0, DEMUX_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_demux_bind_clock fail! ret:%d clock_chn:%d vdec_chn:%d\n", ret, CLOCK_CHN_0, DEMUX_CHN_0);
        return -1;
    }

    ret = mpp_comm_vo_bind_clock(CLOCK_CHN_0, VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_bind_clock fail! ret:%d clock_chn:%d vo_chn:%d\n", ret, CLOCK_CHN_0, VO_CHN_0);
        return -1;
    }


    return 0;
}


static int components_unbind(void)
{
    int ret = 0;

    ret = mpp_comm_vo_unbind_clock(CLOCK_CHN_0, VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_unbind_clock fail! ret:%d clock_chn:%d vo_chn:%d\n", ret, CLOCK_CHN_0, VO_CHN_0);
    }

    ret = mpp_comm_demux_unbind_clock(CLOCK_CHN_0, DEMUX_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_demux_unbind_clock fail! ret:%d clock_chn:%d vdec_chn:%d\n", ret, CLOCK_CHN_0, DEMUX_CHN_0);
    }

    ret = mpp_comm_vdec_unbind_vo(VDEC_CHN_0, VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vdec_unbind_vo fail! ret:%d vdec_chn:%d vo_chn:%d\n", ret, VDEC_CHN_0, VO_CHN_0);
    }

    ret = mpp_comm_demux_unbind_vdec(DEMUX_CHN_0, VDEC_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_demux_unbind_vdec fail! ret:%d demux_chn:%d vdec_chn:%d\n", ret, DEMUX_CHN_0, VDEC_CHN_0);
    }

    return 0;
}


static int components_start(void)
{
    int ret = 0;

    ret = AW_MPI_CLOCK_Start(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_CLOCK_Start fail! clock_chn:%d ret:%d\n", CLOCK_CHN_0, ret);
        return -1;
    }

    ret = AW_MPI_VDEC_StartRecvStream(VDEC_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VDEC_StartRecvStream fail! vdec_chn:%d ret:%d\n", VDEC_CHN_0, ret);
        return -1;
    }

    ret = mpp_comm_vo_chn_start(VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_chn_start vo_chn:%d fail! ret:%d\n", VO_CHN_0, ret);
        return -1;
    }

    ret = AW_MPI_DEMUX_Start(DEMUX_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_DEMUX_Start fail! demux_chn:%d  ret:%d\n", DEMUX_CHN_0, ret);
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

    ret = AW_MPI_VDEC_StopRecvStream(VDEC_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VDEC_StopRecvStream vdec_chn:%d fail! ret:%d\n", VDEC_CHN_0, ret);
    }

    ret = AW_MPI_DEMUX_Stop(DEMUX_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_DEMUX_Stop fail! demux_chn:%d  ret:%d\n", DEMUX_CHN_0, ret);
    }

    ret = AW_MPI_CLOCK_Stop(CLOCK_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_CLOCK_Stop fail! clock_chn:%d  ret:%d\n", CLOCK_CHN_0, ret);
    }

    return 0;
}


#ifdef SAMPLE_MODE
int main()
#else
int SampleDemuxVdecVo(void *pData, char *pTitle)
#endif
{
    int ret = 0;
    VDEC_CFG_S vdec_cfg;

    printf("\033[2J");
    printf("\n\n\nDo sample demux+vdec+vo start.\n");
    RunMenuCtrl(g_menu_guide);

    memset(&g_cb_info,    0, sizeof(MPPCallbackInfo));
    memset(&g_demux_info, 0, sizeof(DEMUX_INFO_S));
    g_cb_info.cookie   = &g_demux_param;
    g_cb_info.callback = mpp_comm_demux_callbcak;

    g_demux_param.demux_chn = DEMUX_CHN_0;
    g_demux_param.vdec_chn  = VDEC_CHN_0;
    g_demux_param.vo_chn    = VO_CHN_0;
    g_demux_param.run_flag  = 1;
    g_demux_param.fd        = -1;

    /* Step 1. Init mpp system */
    ret = mpp_comm_sys_init();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_init fail! ret:%d \n", ret);
        goto _exit_0;
    }

    /* Step 2. Create demux channel. */
    ret = mpp_comm_demux_create(DEMUX_CHN_0, SRC_FILE_NAME, &g_cb_info, &g_demux_info);
    if (ret) {
        ERR_PRT("Do mpp_comm_demux_create fail!  demux_chn:%d  ret:%d\n", DEMUX_CHN_0, ret);
        goto _exit_1;
    }

    /* Step 3. Create vdec channel */
    DB_PRT("Demux info: w:%d h:%d codec_type:%d\n", g_demux_info.width, g_demux_info.height, g_demux_info.codec_type);
    vdec_cfg.width      = g_demux_info.width;
    vdec_cfg.height     = g_demux_info.height;
    vdec_cfg.codec_type = g_demux_info.codec_type;
    vdec_cfg.rotation   = g_rotate;
    ret = mpp_comm_vdec_create(VDEC_CHN_0, &vdec_cfg, &g_cb_info);
    if (ret) {
        ERR_PRT("Do mpp_comm_vdec_create fail!  vdec_chn:%d  ret:%d\n", VDEC_CHN_0, ret);
        goto _exit_2;
    }

    /* Step 4. Create vo device and layer */
    ret = vo_create();
    if (ret) {
        ERR_PRT("Do vo_create fail! ret:%d \n", ret);
        goto _exit_3;
    }

    /* Step 5. Bind clock to vo,  and clock to demux,  demux to vdec */
    ret = components_bind();
    if (ret) {
        ERR_PRT("Do components_bind fail! ret:%d \n", ret);
        goto _exit_4;
    }

    /* Setup 6. Start demux, vdec, clock, vo */
    ret = components_start();
    if (ret) {
        ERR_PRT("Do components_start fail! ret:%d \n", ret);
        goto _exit_5;
    }

    /* Setup 7. Do this block menu control function */
    MenuCtrl();
    while (0 != g_demux_param.run_flag) {
        sleep(1);
    }
    DB_PRT("The src file:%s decode end!\n", SRC_FILE_NAME);

_exit_5:
    components_stop();

_exit_4:
    components_unbind();

_exit_3:
    vo_destroy();

_exit_2:
    ret = mpp_comm_vdec_destroy(VDEC_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vdec_destroy fail!  vdec_chn:%d  ret:%d\n", VDEC_CHN_0, ret);
    }

_exit_1:
    ret = mpp_comm_demux_destroy(DEMUX_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_demux_destroy fail!  demux_chn:%d  ret:%d\n", DEMUX_CHN_0, ret);
    }

_exit_0:
    ret = mpp_comm_sys_exit();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_exit fail! ret:%d \n", ret);
    }

    return ret;
}

