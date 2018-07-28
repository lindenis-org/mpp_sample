/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_comm_vdec.c
 * @brief 该目录是对mpp中VDEC模块的公共操作,参数设置和获取类型进行简单抽象
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
 * @brief  Create vdec channel   .guixing
 * @param
 * - vdec_chn      input
 * - vdec_cfg       input
 * - cb_info          input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_vdec_create(VDEC_CHN vdec_chn, VDEC_CFG_S *vdec_cfg, MPPCallbackInfo *cb_info)
{
    int  ret = 0;
    VDEC_CHN_ATTR_S vdec_attr;


    if (vdec_chn < 0 || vdec_chn >= DEMUX_MAX_CHN_NUM) {
        ERR_PRT("Input vdec_chn:%d is error! 0~%d \n", vdec_chn, DEMUX_MAX_CHN_NUM);
        return -1;
    }

    if (NULL == vdec_cfg || NULL == cb_info) {
        ERR_PRT("Input vdec_cfg or cb_info is null!\n");
        return -1;
    }

    memset(&vdec_attr, 0, sizeof(VDEC_CHN_ATTR_S));
    vdec_attr.mType                         = vdec_cfg->codec_type;
    vdec_attr.mPicWidth                     = vdec_cfg->width;
    vdec_attr.mPicHeight                    = vdec_cfg->height;
    vdec_attr.mInitRotation                 = vdec_cfg->rotation;
    vdec_attr.mOutputPixelFormat            = MM_PIXEL_FORMAT_YVU_PLANAR_420;
    vdec_attr.mVdecVideoAttr.mMode          = VIDEO_MODE_FRAME;
    vdec_attr.mVdecVideoAttr.mSupportBFrame = 0;

    ret = AW_MPI_VDEC_CreateChn(vdec_chn, &vdec_attr);
    if (ERR_VDEC_EXIST == ret) {
        ERR_PRT("Vdec chn[%d] is exist, find next!\n", vdec_chn);
        return -1;
    } else if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_VDEC_CreateChn vdec_chn:%d fail! ret:0x%x\n", vdec_chn, ret);
        return -1;
    }
    DB_PRT("Create vdec_chn:%d success!\n", vdec_chn);

    ret = AW_MPI_VDEC_RegisterCallback(vdec_chn, cb_info);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_DEMUX_RegisterCallback vdec_chn:%d fail! ret:0x%x\n", vdec_chn, ret);
        return -1;
    }

    return 0;
}


int mpp_comm_vdec_destroy(VDEC_CHN vdec_chn)
{
    int ret = 0;

    ret = AW_MPI_VDEC_DestroyChn(vdec_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_VDEC_DestroyChn vdec_chn:%d fail! ret:0x%x\n", vdec_chn, ret);
        return -1;
    }

    return 0;
}


int mpp_comm_vdec_bind_vo(int vdec_chn, int vo_chn)
{
    int       ret = 0;
    MPP_CHN_S VdecChn, VoChn;

    VdecChn.mModId  = MOD_ID_VDEC;
    VdecChn.mDevId  = 0;
    VdecChn.mChnId  = vdec_chn;

    VoChn.mModId = MOD_ID_VOU;
    VoChn.mDevId = HLAY(vo_chn, 0);
    VoChn.mChnId = 0;

    ret = AW_MPI_SYS_Bind(&VdecChn, &VoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind vdec_chn:%d bind vo_chn:%d fail! ret:0x%x\n",
                vdec_chn, vo_chn, ret);
        return -1;
    }

    return ret;
}


int mpp_comm_vdec_unbind_vo(int vdec_chn, int vo_chn)
{
    int       ret = 0;
    MPP_CHN_S VdecChn, VoChn;

    VdecChn.mModId  = MOD_ID_VDEC;
    VdecChn.mDevId  = 0;
    VdecChn.mChnId  = vdec_chn;

    VoChn.mModId = MOD_ID_VOU;
    VoChn.mDevId = HLAY(vo_chn, 0);
    VoChn.mChnId = 0;

    ret = AW_MPI_SYS_UnBind(&VdecChn, &VoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind vdec_chn:%d unbind vo_chn:%d fail! ret:0x%x\n",
                vdec_chn, vo_chn, ret);
        return -1;
    }

    return ret;
}

