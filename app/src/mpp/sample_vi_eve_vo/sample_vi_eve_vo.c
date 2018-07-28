/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file sample_vi_eve_vo.c
 * @brief 该目录是对mpp中的 VI+EVE 通路sample
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-09-09
 * @author id: wangguixing
 * @version v0.2
 * @date 2017-09-26
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
#include <ctype.h>
#include <pthread.h>

#include "common.h"
#include "menu.h"
#include "mpp_comm.h"
#include "mpp_menu.h"

#include "aw_ai_eve_type.h"
#include "aw_ai_eve_event_interface.h"


/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
#define MAX_EVE_PROC_TIME    38  /*ms*/
#define VI_DELAY_FRAME_NUM   6
#define FACE_ID_ARRY_NUM     128
#define FACE_SNAP_TIME       2
#define SAVE_JPEG_FILE_DIR   "/mnt/extsd/"


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
typedef struct tag_FACE_JPEG_INFO_S {
    int flag;
    int face_id;
    int snap_time;
} FACE_JPEG_INFO_S;


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/

/*** eve parameter ***/
static int g_eve_ready   = 0;
static int g_eve_dmaflag = 0;

static VIDEO_FRAME_INFO_S g_vi_frame = {0};

static AW_AI_EVE_EVENT_FACEDET_PARAM_S g_facedet_param = {0};
static AW_AI_EVE_EVENT_RESULT_S        g_eve_result    = {0};
static AW_AI_EVE_EVENT_RESULT_S        g_eve_venc_res  = {0};

static pthread_mutex_t  g_eve_lock       = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t  g_face_venc_lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_t        g_vi_vo_proc_id;
static pthread_t        g_face_venc_proc_id;
static pthread_t        g_eve_proc_id;

static AW_HANDLE        g_eve_event = AW_NULL;

static MPP_COM_VI_TYPE_E g_vi_type_0 = VI_4K_30FPS;
static MPP_COM_VI_TYPE_E g_vi_type_1 = VI_VGA_30FPS;

static MENU_INODE g_menu_top[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"ISP control",   NULL,   NULL, NULL},
    {(char *)"VI  control",   NULL,   NULL, NULL},
    {(char *)"VO  control",   NULL,   NULL, NULL},
    {(char *)"Quite vi+eve+vo sample",  ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/

int venc_MemOpen(void)
{
    return ion_memOpen();
}

int venc_MemClose(void)
{
    return ion_memClose();
}

unsigned char* venc_allocMem(unsigned int size)
{
    return ion_allocMem(size);
}

int venc_freeMem(void *vir_ptr)
{
    return ion_freeMem(vir_ptr);
}

unsigned int venc_getPhyAddrByVirAddr(void *vir_ptr)
{
    return ion_getMemPhyAddr(vir_ptr);
}

int venc_flushCache(void *vir_ptr, unsigned int size)
{
    return ion_flushCache(vir_ptr, size);
}

static void dmaCallBackFunc(void* pUsr)
{
    g_eve_dmaflag = 1;
}

static ERRORTYPE MPPCallbackFunc(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    return SUCCESS;
}

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


static void *get_vi_yuv_to_vo_proc(void *param)
{
    int      ret    = 0, idx = 0, delay_num = 0;
    uint64_t curPts = 0;
    VI_DEV   ViDev  = VI_DEV_1;
    VI_CHN   ViCh   = VI_CHN_0;
    //VIDEO_FRAME_INFO_S       video_frame    = {0};
    VIDEO_FRAME_INFO_S       frame_array[8] = {0};
    AW_AI_EVE_EVENT_RESULT_S eve_result     = {0};
    int i = 0, sx = 0, sy = 0, ex = 0, ey = 0;

    delay_num = VI_DELAY_FRAME_NUM;
    while (1) {
        memset(&frame_array[idx], 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = AW_MPI_VI_GetFrame(ViDev, ViCh, &frame_array[idx], 70);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_GetFrame fail! ViDev:%d  ViCh:%d ret:0x%x\n", ViDev, ViCh, ret);
            continue;
        }

        idx++;
        idx = idx % VI_DELAY_FRAME_NUM;

        if (delay_num > 0) {
            delay_num--;
            continue;
        }

#if 1
        pthread_mutex_lock(&g_eve_lock);        // mutex_lock
        if (g_eve_ready) {
            memset(&eve_result, 0, sizeof(AW_AI_EVE_EVENT_RESULT_S));
            memcpy(&eve_result, &g_eve_result, sizeof(AW_AI_EVE_EVENT_RESULT_S));
            pthread_mutex_unlock(&g_eve_lock);  // mutex_Unlock


            /*  If EVE result need proc, we cpy YUV and eve_reluse for doing face Jpeg Venc, and save sd card or nfs   */
            ret = pthread_mutex_trylock(&g_face_venc_lock); // try_mutex_lock
            if (0 == ret) {
                memset(&g_eve_venc_res, 0, sizeof(AW_AI_EVE_EVENT_RESULT_S));
                memcpy(&g_eve_venc_res, &eve_result, sizeof(AW_AI_EVE_EVENT_RESULT_S));
                g_vi_frame.VFrame.mWidth        = frame_array[idx].VFrame.mWidth;
                g_vi_frame.VFrame.mHeight       = frame_array[idx].VFrame.mHeight;
                g_vi_frame.VFrame.mOffsetLeft   = 0;
                g_vi_frame.VFrame.mOffsetTop    = 0;
                g_vi_frame.VFrame.mOffsetRight  = frame_array[idx].VFrame.mOffsetLeft + frame_array[idx].VFrame.mWidth;
                g_vi_frame.VFrame.mOffsetBottom = frame_array[idx].VFrame.mOffsetTop  + frame_array[idx].VFrame.mHeight;
                g_vi_frame.VFrame.mField        = VIDEO_FIELD_FRAME;
                g_vi_frame.VFrame.mVideoFormat  = VIDEO_FORMAT_LINEAR;
                g_vi_frame.VFrame.mCompressMode = COMPRESS_MODE_NONE;
                g_vi_frame.VFrame.mpts += (1*1000*1000) / 25;
                memcpy(g_vi_frame.VFrame.mpVirAddr[0], frame_array[idx].VFrame.mpVirAddr[0], g_vi_frame.VFrame.mWidth * g_vi_frame.VFrame.mHeight);
                memcpy(g_vi_frame.VFrame.mpVirAddr[1], frame_array[idx].VFrame.mpVirAddr[1], g_vi_frame.VFrame.mWidth * g_vi_frame.VFrame.mHeight / 2);
                venc_flushCache((void *)g_vi_frame.VFrame.mpVirAddr[0], g_vi_frame.VFrame.mWidth * g_vi_frame.VFrame.mHeight);
                venc_flushCache((void *)g_vi_frame.VFrame.mpVirAddr[1], g_vi_frame.VFrame.mWidth * g_vi_frame.VFrame.mHeight / 2);

                pthread_mutex_unlock(&g_face_venc_lock);  // mutex_Unlock
            }


            for (i = 0; i < g_eve_result.sTarget.s32TargetNum; i++) {
                //DB_PRT(" TargetNum:%d  (%d-%d) (%d-%d)\n", eve_result.sTarget.s32TargetNum, sx, sy, ex, ey);
                sx = (frame_array[idx].VFrame.mWidth  * eve_result.sTarget.astTargets[i].stRect.s16Left) / 640;
                sy = (frame_array[idx].VFrame.mHeight * eve_result.sTarget.astTargets[i].stRect.s16Top)  / 360;
                ex = (frame_array[idx].VFrame.mWidth  * eve_result.sTarget.astTargets[i].stRect.s16Right)  / 640;
                ey = (frame_array[idx].VFrame.mHeight * eve_result.sTarget.astTargets[i].stRect.s16Bottom) / 360;
                draw_rectangle_nv21((unsigned char *)frame_array[idx].VFrame.mpVirAddr[0], (unsigned char *)frame_array[idx].VFrame.mpVirAddr[1],
                                    frame_array[idx].VFrame.mWidth, frame_array[idx].VFrame.mHeight, 3,
                                    sx, sy, ex, ey);
            }
        } else {
            pthread_mutex_unlock(&g_eve_lock); // mutex_Unlock
        }

        frame_array[idx].VFrame.mOffsetLeft   = 0;
        frame_array[idx].VFrame.mOffsetTop    = 0;
        frame_array[idx].VFrame.mOffsetRight  = frame_array[idx].VFrame.mOffsetLeft + frame_array[idx].VFrame.mWidth;
        frame_array[idx].VFrame.mOffsetBottom = frame_array[idx].VFrame.mOffsetTop  + frame_array[idx].VFrame.mHeight;
        frame_array[idx].VFrame.mField        = VIDEO_FIELD_FRAME;
        frame_array[idx].VFrame.mVideoFormat  = VIDEO_FORMAT_LINEAR;
        frame_array[idx].VFrame.mCompressMode = COMPRESS_MODE_NONE;
        frame_array[idx].VFrame.mpts = curPts;
        curPts += (1*1000*1000) / 25;

        ret = AW_MPI_VO_SendFrame(0, VO_CHN_0, &frame_array[idx], 60);
        if (ret) {
            ERR_PRT("Do AW_MPI_VO_SendFrame fail! ViDev:%d  ViCh:%d ret:0x%x\n", ViDev, ViCh, ret);
        }
#endif

        usleep(20 * 1000);
        ret = AW_MPI_VI_ReleaseFrame(ViDev, ViCh, &frame_array[idx]);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_ReleaseFrame fail! ViDev:%d  ViCh:%d ret:0x%x\n", ViDev, ViCh, ret);
            continue;
        }
    }

    DB_PRT("Out this function ... ... \n");
    return NULL;
}


static void *face_yuv_venc_proc(void *param)
{
    int ret = 0, cnt = 0, file_idx = 0, snap_flag = 0;
    int i = 0, j = 0, sx = 0, sy = 0, ex = 0, ey = 0;
    VENC_CFG_S venc_cfg = {0};
    AW_AI_EVE_EVENT_RESULT_S  last_eve_res = {0};
    FACE_JPEG_INFO_S face_info_bak[FACE_ID_ARRY_NUM]  = {{0}};
    FACE_JPEG_INFO_S face_info_jpeg[FACE_ID_ARRY_NUM] = {{0}};
    unsigned int snap_privew_flag = 0;
    unsigned int snap_privew_tv   = 0;
    struct timeval tv;

    FILE *fd = NULL;
    char file_name[256] = {0};

    unsigned char  *buf = NULL;
    unsigned int    len = 0;
    uint64_t        pts = 0;
    int             frame_type;
    MPPCallbackInfo venc_cb;
    VencHeaderData  head_info;
    VENC_CROP_CFG_S crop_cfg;

    memset(&last_eve_res,  0x55, sizeof(last_eve_res));
    memset(&g_eve_venc_res, 0, sizeof(g_eve_venc_res));
    memset(&face_info_bak,  0, sizeof(face_info_bak));
    memset(&face_info_jpeg, 0, sizeof(face_info_jpeg));

    while (1) {
        pthread_mutex_lock(&g_face_venc_lock);        // mutex_lock

        if (0 == g_eve_venc_res.sTarget.s32TargetNum) {
            pthread_mutex_unlock(&g_face_venc_lock);  // mutex_Unlock
            usleep(10 * 1000);
            continue;
        }

        snap_privew_flag = 0;
#if 0
        gettimeofday(&tv, NULL);
        if (snap_privew_tv <= tv.tv_sec) {
            DB_PRT("====>last_NUM:%d  new_NUM:%d  snap_tv:%d  new_tv:%ld \n",
                   last_eve_res.sTarget.s32TargetNum, g_eve_venc_res.sTarget.s32TargetNum, snap_privew_tv, tv.tv_sec);

            snap_privew_flag = 1;
            snap_privew_tv = tv.tv_sec + FACE_SNAP_TIME;
        }
#endif

        /* Check is need snap privew picture */
        for (i = 0; i < last_eve_res.sTarget.s32TargetNum; i++) {
            if (last_eve_res.sTarget.s32TargetNum != g_eve_venc_res.sTarget.s32TargetNum ||
                last_eve_res.sTarget.astTargets[i].u32ID != g_eve_venc_res.sTarget.astTargets[i].u32ID) {

                DB_PRT("====>last_id:%d   new_id:%d  last_NUM:%d  new_NUM:%d  snap_tv:%d  new_tv:%ld \n",
                       last_eve_res.sTarget.astTargets[i].u32ID, g_eve_venc_res.sTarget.astTargets[i].u32ID,
                       last_eve_res.sTarget.s32TargetNum, g_eve_venc_res.sTarget.s32TargetNum, snap_privew_tv, tv.tv_sec);

                snap_privew_flag = 1;
                snap_privew_tv = tv.tv_sec + FACE_SNAP_TIME;
                break;
            }
        }

        if (snap_privew_flag) {
            /* Save privew picture jpeg for preview */
            ret = mpp_comm_venc_get_cfg(VENC_1080P_TO_1080P_8M_30FPS, &venc_cfg);
            if (ret) {
                ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
            }
            ret = mpp_comm_venc_create(VENC_CHN_0, PT_JPEG, 0, 0, 0, &venc_cfg);
            if (ret) {
                ERR_PRT("Do mpp_comm_venc_create fail! ret:%d \n", ret);
                //return -1;
            }

            /* Register Venc call back function */
            venc_cb.cookie   = NULL;
            venc_cb.callback = (MPPCallbackFuncType)&MPPCallbackFunc;
            AW_MPI_VENC_RegisterCallback(VENC_CHN_0, &venc_cb);

            ret = AW_MPI_VENC_StartRecvPic(VENC_CHN_0);
            if (ret) {
                ERR_PRT("Do AW_MPI_VENC_StartRecvPic fail! ret:%d\n", ret);
                //return -1;
            }

            ret = AW_MPI_VENC_SendFrame(VENC_CHN_0, &g_vi_frame, -1);
            if (ret != SUCCESS) {
                ERR_PRT("Do AW_MPI_VENC_SendFrame fail! ret:%d\n", ret);
                //return -1;
            }

            ret = mpp_comm_venc_get_stream(VENC_CHN_0, PT_JPEG, -1, &buf, &len, &pts, &frame_type, &head_info);
            if (ret) {
                ERR_PRT("Do mpp_comm_venc_get_stream fail! ret:%d\n", ret);
                //continue;
            }

            //DB_PRT("===================================================\n");
            file_idx++;
            sprintf(file_name, "%s/%d_Preview_faceNun_%d_1080p.jpg",
                    SAVE_JPEG_FILE_DIR,
                    file_idx,
                    g_eve_venc_res.sTarget.s32TargetNum);
            fd = fopen(file_name, "wb+");
            if (NULL == fd) {
                ERR_PRT("Open %s file error! errno[%d] errinfo[%s] \n",
                        file_name, errno, strerror(errno));
                continue;
            }
            fwrite(buf, len, 1, fd);

            ret = AW_MPI_VENC_StopRecvPic(VENC_CHN_0);
            if (ret) {
                ERR_PRT("Do AW_MPI_VENC_StopRecvPic fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
            }

            ret = mpp_comm_venc_destroy(VENC_CHN_0);
            if (ret) {
                ERR_PRT("Do mpp_comm_venc_destroy fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
            }

            fclose(fd);
            fd = NULL;
            DB_PRT("------> s32TargetNum:%d  Save yuv file:%s end. snap_time:%d  tv_sec:%ld \n",
                   g_eve_venc_res.sTarget.s32TargetNum, file_name, face_info_jpeg[j].snap_time - 2, tv.tv_sec);
        }

        memset(face_info_jpeg, 0, sizeof(FACE_JPEG_INFO_S) * FACE_ID_ARRY_NUM);
        for (i = 0, cnt = 0; i < FACE_ID_ARRY_NUM; i++) {
            for (j = 0; j < g_eve_venc_res.sTarget.s32TargetNum; j++) {
                if (face_info_bak[i].face_id == g_eve_venc_res.sTarget.astTargets[j].u32ID) {
                    face_info_jpeg[cnt].flag = 1;
                    face_info_jpeg[cnt].face_id   = face_info_bak[i].face_id;
                    face_info_jpeg[cnt].snap_time = face_info_bak[i].snap_time;
                    cnt++;
                    break;
                }
            }
        }

        for (i = 0; i < g_eve_venc_res.sTarget.s32TargetNum; i++) {
            for (j = 0; j < FACE_ID_ARRY_NUM; j++) {
                if (face_info_jpeg[j].face_id == g_eve_venc_res.sTarget.astTargets[i].u32ID) {
                    break;
                }

                if (0 == face_info_jpeg[j].face_id) {
                    face_info_jpeg[j].flag      = 1;
                    face_info_jpeg[j].face_id   = g_eve_venc_res.sTarget.astTargets[i].u32ID;
                    face_info_jpeg[j].snap_time = 0;
                    break;
                }
            }
        }

        for (i = 0; i < g_eve_venc_res.sTarget.s32TargetNum; i++) {
            snap_flag = 1;
            for (j = 0; j < FACE_ID_ARRY_NUM; j++) {
                if (face_info_jpeg[j].face_id == g_eve_venc_res.sTarget.astTargets[i].u32ID) {
                    gettimeofday(&tv, NULL);
                    if (face_info_jpeg[j].snap_time <= tv.tv_sec) {
                        snap_flag = 1;
                        face_info_jpeg[j].snap_time = tv.tv_sec + FACE_SNAP_TIME;
                    } else {
                        snap_flag = 0;
                    }
                    break;
                }
            }

            if (0 == snap_flag) {
                continue;
            }

            ret = mpp_comm_venc_get_cfg(VENC_1080P_TO_1080P_8M_30FPS, &venc_cfg);
            if (ret) {
                ERR_PRT("Do mpp_comm_venc_get_cfg fail! ret:%d \n", ret);
            }
            //DB_PRT(" TargetNum:%d  (%d-%d) (%d-%d)\n", eve_result.sTarget.s32TargetNum, sx, sy, ex, ey);
            sx = (g_vi_frame.VFrame.mWidth  * g_eve_venc_res.sTarget.astTargets[i].stRect.s16Left) / 640;
            sy = (g_vi_frame.VFrame.mHeight * g_eve_venc_res.sTarget.astTargets[i].stRect.s16Top)  / 360;
            ex = (g_vi_frame.VFrame.mWidth  * g_eve_venc_res.sTarget.astTargets[i].stRect.s16Right)  / 640;
            ey = (g_vi_frame.VFrame.mHeight * g_eve_venc_res.sTarget.astTargets[i].stRect.s16Bottom) / 360;

            /* Add 20% x,y w,h  */
            sx = (sx * 9) / 10;
            sy = (sy * 9) / 10;
            ex = (ex * 11) / 10;
            ey = (ey * 11) / 10;

            sx = (sx >> 4) << 4;
            sy = (sy >> 4) << 4;
            ex = (ex >> 4) << 4;
            ey = (ey >> 4) << 4;

            venc_cfg.dst_width  = ex - sx;
            venc_cfg.dst_height = ey - sy;

            if(ex > 1920) {
                ex = 1920;
                sx = 1920 - venc_cfg.dst_width;
            }
            if(ey > 1080) {
                ey = 1080;
                sy = 1080 - venc_cfg.dst_height;
            }
            ret = mpp_comm_venc_create(VENC_CHN_0, PT_JPEG, 0, 0, 0, &venc_cfg);
            if (ret) {
                ERR_PRT("Do mpp_comm_venc_create fail! ret:%d \n", ret);
                //return -1;
            }

            /* Register Venc call back function */
            venc_cb.cookie   = NULL;
            venc_cb.callback = (MPPCallbackFuncType)&MPPCallbackFunc;
            AW_MPI_VENC_RegisterCallback(VENC_CHN_0, &venc_cb);

            ret = AW_MPI_VENC_StartRecvPic(VENC_CHN_0);
            if (ret) {
                ERR_PRT("Do AW_MPI_VENC_StartRecvPic fail! ret:%d\n", ret);
                //return -1;
            }

            crop_cfg.bEnable     = TRUE;
            crop_cfg.Rect.X      = sx;
            crop_cfg.Rect.Y      = sy;
            crop_cfg.Rect.Width  = venc_cfg.dst_width;
            crop_cfg.Rect.Height = venc_cfg.dst_height;
            ret = AW_MPI_VENC_SetCrop(VENC_CHN_0, &crop_cfg);
            if (ret) {
                ERR_PRT(" Do AW_MPI_VENC_SetCrop venc_chn:%d enable_flag:%d x-y(%d-%d) w-h(%d-%d) fail! ret:0x%x\n",
                        VENC_CHN_0, crop_cfg.bEnable, crop_cfg.Rect.X, crop_cfg.Rect.Y, crop_cfg.Rect.Width, crop_cfg.Rect.Height, ret);
            }

            ret = AW_MPI_VENC_SendFrame(VENC_CHN_0, &g_vi_frame, -1);
            if (ret != SUCCESS) {
                ERR_PRT("Do AW_MPI_VENC_SendFrame fail! ret:%d\n", ret);
                //return -1;
            }

            ret = mpp_comm_venc_get_stream(VENC_CHN_0, PT_JPEG, -1, &buf, &len, &pts, &frame_type, &head_info);
            if (ret) {
                ERR_PRT("Do mpp_comm_venc_get_stream fail! ret:%d\n", ret);
                //continue;
            }

            //DB_PRT("===================================================\n");
            file_idx++;
            sprintf(file_name, "%s/%d_ID%d_%dx%d.jpg",
                    SAVE_JPEG_FILE_DIR,
                    file_idx,
                    g_eve_venc_res.sTarget.astTargets[i].u32ID,
                    venc_cfg.dst_width,
                    venc_cfg.dst_height);
            fd = fopen(file_name, "wb+");
            if (NULL == fd) {
                ERR_PRT("Open %s file error! errno[%d] errinfo[%s] \n",
                        file_name, errno, strerror(errno));
                continue;
            }
            fwrite(buf, len, 1, fd);

            ret = AW_MPI_VENC_StopRecvPic(VENC_CHN_0);
            if (ret) {
                ERR_PRT("Do AW_MPI_VENC_StopRecvPic fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
            }

            ret = mpp_comm_venc_destroy(VENC_CHN_0);
            if (ret) {
                ERR_PRT("Do mpp_comm_venc_destroy fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
            }

            fclose(fd);
            fd = NULL;
            DB_PRT("------> s32TargetNum:%d  Save yuv file:%s end. snap_time:%d  tv_sec:%ld \n",
                   g_eve_venc_res.sTarget.s32TargetNum, file_name, face_info_jpeg[j].snap_time - 2, tv.tv_sec);
        }

        memcpy(&last_eve_res, &g_eve_venc_res, sizeof(g_eve_venc_res));
        memset(&g_eve_venc_res, 0, sizeof(g_eve_venc_res));
        memcpy(face_info_bak, face_info_jpeg, sizeof(FACE_JPEG_INFO_S) * FACE_ID_ARRY_NUM);
        pthread_mutex_unlock(&g_face_venc_lock);  // mutex_Unlock
    }

    DB_PRT("Out this function ... ... \n");
    return NULL;
}


#if 1
static void *get_yuv_eve_proc(void *param)
{
    int      ret    = 0;
    uint64_t curPts = 0;
    VI_DEV ViDev = VI_DEV_3;
    VI_CHN ViCh  = VI_CHN_0;
    VIDEO_FRAME_INFO_S  video_frame = {0};
    AW_IMAGE_S          eve_image   = {0};
    AW_U32              time_stamp  = 0;
    AW_STATUS_E         status;
    AW_AI_EVE_EVENT_RESULT_S eve_result = {0};

    int use_time = 0, total_proc_time = 0;
    struct timeval start_tv, end_tv;

    DB_PRT("Do this function ... ... \n");

    while (1) {
        memset(&video_frame, 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = AW_MPI_VI_GetFrame(ViDev, ViCh, &video_frame, 70);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_GetFrame fail! ViDev:%d  ViCh:%d ret:0x%x\n", ViDev, ViCh, ret);
            continue;
        }

        if (total_proc_time > 40) {
            //DB_PRT("Do vi release  total_proc_time:%d  ... ... \n", total_proc_time);
            total_proc_time -= 40;
            goto _vi_release;
        }

        /* set image attribution */
        eve_image.mPixelFormat = video_frame.VFrame.mPixelFormat;
        eve_image.mWidth       = video_frame.VFrame.mWidth;
        eve_image.mHeight      = video_frame.VFrame.mHeight;
        eve_image.mStride[0]   = video_frame.VFrame.mWidth;
        eve_image.mpVirAddr[0] = video_frame.VFrame.mpVirAddr[0];
        eve_image.mpVirAddr[1] = video_frame.VFrame.mpVirAddr[1];
        eve_image.mpVirAddr[2] = video_frame.VFrame.mpVirAddr[2];
        eve_image.mPhyAddr[0]  = video_frame.VFrame.mPhyAddr[0];
        eve_image.mPhyAddr[1]  = video_frame.VFrame.mPhyAddr[1];
        eve_image.mPhyAddr[2]  = video_frame.VFrame.mPhyAddr[2];
        time_stamp = video_frame.VFrame.mpts;

        gettimeofday(&start_tv, NULL);

        status = AW_AI_EVE_Event_SetEveSourceAddress(g_eve_event, (void *)eve_image.mPhyAddr[0]);
        if (AW_STATUS_OK != status) {
            ERR_PRT("Do AW_AI_EVE_Event_SetEveSourceAddress fail!  ret:0x%x\n", ret);
            goto _vi_release;
        }

        status = AW_AI_EVE_Event_Process(g_eve_event, &eve_image, curPts, &eve_result);
        if (AW_STATUS_OK != status) {
            ERR_PRT("Do AW_AI_EVE_Event_Process fail!  ret:0x%x\n", ret);
            goto _vi_release;
        }

        pthread_mutex_lock(&g_eve_lock);
        if (AW_STATUS_ERROR != status && eve_result.sTarget.s32TargetNum > 0) {
            g_eve_ready = 1;
            memcpy(&g_eve_result, &eve_result, sizeof(eve_result));
        } else {
            g_eve_ready = 0;
        }
        pthread_mutex_unlock(&g_eve_lock);

        gettimeofday(&end_tv, NULL);
        use_time = (end_tv.tv_sec * 1000 + end_tv.tv_usec/1000) - (start_tv.tv_sec * 1000 + start_tv.tv_usec/1000);
        if (use_time > MAX_EVE_PROC_TIME) {
            total_proc_time = (use_time - MAX_EVE_PROC_TIME);
        } else {
            total_proc_time += 5;
        }
        //DB_PRT("===> use_time:%d  total_proc_time:%d \n", use_time, total_proc_time);

_vi_release:
        ret = AW_MPI_VI_ReleaseFrame(ViDev, ViCh, &video_frame);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_ReleaseFrame fail! ViDev:%d  ViCh:%d ret:0x%x\n", ViDev, ViCh, ret);
            continue;
        }
    }

    return NULL;
}

#else

static void *get_yuv_eve_proc(void *param)
{
    int      ret    = 0, idx = 0, delay_num = 0;;
    uint64_t curPts = 0;
    VI_DEV ViDev = VI_DEV_3;
    VI_CHN ViCh  = VI_CHN_0;
    VIDEO_FRAME_INFO_S  frame_array[8] = {0};
    AW_IMAGE_S          eve_image      = {0};
    AW_U32              time_stamp     = 0;
    AW_STATUS_E         status;
    AW_AI_EVE_EVENT_RESULT_S eve_result = {0};

    int use_time = 0, total_proc_time = 0;
    struct timeval start_tv, end_tv;

    DB_PRT("Do this function ... ... \n");

    delay_num = VI_DELAY_FRAME_NUM - 1;
    while (1) {
        memset(&frame_array[idx], 0, sizeof(VIDEO_FRAME_INFO_S));
        ret = AW_MPI_VI_GetFrame(ViDev, ViCh, &frame_array[idx], 70);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_GetFrame fail! ViDev:%d  ViCh:%d ret:0x%x\n", ViDev, ViCh, ret);
            continue;
        }

        /* set image attribution */
        eve_image.mPixelFormat = frame_array[idx].VFrame.mPixelFormat;
        eve_image.mWidth       = frame_array[idx].VFrame.mWidth;
        eve_image.mHeight      = frame_array[idx].VFrame.mHeight;
        eve_image.mStride[0]   = frame_array[idx].VFrame.mWidth;
        eve_image.mpVirAddr[0] = frame_array[idx].VFrame.mpVirAddr[0];
        eve_image.mpVirAddr[1] = frame_array[idx].VFrame.mpVirAddr[1];
        eve_image.mpVirAddr[2] = frame_array[idx].VFrame.mpVirAddr[2];
        eve_image.mPhyAddr[0]  = frame_array[idx].VFrame.mPhyAddr[0];
        eve_image.mPhyAddr[1]  = frame_array[idx].VFrame.mPhyAddr[1];
        eve_image.mPhyAddr[2]  = frame_array[idx].VFrame.mPhyAddr[2];
        time_stamp = frame_array[idx].VFrame.mpts;

        gettimeofday(&start_tv, NULL);

        status = AW_AI_EVE_Event_SetEveSourceAddress(g_eve_event, (void *)eve_image.mPhyAddr[0]);
        if (AW_STATUS_OK != status) {
            ERR_PRT("Do AW_AI_EVE_Event_SetEveSourceAddress fail!  ret:0x%x\n", ret);
            goto _vi_release;
        }

        status = AW_AI_EVE_Event_Process(g_eve_event, &eve_image, curPts, &eve_result);
        if (AW_STATUS_OK != status) {
            ERR_PRT("Do AW_AI_EVE_Event_Process fail!  ret:0x%x\n", ret);
            goto _vi_release;
        }

        idx++;
        idx = idx % (VI_DELAY_FRAME_NUM - 1);

        if (delay_num > 0) {
            delay_num--;
            continue;
        }

        pthread_mutex_lock(&g_eve_lock);
        if (AW_STATUS_ERROR != status && eve_result.sTarget.s32TargetNum > 0) {
            g_eve_ready = 1;
            memcpy(&g_eve_result, &eve_result, sizeof(eve_result));
        } else {
            g_eve_ready = 0;
        }
        pthread_mutex_unlock(&g_eve_lock);

        gettimeofday(&end_tv, NULL);
        use_time = (end_tv.tv_sec * 1000 + end_tv.tv_usec/1000) - (start_tv.tv_sec * 1000 + start_tv.tv_usec/1000);
        if (use_time > MAX_EVE_PROC_TIME) {
            total_proc_time = (use_time - MAX_EVE_PROC_TIME);
        } else {
            total_proc_time += 5;
        }
        //DB_PRT("===> use_time:%d  total_proc_time:%d \n", use_time, total_proc_time);


        int i = 0, sx = 0, sy = 0, ex = 0, ey = 0;
        if (g_eve_ready && g_eve_result.sTarget.s32TargetNum > 0) {
            for (int i = 0; i < g_eve_result.sTarget.s32TargetNum; i++) {
                //DB_PRT(" TargetNum:%d  (%d-%d) (%d-%d)\n", eve_result.sTarget.s32TargetNum, sx, sy, ex, ey);
                sx = g_eve_result.sTarget.astTargets[i].stRect.s16Left;
                sy = g_eve_result.sTarget.astTargets[i].stRect.s16Top;
                ex = g_eve_result.sTarget.astTargets[i].stRect.s16Right;
                ey = g_eve_result.sTarget.astTargets[i].stRect.s16Bottom;
                draw_rectangle_nv21((unsigned char *)frame_array[idx].VFrame.mpVirAddr[0], (unsigned char *)frame_array[idx].VFrame.mpVirAddr[1],
                                    frame_array[idx].VFrame.mWidth, frame_array[idx].VFrame.mHeight, 2,
                                    sx, sy, ex, ey);
            }
        }

        frame_array[idx].VFrame.mOffsetLeft   = 0;
        frame_array[idx].VFrame.mOffsetTop    = 0;
        frame_array[idx].VFrame.mOffsetRight  = frame_array[idx].VFrame.mOffsetLeft + frame_array[idx].VFrame.mWidth;
        frame_array[idx].VFrame.mOffsetBottom = frame_array[idx].VFrame.mOffsetTop  + frame_array[idx].VFrame.mHeight;
        frame_array[idx].VFrame.mField        = VIDEO_FIELD_FRAME;
        frame_array[idx].VFrame.mVideoFormat  = VIDEO_FORMAT_LINEAR;
        frame_array[idx].VFrame.mCompressMode = COMPRESS_MODE_NONE;
        frame_array[idx].VFrame.mpts = curPts;
        curPts += (1*1000*1000) / 25;

        ret = AW_MPI_VO_SendFrame(0, VO_CHN_0, &frame_array[idx], 60);
        if (ret) {
            ERR_PRT("Do AW_MPI_VO_SendFrame fail! ViDev:%d  ViCh:%d ret:0x%x\n", ViDev, ViCh, ret);
        }

_vi_release:
        ret = AW_MPI_VI_ReleaseFrame(ViDev, ViCh, &frame_array[idx]);
        if (ret) {
            ERR_PRT("Do AW_MPI_VI_ReleaseFrame fail! ViDev:%d  ViCh:%d ret:0x%x\n", ViDev, ViCh, ret);
            continue;
        }
    }

    return NULL;
}


#endif

static int vi_create(void)
{
    int ret = 0;
    VI_ATTR_S vi_attr;

    /**  create vi dev 1  src 1080P@25fps **/
    ret = mpp_comm_vi_get_attr(g_vi_type_0, &vi_attr);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_get_attr fail! ret:%d \n", ret);
        return -1;
    }
    vi_attr.fps    = 25;
    vi_attr.nbufs  = 10;
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

    unsigned int size = 0;
    size = vi_attr.format.width * vi_attr.format.height;
    g_vi_frame.VFrame.mpVirAddr[0] = (void *)venc_allocMem(size);
    if (g_vi_frame.VFrame.mpVirAddr[0] == NULL) {
        ERR_PRT("alloc y_vir_addr size %d error!", size);
        return -1;
    }
    g_vi_frame.VFrame.mpVirAddr[1] = (void *)venc_allocMem(size/2);
    if (g_vi_frame.VFrame.mpVirAddr[1] == NULL) {
        ERR_PRT("alloc uv_vir_addr size %d error!", size/2);
        venc_freeMem(g_vi_frame.VFrame.mpVirAddr[0]);
        g_vi_frame.VFrame.mpVirAddr[0] = NULL;
        return -1;
    }

    g_vi_frame.VFrame.mPhyAddr[0] = (unsigned int)venc_getPhyAddrByVirAddr(g_vi_frame.VFrame.mpVirAddr[0]);
    g_vi_frame.VFrame.mPhyAddr[1] = (unsigned int)venc_getPhyAddrByVirAddr(g_vi_frame.VFrame.mpVirAddr[1]);

    /**  create vi dev 3  src VGA@25fps **/
    ret = mpp_comm_vi_get_attr(g_vi_type_1, &vi_attr);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_get_attr fail! ret:%d \n", ret);
        return -1;
    }
    vi_attr.format.width  = 640;
    vi_attr.format.height = 360;
    vi_attr.fps           = 25;
    vi_attr.nbufs         = 10;
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

    /**  destroy vi dev 1  **/
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
    ret = mpp_comm_vi_dev_destroy(VI_DEV_3);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_dev_destroy fail! ret:%d  vi_dev:%d \n", ret, VI_DEV_3);
    }

    return 0;
}


static int eve_create(void)
{
    int ret = 0;
    int i = 0, mT = 0, iTemp = 0;
    FILE *fileevecfg = NULL;
    VI_ATTR_S        vi_attr;
    AW_STATUS_E      status   = AW_STATUS_ERROR;
    AW_AI_EVE_CTRL_S sEVECtrl = {0};
    static AW_S8    *awKeyNew = (AW_S8*)"1111111111111111"; //key

    /**  create vi dev 3  src VGA **/
    ret = mpp_comm_vi_get_attr(g_vi_type_1, &vi_attr);
    if (ret) {
        ERR_PRT("Do mpp_comm_vi_get_attr fail! ret:%d \n", ret);
        return -1;
    }
    vi_attr.format.width  = 640;
    vi_attr.format.height = 360;
    vi_attr.fps           = 25;

    sEVECtrl.addrInputType  = AW_AI_EVE_ADDR_INPUT_PHY; //ÊäÈëÎïÀíµØÖ·
    sEVECtrl.scale_factor   = 1;
    sEVECtrl.mScanStageNo   = 10;
    sEVECtrl.yStep          = 3;
    sEVECtrl.xStep0         = 1;
    sEVECtrl.xStep1         = 4;
    sEVECtrl.mMidRltStageNo = 10;
    sEVECtrl.mMidRltNum     = 0;
    sEVECtrl.mRltNum        = AW_AI_EVE_MAX_RESULT_NUM;
    sEVECtrl.rltType        = AW_AI_EVE_RLT_OUTPUT_DETAIL;

    sEVECtrl.mDmaOut.s16Width              = vi_attr.format.width;
    sEVECtrl.mDmaOut.s16Height             = vi_attr.format.height;
    sEVECtrl.mPyramidLowestLayel.s16Width  = vi_attr.format.width;
    sEVECtrl.mPyramidLowestLayel.s16Height = vi_attr.format.height;
    sEVECtrl.dmaSrcSize.s16Width           = vi_attr.format.width;
    sEVECtrl.dmaSrcSize.s16Height          = vi_attr.format.height;
    sEVECtrl.dmaDesSize.s16Width           = vi_attr.format.width;
    sEVECtrl.dmaDesSize.s16Height          = vi_attr.format.height;
    sEVECtrl.dmaRoi.s16X                   = 0;
    sEVECtrl.dmaRoi.s16Y                   = 0;
    sEVECtrl.dmaRoi.s16Width               = sEVECtrl.dmaDesSize.s16Width;
    sEVECtrl.dmaRoi.s16Height              = sEVECtrl.dmaDesSize.s16Height;

    sEVECtrl.classifierNum = 5;
    sEVECtrl.classifierPath[0].path = (AW_S8*)"/usr/lib/frontface.ld";
    sEVECtrl.classifierPath[0].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[1].path = (AW_S8*)"/usr/lib/downface.ld";
    sEVECtrl.classifierPath[1].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[2].path = (AW_S8*)"/usr/lib/smallface.ld";
    sEVECtrl.classifierPath[2].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[3].path = (AW_S8*)"/usr/lib/halfdownface.ld";
    sEVECtrl.classifierPath[3].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[4].path = (AW_S8*)"/usr/lib/cutmouthface.ld";
    sEVECtrl.classifierPath[4].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[5].path = (AW_S8*)"/usr/lib/rotleftface.ld";
    sEVECtrl.classifierPath[5].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[6].path = (AW_S8*)"/usr/lib/rotrightface.ld";
    sEVECtrl.classifierPath[6].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[7].path = (AW_S8*)"/usr/lib/smallface.ld";
    sEVECtrl.classifierPath[7].key  = (AW_U8*)awKeyNew;

    g_facedet_param.s32ClassifyFlag   = 0; //close

    sEVECtrl.dmaCallBackFunc   = &dmaCallBackFunc;
    //sEVECtrl.dma_pUsr = (void *)&privCap[vipp_dev][virvi_chn];

    g_eve_event = AW_AI_EVE_Event_Init(&sEVECtrl);
    if (AW_NULL == g_eve_event) {
        ERR_PRT("Do AW_AI_EVE_Event_Init fail!\n");
        return -1;
    }

    g_facedet_param.sRoiSet.s32RoiNum         = 1;
    g_facedet_param.sRoiSet.sID[0]            = 1;
    g_facedet_param.sRoiSet.sRoi[0].s16Top    = 0;
    g_facedet_param.sRoiSet.sRoi[0].s16Bottom = vi_attr.format.height - 5;
    g_facedet_param.sRoiSet.sRoi[0].s16Left   = 0;
    g_facedet_param.sRoiSet.sRoi[0].s16Right  = vi_attr.format.width  - 5;
    g_facedet_param.s32ClassifyFlag           = 0; //close
    g_facedet_param.s32MinFaceSize            = 20;
    g_facedet_param.s32MaxFaceNum             = 128;
    g_facedet_param.s32OverLapCoeff           = 20;
    g_facedet_param.s32MergeThreshold         = 3;
    g_facedet_param.s8Cfgfile                 = "/usr/lib/face_classifier_24X24.cfg";
    g_facedet_param.s8Weightfile              = "/usr/lib/face_classifier_24X24.cweights";
    /*
        ret = AW_AI_EVE_Event_SetEveDMAExecuteMode(g_eve_event, AW_AI_EVE_DMA_EXECUTE_SYNC);
        if (AW_STATUS_OK != status)
        {
            ERR_PRT("Do AW_AI_EVE_Event_SetEveDMAExecuteMode fail! status:%d \n", status);
            return -1;
        }
    */
    status = AW_AI_EVE_Event_SetEventParam(g_eve_event, AW_AI_EVE_EVENT_FACEDETECT, (void *)&g_facedet_param);
    if (AW_STATUS_OK != status) {
        ERR_PRT("Do AW_AI_EVE_Event_SetEventParam fail! status:%d \n", status);
        return -1;
    }

    return 0;
}


static int eve_destroy(void)
{
    AW_AI_EVE_Event_UnInit(g_eve_event);

    return 0;
}


static int vo_create(void)
{
    int ret = 0;
    VO_DEV_TYPE_E vo_type;
    VO_DEV_CFG_S  vo_cfg;
    vo_type           = VO_DEV_HDMI;
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

    return 0;
}


static int vo_destroy(void)
{
    int ret = 0;

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

    ret = AW_MPI_VI_EnableVirChn(VI_DEV_3, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_EnableVirChn fail! ret:%d\n", ret);
        return -1;
    }

    ret = mpp_comm_vo_chn_start(VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_chn_start vo_chn:%d fail! ret:%d\n", VO_CHN_0, ret);
        //return -1;
    }

    usleep(30 * 1000);

    ret = pthread_create(&g_vi_vo_proc_id, NULL, get_vi_yuv_to_vo_proc, NULL);
    if (ret) {
        ERR_PRT("Do pthread_create get_vi_yuv_to_vo_proc fail! ret:%d\n", ret);
    }

    usleep(30 * 1000);

    ret = pthread_create(&g_eve_proc_id, NULL, get_yuv_eve_proc, NULL);
    if (ret) {
        ERR_PRT("Do pthread_create get_yuv_eve_proc fail! ret:%d\n", ret);
    }

    usleep(30 * 1000);

    ret = pthread_create(&g_face_venc_proc_id, NULL, face_yuv_venc_proc, NULL);
    if (ret) {
        ERR_PRT("Do pthread_create face_yuv_venc_proc fail! ret:%d\n", ret);
    }

    /* Just for imx290 sensor */
    ret = AW_MPI_VI_SetVippFlip(VI_DEV_1, 1);
    ret = AW_MPI_VI_SetVippFlip(VI_DEV_3, 1);

    return 0;
}


static int components_stop(void)
{
    int ret = 0;

#if 0
    g_eve_run     = 0;
    g_facedet_run = 0;
    pthread_join(g_eve_proc_id, 0);
    pthread_join(g_facedet_proc_id, 0);
#endif

    ret = mpp_comm_vo_chn_stop(VO_CHN_0);
    if (ret) {
        ERR_PRT("Do mpp_comm_vo_chn_stop vo_chn:%d fail! ret:%d\n", VO_CHN_0, ret);
    }

    ret = AW_MPI_VI_DisableVirChn(VI_DEV_1, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
    }

    ret = AW_MPI_VI_DisableVirChn(VI_DEV_3, VI_CHN_0);
    if (ret) {
        ERR_PRT("Do AW_MPI_VI_DisableVirChn fail! ret:%d  venc_chn:%d \n", ret, VENC_CHN_0);
    }

    return 0;
}


#ifdef SAMPLE_MODE
int main(int argc, char **argv)
#else
int SampleViYuvVenc(void *pData, char *pTitle)
#endif
{
    int ret = 0;

    printf("\033[2J");
    printf("\n\n\nDo sample vi->yuv+EVE+VO.\n");

    //g_vi_type_0   = VI_4K_25FPS;
    g_vi_type_0   = VI_1080P_30FPS;
    g_vi_type_1   = VI_VGA_30FPS;

    /* Step 1. Init mpp system */
    ret = mpp_comm_sys_init();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_init fail! ret:%d \n", ret);
        return -1;
    }

    /* Step 2. Get vi config to create 4K/VGA vi channel */
    ret = vi_create();
    if (ret) {
        ERR_PRT("Do vi_create fail! ret:%d\n", ret);
        goto _exit_1;
    }

    /* Step 3. Get venc config to create VO channel */
    ret = vo_create();
    if (ret) {
        ERR_PRT("Do vo_create fail! ret:%d\n", ret);
        goto _exit_2;
    }

    /* Step 4. Get venc config to create 1080P/720P venc channel */
    ret = eve_create();
    if (ret) {
        ERR_PRT("Do eve_create fail! ret:%d\n", ret);
        goto _exit_3;
    }

    /* Setup 5. Start vi and venc */
    ret = components_start();
    if (ret) {
        ERR_PRT("Do components_start fail! ret:%d\n", ret);
        goto _exit_4;
    }

    /* Setup 6. Do this block menu control function */
    MenuCtrl();
    //while(1) sleep(1);


_exit_4:
    components_stop();


_exit_3:
    eve_destroy();

_exit_2:
    vo_destroy();

_exit_1:
    vi_destroy();

    ret = mpp_comm_sys_exit();
    if (ret) {
        ERR_PRT("Do mpp_comm_sys_exit fail! ret:%d \n", ret);
    }

    return ret;
}

