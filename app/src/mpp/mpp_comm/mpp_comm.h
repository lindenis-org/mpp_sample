/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_common.h
 * @brief 该目录是对mpp中的各模块如VI,VENC,VO等模块的公共操作,参数设置和获取类型进行简单抽象
 *        封装,以达到提高使用率和减少工作量的目的.
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-04-14
 */

#ifndef _MPP_COMMON_H_
#define _MPP_COMMON_H_

/************************************************************************************************/
/*                                      Include Files                                           */
/************************************************************************************************/

#include <stdint.h>

#include "media/mpi_sys.h"
#include "media/mpi_vi.h"
#include "media/mpi_ise.h"
#include "media/mpi_venc.h"
#include "media/mpi_vo.h"
#include "media/mpi_clock.h"
#include "media/mpi_mux.h"
#include "media/mpi_demux.h"
#include "media/mpi_vdec.h"
#include "media/mpi_isp.h"
#include "media/mpi_ai.h"
#include "media/mpi_ao.h"
#include "media/mpi_aenc.h"
#include "media/mpi_adec.h"
#include "media/mpi_region.h"

#include "media/mm_common.h"
#include "media/mm_comm_video.h"
#include "media/mm_comm_venc.h"
#include "media/mm_comm_demux.h"
#include "media/mm_comm_vdec.h"
#include "media/mm_comm_ise.h"
#include "media/mm_comm_region.h"

#include <memoryAdapter.h>
#include "ClockCompPortIndex.h"
#include "plat_type.h"
#include "sc_interface.h"
#include "ion_memmanager.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
/******************  common sys  ******************/
#define MPP_COMM_SYS_ALIGN_WIDTH  32


/******************  common  vi  ******************/
#define ISP_DEV_0   0
#define ISP_DEV_1   1

#define VI_DEV_0    0
#define VI_DEV_1    1
#define VI_DEV_2    2
#define VI_DEV_3    3
#define VI_DEV_4    4
#define VI_DEV_5    5
#define VI_DEV_6    6
#define VI_DEV_7    7
#define VI_DEV_BOTTON  8

#define VI_CHN_0    0
#define VI_CHN_1    1
#define VI_CHN_2    2
#define VI_CHN_3    3
#define VI_CHN_4    4
#define VI_CHN_5    5
#define VI_CHN_6    6
#define VI_CHN_7    7
#define VI_CHN_BOTTON  8

typedef enum tag_MPP_COM_VI_TYPE_E {
    VI_VGA_30FPS = 0,
    VI_D1_30FPS,
    VI_720P_25FPS,
    VI_720P_30FPS,
    VI_1080P_25FPS,
    VI_1080P_30FPS,
    VI_2K_30FPS,
    VI_4K_25FPS,
    VI_4K_30FPS,

    VI_2048x2048_30FPS, /* for ise fish */
    VI_1920x1920_25FPS, /* for ise fish */
    VI_1024x1024_30FPS, /* for ise fish */

    VI_2880x2160_30FPS,
    VI_2592x1944_30FPS,

    VI_TYPE_BOTTON,
}
MPP_COM_VI_TYPE_E;


/******************  common  venc  ******************/
#define VENC_CHN_0    0
#define VENC_CHN_1    1
#define VENC_CHN_2    2
#define VENC_CHN_3    3
#define VENC_CHN_4    4
#define VENC_CHN_5    5
#define VENC_CHN_6    6
#define VENC_CHN_7    7
#define VENC_CHN_BOTTON  8

typedef enum tag_VENC_CFG_TYPE_E {
    VENC_VGA_TO_VGA_1M_30FPS = 0,
    VENC_D1_TO_D1_2M_30FPS,
    VENC_720P_TO_720P_4M_25FPS,
    VENC_720P_TO_720P_4M_30FPS,
    VENC_720P_TO_D1_2M_30FPS,
    VENC_1080P_TO_1080P_8M_30FPS,
    VENC_1080P_TO_720P_4M_30FPS,
    VENC_1080P_TO_D1_2M_30FPS,
    VENC_2K_TO_2K_16M_30FPS,
    VENC_4K_TO_4K_18M_25FPS,
    VENC_4K_TO_4K_20M_30FPS,

    VENC_2880x2160_TO_2880x2160_12M_30FPS,
    VENC_2592x1944_TO_2592x1944_10M_30FPS,

    VENC_1024x1024_TO_1024x1024_4M_30FPS,   /* res for ise fish to venc */
    VENC_2048x2048_TO_2048x2048_10M_30FPS,
    VENC_2048x512_TO_2048x512_8M_30FPS,
    VENC_4096x1024_TO_4096x1024_10M_30FPS,

    VENC_3840x1920_TO_3840x1920_8M_15FPS,   /* res for two_fish to venc */
    VENC_1280x640_TO_1280x640_4M_25FPS,

    VENC_3840x1080_TO_3840x1080_8M_25FPS,   /* res for two_ise to venc */
    VENC_1920x540_TO_1920x540_4M_25FPS,

    VENC_CFG_BOTTON,
} VENC_CFG_TYPE_E;


/******************  common  ise  ******************/
#define ISE_GRP_0        0
#define ISE_GRP_1        1
#define ISE_GRP_2        2
#define ISE_GRP_3        3
#define ISE_GRP_4        4
#define ISE_GRP_5        5
#define ISE_GRP_6        6
#define ISE_GRP_7        7

#define ISE_CHN_0        0
#define ISE_CHN_1        1
#define ISE_CHN_2        2
#define ISE_CHN_3        3

/* 限制: 高必须8整除. 宽必须4整除. Hight: h%8==0   width: w%4==0 */
typedef enum tag_ISE_MODE_CFG_E {
    ISE_FISH_180_MODE_2048_TO_1024x1024 = 0,   /* 鱼眼180度模式,输入图像宽高比为 1:1 GRP输出宽高比必须为 1:1 Chn输出可进行缩放 */
    ISE_FISH_180_MODE_2048_TO_2048x2048,

    ISE_FISH_360_MODE_2048_TO_4096x1024,  /* 鱼眼360度展开模式,输入图像宽高比为 1:1 GRP输出宽高比必须为 4:1 Chn输出可进行缩放 */
    ISE_FISH_360_MODE_2048_TO_2048x512,

    /* 鱼眼 PTZ 一共有3种安装模式:顶装/壁装/地装. 输入图像宽高比为 1:1 GRP最大输出为源分辨率的1/2 */
    ISE_FISH_BOTTOM_4PTZ_MODE_GRP0_2048_TO_1024x1024,   /* 鱼眼 (地装) 4路PTZ 模式, 不同GRP组的角度各不相同 */
    ISE_FISH_BOTTOM_4PTZ_MODE_GRP1_2048_TO_1024x1024,
    ISE_FISH_BOTTOM_4PTZ_MODE_GRP2_2048_TO_1024x1024,
    ISE_FISH_BOTTOM_4PTZ_MODE_GRP3_2048_TO_1024x1024,

    ISE_FISH_WALL_4PTZ_MODE_GRP0_2048_1024x1024,     /* 鱼眼 (壁装) 4路PTZ 模式 */

    ISE_FISH_TOP_4PTZ_MODE_GRP0_2048_1024x1024,      /* 鱼眼 (顶装) 4路PTZ 模式 */

    ISE_TWO_FISH_360_MODE_1920_TO_3840x1920,  /* 双鱼眼360度模式,宽高比必须为 2:1 */
    ISE_TWO_FISH_360_MODE_2048_TO_4096x2048,

    ISE_TWO_ISE_90_MODE_1080P_TO_3840x1080,    /* 双目90度模式  */
    ISE_TWO_ISE_100_MODE_1080P_TO_3840x1080,   /* 双目100度模式 */
    ISE_TWO_ISE_110_MODE_1080P_TO_3840x1080,   /* 双目110度模式 */

    ISE_MODE_CFG_BOTTON,
} ISE_MODE_CFG_E;


/******************  common  vo  ******************/
#define MAX_VO_CHN_NUM  4

#define VO_CHN_0        0
#define VO_CHN_1        1
#define VO_CHN_2        2
#define VO_CHN_3        3
#define VO_CHN_BOTTON   4

#define CLOCK_CHN_0     0
#define CLOCK_CHN_1     1
#define CLOCK_CHN_2     2
#define CLOCK_CHN_3     3
#define CLOCK_BOTTON    4

typedef enum tag_VO_DEV_TYPE_E {
    VO_DEV_CVBS = 0,
    VO_DEV_HDMI,
    VO_DEV_LCD,

    VO_DEV_TYPE_BOTTON,
} VO_DEV_TYPE_E;


/******************  common  mux  ******************/
#define MUX_GRP_0    0
#define MUX_GRP_1    1
#define MUX_GRP_2    2
#define MUX_GRP_3    3
#define MUX_GRP_4    4
#define MUX_GRP_5    5
#define MUX_GRP_6    6
#define MUX_GRP_7    7

#define MUX_CHN_0    0
#define MUX_CHN_1    1
#define MUX_CHN_2    2
#define MUX_CHN_3    3
#define MUX_CHN_4    4
#define MUX_CHN_5    5
#define MUX_CHN_6    6
#define MUX_CHN_7    7

#define MUX_FILE_DURATION      (20*1000)
#define MUX_MAX_FILE_SIZE      (6*1024*1024)
#define MUX_FILE_ALLOCATE_LEN  0
#define MUX_CALL_BACK_FLAG     FALSE
#define MUX_CACHE_SIZE         (4*1024)

typedef enum tag_MUX_VIDEO_CFG_TYPE_E {
    MUX_VIDEO_VGA_30FPS = 0,
    MUX_VIDEO_D1_30FPS,
    MUX_VIDEO_720P_30FPS,
    MUX_VIDEO_1080P_30FPS,
    MUX_VIDEO_2K_30FPS,
    MUX_VIDEO_4K_30FPS,

    MUX_VIDEO_CFG_INVALID,
} MUX_VIDEO_CFG_TYPE_E;


/******************  common  demux  ******************/
#define DEMUX_CHN_0    0
#define DEMUX_CHN_1    1
#define DEMUX_CHN_2    2
#define DEMUX_CHN_3    3
#define DEMUX_CHN_4    4
#define DEMUX_CHN_5    5
#define DEMUX_CHN_6    6
#define DEMUX_CHN_7    7


/******************  common  vdec  ******************/
#define VDEC_CHN_0    0
#define VDEC_CHN_1    1
#define VDEC_CHN_2    2
#define VDEC_CHN_3    3
#define VDEC_CHN_4    4
#define VDEC_CHN_5    5
#define VDEC_CHN_6    6
#define VDEC_CHN_7    7


/******************  common  audio  ******************/
#define AI_DEV_0     0
#define AI_DEV_1     1

#define AI_CHN_0     0
#define AI_CHN_1     1
#define AI_CHN_2     2
#define AI_CHN_3     3

#define AO_DEV_0     0
#define AO_DEV_1     1

#define AO_CHN_0     0
#define AO_CHN_1     1

#define AENC_CHN_0   0
#define AENC_CHN_1   1
#define AENC_CHN_2   2
#define AENC_CHN_3   3

#define ADEC_CHN_0   0
#define ADEC_CHN_1   1
#define ADEC_CHN_2   2
#define ADEC_CHN_3   3

typedef enum tag_AUDIO_CFG_TYPE_E {
    AUDIO_AAC_16BIT_8K_MONO = 0,
    AUDIO_AAC_16BIT_16K_MONO,
    AUDIO_AAC_16BIT_16K_STEREO,

    AUDIO_ADPCM_16BIT_8K_MONO,
    AUDIO_ADPCM_16BIT_16K_MONO,
    AUDIO_ADPCM_16BIT_16K_STEREO,

    AUDIO_MP3_16BIT_8K_MONO,
    AUDIO_MP3_16BIT_16K_MONO,
    AUDIO_MP3_16BIT_16K_STEREO,

    AUDIO_G711A_16BIT_8K_MONO, /* G711A only support mono */
    AUDIO_G711A_16BIT_16K_MONO,

    AUDIO_CFG_INVALID,
} AUDIO_CFG_TYPE_E;



/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/******************  common  venc  ******************/
typedef struct tag_VENC_CFG_S {
    unsigned int src_width;
    unsigned int src_height;
    unsigned int dst_width;
    unsigned int dst_height;

    unsigned int src_fps;
    unsigned int dst_fps;
    unsigned int bitrate;
    unsigned int gop;

    unsigned int max_qp;
    unsigned int min_qp;

    int            is_by_frame;
    VIDEO_FIELD_E  field;
    PIXEL_FORMAT_E pixel_format;
} VENC_CFG_S;


/******************  common  vo  ******************/
typedef struct tag_VO_DEV_CFG_S {
    unsigned int res_width;
    unsigned int res_height;
} VO_DEV_CFG_S;

typedef struct tag_VO_CHN_CFG_S {
    unsigned int top;
    unsigned int left;
    unsigned int width;
    unsigned int height;
    unsigned int vo_buf_num;
} VO_CHN_CFG_S;


/******************  common  mux  ******************/
typedef struct tag_MUX_CB_PARAM_S {
    int   venc_id;
    int   grp_id;
    int   chn_id;
    int   file_cnt;
    char  file_dir[128];
} MUX_CB_PARAM_S;


/*****************  common  demux  *****************/
typedef struct tag_DEMUX_CB_PARAM_S {
    int  demux_type;  /* 0: video_type  1:audio_type */
    int  demux_chn;
    int  vdec_chn;
    int  vo_chn;
    int  adec_chn;
    int  ao_chn;
    int  run_flag;   /* 0:stop  1:running */
    int  fd;
} DEMUX_CB_PARAM_S;

typedef struct tag_DEMUX_INFO_S {
    int  width;
    int  height;
    int  frame_rate;
    PAYLOAD_TYPE_E codec_type;
} DEMUX_INFO_S;


/*****************  common  vdec  *****************/
typedef struct tag_VDEC_CFG_S {
    int width;
    int height;
    int rotation;
    PAYLOAD_TYPE_E codec_type;
} VDEC_CFG_S;


/*****************  common  audio  *****************/
typedef struct tag_AUDIO_CFG_S {
    int   chn_num;
    int   sample_frame;
    AUDIO_BIT_WIDTH_E   bit_width;
    AUDIO_SAMPLE_RATE_E sample_rate;
    PAYLOAD_TYPE_E      codec_type;
    AUDIO_SOUND_MODE_E  audio_mode;
} AUDIO_CFG_S;


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/

/******************  common sys  ******************/
/* This comm_sys_init depend on COMM_SYS_ALIGN_WIDTH */
ERRORTYPE mpp_comm_sys_init();
ERRORTYPE mpp_comm_sys_exit();


/******************   common vi   ******************/
int mpp_comm_vi_get_attr(MPP_COM_VI_TYPE_E vi_type, VI_ATTR_S *p_vi_attr);
int mpp_comm_vi_dev_create(VI_DEV vi_dev, VI_ATTR_S *p_vi_attr);
int mpp_comm_vi_dev_destroy(VI_DEV vi_dev);
int mpp_comm_vi_chn_create(VI_DEV vi_dev,VI_CHN vi_chn);
int mpp_comm_vi_chn_destroy(VI_DEV vi_dev,VI_CHN vi_chn);
int mpp_comm_vi_bind_venc(int vi_dev, int vi_chn, int venc_chn);
int mpp_comm_vi_unbind_venc(int vi_dev, int vi_chn, int venc_chn);
int mpp_comm_vi_bind_vo(int vi_dev, int vi_chn, int vo_chn);
int mpp_comm_vi_unbind_vo(int vi_dev, int vi_chn, int vo_chn);
int mpp_comm_vi_bind_ise(int vi_dev, int vi_chn, int ise_grp);
int mpp_comm_vi_unbind_ise(int vi_dev, int vi_chn, int ise_grp);


/******************   common venc   ******************/
int mpp_comm_venc_get_cfg(VENC_CFG_TYPE_E venc_type, VENC_CFG_S *p_venc_cfg);

/**
 * @brief Create venc chn .guixing
 * @param
 * - venc_chn     input
 * - venc_type   input    H264/H256/MJPEG
 * - rc_mode      input    0:CBR 1:VBR 2:FIXQp  3:ABR
 * - profile          input    0: baseline  1:MP  2:HP  3: SVC-T [0,3];
 * - p_venc_cfg   input   venc config.
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
ERRORTYPE mpp_comm_venc_create(VENC_CHN venc_chn, PAYLOAD_TYPE_E venc_type, unsigned int rc_mode,
                               unsigned int profile, ROTATE_E rotate, VENC_CFG_S *p_venc_cfg);
ERRORTYPE mpp_comm_venc_destroy(VENC_CHN venc_chn);

/**
 * @brief Get venc chn stream .guixing
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
                                   VencHeaderData *head_info);
ERRORTYPE mpp_comm_venc_get_snap(VENC_CHN venc_chn, char *file_name, VENC_CFG_S *p_venc_cfg);
int mpp_comm_venc_bind_mux(int venc_chn, int mux_grp);
int mpp_comm_venc_unbind_mux(int venc_chn, int mux_grp);


/******************   common  ise   ******************/
int mpp_comm_ise_get_cfg(ISE_MODE_CFG_E ise_type, int chn_num, ISE_CHN_ATTR_S *chn_attr);
int mpp_comm_ise_grp_create(ISE_GRP ise_grp, int ise_mode);
int mpp_comm_ise_grp_destroy(ISE_GRP ise_grp);
int mpp_comm_ise_chn_create(ISE_GRP ise_grp, ISE_CHN ise_chn, ISE_CHN_ATTR_S *chn_attr);
int mpp_comm_ise_chn_destroy(ISE_GRP ise_grp, ISE_CHN ise_chn);
int mpp_comm_ise_bind_vo(int ise_grp, int ise_chn, int vo_chn);
int mpp_comm_ise_unbind_vo(int ise_grp, int ise_chn, int vo_chn);
int mpp_comm_ise_bind_venc(int ise_grp, int ise_chn, int venc_chn);
int mpp_comm_ise_unbind_venc(int ise_grp, int ise_chn, int venc_chn);


/******************   common vo   ******************/
int mpp_comm_vo_dev_create(VO_DEV_TYPE_E dev_type, VO_DEV_CFG_S *dev_cfg);
int mpp_comm_vo_dev_destroy(VO_DEV_TYPE_E dev_type);
int mpp_comm_vo_chn_create(int vo_chn, VO_CHN_CFG_S *vo_chn_cfg);
int mpp_comm_vo_chn_destroy(int vo_chn);
int mpp_comm_vo_chn_start(int vo_chn);
int mpp_comm_vo_chn_stop(int vo_chn);
int mpp_comm_vo_bind_clock(int clock_chn,int vo_chn);
int mpp_comm_vo_unbind_clock(int clock_chn,int vo_chn);
int mpp_comm_vo_clock_create(CLOCK_CHN clock_chn);
int mpp_comm_vo_clock_destroy(CLOCK_CHN clock_chn);


/******************   common mux   ******************/
int mpp_comm_mux_video_full_cfg(PAYLOAD_TYPE_E payload_type, int width, int height, int frm_rate, MUX_GRP_ATTR_S *p_grp_attr);
int mpp_comm_mux_audio_full_cfg(AUDIO_CFG_TYPE_E audio_cfg_type,MUX_GRP_ATTR_S * p_grp_attr);
int mpp_comm_mux_grp_create(MUX_GRP mux_grp,MUX_GRP_ATTR_S * p_grp_attr,MPPCallbackInfo * cb_info);
int mpp_comm_mux_grp_destroy(MUX_GRP mux_grp);
int mpp_comm_mux_chn_create(MUX_GRP mux_grp, MUX_CHN mux_chn, MEDIA_FILE_FORMAT_E file_format, const char* file_path);
int mpp_comm_mux_chn_destroy(MUX_GRP mux_grp, MUX_CHN mux_chn);
ERRORTYPE mpp_comm_mux_callbcak(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData);


/******************   common demux   ******************/
int mpp_comm_demux_create(DEMUX_CHN demux_chn, const char *file_path, MPPCallbackInfo *cb_info, DEMUX_INFO_S *demux_info);
int mpp_comm_demux_destroy(DEMUX_CHN demux_chn);
int mpp_comm_demux_bind_vdec(int demux_chn, int vdec_chn);
int mpp_comm_demux_unbind_vdec(int demux_chn, int vdec_chn);
int mpp_comm_demux_bind_adec(int demux_chn, int adec_chn);
int mpp_comm_demux_unbind_adec(int demux_chn, int adec_chn);
int mpp_comm_demux_bind_clock(int clock_chn, int demux_chn);
int mpp_comm_demux_unbind_clock(int demux_chn, int clock_chn);
ERRORTYPE mpp_comm_demux_callbcak(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData);


/******************   common vdec   ******************/
int mpp_comm_vdec_create(VDEC_CHN vdec_chn, VDEC_CFG_S *vdec_cfg, MPPCallbackInfo *cb_info);
int mpp_comm_vdec_destroy(VDEC_CHN vdec_chn);
int mpp_comm_vdec_bind_vo(int vdec_chn, int vo_chn);
int mpp_comm_vdec_unbind_vo(int vdec_chn, int vo_chn);


/******************   common audio ******************/
int mpp_comm_get_audio_cfg(AUDIO_CFG_TYPE_E audio_type, AUDIO_CFG_S *audio_cfg);
int mpp_comm_ai_create(AI_CHN ai_chn, AUDIO_CFG_S *audio_cfg);
int mpp_comm_ai_destroy(AI_CHN ai_chn);
int mpp_comm_ai_bind_ao(int ai_chn, int ao_chn);
int mpp_comm_ai_unbind_ao(int ai_chn, int ao_chn);
int mpp_comm_ai_bind_aenc(int ai_chn, int aenc_chn);
int mpp_comm_ai_unbind_aenc(int ai_chn, int aenc_chn);
int mpp_comm_ao_create(AO_CHN ao_chn, AUDIO_CFG_S *audio_cfg, MPPCallbackInfo *cb_info);
int mpp_comm_ao_destroy(AO_CHN ao_chn);
int mpp_comm_ao_clock_create(CLOCK_CHN clock_chn);
int mpp_comm_ao_clock_destroy(CLOCK_CHN clock_chn);
int mpp_comm_ao_bind_clock(int clock_chn, int ao_chn);
int mpp_comm_ao_unbind_clock(int clock_chn, int ao_chn);
int mpp_comm_aenc_create(AENC_CHN aenc_chn, AUDIO_CFG_S *audio_cfg);
int mpp_comm_aenc_destroy(AENC_CHN aenc_chn);
int mpp_comm_adec_create(ADEC_CHN adec_chn, AUDIO_CFG_S *audio_cfg, MPPCallbackInfo *cb_info);
int mpp_comm_adec_destroy(ADEC_CHN adec_chn);
int mpp_comm_adec_bind_ao(int adec_chn,int ao_chn);
int mpp_comm_adec_unbind_ao(int adec_chn,int ao_chn);
int mpp_comm_audio_demux_create(DEMUX_CHN demux_chn, const char *file_path, MPPCallbackInfo *cb_info);
int mpp_comm_audio_demux_destroy(DEMUX_CHN demux_chn);
ERRORTYPE mpp_comm_audio_demux_callbcak(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* _MPP_COMMON_H_ */
