/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_vi_venc.cpp
 * @brief 该目录是对mpp中的 VI+VO 通路sample
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


/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
//#define ENABLE_VO_CLOCK


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static MENU_INODE g_menu_top[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"ISP control",   NULL,   NULL, NULL},
    {(char *)"VI  control",   NULL,   NULL, NULL},
    {(char *)"VO  control",   NULL,   NULL, NULL},
    {(char *)"Quite vi+vo sample",  ExitCurrentMenuLevel, NULL, NULL},
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

    ret = mpp_menu_vo_get(&g_menu_top[2].subMenu);
    if (ret) {
        ERR_PRT("Do mpp_menu_vo_get fail! ret:%d\n", ret);
    }

    RunMenuCtrl(g_menu_top);
}


static int vi_create(void)
{
    int ret = 0;
    VI_ATTR_S vi_attr;

    /**  create vi dev 2  src 1080P **/
    ret = mpp_comm_vi_get_attr(VI_1080P_30FPS, &vi_attr);
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

    /*  Run isp server */
    AW_MPI_ISP_Init();
    AW_MPI_ISP_Run(ISP_DEV_0);

    return 0;
}


static int vi_destroy(void)
{
    int ret = 0;

    /**  destroy vi dev 2  **/
    ret = mpp_comm_vi_chn_destroy(VI_DEV_1, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_chn_destroy fail! ret:%d  vi_dev:%d  vi_chn:%d \n", ret, VI_DEV_1, VI_CHN_0);
    }

    /*  Stop isp server */
    AW_MPI_ISP_Stop(ISP_DEV_0);
    AW_MPI_ISP_Exit();

    ret = mpp_comm_vi_dev_destroy(VI_DEV_1);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_dev_destroy fail! ret:%d  vi_dev:%d \n", ret, VI_DEV_1);
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


static int components_start(void)
{
    int ret = 0;

    ret = AW_MPI_VI_EnableVirChn(VI_DEV_1, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_EnableVirChn fail! ret:%d\n", ret);
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

    ret = AW_MPI_VI_DisableVirChn(VI_DEV_1, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
    }

    return 0;
}

static int components_bind(void)
{
    int ret = 0;

#ifdef ENABLE_VO_CLOCK
    ret = mpp_comm_vo_bind_clock(CLOCK_CHN_0, VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_bind_clock fail! ret:%d clock_chn:%d vo_chn:%d\n", ret, CLOCK_CHN_0, VO_CHN_0);
        return -1;
    }
#endif

    ret = mpp_comm_vi_bind_vo(VI_DEV_1, VI_CHN_0, VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_bind_vo fail! ret:%d \n", ret);
        return -1;
    }

    return 0;
}


static int components_unbind(void)
{
    int ret = 0;

    ret = mpp_comm_vi_unbind_vo(VI_DEV_1, VI_CHN_0, VO_CHN_0);
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


#ifdef SAMPLE_MODE
int main()
#else
int SampleViVo(void *pData, char *pTitle)
#endif
{
    int ret = 0;

    DB_PRT("Do sample_vi_vo start!\n");

    /* Step 1. Init mpp system */
    ret = mpp_comm_sys_init();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_init fail! ret:%d \n", ret);
        return -1;
    }

    /* Step 2. Get vi config to create 1080P vi channel */
    ret = vi_create();
    if (ret) {
        ERR_PRT("Do vi_create fail! ret:%d \n", ret);
        goto _exit_1;
    }

    /* Step 3. Create vo device and layer */
    ret = vo_create();
    if (ret) {
        ERR_PRT("Do vo_create fail! ret:%d \n", ret);
        goto _exit_2;
    }

    /* Step 5. Bind clock to vo,  and vi to vo */
    ret = components_bind();
    if (ret) {
        ERR_PRT("Do components_bind fail! ret:%d \n", ret);
        goto _exit_3;
    }

    /* Setup 6. Start  vi  clock  vo */
    ret = components_start();
    if (ret) {
        ERR_PRT("Do components_start fail! ret:%d \n", ret);
        goto _exit_4;
    }

    /* Setup 7. Do this block menu control function */
    MenuCtrl();

_exit_4:
    components_stop();

_exit_3:
    components_unbind();

_exit_2:
    vo_destroy();

_exit_1:
    vi_destroy();

_exit_0:
    ret = mpp_comm_sys_exit();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_exit fail! ret:%d \n", ret);
    }

    return ret;
}
