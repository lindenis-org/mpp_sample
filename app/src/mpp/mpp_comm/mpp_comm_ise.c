/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_comm_ise.c
 * @brief 该目录是对mpp中 ISE 模块的公共操作,参数设置和获取类型进行简单抽象
 *        封装,以达到提高使用率和减少工作量的目的.
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-05-8
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
/* None */


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/

/**
 * @brief  Create ise group create
 * @param
 * - ise_grp         input
 * - ise_mode     input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_ise_grp_create(ISE_GRP ise_grp, int ise_mode)
{
    int ret = 0;
    ISE_GROUP_ATTR_S grp_attr;

    if (ise_grp < 0 || ise_grp >= ISE_MAX_GRP_NUM) {
        ERR_PRT("Input ise_grp:%d error!  0~%d is ok.\n", ise_grp, ISE_MAX_GRP_NUM);
        return -1;
    }

    switch (ise_mode) {
    case ISEMODE_ONE_FISHEYE:
        grp_attr.iseMode   = ISEMODE_ONE_FISHEYE;
        //grp_attr.fish_mode = 0;
        break;
    case ISEMODE_TWO_FISHEYE:
        grp_attr.iseMode   = ISEMODE_TWO_FISHEYE;
        //grp_attr.fish_mode = 0;
        break;
    case ISEMODE_TWO_ISE:
        grp_attr.iseMode   = ISEMODE_TWO_ISE;
        //grp_attr.fish_mode = 0;
        break;
    default:
        ERR_PRT("Input ise_mode:%d don't support!\n", ise_mode);
        return -1;
    }

    ret = AW_MPI_ISE_CreateGroup(ise_grp, &grp_attr);
    if(ret < 0) {
        ERR_PRT("Do AW_MPI_ISE_CreateGroup ise_grp:%d ise_mode:%d fail! ret:0x%x\n", ise_grp, ise_mode, ret);
        return ret ;
    }
    ret = AW_MPI_ISE_SetGrpAttr(ise_grp, &grp_attr);
    if(ret < 0) {
        ERR_PRT("Do AW_MPI_ISE_SetGrpAttr ise_grp:%d ise_mode:%d fail! ret:0x%x\n", ise_grp, ise_mode, ret);
        return ret ;
    }

    return 0;
}


int mpp_comm_ise_grp_destroy(ISE_GRP ise_grp)
{
    int ret = 0;

    ret = AW_MPI_ISE_DestroyGroup(ise_grp);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_ISE_DestroyGroup ise_grp:%d fail! ret:0x%x\n", ise_grp, ret);
        return -1;
    }

    return 0;
}


int mpp_comm_ise_get_cfg(ISE_MODE_CFG_E ise_type, int chn_num, ISE_CHN_ATTR_S *chn_attr)
{
    int i = 0;

    double cam_matr[3][3] = {{1.0115217430885405e+003, 0., 9.60e+002},
        {0., 1.0115217430885405e+003, 5.40e+002},
        {0., 0., 1.}
    };

    double dist[8] = {6.1446514462273027e-001, -2.5223829431213818e-001,
                      -7.6746144767994573e-004, -2.0856862740027446e-004,
                      -6.2572787678747341e-002, 9.5921588321979923e-001,
                      -9.8592064991125133e-002, -1.7993258997361919e-001
                     };

    double cam_matr_prime[3][3] = {{5.5839984130859375e+002, 0., 9.60e+002},
        {0., 5.5591241455078125e+002, 5.40e+002},
        {0., 0., 1.}
    };

    if (NULL == chn_attr) {
        ERR_PRT("Input chn_attr is NULL!\n");
        return -1;
    }

    if (chn_num <= 0) {
        chn_num = 1;
    } else if (chn_num >= ISE_MAX_CHN_NUM) {
        ERR_PRT("Input chn_num:%d error!\n", chn_num);
    }

    memset(chn_attr, 0, sizeof(ISE_CHN_ATTR_S));

    switch (ise_type) {
    case ISE_FISH_180_MODE_2048_TO_2048x2048: {
        chn_attr->mode_attr.mFish.ise_cfg.dewarp_mode     = WARP_PANO180;
        chn_attr->mode_attr.mFish.ise_cfg.mount_mode      = MOUNT_BOTTOM;
        chn_attr->mode_attr.mFish.ise_cfg.in_w            = 2048;
        chn_attr->mode_attr.mFish.ise_cfg.in_h            = 2048;
        chn_attr->mode_attr.mFish.ise_cfg.in_luma_pitch   = 2048;
        chn_attr->mode_attr.mFish.ise_cfg.in_chroma_pitch = 2048;
        chn_attr->mode_attr.mFish.ise_cfg.in_yuv_type     = 0;
        chn_attr->mode_attr.mFish.ise_cfg.out_yuv_type    = 0;
        chn_attr->mode_attr.mFish.ise_cfg.p               = 2048/3.1415;
        chn_attr->mode_attr.mFish.ise_cfg.cx              = 2048/2;
        chn_attr->mode_attr.mFish.ise_cfg.cy              = 2048/2;

        for (i = 0; i < chn_num; i++) {
            chn_attr->mode_attr.mFish.ise_cfg.out_en[i]           = 1;
            chn_attr->mode_attr.mFish.ise_cfg.out_w[i]            = 2048;
            chn_attr->mode_attr.mFish.ise_cfg.out_h[i]            = 2048;
            chn_attr->mode_attr.mFish.ise_cfg.out_flip[i]         = 0;
            chn_attr->mode_attr.mFish.ise_cfg.out_mirror[i]       = 0;
            chn_attr->mode_attr.mFish.ise_cfg.out_luma_pitch[i]   = 2048; /* pitch%32 == 0*/
            chn_attr->mode_attr.mFish.ise_cfg.out_chroma_pitch[i] = 2048;
        }
    }
    break;

    case ISE_FISH_360_MODE_2048_TO_4096x1024: {
        chn_attr->mode_attr.mFish.ise_cfg.dewarp_mode     = WARP_PANO360;
        chn_attr->mode_attr.mFish.ise_cfg.mount_mode      = MOUNT_BOTTOM;
        chn_attr->mode_attr.mFish.ise_cfg.in_w            = 2048;
        chn_attr->mode_attr.mFish.ise_cfg.in_h            = 2048;
        chn_attr->mode_attr.mFish.ise_cfg.in_luma_pitch   = 2048;
        chn_attr->mode_attr.mFish.ise_cfg.in_chroma_pitch = 2048;
        chn_attr->mode_attr.mFish.ise_cfg.in_yuv_type     = 0;
        chn_attr->mode_attr.mFish.ise_cfg.out_yuv_type    = 0;
        chn_attr->mode_attr.mFish.ise_cfg.p               = 2048/3.1415;
        chn_attr->mode_attr.mFish.ise_cfg.cx              = 2048/2;
        chn_attr->mode_attr.mFish.ise_cfg.cy              = 2048/2;

        for (i = 0; i < chn_num; i++) {
            chn_attr->mode_attr.mFish.ise_cfg.out_en[i]           = 1;
            chn_attr->mode_attr.mFish.ise_cfg.out_w[i]            = 4096;
            chn_attr->mode_attr.mFish.ise_cfg.out_h[i]            = 1024;
            chn_attr->mode_attr.mFish.ise_cfg.out_flip[i]         = 1;
            chn_attr->mode_attr.mFish.ise_cfg.out_mirror[i]       = 0;
            chn_attr->mode_attr.mFish.ise_cfg.out_luma_pitch[i]   = 4096; /* pitch%32 == 0*/
            chn_attr->mode_attr.mFish.ise_cfg.out_chroma_pitch[i] = 4096;
        }
    }
    break;

    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024:
    case ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024: {
        chn_attr->mode_attr.mFish.ise_cfg.dewarp_mode     = WARP_NORMAL;
        chn_attr->mode_attr.mFish.ise_cfg.mount_mode      = MOUNT_BOTTOM;
        chn_attr->mode_attr.mFish.ise_cfg.in_w            = 2048;
        chn_attr->mode_attr.mFish.ise_cfg.in_h            = 2048;
        chn_attr->mode_attr.mFish.ise_cfg.in_luma_pitch   = 2048;
        chn_attr->mode_attr.mFish.ise_cfg.in_chroma_pitch = 2048;
        chn_attr->mode_attr.mFish.ise_cfg.in_yuv_type     = 0;
        chn_attr->mode_attr.mFish.ise_cfg.out_yuv_type    = 0;
        chn_attr->mode_attr.mFish.ise_cfg.p               = 2048/3.1415;
        chn_attr->mode_attr.mFish.ise_cfg.cx              = 2048/2;
        chn_attr->mode_attr.mFish.ise_cfg.cy              = 2048/2;
        for (i = 0; i < chn_num; i++) {
            chn_attr->mode_attr.mFish.ise_cfg.out_en[i]           = 1;
            chn_attr->mode_attr.mFish.ise_cfg.out_w[i]            = 1024;
            chn_attr->mode_attr.mFish.ise_cfg.out_h[i]            = 1024;
            chn_attr->mode_attr.mFish.ise_cfg.out_flip[i]         = 1;
            chn_attr->mode_attr.mFish.ise_cfg.out_mirror[i]       = 0;
            chn_attr->mode_attr.mFish.ise_cfg.out_luma_pitch[i]   = 1024; /* pitch%32 == 0*/
            chn_attr->mode_attr.mFish.ise_cfg.out_chroma_pitch[i] = 1024;
        }
        chn_attr->mode_attr.mFish.ise_cfg.mount_mode = MOUNT_BOTTOM;
        chn_attr->mode_attr.mFish.ise_cfg.tilt       = 45;
        chn_attr->mode_attr.mFish.ise_cfg.zoom       = 2;
        if (ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024 == ise_type) {
            chn_attr->mode_attr.mFish.ise_cfg.pan = 0; /* 45, 90, 135, 225 */
        } else if (ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024 == ise_type) {
            chn_attr->mode_attr.mFish.ise_cfg.pan = 90; /* 45, 90, 135, 225 */
        } else if (ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024 == ise_type) {
            chn_attr->mode_attr.mFish.ise_cfg.pan = 180; /* 45, 90, 135, 225 */
        } else if (ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024 == ise_type) {
            chn_attr->mode_attr.mFish.ise_cfg.pan = 270; /* 45, 90, 135, 225 */
        }
    }
    break;

    case ISE_TWO_FISH_360_MODE_1920_TO_3840x1920: {
        chn_attr->mode_attr.mDFish.ise_cfg.in_w            = 1920;
        chn_attr->mode_attr.mDFish.ise_cfg.in_h            = 1920;
        chn_attr->mode_attr.mDFish.ise_cfg.in_luma_pitch   = 1920;
        chn_attr->mode_attr.mDFish.ise_cfg.in_chroma_pitch = 1920;
        chn_attr->mode_attr.mDFish.ise_cfg.in_yuv_type     = 0;
        chn_attr->mode_attr.mDFish.ise_cfg.out_yuv_type    = 0;
        chn_attr->mode_attr.mDFish.ise_cfg.p0              = 1920/3.1415;
        chn_attr->mode_attr.mDFish.ise_cfg.cx0             = 1920/2;
        chn_attr->mode_attr.mDFish.ise_cfg.cy0             = 1920/2;
        chn_attr->mode_attr.mDFish.ise_cfg.p1              = 1920/3.1415;
        chn_attr->mode_attr.mDFish.ise_cfg.cx1             = 1920/2;
        chn_attr->mode_attr.mDFish.ise_cfg.cy1             = 1920/2;

        chn_attr->mode_attr.mDFish.ise_cfg.out_en[0]           = 1;
        chn_attr->mode_attr.mDFish.ise_cfg.out_w[0]            = 3840;
        chn_attr->mode_attr.mDFish.ise_cfg.out_h[0]            = 1920;
        chn_attr->mode_attr.mDFish.ise_cfg.out_flip[0]         = 1;
        chn_attr->mode_attr.mDFish.ise_cfg.out_mirror[0]       = 0;
        chn_attr->mode_attr.mDFish.ise_cfg.out_luma_pitch[0]   = 3840;
        chn_attr->mode_attr.mDFish.ise_cfg.out_chroma_pitch[0] = 3840;

        chn_attr->mode_attr.mDFish.ise_cfg.out_en[1]           = 1;
        chn_attr->mode_attr.mDFish.ise_cfg.out_w[1]            = 1280;
        chn_attr->mode_attr.mDFish.ise_cfg.out_h[1]            = 640;
        chn_attr->mode_attr.mDFish.ise_cfg.out_flip[1]         = 1;
        chn_attr->mode_attr.mDFish.ise_cfg.out_mirror[1]       = 0;
        chn_attr->mode_attr.mDFish.ise_cfg.out_luma_pitch[1]   = 1280;
        chn_attr->mode_attr.mDFish.ise_cfg.out_chroma_pitch[1] = 1280;
    }
    break;

    case ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080: {
        chn_attr->mode_attr.mIse.ise_cfg.ncam     = 2;
        chn_attr->mode_attr.mIse.ise_cfg.in_w     = 1920;
        chn_attr->mode_attr.mIse.ise_cfg.in_h     = 1080;
        chn_attr->mode_attr.mIse.ise_cfg.pano_w   = 3840;
        chn_attr->mode_attr.mIse.ise_cfg.pano_h   = 1080;
        chn_attr->mode_attr.mIse.ise_cfg.p0       = 51.5f;
        chn_attr->mode_attr.mIse.ise_cfg.p1       = 128.5f;
        chn_attr->mode_attr.mIse.ise_cfg.ov       = 320;
        chn_attr->mode_attr.mIse.ise_cfg.yuv_type = 0;

        chn_attr->mode_attr.mIse.ise_cfg.stre_coeff        = 1.0;
        chn_attr->mode_attr.mIse.ise_cfg.offset_r2l        = 0;
        chn_attr->mode_attr.mIse.ise_cfg.pano_fov          = 186.0f;
        chn_attr->mode_attr.mIse.ise_cfg.t_angle           = 0.0f;
        chn_attr->mode_attr.mIse.ise_cfg.hfov              = 78.0f;
        chn_attr->mode_attr.mIse.ise_cfg.wfov              = 124.0f;
        chn_attr->mode_attr.mIse.ise_cfg.wfov_rev          = 124.0f;
        chn_attr->mode_attr.mIse.ise_cfg.in_luma_pitch     = 1920;
        chn_attr->mode_attr.mIse.ise_cfg.in_chroma_pitch   = 1920;
        chn_attr->mode_attr.mIse.ise_cfg.pano_luma_pitch   = 3840;
        chn_attr->mode_attr.mIse.ise_cfg.pano_chroma_pitch = 3840;
        chn_attr->mode_attr.mIse.ise_proccfg.pano_flip     = 0;
        chn_attr->mode_attr.mIse.ise_proccfg.pano_mirr     = 0;

        memcpy(chn_attr->mode_attr.mIse.ise_cfg.calib_matr, cam_matr, 3 * 3 * sizeof(double));
        memcpy(chn_attr->mode_attr.mIse.ise_cfg.calib_matr_cv, cam_matr_prime, 3 * 3 * sizeof(double));
        memcpy(chn_attr->mode_attr.mIse.ise_cfg.distort, dist, 8 * sizeof(double));

        chn_attr->mode_attr.mIse.ise_proccfg.scalar_en[0]           = 1;
        chn_attr->mode_attr.mIse.ise_proccfg.scalar_w[0]            = 1920;
        chn_attr->mode_attr.mIse.ise_proccfg.scalar_h[0]            = 540;
        chn_attr->mode_attr.mIse.ise_proccfg.scalar_flip[0]         = 0;
        chn_attr->mode_attr.mIse.ise_proccfg.scalar_mirr[0]         = 0;
        chn_attr->mode_attr.mIse.ise_proccfg.scalar_luma_pitch[0]   = 1920;
        chn_attr->mode_attr.mIse.ise_proccfg.scalar_chroma_pitch[0] = 1920;
    }
    break;

    default:
        ERR_PRT("Input ise_type:%d is not support!\n", ise_type);
        return -1;
        break;
    }

    return 0;
}


int mpp_comm_ise_chn_create(ISE_GRP ise_grp, ISE_CHN ise_chn, ISE_CHN_ATTR_S *chn_attr)
{
    int ret = 0;

    if (NULL == chn_attr) {
        ERR_PRT("Input chn_attr is null!\n");
        return -1;
    }

    if (ise_grp < 0 || ise_grp >= ISE_MAX_GRP_NUM) {
        ERR_PRT("Input ise_grp:%d error!  0~%d is ok.\n", ise_grp, ISE_MAX_GRP_NUM);
        return -1;
    }

    if (ise_chn < 0 || ise_chn >= ISE_MAX_CHN_NUM) {
        ERR_PRT("Input ise_chn:%d error!  0~%d is ok.\n", ise_chn, ISE_MAX_CHN_NUM);
        return -1;
    }

    ret = AW_MPI_ISE_CreatePort(ise_grp, ise_chn, chn_attr);
    if(ret < 0) {
        ERR_PRT("Do AW_MPI_ISE_SetGrpAttr ise_grp:%d ise_chn:%d fail! ret:0x%x\n", ise_grp, ise_chn, ret);
        return ret ;
    }
    ret = AW_MPI_ISE_SetPortAttr(ise_grp, ise_chn, chn_attr);
    if(ret < 0) {
        ERR_PRT("Do AW_MPI_ISE_SetPortAttr ise_grp:%d ise_chn:%d fail! ret:0x%x\n", ise_grp, ise_chn, ret);
        return ret ;
    }

    return ret;
}


int mpp_comm_ise_chn_destroy(ISE_GRP ise_grp, ISE_CHN ise_chn)
{
    int ret = 0;

    ret = AW_MPI_ISE_DestroyPort(ise_grp, ise_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_ISE_DestroyPort  ise_grp:%d  ise_chn:%d fail! ret:0x%x\n", ise_grp, ise_chn, ret);
        return -1;
    }

    return 0;
}


int mpp_comm_ise_bind_vo(int ise_grp, int ise_chn, int vo_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S VoChn, IseChn;

    IseChn.mModId = MOD_ID_ISE;
    IseChn.mDevId = ise_grp;
    IseChn.mChnId = ise_chn;

    VoChn.mModId = MOD_ID_VOU;
    VoChn.mDevId = HLAY(vo_chn, 0);
    VoChn.mChnId = 0;

    ret = AW_MPI_SYS_Bind(&IseChn, &VoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind ise bind vo fail! ret:%d\n", ret);
        return FAILURE;
    }

    return ret;
}


int mpp_comm_ise_unbind_vo(int ise_grp, int ise_chn, int vo_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S VoChn, IseChn;

    IseChn.mModId = MOD_ID_ISE;
    IseChn.mDevId = ise_grp;
    IseChn.mChnId = ise_chn;

    VoChn.mModId = MOD_ID_VOU;
    VoChn.mDevId = HLAY(vo_chn, 0);
    VoChn.mChnId = 0;

    ret = AW_MPI_SYS_UnBind(&IseChn, &VoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind ise bind vo fail! ret:%d\n", ret);
        return FAILURE;
    }

    return ret;
}


int mpp_comm_ise_bind_venc(int ise_grp, int ise_chn, int venc_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S VeChn, IseChn;

    IseChn.mModId = MOD_ID_ISE;
    IseChn.mDevId = ise_grp;
    IseChn.mChnId = ise_chn;

    VeChn.mModId = MOD_ID_VENC;
    VeChn.mDevId = 0;
    VeChn.mChnId = venc_chn;

    ret = AW_MPI_SYS_Bind(&IseChn, &VeChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind ise bind venc fail! ret:%d\n", ret);
        return FAILURE;
    }

    return ret;
}

int mpp_comm_ise_unbind_venc(int ise_grp, int ise_chn, int venc_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S VeChn, IseChn;

    IseChn.mModId = MOD_ID_ISE;
    IseChn.mDevId = ise_grp;
    IseChn.mChnId = ise_chn;

    VeChn.mModId = MOD_ID_VENC;
    VeChn.mDevId = 0;
    VeChn.mChnId = venc_chn;

    ret = AW_MPI_SYS_UnBind(&IseChn, &VeChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind ise bind venc fail! ret:%d\n", ret);
        return FAILURE;
    }

    return ret;
}

