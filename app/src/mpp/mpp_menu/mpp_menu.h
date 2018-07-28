/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_menu.h
 * @brief 该目录是对mpp中的各模块进行菜单设置项控制.
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-06-1
 */

#ifndef _MPP_MENU_H_
#define _MPP_MENU_H_

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

//#include "mpi_venc_common.h"
//#include "mpi_vi_common.h"
#include "ClockCompPortIndex.h"
#include "plat_type.h"

#include "menu.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
typedef enum tag_MPP_MENU_VI_VENC_CFG_E {
    VI_4K_30FPS_VE_4K_30FPS_VE_VGA_30FPS = 0,
    VI_4K_25FPS_VE_4K_25FPS_VE_720P_25FPS,
    VI_2K_30FPS_VE_2K_30FPS_VE_720P_30FPS,
    VI_1080P_30FPS_VE_1080P_30FPS_VE_720P_30FPS,

    VI_2880x2160_30FPS_VE_2880x2160_30FPS_VE_1080P_30FPS,
    VI_2592x1944_30FPS_VE_2592x1944_30FPS_VE_1080P_30FPS,

    MPP_MENU_VI_VENC_CFG_BOTTON,
}
MPP_MENU_VI_VENC_CFG_E;


typedef enum tag_MPP_MENU_ISE_CFG_E {
    MENU_ISE_FISH_180_MODE_2048x2048 = 0,

    MENU_ISE_FISH_360_MODE_4096x1024,

    MENU_ISE_FISH_BOTTOM_4PTZ_MODE_1024x1024,
    MENU_ISE_FISH_WALL_4PTZ_MODE_1024x1024,
    MENU_ISE_FISH_TOP_4PTZ_MODE_1024x1024,

    MENU_ISE_TWO_FISH_360_MODE_3840x1920,
    MENU_ISE_TWO_FISH_360_MODE_4096x2048,

    MENU_ISE_TWO_ISE_90_MODE_3840x1080,
    MENU_ISE_TWO_ISE_100_MODE_3840x1080,
    MENU_ISE_TWO_ISE_110_MODE_3840x1080,

    MPP_MENU_ISE_CFG_BOTTON,
} MPP_MENU_ISE_CFG_E;


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

int mpp_menu_isp_get(PMENU_INODE *pmenu_list_isp);
int mpp_menu_vi_get(PMENU_INODE * pmenu_list_vi);
int mpp_menu_venc_get(PMENU_INODE *pmenu_list_venc);
int mpp_menu_vdec_get(PMENU_INODE *pmenu_list_vdec);
int mpp_menu_vo_get(PMENU_INODE *pmenu_list_vo);
int mpp_menu_region_get(PMENU_INODE *pmenu_list_region);
int mpp_menu_audio_get(PMENU_INODE *pmenu_list_audio);

int mpp_menu_venc_scene_choose(void * pData,char * pTitle);
int mpp_menu_venc_payload_type_set(void *pData, char *pTitle);
int mpp_menu_venc_rc_mode_set(void *pData, char *pTitle);
int mpp_menu_venc_profile_set(void *pData, char *pTitle);
int mpp_menu_venc_rotate_set(void *pData, char *pTitle);

int mpp_menu_ise_mode_select(void *pData, char *pTitle);

int mpp_menu_vdec_rotate_set(void *pData, char *pTitle);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* _MPP_MENU_H_ */
