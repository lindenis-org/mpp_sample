/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_comm_vi.c
 * @brief 该目录是对VI模块的公共操作,参数设置和获取类型进行简单抽象
 *        封装,以达到提高使用率和减少工作量的目的.
 * @author id: wangguixing
 * @version v0.1 create
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
#include "mpp_comm.h"


/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
#define HLAY(chn, lyl)  (chn*4+lyl)


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/

static VI_ATTR_S g_viAttr_VGA = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 640,
    .format.height      = 480,
    .fps     = 30,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

static VI_ATTR_S g_viAttr_D1 = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 720,
    .format.height      = 576,
    .fps     = 30,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

static VI_ATTR_S g_viAttr_720p_25fps = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 1280,
    .format.height      = 720,
    .fps     = 25,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

static VI_ATTR_S g_viAttr_720p = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 1280,
    .format.height      = 720,
    .fps     = 30,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

static VI_ATTR_S g_viAttr_1080p_25fps = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 1920,
    .format.height      = 1080,
    .fps     = 25,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

static VI_ATTR_S g_viAttr_1080p = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 1920,
    .format.height      = 1080,
    .fps     = 30,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

static VI_ATTR_S g_viAttr_2k = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 2560,
    .format.height      = 1440,
    .fps     = 30,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

static VI_ATTR_S g_viAttr_4k = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 3840,
    .format.height      = 2160,
    .fps     = 30,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

static VI_ATTR_S g_viAttr_4k_25fps = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 3840,
    .format.height      = 2160,
    .fps     = 25,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

/* 2048x2048 , 1920x1920 and 1024x1024 for ise fish */
static VI_ATTR_S g_viAttr_2048x2048 = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 2048,
    .format.height      = 2048,
    .fps     = 30,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

static VI_ATTR_S g_viAttr_1920x1920 = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 1920,
    .format.height      = 1920,
    .fps     = 25,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

static VI_ATTR_S g_viAttr_1024x1024 = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 1024,
    .format.height      = 1024,
    .fps     = 30,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

static VI_ATTR_S g_viAttr_2880x2160 = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 2880,
    .format.height      = 2160,
    .fps     = 30,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};

static VI_ATTR_S g_viAttr_2592x1944 = {
    .format.pixelformat = V4L2_PIX_FMT_NV21M,
    .format.field       = V4L2_FIELD_NONE,
    .format.width       = 2592,
    .format.height      = 1944,
    .fps     = 30,
    .type    = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE,
    .memtype = V4L2_MEMORY_MMAP,
    .nbufs   = 5,
    .nplanes = 2,
};


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
/**
 * @brief Get vi config attr form vi_type.  guixing
 * @param
 * - vi_type           input
 * - p_vi_attr        output
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */

int mpp_comm_vi_get_attr(MPP_COM_VI_TYPE_E vi_type, VI_ATTR_S *p_vi_attr)
{
    if (NULL == p_vi_attr) {
        ERR_PRT("Input p_vi_attr is NULL!\n");
        return -1;
    }

    switch(vi_type) {
    case VI_VGA_30FPS:
        memcpy(p_vi_attr, &g_viAttr_VGA, sizeof(VI_ATTR_S));
        break;

    case VI_D1_30FPS:
        memcpy(p_vi_attr, &g_viAttr_D1, sizeof(VI_ATTR_S));
        break;

    case VI_720P_25FPS:
        memcpy(p_vi_attr, &g_viAttr_720p_25fps, sizeof(VI_ATTR_S));
        break;

    case VI_720P_30FPS:
        memcpy(p_vi_attr, &g_viAttr_720p, sizeof(VI_ATTR_S));
        break;

    case VI_1080P_25FPS:
        memcpy(p_vi_attr, &g_viAttr_1080p_25fps, sizeof(VI_ATTR_S));
        break;

    case VI_1080P_30FPS:
        memcpy(p_vi_attr, &g_viAttr_1080p, sizeof(VI_ATTR_S));
        break;

    case VI_2K_30FPS:
        memcpy(p_vi_attr, &g_viAttr_2k, sizeof(VI_ATTR_S));
        break;

    case VI_4K_30FPS:
        memcpy(p_vi_attr, &g_viAttr_4k, sizeof(VI_ATTR_S));
        break;

    case VI_4K_25FPS:
        memcpy(p_vi_attr, &g_viAttr_4k_25fps, sizeof(VI_ATTR_S));
        break;

    case VI_2880x2160_30FPS:
        memcpy(p_vi_attr, &g_viAttr_2880x2160, sizeof(VI_ATTR_S));
        break;

    case VI_2592x1944_30FPS:
        memcpy(p_vi_attr, &g_viAttr_2592x1944, sizeof(VI_ATTR_S));
        break;

        /* resulotion for ise fish */
    case VI_2048x2048_30FPS:
        memcpy(p_vi_attr, &g_viAttr_2048x2048, sizeof(VI_ATTR_S));
        break;
    case VI_1024x1024_30FPS:
        memcpy(p_vi_attr, &g_viAttr_1024x1024, sizeof(VI_ATTR_S));
        break;
    case VI_1920x1920_25FPS:
        memcpy(p_vi_attr, &g_viAttr_1920x1920, sizeof(VI_ATTR_S));
        break;

    default:
        ERR_PRT("Input vi_type:%d is not support!\n", vi_type);
        return -1;
    }

    return 0;
}


/**
 * @brief Create vi device.  guixing
 * @param
 * - vi_dev          input
 * - vi_chn          input
 * - p_vi_attr     input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_vi_dev_create(VI_DEV vi_dev, VI_ATTR_S *p_vi_attr)
{
    int ret = 0;

    /* Check input param */
    if (vi_dev < 0 || vi_dev > 100) {
        ERR_PRT("Input vi_dev:%d error!\n", vi_dev);
        return -1;
    }

    if (NULL == p_vi_attr) {
        ERR_PRT("Input p_vi_attr is NULL!\n");
        return -1;
    }

    /* Create VI device and channel */
    ret = AW_MPI_VI_CreateVipp(vi_dev);
    if (ret != SUCCESS) {
        ERR_PRT("vi_dev:%d AW_MPI_VI CreateVipp failed! ret:%d \n", vi_dev, ret);
        return -1;
    }

    ret = AW_MPI_VI_SetVippAttr(vi_dev, p_vi_attr);
    if (ret != SUCCESS) {
        ERR_PRT("vi_dev:%d AW_MPI_VI SetVippAttr failed! ret:%d \n", vi_dev, ret);
        return -1;
    }

    ret = AW_MPI_VI_EnableVipp(vi_dev);
    if (ret != SUCCESS) {
        ERR_PRT("vi_dev:%d AW_MPI_VI_EnableVipp failed! ret:%d \n", vi_dev, ret);
        return -1;
    }

    return ret;
}


/**
 * @brief Destroy vi .guixing
 * @param
 * - vi_dev          input
 * - vi_chn          input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_vi_dev_destroy(VI_DEV vi_dev)
{
    int ret = SUCCESS;

    /* Check input param */
    if (vi_dev < 0 || vi_dev > 100) {
        ERR_PRT("Input vi_dev:%d error!\n", vi_dev);
        return -1;
    }

    ret = AW_MPI_VI_DisableVipp(vi_dev);
    if (ret != SUCCESS) {
        ERR_PRT("Do AW_MPI_VI_DisableVipp vi_dev:%d failed! ret:%d \n", vi_dev, ret);
        return -1;
    }

    ret = AW_MPI_VI_DestoryVipp(vi_dev);
    if (ret != SUCCESS) {
        ERR_PRT("Do AW_MPI_VI_DestoryVipp vi_dev:%d failed! ret:%d \n", vi_dev, ret);
        return -1;
    }

    return ret;
}


/**
 * @brief Create vi .guixing
 * @param
 * - vi_dev          input
 * - vi_chn          input
 * - p_vi_attr     input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_vi_chn_create(VI_DEV vi_dev, VI_CHN vi_chn)
{
    int ret = SUCCESS;

    /* Check input param */
    if (vi_dev < 0 || vi_dev > 100) {
        ERR_PRT("Input vi_dev:%d error!\n", vi_dev);
        return -1;
    }

    if (vi_chn < 0 || vi_chn > 100) {
        ERR_PRT("Input vi_chn:%d error!\n", vi_chn);
        return -1;
    }

    /* Create VI device of channel */
    ret = AW_MPI_VI_CreateVirChn(vi_dev, vi_chn, NULL);
    if (ret != SUCCESS) {
        ERR_PRT("AW_MPI_VI_CreateVirChn vi_dev:%d vi_chn:%d failed! ret:%d \n", vi_dev, vi_chn, ret);
        return -1;
    }

    return ret;
}



/**
 * @brief Destroy vi .guixing
 * @param
 * - vi_dev          input
 * - vi_chn          input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_vi_chn_destroy(VI_DEV vi_dev, VI_CHN vi_chn)
{
    int ret = SUCCESS;

    /* Check input param */
    if (vi_dev < 0 || vi_dev > 100) {
        ERR_PRT("Input vi_dev:%d error!\n", vi_dev);
        return -1;
    }

    if (vi_chn < 0 || vi_chn > 100) {
        ERR_PRT("Input vi_chn:%d error!\n", vi_chn);
        return -1;
    }

    ret = AW_MPI_VI_DisableVirChn(vi_dev, vi_chn);
    if (ret != SUCCESS) {
        ERR_PRT("Do AW_MPI_VI_CreateVirChn vi_dev:%d vi_chn:%d failed! ret:%d \n", vi_dev, vi_chn, ret);
        return -1;
    }

    ret = AW_MPI_VI_DestoryVirChn(vi_dev, vi_chn);
    if (ret != SUCCESS) {
        ERR_PRT("Do AW_MPI_VI_DestoryVirChn vi_dev:%d vi_chn:%d failed! ret:%d \n", vi_dev, vi_chn, ret);
        return -1;
    }

    return ret;
}


int mpp_comm_vi_bind_venc(int vi_dev, int vi_chn, int venc_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S ViChn, VeChn;

    ViChn.mModId = MOD_ID_VIU;
    ViChn.mDevId = vi_dev;
    ViChn.mChnId = vi_chn;

    VeChn.mModId = MOD_ID_VENC;
    VeChn.mDevId = 0;
    VeChn.mChnId = venc_chn;

    ret = AW_MPI_SYS_Bind(&ViChn, &VeChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind fail! ret:%d  src(%d-%d) dst(%d-%d)\n", ret,
                ViChn.mDevId, ViChn.mChnId, VeChn.mDevId, VeChn.mChnId);
        return FAILURE;
    }

    return ret;
}


ERRORTYPE mpp_comm_vi_unbind_venc(int vi_dev, int vi_chn, int venc_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S ViChn, VeChn;

    ViChn.mModId = MOD_ID_VIU;
    ViChn.mDevId = vi_dev;
    ViChn.mChnId = vi_chn;

    VeChn.mModId = MOD_ID_VENC;
    VeChn.mDevId = 0;
    VeChn.mChnId = venc_chn;

    ret = AW_MPI_SYS_UnBind(&ViChn, &VeChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind fail! ret:%d  src(%d-%d) dst(%d-%d)\n", ret,
                ViChn.mDevId, ViChn.mChnId, VeChn.mDevId, VeChn.mChnId);
        return FAILURE;
    }

    return ret;
}


int mpp_comm_vi_bind_vo(int vi_dev, int vi_chn, int vo_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S ViChn, VoChn;

    ViChn.mModId = MOD_ID_VIU;
    ViChn.mDevId = vi_dev;
    ViChn.mChnId = vi_chn;

    VoChn.mModId = MOD_ID_VOU;
    VoChn.mDevId = HLAY(vo_chn, 0);
    VoChn.mChnId = 0;

    ret = AW_MPI_SYS_Bind(&ViChn, &VoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind vi bind vo fail! ret:%d\n", ret);
        return FAILURE;
    }

    return ret;
}


int mpp_comm_vi_unbind_vo(int vi_dev, int vi_chn, int vo_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S ViChn, VoChn;

    ViChn.mModId = MOD_ID_VIU;
    ViChn.mDevId = vi_dev;
    ViChn.mChnId = vi_chn;

    VoChn.mModId = MOD_ID_VOU;
    VoChn.mDevId = HLAY(vo_chn, 0);
    VoChn.mChnId = 0;

    ret = AW_MPI_SYS_UnBind(&ViChn, &VoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind vi unbind vo fail! ret:%d\n", ret);
        return FAILURE;
    }

    return ret;
}


int mpp_comm_vi_bind_ise(int vi_dev, int vi_chn, int ise_grp)
{
    int       ret = SUCCESS;
    MPP_CHN_S ViChn, IseChn;

    ViChn.mModId = MOD_ID_VIU;
    ViChn.mDevId = vi_dev;
    ViChn.mChnId = vi_chn;

    IseChn.mModId = MOD_ID_ISE;
    IseChn.mDevId = ise_grp;
    IseChn.mChnId = 0;

    ret = AW_MPI_SYS_Bind(&ViChn, &IseChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind vi bind ise fail! ret:%d\n", ret);
        return FAILURE;
    }

    return ret;
}

int mpp_comm_vi_unbind_ise(int vi_dev, int vi_chn, int ise_grp)
{
    int       ret = SUCCESS;
    MPP_CHN_S ViChn, IseChn;

    ViChn.mModId = MOD_ID_VIU;
    ViChn.mDevId = vi_dev;
    ViChn.mChnId = vi_chn;

    IseChn.mModId = MOD_ID_ISE;
    IseChn.mDevId = ise_grp;
    IseChn.mChnId = 0;

    ret = AW_MPI_SYS_UnBind(&ViChn, &IseChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind vi bind ise fail! ret:%d\n", ret);
        return FAILURE;
    }

    return ret;
}

