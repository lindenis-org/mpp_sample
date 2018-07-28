/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_comm_venc.c
 * @brief 该目录是对mpp中VENC模块的公共操作,参数设置和获取类型进行简单抽象
 *        封装,以达到提高使用率和减少工作量的目的.
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-04-14
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
#define MAX_VENC_CHN 16
#define MAX_FRAME_BUF_SiZE  4096*2160*3  /* 4K YUV444 */


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/

static unsigned char *g_stream_buf[MAX_VENC_CHN] = {0};

static VENC_CFG_S g_cfg_VGA_to_VGA_1M_30fps = {
    .src_width      = 640,
    .src_height     = 480,
    .dst_width      = 640,
    .dst_height     = 480,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 1*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};

static VENC_CFG_S g_cfg_D1_to_D1_2M_30fps = {
    .src_width      = 720,
    .src_height     = 576,
    .dst_width      = 720,
    .dst_height     = 576,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 2*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};


static VENC_CFG_S g_cfg_720p_to_720p_4M_25fps = {
    .src_width      = 1280,
    .src_height     = 720,
    .dst_width      = 1280,
    .dst_height     = 720,
    .src_fps        = 25,
    .dst_fps        = 25,
    .bitrate        = 4*1000*1000,
    .gop            = 25,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};


static VENC_CFG_S g_cfg_720p_to_720p_4M_30fps = {
    .src_width      = 1280,
    .src_height     = 720,
    .dst_width      = 1280,
    .dst_height     = 720,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 4*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};

static VENC_CFG_S g_cfg_720p_to_D1_2M_30fps = {
    .src_width      = 1280,
    .src_height     = 720,
    .dst_width      = 720,
    .dst_height     = 576,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 2*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};

static VENC_CFG_S g_cfg_1080p_to_1080p_8M_30fps = {
    .src_width      = 1920,
    .src_height     = 1080,
    .dst_width      = 1920,
    .dst_height     = 1080,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 8*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};

static VENC_CFG_S g_cfg_1080p_to_720p_4M_30fps = {
    .src_width      = 1920,
    .src_height     = 1080,
    .dst_width      = 1280,
    .dst_height     = 720,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 4*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};

static VENC_CFG_S g_cfg_1080p_to_D1_2M_30fps = {
    .src_width      = 1920,
    .src_height     = 1080,
    .dst_width      = 720,
    .dst_height     = 576,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 2*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};


static VENC_CFG_S g_cfg_2K_to_2K_16M_30fps = {
    .src_width      = 2560,
    .src_height     = 1440,
    .dst_width      = 2560,
    .dst_height     = 1440,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 16*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};


static VENC_CFG_S g_cfg_4K_to_4K_18M_25fps = {
    .src_width      = 3840,
    .src_height     = 2160,
    .dst_width      = 3840,
    .dst_height     = 2160,
    .src_fps        = 25,
    .dst_fps        = 25,
    .bitrate        = 18*1000*1000,
    .gop            = 25,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};


static VENC_CFG_S g_cfg_4K_to_4K_20M_30fps = {
    .src_width      = 3840,
    .src_height     = 2160,
    .dst_width      = 3840,
    .dst_height     = 2160,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 20*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};


static VENC_CFG_S g_cfg_1024_to_1024_4M_30fps = {
    .src_width      = 1024,
    .src_height     = 1024,
    .dst_width      = 1024,
    .dst_height     = 1024,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 4*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};

static VENC_CFG_S g_cfg_2048_to_2048_10M_30fps = {
    .src_width      = 2048,
    .src_height     = 2048,
    .dst_width      = 2048,
    .dst_height     = 2048,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 10*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};

static VENC_CFG_S g_cfg_2048x512_to_2048x512_8M_30fps = {
    .src_width      = 2048,
    .src_height     = 512,
    .dst_width      = 2048,
    .dst_height     = 512,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 8*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};

static VENC_CFG_S g_cfg_4096x1024_to_4096x1024_10M_30fps = {
    .src_width      = 4096,
    .src_height     = 1024,
    .dst_width      = 4096,
    .dst_height     = 1024,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 10*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};

static VENC_CFG_S g_cfg_3840x1920_to_3840x1920_8M_15fps = {
    .src_width      = 3840,
    .src_height     = 1920,
    .dst_width      = 3840,
    .dst_height     = 1920,
    .src_fps        = 25,
    .dst_fps        = 15,
    .bitrate        = 8*1000*1000,
    .gop            = 15,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};

static VENC_CFG_S g_cfg_1280x640_to_1280x640_4M_25fps = {
    .src_width      = 1280,
    .src_height     = 640,
    .dst_width      = 1280,
    .dst_height     = 640,
    .src_fps        = 25,
    .dst_fps        = 25,
    .bitrate        = 4*1000*1000,
    .gop            = 25,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};


static VENC_CFG_S g_cfg_3840x1080_to_3840x1080_8M_25fps = {
    .src_width      = 3840,
    .src_height     = 1080,
    .dst_width      = 3840,
    .dst_height     = 1080,
    .src_fps        = 25,
    .dst_fps        = 25,
    .bitrate        = 8*1000*1000,
    .gop            = 25,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};


static VENC_CFG_S g_cfg_1920x540_to_1920x540_4M_25fps = {
    .src_width      = 1920,
    .src_height     = 540,
    .dst_width      = 1920,
    .dst_height     = 540,
    .src_fps        = 25,
    .dst_fps        = 25,
    .bitrate        = 4*1000*1000,
    .gop            = 25,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};


static VENC_CFG_S g_cfg_2880x2160_to_2880x2160_12M_30fps = {
    .src_width      = 2880,
    .src_height     = 2160,
    .dst_width      = 2880,
    .dst_height     = 2160,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 12*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};


static VENC_CFG_S g_cfg_2592x1944_to_2592x1944_10M_30fps = {
    .src_width      = 2592,
    .src_height     = 1944,
    .dst_width      = 2592,
    .dst_height     = 1944,
    .src_fps        = 30,
    .dst_fps        = 30,
    .bitrate        = 10*1000*1000,
    .gop            = 30,
    .is_by_frame    = 1,
    .field          = VIDEO_FIELD_FRAME,
    .pixel_format   = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    .max_qp         = 40,
    .min_qp         = 10,
};


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/

int mpp_comm_venc_get_cfg(VENC_CFG_TYPE_E venc_type, VENC_CFG_S *p_venc_cfg)
{
    if (NULL == p_venc_cfg) {
        ERR_PRT("Input p_venc_cfg is NULL!\n");
        return -1;
    }

    switch (venc_type) {
    case VENC_VGA_TO_VGA_1M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_VGA_to_VGA_1M_30fps, sizeof(VENC_CFG_S));
        break;
    case VENC_D1_TO_D1_2M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_D1_to_D1_2M_30fps, sizeof(VENC_CFG_S));
        break;
    case VENC_720P_TO_720P_4M_25FPS:
        memcpy(p_venc_cfg, &g_cfg_720p_to_720p_4M_25fps, sizeof(VENC_CFG_S));
        break;
    case VENC_720P_TO_720P_4M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_720p_to_720p_4M_30fps, sizeof(VENC_CFG_S));
        break;
    case VENC_720P_TO_D1_2M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_720p_to_D1_2M_30fps, sizeof(VENC_CFG_S));
        break;
    case VENC_1080P_TO_1080P_8M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_1080p_to_1080p_8M_30fps, sizeof(VENC_CFG_S));
        break;
    case VENC_1080P_TO_720P_4M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_1080p_to_720p_4M_30fps, sizeof(VENC_CFG_S));
        break;
    case VENC_1080P_TO_D1_2M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_1080p_to_D1_2M_30fps, sizeof(VENC_CFG_S));
        break;
    case VENC_2K_TO_2K_16M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_2K_to_2K_16M_30fps, sizeof(VENC_CFG_S));
        break;
    case VENC_4K_TO_4K_18M_25FPS:
        memcpy(p_venc_cfg, &g_cfg_4K_to_4K_18M_25fps, sizeof(VENC_CFG_S));
        break;
    case VENC_4K_TO_4K_20M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_4K_to_4K_20M_30fps, sizeof(VENC_CFG_S));
        break;

    case VENC_2880x2160_TO_2880x2160_12M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_2880x2160_to_2880x2160_12M_30fps, sizeof(VENC_CFG_S));
        break;
    case VENC_2592x1944_TO_2592x1944_10M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_2592x1944_to_2592x1944_10M_30fps, sizeof(VENC_CFG_S));
        break;

        /* for ise one_fish mode venc cfg */
    case VENC_1024x1024_TO_1024x1024_4M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_1024_to_1024_4M_30fps, sizeof(VENC_CFG_S));
        break;
    case VENC_2048x2048_TO_2048x2048_10M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_2048_to_2048_10M_30fps, sizeof(VENC_CFG_S));
        break;
    case VENC_4096x1024_TO_4096x1024_10M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_4096x1024_to_4096x1024_10M_30fps, sizeof(VENC_CFG_S));
        break;
    case VENC_2048x512_TO_2048x512_8M_30FPS:
        memcpy(p_venc_cfg, &g_cfg_2048x512_to_2048x512_8M_30fps, sizeof(VENC_CFG_S));
        break;

        /* for ise two_fish mode venc cfg */
    case VENC_3840x1920_TO_3840x1920_8M_15FPS:
        memcpy(p_venc_cfg, &g_cfg_3840x1920_to_3840x1920_8M_15fps, sizeof(VENC_CFG_S));
        break;
    case VENC_1280x640_TO_1280x640_4M_25FPS:
        memcpy(p_venc_cfg, &g_cfg_1280x640_to_1280x640_4M_25fps, sizeof(VENC_CFG_S));
        break;

        /* for ise two_ise mode venc cfg */
    case VENC_3840x1080_TO_3840x1080_8M_25FPS:
        memcpy(p_venc_cfg, &g_cfg_3840x1080_to_3840x1080_8M_25fps, sizeof(VENC_CFG_S));
        break;
    case VENC_1920x540_TO_1920x540_4M_25FPS:
        memcpy(p_venc_cfg, &g_cfg_1920x540_to_1920x540_4M_25fps, sizeof(VENC_CFG_S));
        break;

    default:
        ERR_PRT("Input venc_type:%d is not support!\n", venc_type);
        return -1;
    }

    return 0;
}


static int venc_get_rc_enum(PAYLOAD_TYPE_E venc_type, unsigned int rc_mode, VENC_RC_MODE_E *p_rc_enum)
{
    if (NULL == p_rc_enum) {
        ERR_PRT("Input p_rc_enum is null!\n");
        return -1;
    }

    if (PT_H264 == venc_type) {
        switch(rc_mode) {
        case 0: /* CBR mode */
            *p_rc_enum = VENC_RC_MODE_H264CBR;
            break;
        case 1: /* VBR mode */
            *p_rc_enum = VENC_RC_MODE_H264VBR;
            break;
        case 2: /* FIXQp mode */
            *p_rc_enum = VENC_RC_MODE_H264FIXQP;
            break;
        case 3: /* ABR mode */
            *p_rc_enum = VENC_RC_MODE_H264ABR;
            break;
        default:
            *p_rc_enum = VENC_RC_MODE_BUTT;
            ERR_PRT("H264 Don't support this rc_mode:%d !\n", rc_mode);
            return -1;
        }
    } else if (PT_H265 == venc_type) {
        switch(rc_mode) {
        case 0: /* CBR mode */
            *p_rc_enum = VENC_RC_MODE_H265CBR;
            break;
        case 1: /* VBR mode */
            *p_rc_enum = VENC_RC_MODE_H265VBR;
            break;
        case 2: /* FIXQp mode */
            *p_rc_enum = VENC_RC_MODE_H265FIXQP;
            break;
        default:
            *p_rc_enum = VENC_RC_MODE_BUTT;
            ERR_PRT("H265 Don't support this rc_mode:%d !\n", rc_mode);
            return -1;
        }
    } else if (PT_MJPEG == venc_type) {
        switch(rc_mode) {
        case 0: /* CBR mode */
            *p_rc_enum = VENC_RC_MODE_MJPEGCBR;
            break;
        case 1: /* VBR mode */
            *p_rc_enum = VENC_RC_MODE_MJPEGVBR;
            break;
        case 2: /* FIXQp mode */
            *p_rc_enum = VENC_RC_MODE_MJPEGFIXQP;
            break;
        case 3: /* ABR mode */
            *p_rc_enum = VENC_RC_MODE_MJPEGABR;
            break;
        default:
            *p_rc_enum = VENC_RC_MODE_BUTT;
            ERR_PRT("MJPEG Don't support this rc_mode:%d !\n", rc_mode);
            return -1;
        }
    } else if (PT_JPEG == venc_type) {
        return 0;
    } else {
        ERR_PRT("Input venc_type:%d error!\n", venc_type);
        return -1;
    }

    return 0;
}


/**
 * @brief Get venc chn stream .guixing
 * @param
 * - venc_chn     input
 * - venc_type   input    H264/H256/MJPEG
 * - rc_mode      input    0:CBR 1:VBR 2:FIXQp  3:ABR
 * - profile          input    0: baseline  1:MP  2:HP  3: SVC-T [0,3]
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
ERRORTYPE mpp_comm_venc_create(VENC_CHN venc_chn, PAYLOAD_TYPE_E venc_type, unsigned int rc_mode,
                               unsigned int profile, ROTATE_E rotate, VENC_CFG_S *p_venc_cfg)
{
    ERRORTYPE ret;
    BOOL nSuccessFlag = FALSE;
    VENC_CHN_ATTR_S mVencChnAttr;
    VENC_RC_MODE_E  mRcMode;

    if (NULL == p_venc_cfg) {
        ERR_PRT("Input venc_cfg is null!\n");
        return -1;
    }

    ret = venc_get_rc_enum(venc_type, rc_mode, &mRcMode);
    if (ret) {
        ERR_PRT("Do venc_get_rc_enum fail! ret:%d \n", ret);
        return -1;
    }

    memset(&mVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));

    mVencChnAttr.VeAttr.Type           = venc_type;
    mVencChnAttr.VeAttr.Rotate         = rotate;
    mVencChnAttr.VeAttr.SrcPicWidth    = p_venc_cfg->src_width;
    mVencChnAttr.VeAttr.SrcPicHeight   = p_venc_cfg->src_height;
    mVencChnAttr.VeAttr.PixelFormat    = p_venc_cfg->pixel_format;
    mVencChnAttr.VeAttr.Field          = p_venc_cfg->field;
    mVencChnAttr.VeAttr.MaxKeyInterval = p_venc_cfg->dst_fps * 2;

    DB_PRT("==== SrcPicWidth:%d  SrcPicHeight:%d  dst_width:%d  dst_height:%d ====\n",
           mVencChnAttr.VeAttr.SrcPicWidth, mVencChnAttr.VeAttr.SrcPicHeight,
           p_venc_cfg->dst_width, p_venc_cfg->dst_height);

    if (PT_H264 == mVencChnAttr.VeAttr.Type) {
        mVencChnAttr.VeAttr.AttrH264e.bByFrame       = p_venc_cfg->is_by_frame;
        mVencChnAttr.VeAttr.AttrH264e.PicWidth       = p_venc_cfg->dst_width;
        mVencChnAttr.VeAttr.AttrH264e.PicHeight      = p_venc_cfg->dst_height;
        mVencChnAttr.VeAttr.AttrH264e.Profile        = profile;
        mVencChnAttr.VeAttr.AttrH264e.mbPIntraEnable = TRUE;
        mVencChnAttr.RcAttr.mRcMode                  = mRcMode;
        switch (rc_mode) {
        case 0: /* CBR */
            mVencChnAttr.RcAttr.mAttrH264Cbr.mGop           = p_venc_cfg->gop;
            mVencChnAttr.RcAttr.mAttrH264Cbr.mSrcFrmRate    = p_venc_cfg->src_fps;
            mVencChnAttr.RcAttr.mAttrH264Cbr.fr32DstFrmRate = p_venc_cfg->dst_fps;
            mVencChnAttr.RcAttr.mAttrH264Cbr.mBitRate       = p_venc_cfg->bitrate;
            break;

        case 1: /* VBR */
            mVencChnAttr.RcAttr.mAttrH264Vbr.mGop           = p_venc_cfg->gop;
            mVencChnAttr.RcAttr.mAttrH264Vbr.mSrcFrmRate    = p_venc_cfg->src_fps;
            mVencChnAttr.RcAttr.mAttrH264Vbr.fr32DstFrmRate = p_venc_cfg->dst_fps;
            mVencChnAttr.RcAttr.mAttrH264Vbr.mMaxBitRate    = p_venc_cfg->bitrate;
            mVencChnAttr.RcAttr.mAttrH264Vbr.mMaxQp         = p_venc_cfg->max_qp;
            mVencChnAttr.RcAttr.mAttrH264Vbr.mMinQp         = p_venc_cfg->min_qp;
            break;

        case 2: /* FIXQp */
            mVencChnAttr.RcAttr.mAttrH264FixQp.mGop           = p_venc_cfg->gop;
            mVencChnAttr.RcAttr.mAttrH264FixQp.mSrcFrmRate    = p_venc_cfg->src_fps;
            mVencChnAttr.RcAttr.mAttrH264FixQp.fr32DstFrmRate = p_venc_cfg->dst_fps;
            mVencChnAttr.RcAttr.mAttrH264FixQp.mIQp           = (p_venc_cfg->max_qp + p_venc_cfg->min_qp) / 2;
            mVencChnAttr.RcAttr.mAttrH264FixQp.mPQp           = (p_venc_cfg->max_qp + p_venc_cfg->min_qp) / 2;
            break;
        }
    } else if (PT_H265 == mVencChnAttr.VeAttr.Type) {
        mVencChnAttr.VeAttr.AttrH265e.mbByFrame      = p_venc_cfg->is_by_frame;
        mVencChnAttr.VeAttr.AttrH265e.mPicWidth      = p_venc_cfg->dst_width;
        mVencChnAttr.VeAttr.AttrH265e.mPicHeight     = p_venc_cfg->dst_height;
        mVencChnAttr.VeAttr.AttrH265e.mProfile       = profile;
        mVencChnAttr.VeAttr.AttrH265e.mbPIntraEnable = TRUE;
        mVencChnAttr.RcAttr.mRcMode                  = mRcMode;

        switch (rc_mode) {
        case 0: /* CBR */
            mVencChnAttr.RcAttr.mAttrH265Cbr.mGop           = p_venc_cfg->gop;
            mVencChnAttr.RcAttr.mAttrH265Cbr.mSrcFrmRate    = p_venc_cfg->src_fps;
            mVencChnAttr.RcAttr.mAttrH265Cbr.fr32DstFrmRate = p_venc_cfg->dst_fps;
            mVencChnAttr.RcAttr.mAttrH265Cbr.mBitRate       = p_venc_cfg->bitrate;
            break;

        case 1: /* VBR */
            mVencChnAttr.RcAttr.mAttrH265Vbr.mGop           = p_venc_cfg->gop;
            mVencChnAttr.RcAttr.mAttrH265Vbr.mSrcFrmRate    = p_venc_cfg->src_fps;
            mVencChnAttr.RcAttr.mAttrH265Vbr.fr32DstFrmRate = p_venc_cfg->dst_fps;
            mVencChnAttr.RcAttr.mAttrH265Vbr.mMaxBitRate    = p_venc_cfg->bitrate;
            mVencChnAttr.RcAttr.mAttrH265Vbr.mMaxQp         = p_venc_cfg->max_qp;
            mVencChnAttr.RcAttr.mAttrH265Vbr.mMinQp         = p_venc_cfg->min_qp;
            break;

        case 2: /* FIXQp */
            mVencChnAttr.RcAttr.mAttrH265FixQp.mGop           = p_venc_cfg->gop;
            mVencChnAttr.RcAttr.mAttrH265FixQp.mSrcFrmRate    = p_venc_cfg->src_fps;
            mVencChnAttr.RcAttr.mAttrH265FixQp.fr32DstFrmRate = p_venc_cfg->dst_fps;
            mVencChnAttr.RcAttr.mAttrH265FixQp.mIQp           = (p_venc_cfg->max_qp + p_venc_cfg->min_qp) / 2;
            mVencChnAttr.RcAttr.mAttrH265FixQp.mPQp           = (p_venc_cfg->max_qp + p_venc_cfg->min_qp) / 2;
            break;
        }
    } else if (PT_MJPEG == mVencChnAttr.VeAttr.Type) {
        mVencChnAttr.VeAttr.AttrMjpeg.mbByFrame     = p_venc_cfg->is_by_frame;
        mVencChnAttr.VeAttr.AttrMjpeg.mPicWidth     = p_venc_cfg->dst_width;
        mVencChnAttr.VeAttr.AttrMjpeg.mPicHeight    = p_venc_cfg->dst_height;
        mVencChnAttr.RcAttr.mRcMode                 = mRcMode;
        mVencChnAttr.RcAttr.mAttrMjpegeCbr.mBitRate = p_venc_cfg->bitrate;
    } else if (PT_JPEG == mVencChnAttr.VeAttr.Type) {
        mVencChnAttr.VeAttr.AttrJpeg.MaxPicWidth  = p_venc_cfg->src_width;
        mVencChnAttr.VeAttr.AttrJpeg.MaxPicHeight = p_venc_cfg->src_width;
        mVencChnAttr.VeAttr.AttrJpeg.BufSize      = 0;
        mVencChnAttr.VeAttr.AttrJpeg.bByFrame     = TRUE;
        mVencChnAttr.VeAttr.AttrJpeg.PicWidth     = p_venc_cfg->dst_width;
        mVencChnAttr.VeAttr.AttrJpeg.PicHeight    = p_venc_cfg->dst_height;
    } else {
        ERR_PRT("Input venc_type:%d is not support!\n", venc_type);
        return -1;
    }

    ret = AW_MPI_VENC_CreateChn(venc_chn, &mVencChnAttr);
    if (SUCCESS == ret) {
        nSuccessFlag = TRUE;
        DB_PRT("create venc channel[%d] success!\n", venc_chn);
    } else if (ERR_VENC_EXIST == ret) {
        ERR_PRT("venc channel[%d] is exist, find next!\n", venc_chn);
    } else {
        ERR_PRT("create venc channel[%d] ret[0x%x], find next!\n", venc_chn, ret);
    }

    if (nSuccessFlag == FALSE) {
        ERR_PRT("fatal error! create venc channel fail!\n");
        return FAILURE;
    }

    VENC_FRAME_RATE_S stFrameRate;
    stFrameRate.SrcFrmRate = stFrameRate.DstFrmRate = p_venc_cfg->dst_fps;
    AW_MPI_VENC_SetFrameRate(venc_chn, &stFrameRate);

    return SUCCESS;
}


ERRORTYPE mpp_comm_venc_destroy(VENC_CHN venc_chn)
{
    ERRORTYPE ret = SUCCESS;

    ret = AW_MPI_VENC_ResetChn(venc_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_VENC_ResetChn fail! venc_chn:%d ret:%d \n", venc_chn, ret);
    }

    ret = AW_MPI_VENC_DestroyChn(venc_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_VENC_DestroyChn fail! venc_chn:%d ret:%d \n", venc_chn, ret);
    }

    for (int cnt = 0; cnt < MAX_VENC_CHN; cnt++) {
        if (NULL != g_stream_buf[cnt]) {
            free(g_stream_buf[cnt]);
            g_stream_buf[cnt] = NULL;
        }
    }

    return ret;
}


/**
 * @brief Create venc chn .guixing
 * @param
 * - venc_chn     input
 * - milli_sec       input    -1:bolck  0:nonblock   >0 : overtime
 * - buf               output    stream buf
 * - len               output    cur stream size
 * - frame_type  output  cur frame type (I, P, B)
 * - head_info   output   If cur frame is I frame , so will return  sps/pps info to user.
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
ERRORTYPE mpp_comm_venc_get_stream(VENC_CHN venc_chn, PAYLOAD_TYPE_E venc_type, int milli_sec,
                                   unsigned char **buf, unsigned int *len, uint64_t *pts, int *frame_type,
                                   VencHeaderData *head_info)
{
    ERRORTYPE     ret     = SUCCESS;
    VENC_PACK_S   mpPack;
    VENC_STREAM_S vencFrame;

    if (venc_chn < 0 || venc_chn >= MAX_VENC_CHN) {
        ERR_PRT("Input venc_chn:%d error!\n", venc_chn);
        return -1;
    }

    if (NULL == buf || NULL == len || NULL == pts || NULL == frame_type || NULL == head_info) {
        ERR_PRT("Input param is NULL! buf:%p len:%p pts:%p frame_type:%p head_info:%p \n", buf, len, pts, frame_type, head_info);
        return -1;
    }

    if (NULL == g_stream_buf[venc_chn]) {
        g_stream_buf[venc_chn] = (unsigned char *)malloc(MAX_FRAME_BUF_SiZE);
        if (NULL == g_stream_buf[venc_chn]) {
            ERR_PRT("Do venc_chn:%d g_stream_buf[] malloc(%d) error! errno[%d] errinfo[%s]\n",
                    venc_chn, MAX_FRAME_BUF_SiZE, errno, strerror(errno));
            return -1;
        }
    }

    memset(&mpPack,    0, sizeof(VENC_PACK_S));
    memset(&vencFrame, 0, sizeof(VENC_STREAM_S));

    vencFrame.mpPack     = &mpPack;
    vencFrame.mPackCount = 1;
    ret = AW_MPI_VENC_GetStream(venc_chn, &vencFrame, milli_sec); /* -1:bolck  0:nonblock   >0 : overtime  */
    if (SUCCESS == ret) {
        if (vencFrame.mpPack != NULL && vencFrame.mpPack->mLen0 > 0) {
            *pts = vencFrame.mpPack->mPTS;

            /* Check I/P frame type. */
            switch (venc_type) {
            case PT_H264:
                if (H264E_NALU_ISLICE == vencFrame.mpPack->mDataType.enH264EType) {
                    /* Get sps/pps first */
                    ret = AW_MPI_VENC_GetH264SpsPpsInfo(venc_chn, head_info);
                    if (SUCCESS != ret) {
                        ERR_PRT("Do AW_MPI_VENC_GetH264SpsPpsInfo fail! ret:%d \n", ret);
                    }

                    *frame_type = 1;
                } else if (H264E_NALU_PSLICE == vencFrame.mpPack->mDataType.enH264EType) {
                    *frame_type = 0;
                }
                break;

            case PT_H265:
                if (H265E_NALU_ISLICE == vencFrame.mpPack->mDataType.enH265EType) {
                    /* Get sps/pps first */
                    ret = AW_MPI_VENC_GetH265SpsPpsInfo(venc_chn, head_info);
                    if (SUCCESS != ret) {
                        ERR_PRT("Do AW_MPI_VENC_GetH264SpsPpsInfo fail! ret:%d \n", ret);
                    }

                    *frame_type = 1;
                } else if (H265E_NALU_PSLICE == vencFrame.mpPack->mDataType.enH265EType) {
                    *frame_type = 0;
                }
                break;

            case PT_JPEG:
            case PT_MJPEG:
                break;

            default:
                AW_MPI_VENC_ReleaseStream(venc_chn, &vencFrame);
                ERR_PRT("Do venc_chn:%d input venc_type:%d not support!\n", venc_chn, venc_type);
                return -1;
            }

            if (MAX_FRAME_BUF_SiZE > vencFrame.mpPack->mLen0) {
                memcpy(g_stream_buf[venc_chn], vencFrame.mpPack->mpAddr0, vencFrame.mpPack->mLen0);
                *buf = g_stream_buf[venc_chn];
                *len = vencFrame.mpPack->mLen0;
            } else {
                AW_MPI_VENC_ReleaseStream(venc_chn, &vencFrame);
                ERR_PRT("Output frame stream is too big > MAX_FRAME_BUF_SiZE! mLen0:%d\n", vencFrame.mpPack->mLen0);
                return -1;
            }

            if (vencFrame.mpPack->mLen1 > 0) {
                if (MAX_FRAME_BUF_SiZE > (vencFrame.mpPack->mLen0 + vencFrame.mpPack->mLen1)) {
                    memcpy(g_stream_buf[venc_chn] + vencFrame.mpPack->mLen0, vencFrame.mpPack->mpAddr1, vencFrame.mpPack->mLen1);
                    *buf = g_stream_buf[venc_chn];
                    *len += vencFrame.mpPack->mLen1;
                } else {
                    AW_MPI_VENC_ReleaseStream(venc_chn, &vencFrame);
                    ERR_PRT("Output frame stream is too big > MAX_FRAME_BUF_SiZE:%d! mLen0:%d  mLen1:%d\n",
                            MAX_FRAME_BUF_SiZE, vencFrame.mpPack->mLen0, vencFrame.mpPack->mLen0);
                    return -1;
                }
            }
        }

        AW_MPI_VENC_ReleaseStream(venc_chn, &vencFrame);
    }

    return ret;
}


ERRORTYPE mpp_comm_venc_get_snap(VENC_CHN venc_chn, char *file_name, VENC_CFG_S *p_venc_cfg)
{
    ERRORTYPE ret = SUCCESS;

    /* Step 1. create jpage venc chn . guixing*/

    /* Step 2. Start jpage venc recvice picture */

    /* Step 3. Get jpage picture from venc */

    /* Step 4. Save jpage to file name */

    /* Step 5. destroy jpage venc chn */

    return ret;
}


int mpp_comm_venc_bind_mux(int venc_chn, int mux_grp)
{
    int       ret = 0;
    MPP_CHN_S VeChn, MuxGrp;

    VeChn.mModId  = MOD_ID_VENC;
    VeChn.mDevId  = 0;
    VeChn.mChnId  = venc_chn;

    MuxGrp.mModId = MOD_ID_MUX;
    MuxGrp.mDevId = 0;
    MuxGrp.mChnId = mux_grp;

    ret = AW_MPI_SYS_Bind(&VeChn, &MuxGrp);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind venc_chn:%d bind mux_grp:%d fail! ret:0x%x\n",
                venc_chn, mux_grp, ret);
        return -1;
    }

    return ret;
}


int mpp_comm_venc_unbind_mux(int venc_chn, int mux_grp)
{
    int       ret = 0;
    MPP_CHN_S VeChn, MuxGrp;

    VeChn.mModId  = MOD_ID_VENC;
    VeChn.mDevId  = 0;
    VeChn.mChnId  = venc_chn;

    MuxGrp.mModId = MOD_ID_MUX;
    MuxGrp.mDevId = 0;
    MuxGrp.mChnId = mux_grp;

    ret = AW_MPI_SYS_UnBind(&VeChn, &MuxGrp);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind venc_chn:%d bind mux_grp:%d fail! ret:0x%x\n",
                venc_chn, mux_grp, ret);
        return -1;
    }

    return ret;
}

