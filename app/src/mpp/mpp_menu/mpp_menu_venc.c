/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_menu_venc.c
 * @brief 该目录是对mpp中的 VENC 模块进行菜单设置项控制.
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-05-28
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
#include "mpp_menu.h"


/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
static int mpp_menu_venc_chn_attr_get(void *pData, char *pTitle);
static int mpp_menu_venc_bitrate_set(void *pData, char *pTitle);
static int mpp_menu_venc_bitrate_get(void *pData, char *pTitle);
static int mpp_menu_venc_framerate_set(void * pData,char * pTitle);
static int mpp_menu_venc_framerate_get(void * pData,char * pTitle);
static int mpp_menu_venc_request_idr(void *pData, char *pTitle);
static int mpp_menu_venc_gop_set(void *pData, char *pTitle);
static int mpp_menu_venc_gop_get(void *pData, char *pTitle);
static int mpp_menu_venc_crop_set(void *pData, char *pTitle);
static int mpp_menu_venc_crop_get(void *pData, char *pTitle);
static int mpp_menu_venc_roi_set(void *pData, char *pTitle);
static int mpp_menu_venc_roi_get(void *pData, char *pTitle);
static int mpp_menu_venc_3dnr_set(void *pData, char *pTitle);
static int mpp_menu_venc_3dnr_get(void *pData, char *pTitle);
static int mpp_menu_venc_color2grey_set(void * pData,char * pTitle);
static int mpp_menu_venc_color2grey_get(void * pData,char * pTitle);
static int mpp_menu_venc_intra_refresh_set(void *pData, char *pTitle);
static int mpp_menu_venc_intra_refresh_get(void *pData, char *pTitle);
static int mpp_menu_venc_smart_p_set(void *pData, char *pTitle);
static int mpp_menu_venc_smart_p_get(void *pData, char *pTitle);
static int mpp_menu_venc_frep_set(void *pData, char *pTitle);

#if 0
static int mpp_menu_venc_stream_duration_set(void *pData, char *pTitle);
static int mpp_menu_venc_stream_duration_get(void *pData, char *pTitle);
#endif


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static MENU_INODE g_venc_menu_list[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"venc Get channel attr", mpp_menu_venc_chn_attr_get,  NULL, NULL},

    {(char *)"venc set bitrate",     mpp_menu_venc_bitrate_set,    NULL, NULL},
    {(char *)"venc Get bitrate",     mpp_menu_venc_bitrate_get,    NULL, NULL},

    {(char *)"venc set framerate",   mpp_menu_venc_framerate_set,  NULL, NULL},
    {(char *)"venc Get framerate",   mpp_menu_venc_framerate_get,  NULL, NULL},

    {(char *)"venc set gop",         mpp_menu_venc_gop_set,        NULL, NULL},
    {(char *)"venc Get gop",         mpp_menu_venc_gop_get,        NULL, NULL},

    {(char *)"venc set crop",        mpp_menu_venc_crop_set,       NULL, NULL},
    {(char *)"venc Get crop",        mpp_menu_venc_crop_get,       NULL, NULL},

    {(char *)"venc set ROI",         mpp_menu_venc_roi_set,        NULL, NULL},
    {(char *)"venc Get ROI",         mpp_menu_venc_roi_get,        NULL, NULL},

    {(char *)"venc set smart P",     mpp_menu_venc_smart_p_set,    NULL, NULL},
    {(char *)"venc Get smart P",     mpp_menu_venc_smart_p_get,    NULL, NULL},

    {(char *)"venc set 3D_nr",       mpp_menu_venc_3dnr_set,       NULL, NULL},
    {(char *)"venc Get 3D_nr",       mpp_menu_venc_3dnr_get,       NULL, NULL},

    {(char *)"venc set color to grey", mpp_menu_venc_color2grey_set, NULL, NULL},
    {(char *)"venc Get color to grey", mpp_menu_venc_color2grey_get, NULL, NULL},

    {(char *)"venc set intra_refresh", mpp_menu_venc_intra_refresh_set, NULL, NULL},
    {(char *)"venc Get intra_refresh", mpp_menu_venc_intra_refresh_get, NULL, NULL},
#if 0
    {(char *)"venc set duration time",  mpp_menu_venc_stream_duration_set, NULL, NULL},
    {(char *)"venc Get duration time",  mpp_menu_venc_stream_duration_get, NULL, NULL},
#endif
    {(char *)"venc request IDR",     mpp_menu_venc_request_idr,    NULL, NULL},

    {(char *)"venc set frep (Units:MHz)", mpp_menu_venc_frep_set,  NULL, NULL},

    {(char *)"Previous Step (Quit VENC setting)", ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
int mpp_menu_venc_get(PMENU_INODE *pmenu_list_venc)
{
    if (NULL == pmenu_list_venc) {
        DB_PRT("Input pmenu_list_venc is NULL!\n");
        return -1;
    }

    *pmenu_list_venc = g_venc_menu_list;

    return 0;
}

int mpp_menu_venc_scene_choose(void *pData, char *pTitle)
{
    int  index    = 0;
    char str[256] = {0};
    MPP_MENU_VI_VENC_CFG_E *p_cfg_type = NULL;

    if (NULL == pData) {
        ERR_PRT("Input pData is NULL\n");
        return -1;
    }

    p_cfg_type = (MPP_MENU_VI_VENC_CFG_E *)pData;

    while (1) {
        printf("\n***************** Choice VI+VENC scene **************************\n");
        printf(" [0]:VI_4K@30fps + VENC(4K@30fps+VGA@30fps) \n");
        printf(" [1]:VI_4K@25fps + VENC(4K@25fps+720P@25fps) [default] \n");
        printf(" [2]:VI_2K@30fps + VENC(2K@30fps+720P@30fps) \n");
        printf(" [3]:VI_1080P@30fps + VENC(1080P@30fps+720P@30fps) \n");
        printf(" [4]:VI_2880x2160@30fps + VENC(2880x2160@30fps+1080P@30fps) \n");
        printf(" [5]:VI_2592x1944@30fps + VENC(2592x1944@30fps+1080P@30fps) \n");
        printf(" Please choose VI+VENC scene ID 0~3 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        index = is_digit_str(str);
        if (index) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        index = atoi(str);
        if (index < 0 || index > 5) {
            printf(" Input Venc scene id:%d error!\n", index);
            continue;
        }

        switch(index) {
        case 0:
            *p_cfg_type = VI_4K_30FPS_VE_4K_30FPS_VE_VGA_30FPS;
            break;
        case 1:
            *p_cfg_type = VI_4K_25FPS_VE_4K_25FPS_VE_720P_25FPS;
            break;
        case 2:
            *p_cfg_type = VI_2K_30FPS_VE_2K_30FPS_VE_720P_30FPS;
            break;
        case 3:
            *p_cfg_type = VI_1080P_30FPS_VE_1080P_30FPS_VE_720P_30FPS;
            break;
        case 4:
            *p_cfg_type = VI_2880x2160_30FPS_VE_2880x2160_30FPS_VE_1080P_30FPS;
            break;
        case 5:
            *p_cfg_type = VI_2592x1944_30FPS_VE_2592x1944_30FPS_VE_1080P_30FPS;
            break;

        default:
            break;
        }

        return 0;
    }

    return 0;
}


int mpp_menu_venc_payload_type_set(void *pData, char *pTitle)
{
    int  index    = 0;
    char str[256] = {0};
    PAYLOAD_TYPE_E *p_type = NULL;

    if (NULL == pData) {
        ERR_PRT("Input pData is NULL\n");
        return -1;
    }

    p_type = (PAYLOAD_TYPE_E *)pData;
    while (1) {
        printf("\n\n***************** Set VENC payload type **************************\n");
        printf(" [0]:H264 [default] \n");
        printf(" [1]:H265\n");
        printf(" Please choose payload type or (q-Quit): \n");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        index = is_digit_str(str);
        if (index) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        index = atoi(str);
        if (index < 0 || index > 1) {
            printf(" Input Venc payload type:%d error!\n", index);
            continue;
        }

        switch(index) {
        case 0:
            *p_type = PT_H264;
            break;
        case 1:
            *p_type = PT_H265;
            break;
        default:
            break;
        }
        return 0;
    }

    return 0;
}


int mpp_menu_venc_rc_mode_set(void *pData, char *pTitle)
{
    int  index    = 0;
    char str[256] = {0};
    unsigned int *rc_type = NULL;

    if (NULL == pData) {
        ERR_PRT("Input pData is NULL\n");
        return -1;
    }

    /* 0:CBR 1:VBR 2:FIXQp  3:ABR 4:QPMAP */
    rc_type = (unsigned int *)pData;
    while (1) {
        printf("\n\n***************** Set VENC RC mode **************************\n");
        printf(" [0]:CBR [default] \n");
        printf(" [1]:VBR\n");
        printf(" [2]:FIXQP\n");
        printf(" [3]:ABR\n");
        printf(" [4]:QPMAP\n");
        printf(" Please choose RC mode or (q-Quit): \n");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        index = is_digit_str(str);
        if (index) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        index = atoi(str);
        if (index < 0 || index > 4) {
            printf(" Input Venc code type:%d error!\n", index);
            continue;
        }

        *rc_type = index;
        return 0;
    }

    return 0;
}


int mpp_menu_venc_profile_set(void *pData, char *pTitle)
{
    int  index    = 0;
    char str[256] = {0};
    unsigned int *p_profile = NULL;

    if (NULL == pData) {
        ERR_PRT("Input pData is NULL\n");
        return -1;
    }

    /* 0: baseline  1:MP  2:HP  3: SVC-T [0,3] */
    p_profile = (unsigned int *)pData;
    while (1) {
        printf("\n\n***************** Set VENC Profile **************************\n");
        printf(" [0]:Baseline\n");
        printf(" [1]:MP [default] \n");
        printf(" [2]:HP\n");
        printf(" [3]:SVC-T\n");
        printf(" Please choose Profile or (q-Quit): \n");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        index = is_digit_str(str);
        if (index) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        index = atoi(str);
        if (index < 0 || index > 3) {
            printf(" Input Venc code type:%d error!\n", index);
            continue;
        }

        *p_profile = index;

        return 0;
    }

    return 0;
}


int mpp_menu_venc_rotate_set(void *pData, char *pTitle)
{
    int  index    = 0;
    char str[256] = {0};
    ROTATE_E *p_rotate = NULL;

    if (NULL == pData) {
        ERR_PRT("Input pData is NULL\n");
        return -1;
    }

    p_rotate = (ROTATE_E *)pData;
    while (1) {
        printf("\n\n***************** Set VENC rotate type **************************\n");
        printf(" [0]:ROTATE_NONE [default] \n");
        printf(" [1]:ROTATE_90  \n");
        printf(" [2]:ROTATE_180 \n");
        printf(" [3]:ROTATE_270 \n");
        printf(" Please choose rotate or (q-Quit): \n");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        index = is_digit_str(str);
        if (index) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        index = atoi(str);
        if (index < 0 || index > 3) {
            printf(" Input Venc rotate type:%d error!\n", index);
            continue;
        }

        switch(index) {
        case 0:
            *p_rotate = ROTATE_NONE;
            break;
        case 1:
            *p_rotate = ROTATE_90;
            break;
        case 2:
            *p_rotate = ROTATE_180;
            break;
        case 3:
            *p_rotate = ROTATE_270;
            break;
        default:
            break;
        }
        return 0;
    }

    return 0;
}


static int mpp_menu_venc_chn_attr_get(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  chn      = 0;
    char str[256] = {0};
    VENC_CHN_ATTR_S venc_attr = {0};

    while (1) {
        printf("\n\n***************** Get VENC channel attr **************************\n");
        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VENC_GetChnAttr(chn, &venc_attr);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_GetChnAttr chn:%d fail. ret:0x%x \n", chn, ret);
            return ret;
        }

        printf("\n================ VENC Chn[%d] VeAttr ===============\n", chn);
        printf(" VeAttr.Type:%d \n", venc_attr.VeAttr.Type);
        printf(" VeAttr.SrcPicWidth:%d  SrcPicHeight:%d\n", venc_attr.VeAttr.SrcPicWidth, venc_attr.VeAttr.SrcPicHeight);
        printf(" VeAttr.MaxKeyInterval:%d  Field:%d  PixelFormat:%d  Rotate:%d\n\n",
               venc_attr.VeAttr.MaxKeyInterval, venc_attr.VeAttr.Field, venc_attr.VeAttr.PixelFormat, venc_attr.VeAttr.Rotate);

        switch (venc_attr.VeAttr.Type) {
        case PT_H264:
            printf(" VENC_ATTR_H264.MaxPicWidth:%d  MaxPicHeight:%d  PicWidth:%d  PicHeight:%d\n",
                   venc_attr.VeAttr.AttrH264e.MaxPicWidth, venc_attr.VeAttr.AttrH264e.MaxPicHeight,
                   venc_attr.VeAttr.AttrH264e.PicWidth, venc_attr.VeAttr.AttrH264e.PicHeight);
            printf(" VENC_ATTR_H264.Profile:%d  bByFrame:%d  BFrameNum:%d  RefNum:%d\n",
                   venc_attr.VeAttr.AttrH264e.Profile, venc_attr.VeAttr.AttrH264e.bByFrame,
                   venc_attr.VeAttr.AttrH264e.BFrameNum, venc_attr.VeAttr.AttrH264e.RefNum);
            printf(" VENC_ATTR_H264.BufSize:%d  FastEncFlag:%d  IQpOffset:%d\n",
                   venc_attr.VeAttr.AttrH264e.BufSize, venc_attr.VeAttr.AttrH264e.FastEncFlag, venc_attr.VeAttr.AttrH264e.IQpOffset);
            printf(" VENC_ATTR_H264.mbPIntraEnable:%d\n", venc_attr.VeAttr.AttrH264e.mbPIntraEnable);
            break;

        case PT_H265:
            printf(" VENC_ATTR_H265.mMaxPicWidth:%d  mMaxPicHeight:%d  mPicWidth:%d  mPicHeight:%d\n",
                   venc_attr.VeAttr.AttrH265e.mMaxPicWidth, venc_attr.VeAttr.AttrH265e.mMaxPicHeight,
                   venc_attr.VeAttr.AttrH265e.mPicWidth, venc_attr.VeAttr.AttrH265e.mPicHeight);
            printf(" VENC_ATTR_H265.mProfile:%d  mbByFrame:%d  mRefNum:%d\n",
                   venc_attr.VeAttr.AttrH265e.mProfile, venc_attr.VeAttr.AttrH265e.mbByFrame, venc_attr.VeAttr.AttrH265e.mRefNum);
            printf(" VENC_ATTR_H265.mBufSize:%d  mBFrameNum:%d  mbPIntraEnable:%d\n",
                   venc_attr.VeAttr.AttrH265e.mBufSize, venc_attr.VeAttr.AttrH265e.mBFrameNum, venc_attr.VeAttr.AttrH265e.mbPIntraEnable);
            break;

        default:
            DB_PRT("Venc_chn:%d input mRcMode:%d don't support!\n", chn, venc_attr.RcAttr.mRcMode);
            return -1;
            break;
        }

        printf("\n================ VENC Chn[%d] RcAttr ================\n", chn);
        printf(" RcAttr.mRcMode:%d \n", venc_attr.RcAttr.mRcMode);
        switch (venc_attr.RcAttr.mRcMode) {
        case VENC_RC_MODE_H264CBR:
            printf(" VENC_ATTR_H264_CBR.mBitRate:%d  SrcFrmRate:%d  DstFrmRate:%d  mGop:%d \n",
                   venc_attr.RcAttr.mAttrH264Cbr.mBitRate, venc_attr.RcAttr.mAttrH264Cbr.mSrcFrmRate,
                   venc_attr.RcAttr.mAttrH264Cbr.fr32DstFrmRate, venc_attr.RcAttr.mAttrH264Cbr.mGop);
            printf(" VENC_ATTR_H264_CBR.mStatTime:%d  mFluctuateLevel:%d\n",
                   venc_attr.RcAttr.mAttrH264Cbr.mStatTime, venc_attr.RcAttr.mAttrH264Cbr.mFluctuateLevel);
            break;
        case VENC_RC_MODE_H264VBR:
            printf(" VENC_ATTR_H264_VBR.mMaxBitRate:%d  SrcFrmRate:%d  DstFrmRate:%d  mGop:%d \n",
                   venc_attr.RcAttr.mAttrH264Vbr.mMaxBitRate, venc_attr.RcAttr.mAttrH264Vbr.mSrcFrmRate,
                   venc_attr.RcAttr.mAttrH264Vbr.fr32DstFrmRate, venc_attr.RcAttr.mAttrH264Vbr.mGop);
            printf(" VENC_ATTR_H264_VBR.mMaxQp:%d  mMinQp:%d  mStatTime:%d \n",
                   venc_attr.RcAttr.mAttrH264Vbr.mMaxQp, venc_attr.RcAttr.mAttrH264Vbr.mMinQp,
                   venc_attr.RcAttr.mAttrH264Vbr.mStatTime);
            break;
        case VENC_RC_MODE_H264ABR:
            printf(" VENC_ATTR_H264_ABR.mAvgBitRate:%d  mMaxBitRate:%d  SrcFrmRate:%d  DstFrmRate:%d  \n",
                   venc_attr.RcAttr.mAttrH264Abr.mAvgBitRate, venc_attr.RcAttr.mAttrH264Abr.mMaxBitRate,
                   venc_attr.RcAttr.mAttrH264Abr.mSrcFrmRate, venc_attr.RcAttr.mAttrH264Abr.fr32DstFrmRate);
            printf(" VENC_ATTR_H264_ABR.mGop:%d  mStatTime:%d  mMinStaticPercent:%d  mMaxIQp:%d  mMinIQp:%d \n",
                   venc_attr.RcAttr.mAttrH264Abr.mGop, venc_attr.RcAttr.mAttrH264Abr.mStatTime,
                   venc_attr.RcAttr.mAttrH264Abr.mMinStaticPercent, venc_attr.RcAttr.mAttrH264Abr.mMaxIQp,
                   venc_attr.RcAttr.mAttrH264Abr.mMinIQp);
            break;
        case VENC_RC_MODE_H264FIXQP:
            printf(" VENC_ATTR_H264_FIXQP.SrcFrmRate:%d  DstFrmRate:%d  mGop:%d  mIQp:%d  mPQp:%d\n",
                   venc_attr.RcAttr.mAttrH264FixQp.mSrcFrmRate, venc_attr.RcAttr.mAttrH264FixQp.fr32DstFrmRate,
                   venc_attr.RcAttr.mAttrH264FixQp.mGop, venc_attr.RcAttr.mAttrH264FixQp.mIQp, venc_attr.RcAttr.mAttrH264FixQp.mPQp);
            break;
        case VENC_RC_MODE_H264QPMAP:
            printf(" VENC_ATTR_H264_QPMAP.SrcFrmRate:%d  DstFrmRate:%d  mGop:%d \n",
                   venc_attr.RcAttr.mAttrH264QpMap.mSrcFrmRate, venc_attr.RcAttr.mAttrH264QpMap.fr32DstFrmRate,
                   venc_attr.RcAttr.mAttrH264QpMap.mGop);
            break;

        case VENC_RC_MODE_H265CBR:
            printf(" VENC_ATTR_H265_CBR.mBitRate:%d  SrcFrmRate:%d  DstFrmRate:%d  mGop:%d \n",
                   venc_attr.RcAttr.mAttrH265Cbr.mBitRate, venc_attr.RcAttr.mAttrH265Cbr.mSrcFrmRate,
                   venc_attr.RcAttr.mAttrH265Cbr.fr32DstFrmRate, venc_attr.RcAttr.mAttrH265Cbr.mGop);
            printf(" VENC_ATTR_H265_CBR.mStatTime:%d  mFluctuateLevel:%d\n",
                   venc_attr.RcAttr.mAttrH265Cbr.mStatTime, venc_attr.RcAttr.mAttrH265Cbr.mFluctuateLevel);
            break;
        case VENC_RC_MODE_H265VBR:
            printf(" VENC_ATTR_H265_VBR.mMaxBitRate:%d  SrcFrmRate:%d  DstFrmRate:%d  mGop:%d \n",
                   venc_attr.RcAttr.mAttrH265Vbr.mMaxBitRate, venc_attr.RcAttr.mAttrH265Vbr.mSrcFrmRate,
                   venc_attr.RcAttr.mAttrH265Vbr.fr32DstFrmRate, venc_attr.RcAttr.mAttrH265Vbr.mGop);
            printf(" VENC_ATTR_H265_VBR.mMaxQp:%d  mMinQp:%d  mStatTime:%d \n",
                   venc_attr.RcAttr.mAttrH265Vbr.mMaxQp, venc_attr.RcAttr.mAttrH265Vbr.mMinQp,
                   venc_attr.RcAttr.mAttrH265Vbr.mStatTime);
            break;
        case VENC_RC_MODE_H265FIXQP:
            printf(" VENC_ATTR_H265_FIXQP.SrcFrmRate:%d  DstFrmRate:%d  mGop:%d  mIQp:%d  mPQp:%d \n",
                   venc_attr.RcAttr.mAttrH265FixQp.mSrcFrmRate, venc_attr.RcAttr.mAttrH265FixQp.fr32DstFrmRate,
                   venc_attr.RcAttr.mAttrH265FixQp.mGop, venc_attr.RcAttr.mAttrH265FixQp.mIQp, venc_attr.RcAttr.mAttrH265FixQp.mPQp);
            break;
        case VENC_RC_MODE_H265QPMAP:
            printf(" VENC_ATTR_H265_QPMAP.SrcFrmRate:%d  DstFrmRate:%d  mGop:%d \n",
                   venc_attr.RcAttr.mAttrH265QpMap.mSrcFrmRate, venc_attr.RcAttr.mAttrH265QpMap.fr32DstFrmRate,
                   venc_attr.RcAttr.mAttrH265QpMap.mGop);
            break;

        default:
            DB_PRT("Venc_chn:%d input mRcMode:%d don't support!\n", chn, venc_attr.RcAttr.mRcMode);
            return -1;
            break;
        }

        //return ret;
    }

    return 0;
}


static int mpp_menu_venc_bitrate_set(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  val      = 0;
    int  tmp      = 0;
    int  chn      = 0;
    char str[256] = {0};
    VENC_CHN_ATTR_S venc_attr = {0};

    while (1) {
        printf("\n***************** Setting VENC BitRate kbps **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        printf("\n Please Input BitRate 16kbps~20000kbps or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        val = atoi(str);
        if (val < 16 || val > 20000) {
            printf(" Input BitRate value:%dkbps error!\n", val);
            continue;
        }

        ret = AW_MPI_VENC_GetChnAttr(chn, &venc_attr);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_GetChnAttr chn:%d fail. ret:0x%x \n", chn, ret);
            return ret;
        }
        switch (venc_attr.RcAttr.mRcMode) {
        case VENC_RC_MODE_H264CBR:
            tmp = venc_attr.RcAttr.mAttrH264Cbr.mBitRate / 1000;
            venc_attr.RcAttr.mAttrH264Cbr.mBitRate = val * 1000;
            break;
        case VENC_RC_MODE_H264VBR:
            tmp = venc_attr.RcAttr.mAttrH264Vbr.mMaxBitRate / 1000;
            venc_attr.RcAttr.mAttrH264Vbr.mMaxBitRate = val * 1000;
            break;
        case VENC_RC_MODE_H264ABR:
            tmp = venc_attr.RcAttr.mAttrH264Abr.mAvgBitRate / 1000;
            venc_attr.RcAttr.mAttrH264Abr.mAvgBitRate = val * 1000;
            break;
        case VENC_RC_MODE_H264FIXQP:
        case VENC_RC_MODE_H264QPMAP:
            break;

        case VENC_RC_MODE_H265CBR:
            tmp = venc_attr.RcAttr.mAttrH265Cbr.mBitRate / 1000;
            venc_attr.RcAttr.mAttrH265Cbr.mBitRate = val * 1000;
            break;
        case VENC_RC_MODE_H265VBR:
            tmp = venc_attr.RcAttr.mAttrH265Vbr.mMaxBitRate / 1000;
            venc_attr.RcAttr.mAttrH265Vbr.mMaxBitRate = val * 1000;
            break;
        case VENC_RC_MODE_H265FIXQP:
        case VENC_RC_MODE_H265QPMAP:
            break;

        default:
            DB_PRT("Input mRcMode:%d don't support!\n", venc_attr.RcAttr.mRcMode);
            return -1;
            break;
        }

        ret = AW_MPI_VENC_SetChnAttr(chn, &venc_attr);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_SetChnAttr venc_chn:%d bitrate:(%dkbps)-->(%dkbps) fail! ret:0x%x\n", chn, tmp, val, ret);
        } else {
            DB_PRT(" Do AW_MPI_VENC_SetChnAttr venc_chn:%d bitrate:(%dkbps)-->(%dkbps) success! ret:0x%x\n", chn, tmp, val, ret);
        }
        return ret;
    }

    return 0;
}


static int mpp_menu_venc_bitrate_get(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  val      = 0;
    int  chn      = 0;
    char str[256] = {0};
    VENC_CHN_ATTR_S venc_attr = {0};

    while (1) {
        printf("\n***************** Get VENC BitRate kbps **************************\n");
        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VENC_GetChnAttr(chn, &venc_attr);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_GetChnAttr chn:%d fail. ret:0x%x \n", chn, ret);
            return ret;
        }

        switch (venc_attr.RcAttr.mRcMode) {
        case VENC_RC_MODE_H264CBR:
            val = venc_attr.RcAttr.mAttrH264Cbr.mBitRate;
            break;
        case VENC_RC_MODE_H264VBR:
            val = venc_attr.RcAttr.mAttrH264Vbr.mMaxBitRate;
            break;
        case VENC_RC_MODE_H264ABR:
            val = venc_attr.RcAttr.mAttrH264Abr.mAvgBitRate;
            break;
        case VENC_RC_MODE_H264FIXQP:
        case VENC_RC_MODE_H264QPMAP:
            break;

        case VENC_RC_MODE_H265CBR:
            val = venc_attr.RcAttr.mAttrH265Cbr.mBitRate;
            break;
        case VENC_RC_MODE_H265VBR:
            val = venc_attr.RcAttr.mAttrH265Vbr.mMaxBitRate;
            break;
        case VENC_RC_MODE_H265FIXQP:
        case VENC_RC_MODE_H265QPMAP:
            break;

        default:
            DB_PRT("Venc_chn:%d input mRcMode:%d don't support!\n", chn, venc_attr.RcAttr.mRcMode);
            return -1;
            break;
        }

        val = val / 1000;
        DB_PRT("Do AW_MPI_VENC_GetChnAttr chn:%d (bitrate:%dkbps) ret:0x%x\n", chn, val, ret);
        //return ret;
    }

    return 0;
}


static int mpp_menu_venc_framerate_set(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  chn      = 0;
    char str[256] = {0};
    VENC_FRAME_RATE_S old_framerate = {0};
    VENC_FRAME_RATE_S new_framerate = {0};

    while (1) {
        printf("\n***************** Setting VENC FrameRate fps **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VENC_GetFrameRate(chn, &old_framerate);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_GetFrameRate chn:%d fail. ret:0x%x \n", chn, ret);
            return ret;
        }

        printf(" The current frame rate src:%dfps dst:%dfps \n", old_framerate.SrcFrmRate, old_framerate.DstFrmRate);
        printf("\n Please Input FrameRate Src 1fps~30fps or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        new_framerate.SrcFrmRate = atoi(str);
        if (new_framerate.SrcFrmRate < 1 || new_framerate.SrcFrmRate > 30) {
            printf(" Input src FrameRate:%dfps error!\n", new_framerate.SrcFrmRate);
            continue;
        }

        printf(" Please Input FrameRate Dst 1fps~30fps or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        new_framerate.DstFrmRate = atoi(str);
        if (new_framerate.DstFrmRate < 1 || new_framerate.DstFrmRate > 30) {
            printf(" Input dst FrameRate:%dfps error!\n", new_framerate.DstFrmRate);
            continue;
        }

        ret = AW_MPI_VENC_SetFrameRate(chn, &new_framerate);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_SetFrameRate chn:%d fail. ret:0x%x \n", chn, ret);
            return ret;
        }
        DB_PRT(" Do AW_MPI_VENC_SetFrameRate venc_chn:%d FrameRate:(%d-%d)fps-->(%d-%d)fps success! ret:0x%x\n",
               chn, old_framerate.SrcFrmRate, old_framerate.DstFrmRate,
               new_framerate.SrcFrmRate, new_framerate.DstFrmRate, ret);
        return ret;
    }

    return 0;
}


static int mpp_menu_venc_framerate_get(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  chn      = 0;
    char str[256] = {0};
    VENC_FRAME_RATE_S framerate = {0};

    while (1) {
        printf("\n***************** Setting VENC FrameRate fps **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VENC_GetFrameRate(chn, &framerate);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_GetFrameRate chn:%d fail. ret:0x%x \n", chn, ret);
            return ret;
        }

        printf(" The current frame rate src:%dfps dst:%dfps \n", framerate.SrcFrmRate, framerate.DstFrmRate);
    }

    return 0;
}


static int mpp_menu_venc_gop_set(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  val      = 0;
    int  tmp      = 0;
    int  chn      = 0;
    char str[256] = {0};
    VENC_CHN_ATTR_S venc_attr = {0};

    while (1) {
        printf("\n***************** Setting VENC GOP value **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        printf("\n Please Input GOP 1~1000 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        val = atoi(str);
        if (val < 1 || val > 1000) {
            printf(" Input GOP value:%d error!\n", val);
            continue;
        }

        ret = AW_MPI_VENC_GetChnAttr(chn, &venc_attr);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_GetChnAttr chn:%d fail. ret:0x%x \n", chn, ret);
            return ret;
        }

        tmp = venc_attr.VeAttr.MaxKeyInterval;
        venc_attr.VeAttr.MaxKeyInterval = val;
        switch (venc_attr.RcAttr.mRcMode) {
        case VENC_RC_MODE_H264CBR:
            tmp = venc_attr.RcAttr.mAttrH264Cbr.mGop;
            venc_attr.RcAttr.mAttrH264Cbr.mGop= val;
            break;
        case VENC_RC_MODE_H264VBR:
            tmp = venc_attr.RcAttr.mAttrH264Vbr.mGop;
            venc_attr.RcAttr.mAttrH264Vbr.mGop = val;
            break;
        case VENC_RC_MODE_H264ABR:
            tmp = venc_attr.RcAttr.mAttrH264Abr.mGop;
            venc_attr.RcAttr.mAttrH264Abr.mGop = val;
            break;
        case VENC_RC_MODE_H264FIXQP:
        case VENC_RC_MODE_H264QPMAP:
            break;

        case VENC_RC_MODE_H265CBR:
            tmp = venc_attr.RcAttr.mAttrH265Cbr.mGop;
            venc_attr.RcAttr.mAttrH265Cbr.mGop = val;
            break;
        case VENC_RC_MODE_H265VBR:
            tmp = venc_attr.RcAttr.mAttrH265Vbr.mGop;
            venc_attr.RcAttr.mAttrH265Vbr.mGop = val;
            break;
        case VENC_RC_MODE_H265FIXQP:
        case VENC_RC_MODE_H265QPMAP:
            break;

        default:
            DB_PRT("Input mRcMode:%d don't support!\n", venc_attr.RcAttr.mRcMode);
            return -1;
            break;
        }

        ret = AW_MPI_VENC_SetChnAttr(chn, &venc_attr);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_SetChnAttr venc_chn:%d gop:(%d)-->(%d) fail! ret:0x%x\n", chn, tmp, val, ret);
        } else {
            DB_PRT(" Do AW_MPI_VENC_SetChnAttr venc_chn:%d gop:(%d)-->(%d) success! ret:0x%x\n", chn, tmp, val, ret);
        }
        return ret;
    }

    return 0;
}


static int mpp_menu_venc_gop_get(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  val      = 0;
    int  chn      = 0;
    char str[256] = {0};
    VENC_CHN_ATTR_S venc_attr = {0};

    while (1) {
        printf("\n***************** Get VENC GOP value **************************\n");
        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VENC_GetChnAttr(chn, &venc_attr);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_GetChnAttr chn:%d fail. ret:0x%x \n", chn, ret);
            return ret;
        }

        switch (venc_attr.RcAttr.mRcMode) {
        case VENC_RC_MODE_H264CBR:
            val = venc_attr.RcAttr.mAttrH264Cbr.mGop;
            break;
        case VENC_RC_MODE_H264VBR:
            val = venc_attr.RcAttr.mAttrH264Vbr.mGop;
            break;
        case VENC_RC_MODE_H264ABR:
            val = venc_attr.RcAttr.mAttrH264Abr.mGop;
            break;
        case VENC_RC_MODE_H264FIXQP:
        case VENC_RC_MODE_H264QPMAP:
            break;

        case VENC_RC_MODE_H265CBR:
            val = venc_attr.RcAttr.mAttrH265Cbr.mGop;
            break;
        case VENC_RC_MODE_H265VBR:
            val = venc_attr.RcAttr.mAttrH265Vbr.mGop;
            break;
        case VENC_RC_MODE_H265FIXQP:
        case VENC_RC_MODE_H265QPMAP:
            break;

        default:
            DB_PRT("Venc_chn:%d input mRcMode:%d don't support!\n", chn, venc_attr.RcAttr.mRcMode);
            return -1;
            break;
        }

        DB_PRT("Do AW_MPI_VENC_GetChnAttr chn:%d (GOP:%d) ret:0x%x\n\n", chn, val, ret);
        //return ret;
    }

    return 0;
}


static int mpp_menu_venc_crop_set(void *pData, char *pTitle)
{
    int  ret = 0, i = 0, num = 0, end = 0, val = 0;
    int  error_flag = 0, chn = 0, cnt = 0;
    char tmp[128] = {0};
    char str[256] = {0};
    VENC_CROP_CFG_S crop_cfg = {0};

    while (1) {
        printf("\n***************** Setting VENC Crop **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        printf("\n Please Input crop_enable [flag x y w h] val. \ne.g 1 32 32 640 640 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        printf("---%s\n", str);
        memset(tmp, 0, sizeof(tmp));
        for (i = 0, num = 0, end = 0; 0 != str[i] && 0 == error_flag; i++) {
            if (' ' != str[i]) {
                tmp[cnt] = str[i];
                end = 1;
                cnt++;
            }

            if ((0 != end && ' ' == str[i]) || 0 == str[i+1]) {
                ret = is_digit_str(tmp);
                if (ret) {
                    printf(" Input %s error.\n\n", tmp);
                    error_flag = 1;
                    break;
                }

                val = atoi(tmp);
                switch(num) {
                case 0:
                    if (val)
                        crop_cfg.bEnable = TRUE;
                    else
                        crop_cfg.bEnable = FALSE;
                    break;
                case 1:
                    if ((val < 0) || (0 != (val%16))) {
                        printf(" Input X:%d value error! must be x>0 && 0==(x%%16) \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    crop_cfg.Rect.X = val;
                    break;
                case 2:
                    if (val < 0) {
                        printf(" Input Y:%d value error! must be y>0 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    crop_cfg.Rect.Y = val;
                    break;
                case 3:
                    if (val < 0) {
                        printf(" Input Width:%d value error! must be Width>0 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    crop_cfg.Rect.Width = val;
                    break;
                case 4:
                    if (val < 0) {
                        printf(" Input Height:%d value error! must be Height>0 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    crop_cfg.Rect.Height = val;
                    break;
                default:
                    break;
                }

                num++;
                end = 0;
                cnt = 0;
                memset(tmp, 0, sizeof(tmp));
            }
        }

        if (0 != error_flag || 5 != num) {
            ERR_PRT(" Input param error error_flag:%d num:%d! OK for e.g 1 32 32 640 640 or (q-Quit) \n\n",
                    error_flag, num);
            continue;
        }

        ret = AW_MPI_VENC_SetCrop(chn, &crop_cfg);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_SetCrop venc_chn:%d enable_flag:%d x-y(%d-%d) w-h(%d-%d) fail! ret:0x%x\n",
                    chn, crop_cfg.bEnable, crop_cfg.Rect.X, crop_cfg.Rect.Y, crop_cfg.Rect.Width, crop_cfg.Rect.Height, ret);
        } else {
            DB_PRT(" Do AW_MPI_VENC_SetCrop venc_chn:%d enable_flag:%d x-y(%d-%d) w-h(%d-%d) success! ret:0x%x\n",
                   chn, crop_cfg.bEnable, crop_cfg.Rect.X, crop_cfg.Rect.Y, crop_cfg.Rect.Width, crop_cfg.Rect.Height, ret);
        }
        return ret;
    }

    return 0;
}


static int mpp_menu_venc_crop_get(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  chn      = 0;
    char str[256] = {0};
    VENC_CROP_CFG_S crop_cfg = {0};

    while (1) {
        printf("\n***************** Get VENC Crop **************************\n");
        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VENC_GetCrop(chn, &crop_cfg);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_GetCrop fail! chn:%d crop_enable:%d x-y:(%d-%d) w-h:(%d-%d) ret:0x%x\n", chn,
                    crop_cfg.bEnable, crop_cfg.Rect.X, crop_cfg.Rect.Y,
                    crop_cfg.Rect.Width, crop_cfg.Rect.Height, ret);
            return ret;
        }

        DB_PRT("Do AW_MPI_VENC_GetCrop success! chn:%d crop_enable:%d x-y:(%d-%d) w-h:(%d-%d) ret:0x%x\n", chn,
               crop_cfg.bEnable, crop_cfg.Rect.X, crop_cfg.Rect.Y,
               crop_cfg.Rect.Width, crop_cfg.Rect.Height, ret);
        //return ret;
    }

    return 0;
}


static int mpp_menu_venc_roi_set(void *pData, char *pTitle)
{
    int  ret = 0, i = 0, num = 0, end = 0, val = 0;
    int  error_flag = 0, chn = 0, cnt = 0;
    char tmp[128] = {0};
    char str[256] = {0};
    VENC_ROI_CFG_S roi_cfg = {0};

    while (1) {
        printf("\n***************** Setting VENC ROI **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        printf("\n Please Input Venc ROI index 0~7 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        roi_cfg.Index = atoi(str);
        if (roi_cfg.Index < 0 || roi_cfg.Index > 7) {
            printf(" Input Venc channel id:%d error!\n", roi_cfg.Index);
            continue;
        }

        printf("\n Please Input ROI cfg [bEnable bAbsQp Qp x y w h] val. \ne.g 1 1 40 32 32 640 640 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        printf("---%s\n", str);
        memset(tmp, 0, sizeof(tmp));
        for (i = 0, num = 0, end = 0; 0 != str[i] && 0 == error_flag; i++) {
            if (' ' != str[i]) {
                tmp[cnt] = str[i];
                end = 1;
                cnt++;
            }

            if ((0 != end && ' ' == str[i]) || 0 == str[i+1]) {
                ret = is_digit_str(tmp);
                if (ret) {
                    printf(" Input %s error.\n\n", tmp);
                    error_flag = 1;
                    break;
                }

                val = atoi(tmp);
                switch(num) {
                case 0:
                    if (val)
                        roi_cfg.bEnable = TRUE;
                    else
                        roi_cfg.bEnable = FALSE;
                    break;
                case 1:
                    if (val)
                        roi_cfg.bAbsQp = TRUE;
                    else
                        roi_cfg.bAbsQp = FALSE;
                    break;
                case 2:
                    if ((val < -50) || (val > 50)) {
                        printf(" Input Qp:%d value error! must be Qp >-51 && Qp < 51 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    roi_cfg.Qp = val;
                    break;
                case 3:
                    if ((val < 0) || (0 != (val%16))) {
                        printf(" Input X:%d value error! must be x>0 && 0==(x%%16) \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    roi_cfg.Rect.X = val;
                    break;
                case 4:
                    if ((val < 0) || (0 != (val%16))) {
                        printf(" Input Y:%d value error! must be x>0 && 0==(x%%16) \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    roi_cfg.Rect.Y = val;
                    break;
                case 5:
                    if ((val < 0) || (0 != (val%16))) {
                        printf(" Input Width:%d value error! must be Width>0 && 0==(x%%16) \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    roi_cfg.Rect.Width = val;
                    break;
                case 6:
                    if ((val < 0) || (0 != (val%16))) {
                        printf(" Input Height:%d value error! must be Height>0 && 0==(x%%16) \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    roi_cfg.Rect.Height = val;
                    break;
                default:
                    break;
                }

                num++;
                end = 0;
                cnt = 0;
                memset(tmp, 0, sizeof(tmp));
            }
        }

        if (0 != error_flag || 7 != num) {
            ERR_PRT(" Input param error error_flag:%d param_num:%d! OK for e.g 0 1 1 40 32 32 640 640 or (q-Quit) \n\n",
                    error_flag, num);
            continue;
        }

        ret = AW_MPI_VENC_SetRoiCfg(chn, &roi_cfg);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_GetRoiCfg fail! chn:%d  Index:%d  enable:%d  bAbsQp:%d  Qp:%d  x-y:(%d-%d) w-h:(%d-%d) ret:0x%x\n", chn,
                    roi_cfg.Index, roi_cfg.bEnable, roi_cfg.bAbsQp, roi_cfg.Qp,
                    roi_cfg.Rect.X, roi_cfg.Rect.Y, roi_cfg.Rect.Width, roi_cfg.Rect.Height, ret);
        } else {
            DB_PRT("Do AW_MPI_VENC_GetRoiCfg success! chn:%d  Index:%d  enable:%d  bAbsQp:%d  Qp:%d  x-y:(%d-%d) w-h:(%d-%d) ret:0x%x\n", chn,
                   roi_cfg.Index, roi_cfg.bEnable, roi_cfg.bAbsQp, roi_cfg.Qp,
                   roi_cfg.Rect.X, roi_cfg.Rect.Y, roi_cfg.Rect.Width, roi_cfg.Rect.Height, ret);
        }
        return ret;
    }

    return 0;
}


static int mpp_menu_venc_roi_get(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  chn      = 0;
    int  index    = 0;
    char str[256] = {0};
    VENC_ROI_CFG_S roi_cfg = {0};

    while (1) {
        printf("\n***************** Get VENC ROI **************************\n");
        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        printf("\n Please Input Venc ROI index 0~7 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        index = atoi(str);
        if (index < 0 || index > 7) {
            printf(" Input Venc channel id:%d error!\n", index);
            continue;
        }

        ret = AW_MPI_VENC_GetRoiCfg(chn, index, &roi_cfg);
        if (ret) {
            if (0xa0088007 == ret)
                ERR_PRT("Don't configed this chn:%d roi, so error!\n", chn);
            ERR_PRT("Do AW_MPI_VENC_GetRoiCfg fail! chn:%d  index:%d  Index:%d  enable:%d  bAbsQp:%d  Qp:%d  x-y:(%d-%d) w-h:(%d-%d) ret:0x%x\n", chn, index,
                    roi_cfg.Index, roi_cfg.bEnable, roi_cfg.bAbsQp, roi_cfg.Qp,
                    roi_cfg.Rect.X, roi_cfg.Rect.Y, roi_cfg.Rect.Width, roi_cfg.Rect.Height, ret);
            return ret;
        }

        DB_PRT("Do AW_MPI_VENC_GetRoiCfg success! chn:%d  index:%d  Index:%d  enable:%d  bAbsQp:%d  Qp:%d  x-y:(%d-%d) w-h:(%d-%d) ret:0x%x\n", chn, index,
               roi_cfg.Index, roi_cfg.bEnable, roi_cfg.bAbsQp, roi_cfg.Qp,
               roi_cfg.Rect.X, roi_cfg.Rect.Y, roi_cfg.Rect.Width, roi_cfg.Rect.Height, ret);
        //return ret;
    }

    return 0;
}


static int mpp_menu_venc_intra_refresh_set(void *pData, char *pTitle)
{
    int  ret = 0, i = 0, num = 0, end = 0, val = 0;
    int  error_flag = 0, chn = 0, cnt = 0;
    char tmp[128] = {0};
    char str[256] = {0};
    VENC_PARAM_INTRA_REFRESH_S intra_refresh = {0};

    while (1) {
        printf("\n***************** Setting VENC intra refresh attr **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        printf("\n Please Input intra_refresh [RefreshEnable  bISliceEnable  RefreshLineNum  ReqIQp] val. \ne.g [1 0 3 40] or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        printf("---%s\n", str);
        memset(tmp, 0, sizeof(tmp));
        for (i = 0, num = 0, end = 0; 0 != str[i] && 0 == error_flag; i++) {
            if (' ' != str[i]) {
                tmp[cnt] = str[i];
                end = 1;
                cnt++;
            }

            if ((0 != end && ' ' == str[i]) || 0 == str[i+1]) {
                ret = is_digit_str(tmp);
                if (ret) {
                    printf(" Input %s error.\n\n", tmp);
                    error_flag = 1;
                    break;
                }

                val = atoi(tmp);
                switch(num) {
                case 0:
                    if (val)
                        intra_refresh.bRefreshEnable = TRUE;
                    else
                        intra_refresh.bRefreshEnable = FALSE;
                    break;
                case 1:
                    if (val)
                        intra_refresh.bISliceEnable = TRUE;
                    else
                        intra_refresh.bISliceEnable = FALSE;
                    break;
                case 2:
                    if (val < 0 || val > 7) {
                        printf(" Input RefreshLineNum:%d value error! must be RefreshLineNum > 0 && RefreshLineNum < 8\n\n", val);
                        error_flag = 1;
                        break;
                    }
                    intra_refresh.RefreshLineNum = val;
                    break;
                case 3:
                    if (val < 0) {
                        printf(" Input ReqIQp:%d value error! must be ReqIQp>0 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    intra_refresh.ReqIQp = val;
                    break;
                default:
                    break;
                }

                num++;
                end = 0;
                cnt = 0;
                memset(tmp, 0, sizeof(tmp));
            }
        }

        if (0 != error_flag || 4 != num) {
            ERR_PRT(" Input param error error_flag:%d num:%d! OK for e.g 1 0 3 40 or (q-Quit) \n\n",
                    error_flag, num);
            continue;
        }

        ret = AW_MPI_VENC_SetIntraRefresh(chn, &intra_refresh);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_GetIntraRefresh fail! chn:%d  bRefreshEnable:%d  bISliceEnable:%d  RefreshLineNum:%d  ReqIQp:%d ret:0x%x\n", chn,
                    intra_refresh.bRefreshEnable, intra_refresh.bISliceEnable,
                    intra_refresh.RefreshLineNum, intra_refresh.ReqIQp, ret);
        } else {
            DB_PRT("Do AW_MPI_VENC_GetIntraRefresh success! chn:%d  bRefreshEnable:%d  bISliceEnable:%d  RefreshLineNum:%d  ReqIQp:%d ret:0x%x\n", chn,
                   intra_refresh.bRefreshEnable, intra_refresh.bISliceEnable,
                   intra_refresh.RefreshLineNum, intra_refresh.ReqIQp, ret);
        }
        return ret;
    }

    return 0;
}


static int mpp_menu_venc_intra_refresh_get(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  chn      = 0;
    char str[256] = {0};
    VENC_PARAM_INTRA_REFRESH_S intra_refresh = {0};

    while (1) {
        printf("\n***************** Get VENC intra refresh attr **************************\n");
        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VENC_GetIntraRefresh(chn, &intra_refresh);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_GetIntraRefresh fail! chn:%d  bRefreshEnable:%d  bISliceEnable:%d  RefreshLineNum:%d  ReqIQp:%d ret:0x%x\n", chn,
                    intra_refresh.bRefreshEnable, intra_refresh.bISliceEnable,
                    intra_refresh.RefreshLineNum, intra_refresh.ReqIQp, ret);
            return ret;
        }

        DB_PRT("Do AW_MPI_VENC_GetIntraRefresh success! chn:%d  bRefreshEnable:%d  bISliceEnable:%d  RefreshLineNum:%d  ReqIQp:%d ret:0x%x\n", chn,
               intra_refresh.bRefreshEnable, intra_refresh.bISliceEnable,
               intra_refresh.RefreshLineNum, intra_refresh.ReqIQp, ret);
        //return ret;
    }

    return 0;
}


static int mpp_menu_venc_smart_p_set(void *pData, char *pTitle)
{
    int  ret = 0, i = 0, num = 0, end = 0, val = 0;
    int  error_flag = 0, chn = 0, cnt = 0;
    char tmp[128] = {0};
    char str[256] = {0};
    VencSmartFun  cfg = {0};

    while (1) {
        printf("\n***************** Setting VENC Smart P **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        printf("\n Please Input Smart P cfg [smart_fun_en img_bin_en img_bin_th shift_bits] val. \ne.g 1 1 0 2 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        printf("---%s\n", str);
        memset(tmp, 0, sizeof(tmp));
        for (i = 0, num = 0, end = 0; 0 != str[i] && 0 == error_flag; i++) {
            if (' ' != str[i]) {
                tmp[cnt] = str[i];
                end = 1;
                cnt++;
            }

            if ((0 != end && ' ' == str[i]) || 0 == str[i+1]) {
                ret = is_digit_str(tmp);
                if (ret) {
                    printf(" Input %s error.\n\n", tmp);
                    error_flag = 1;
                    break;
                }

                val = atoi(tmp);
                switch(num) {
                case 0:
                    if (val)
                        cfg.smart_fun_en = TRUE;
                    else
                        cfg.smart_fun_en = FALSE;
                    break;
                case 1:
                    if (val)
                        cfg.img_bin_en = TRUE;
                    else
                        cfg.img_bin_en = FALSE;
                    break;
                case 2:
                    cfg.img_bin_th = val;
                    break;
                case 3:
                    cfg.shift_bits = val;
                    break;
                default:
                    break;
                }

                num++;
                end = 0;
                cnt = 0;
                memset(tmp, 0, sizeof(tmp));
            }
        }

        if (0 != error_flag || 4 != num) {
            ERR_PRT(" Input param error error_flag:%d param_num:%d! OK for e.g 1 1 0 2 or (q-Quit): \n\n",
                    error_flag, num);
            continue;
        }

        ret = AW_MPI_VENC_SetSmartP(chn, &cfg);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_SetSmartP fail! chn:%d  smart_fun_en:%d  img_bin_en:%d  img_bin_th:%d  shift_bits:%d ret:0x%x\n", chn,
                    cfg.smart_fun_en, cfg.img_bin_en, cfg.img_bin_th, cfg.shift_bits, ret);
        } else {
            DB_PRT("Do AW_MPI_VENC_SetSmartP success! chn:%d  smart_fun_en:%d  img_bin_en:%d  img_bin_th:%d  shift_bits:%d ret:0x%x\n", chn,
                   cfg.smart_fun_en, cfg.img_bin_en, cfg.img_bin_th, cfg.shift_bits, ret);
        }
        return ret;
    }

    return 0;
}


static int mpp_menu_venc_smart_p_get(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  chn      = 0;
    char str[256] = {0};
    VencSmartFun  cfg = {0};

    while (1) {
        printf("\n***************** Get VENC Smart P config **************************\n");
        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VENC_GetSmartP(chn, &cfg);
        if (ret) {
            ERR_PRT("Do AW_MPI_VENC_GetSmartP fail! chn:%d  smart_fun_en:%d  img_bin_en:%d  img_bin_th:%d  shift_bits:%d ret:0x%x\n", chn,
                    cfg.smart_fun_en, cfg.img_bin_en, cfg.img_bin_th, cfg.shift_bits, ret);
            return ret;
        }

        DB_PRT("Do AW_MPI_VENC_GetSmartP success! chn:%d  smart_fun_en:%d  img_bin_en:%d  img_bin_th:%d  shift_bits:%d ret:0x%x\n", chn,
               cfg.smart_fun_en, cfg.img_bin_en, cfg.img_bin_th, cfg.shift_bits, ret);
        //return ret;
    }

    return 0;
}


static int mpp_menu_venc_request_idr(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  val      = 0;
    int  chn      = 0;
    char str[256] = {0};
    BOOL bInstant = FALSE;

    while (1) {
        printf("\n***************** Setting VENC request IDR **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        printf("\n Please Input I frame bInstant 0~1 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        val = atoi(str);
        if (val) {
            bInstant = TRUE;
        } else {
            bInstant = FALSE;
        }

        ret = AW_MPI_VENC_RequestIDR(chn, bInstant);
        if (ret) {
            ERR_PRT("\n Do AW_MPI_VENC_RequestIDR chn:%d  bInstant:%d fail! ret:0x%x \n", chn, bInstant, ret);
            return ret;
        }

        DB_PRT("\n Do AW_MPI_VENC_RequestIDR chn:%d  bInstant:%d success! ret:0x%x \n", chn, bInstant, ret);

        return ret;
    }

    return 0;
}

static int mpp_menu_venc_3dnr_set(void *pData, char *pTitle)
{
    int    ret      = 0;
    int    chn      = 0;
    int    val      = 0;
    char   str[256] = {0};

    while (1) {
        printf("\n***************** Set VENC 3D_nr **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        printf(" [0]: Disable 3D_NR \n");
        printf(" [1]: Enable  3D_NR \n");
        printf("\n Please Input (0~1) or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        val = atoi(str);

        if (val) {
            ret = AW_MPI_VENC_Set3DNR(chn, TRUE);
        } else {
            ret = AW_MPI_VENC_Set3DNR(chn, FALSE);
        }
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_Set3DNR venc_chn:%d  enable_flag:(%d) fail! ret:0x%x\n", chn, val, ret);
        } else {
            DB_PRT(" Do AW_MPI_VENC_Set3DNR venc_chn:%d  enable_flag:(%d) success! ret:0x%x\n", chn, val, ret);
        }
        return ret;
    }

    return 0;
}

static int mpp_menu_venc_3dnr_get(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  chn      = 0;
    char str[256] = {0};
    BOOL b3DNRFlag = FALSE;

    while (1) {
        printf("\n***************** Get VENC 3D_nr **************************\n");
        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VENC_Get3DNR(chn, &b3DNRFlag);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_Get3DNR  chn:%d fail. ret:0x%x \n", chn, ret);
            return ret;
        }
        DB_PRT("Do AW_MPI_VENC_Get3DNR chn:%d (3DNRFlag:%d) ret:0x%x\n\n", chn, b3DNRFlag, ret);
        return ret;
    }

    return 0;
}


static int mpp_menu_venc_color2grey_set(void *pData, char *pTitle)
{
    int    ret      = 0;
    int    chn      = 0;
    int    val      = 0;
    char   str[256] = {0};
    VENC_COLOR2GREY_S chnColor2Grey;

    while (1) {
        printf("\n***************** Set VENC Color to Grey **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        printf(" [0]: Disable color2grey \n");
        printf(" [1]: Enable  color2grey \n");
        printf("\n Please Input (0~1) or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        val = atoi(str);

        if (val) {
            chnColor2Grey.bColor2Grey = TRUE;
            ret = AW_MPI_VENC_SetColor2Grey(chn, &chnColor2Grey);
        } else {
            chnColor2Grey.bColor2Grey = FALSE;
            ret = AW_MPI_VENC_SetColor2Grey(chn, &chnColor2Grey);
        }
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_SetColor2Grey venc_chn:%d  enable_flag:(%d) fail! ret:0x%x\n", chn, val, ret);
        } else {
            DB_PRT(" Do AW_MPI_VENC_SetColor2Grey venc_chn:%d  enable_flag:(%d) success! ret:0x%x\n", chn, val, ret);
        }
        return ret;
    }

    return 0;
}

static int mpp_menu_venc_color2grey_get(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  chn      = 0;
    char str[256] = {0};
    VENC_COLOR2GREY_S chnColor2Grey;

    while (1) {
        printf("\n***************** Get VENC Color to Grey value **************************\n");
        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VENC_GetColor2Grey(chn, &chnColor2Grey);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_GetColor2Grey  chn:%d fail. ret:0x%x \n", chn, ret);
            return ret;
        }
        DB_PRT("Do AW_MPI_VENC_GetColor2Grey chn:%d (enbleFlag:%d) ret:0x%x\n\n", chn, chnColor2Grey.bColor2Grey, ret);
        return ret;
    }

    return 0;
}


static int mpp_menu_venc_frep_set(void *pData, char *pTitle)
{
    int    ret      = 0;
    int    chn      = 0;
    int    val      = 0;
    char   str[256] = {0};

    while (1) {
        printf("\n***************** Set VENC frep **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        printf("\n Please Input VE frep (Units:MHz) or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        val = atoi(str);


        ret = AW_MPI_VENC_SetVEFreq(chn, val);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_SetVEFreq venc_chn:%d VEFreq:(%d) fail! ret:0x%x\n", chn, val, ret);
        } else {
            DB_PRT(" Do AW_MPI_VENC_SetVEFreq venc_chn:%d VEFreq:(%d) success! ret:0x%x\n", chn, val, ret);
        }
        return ret;
    }

    return 0;
}

#if 0
static int mpp_menu_venc_stream_duration_set(void *pData, char *pTitle)
{
    int    ret      = 0;
    int    chn      = 0;
    int    val      = 0;
    char   str[256] = {0};
    double duration = 0;

    while (1) {
        printf("\n***************** Set VENC stream duration time **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        printf("\n Please Input stream duration time (Units:ms) or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        val = atoi(str);

        duration = (val * 1.0) / 1000.0;

        ret = AW_MPI_VENC_SetMaxStreamDuration(chn, duration);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_SetMaxStreamDuration venc_chn:%d duration:(%f) fail! ret:0x%x\n", chn, duration, ret);
        } else {
            DB_PRT(" Do AW_MPI_VENC_SetMaxStreamDuration venc_chn:%d duration:(%f) success! ret:0x%x\n", chn, duration, ret);
        }
        return ret;
    }

    return 0;
}


static int mpp_menu_venc_stream_duration_get(void *pData, char *pTitle)
{
    int    ret      = 0;
    int    chn      = 0;
    char   str[256] = {0};
    double duration = 0;

    while (1) {
        printf("\n***************** Get VENC stream duration time **************************\n");

        printf(" Please Input Venc channel id 0~15 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 15) {
            printf(" Input Venc channel id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VENC_GetMaxStreamDuration(chn, &duration);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VENC_GetMaxStreamDuration venc_chn:%d duration:(%f) fail! ret:0x%x\n", chn, duration, ret);
        } else {
            DB_PRT(" Do AW_MPI_VENC_GetMaxStreamDuration venc_chn:%d duration:(%f) success! ret:0x%x\n", chn, duration, ret);
        }
        return ret;
    }

    return 0;
}
#endif

