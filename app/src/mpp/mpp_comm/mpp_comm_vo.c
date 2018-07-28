/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_comm_vo.c
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
#define DISP_DEV        "/dev/disp"
#define FB_ANDROID_DEV  "/dev/graphics/fb%d"

#define HLAY(chn, lyl)  (chn*4+lyl)

/*
#define LYL_NUM 4
#define LAYER_DE 16
#define HLAY(chn, lyl) (chn*4+lyl)
#define HD2CHN(hlay) (hlay/4)
#define HD2LYL(hlay) (hlay%4)
*/


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
MPPCallbackInfo g_vo_cbInfo;
MPPCallbackInfo g_clock_cbInfo;


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/

static ERRORTYPE mpp_comm_vo_callback(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    ERRORTYPE ret = SUCCESS;

    if (NULL != pChn) {
        //DB_PRT(" pChn->mModId:%d \n", pChn->mModId);
    }

    return ret;
}

static ERRORTYPE mpp_comm_clock_callback(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    if (NULL != pChn)
        DB_PRT("clock channel[%d] has some event[0x%x]", pChn->mChnId, event);
    return SUCCESS;
}


/**
 * @brief Get vo config attr form vo_type.  guixing
 * @param
 * - xxxx         input
 * - xxxx         output
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_vo_get_attr()
{
    return 0;
}


/**
 * @brief Create vo device.   guixing
 * @param
 * - dev_type     input
 * - dev_attr      input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_vo_dev_create(VO_DEV_TYPE_E dev_type, VO_DEV_CFG_S *dev_cfg)
{
    int    ret    = 0;
    VO_DEV vo_dev = 0;

    /* Check input param */
    if (dev_type < 0 || dev_type > VO_DEV_TYPE_BOTTON) {
        ERR_PRT("Input dev_type:%d is error!\n", dev_type);
        return -1;
    }

    if (NULL == dev_cfg) {
        ERR_PRT("Input dev_attr is NULL!\n");
        return -1;
    }

    /* Enable vo device */
    ret = AW_MPI_VO_Enable(vo_dev);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_VO_Enable fail! ret:%d\n", ret);
        return ret;
    }

    /* Because MINI GUI must used pipe_1 ,
          * so video used pipe_0, pipe_2, pipe_3. in V5 chip.
          */
    VO_LAYER  vo_layer = HLAY(2, 0);
    AW_MPI_VO_AddOutsideVideoLayer(vo_layer);
    AW_MPI_VO_CloseVideoLayer(vo_layer);    //close ui layer.

    /* Switch hw display to CVBS/HDMI/LCD */
    VO_PUB_ATTR_S stPubAttr;
    ret = AW_MPI_VO_GetPubAttr(0, &stPubAttr);
    if (0 == ret) {
        DB_PRT("Do AW_MPI_VO_GetPubAttr succeed VO_DEV:0  ret:%d \n",ret);

        /*second,set pub attr*/
        stPubAttr.enIntfType = VO_INTF_HDMI;
        stPubAttr.enIntfSync = VO_OUTPUT_1080P60;
        ret = AW_MPI_VO_SetPubAttr(0, &stPubAttr);
        if (0 == ret) {
            DB_PRT("Do AW_MPI_VO_SetPubAttr succeed VO_DEV:0  ret:%d \n",ret);
            return 0;
        } else {
            ERR_PRT("Do AW_MPI_VO_GetPubAttr fail VO_DEV:0  ret:%d \n", ret);
            return -1;
        }
    } else {
        ERR_PRT("Do AW_MPI_VO_GetPubAttr fail VO_DEV:0  ret:%d \n", ret);
        return -1;
    }

    return ret;
}


/**
 * @brief Create vo device.   guixing
 * @param
 * - dev_type        input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_vo_dev_destroy(VO_DEV_TYPE_E dev_type)
{
    int    ret    = 0;
    VO_DEV vo_dev = 0;

    /* Check input param */
    if (dev_type < 0 || dev_type > VO_DEV_TYPE_BOTTON) {
        ERR_PRT("Input dev_type:%d is error!\n", dev_type);
        return -1;
    }

    /* Enable vo device */
    ret = AW_MPI_VO_Disable(vo_dev);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_VO_Disable fail! ret:%d\n", ret);
        return ret;
    }

    /* Because MINI GUI must used pipe_1 ,
          * so video used pipe_0, pipe_2, pipe_3. in V5 chip.
          */
    VO_LAYER  vo_layer = HLAY(2, 0);
    AW_MPI_VO_AddOutsideVideoLayer(vo_layer);
    AW_MPI_VO_CloseVideoLayer(vo_layer);    //close ui layer.

    return ret;
}


/**
 * @brief Create vo channel   .guixing
 * @param
 * - vo_chn               input
 * - vo_chn_attr      input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_vo_chn_create(int vo_chn, VO_CHN_CFG_S *vo_chn_cfg)
{
    int       ret      = SUCCESS;
    VO_CHN    chn      = 0;
    VO_LAYER  vo_layer = 0;
    VO_VIDEO_LAYER_ATTR_S vo_layer_attr;

    /* Check input param */
    if (vo_chn < 0 || vo_chn >= MAX_VO_CHN_NUM) {
        ERR_PRT("Input vo_chn:%d error!\n", vo_chn);
        return -1;
    }

    if (NULL == vo_chn_cfg) {
        ERR_PRT("Input vo_chn_attr is NULL!\n");
        return -1;
    }

    /* enable vo layer */
    vo_layer = HLAY(vo_chn, 0);
    ret = AW_MPI_VO_EnableVideoLayer(vo_layer);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_VO_EnableVideoLayer fail! vo_layer:%d ret:%d\n", vo_layer, ret);
        return -1;
    }

    ret = AW_MPI_VO_GetVideoLayerAttr(vo_layer, &vo_layer_attr);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_VO_GetVideoLayerAttr fail! vo_layer:%d ret:%d\n", vo_layer, ret);
        return ret;
    }
    vo_layer_attr.stDispRect.X      = vo_chn_cfg->top;
    vo_layer_attr.stDispRect.Y      = vo_chn_cfg->left;
    vo_layer_attr.stDispRect.Width  = vo_chn_cfg->width;
    vo_layer_attr.stDispRect.Height = vo_chn_cfg->height;
    //vo_layer_attr.enPixFormat       = MM_PIXEL_FORMAT_YVU_PLANAR_420;

    ret = AW_MPI_VO_SetVideoLayerAttr(vo_layer, &vo_layer_attr);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_VO_SetVideoLayerAttr fail! vo_layer:%d ret:%d\n", vo_layer, ret);
        return ret;
    }

    ret = AW_MPI_VO_EnableChn(vo_layer, chn);
    if (ERR_VO_CHN_NOT_DISABLE == ret) {
        ERR_PRT("vo vo_layer:%d vo_chn:%d is exist! ret:%d\n", vo_layer, chn, ret);
        return -1;
    } else if (SUCCESS != ret) {
        ERR_PRT("create vo vo_layer:%d vo_chn:%d! ret:%d\n", vo_layer, chn, ret);
        return -1;
    }
    DB_PRT("Create vo vo_layer[%d] vo_chn[%d] success!\n", vo_layer, vo_chn);

    g_vo_cbInfo.cookie   = NULL;
    g_vo_cbInfo.callback = (MPPCallbackFuncType)&mpp_comm_vo_callback;
    AW_MPI_VO_RegisterCallback(vo_layer, vo_chn, &g_vo_cbInfo);

    ret = AW_MPI_VO_SetChnDispBufNum(vo_layer, vo_chn, vo_chn_cfg->vo_buf_num);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_VO_SetChnDispBufNum fail! vo_layer:%d ret:%d\n", vo_layer, ret);
        return ret;
    }

    return ret;
}


/**
 * @brief Destroy vo channel.  guixing
 * @param
 * - vo_chn        input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_vo_chn_destroy(int vo_chn)
{
    int       ret      = 0;
    VO_CHN    chn      = 0;
    VO_LAYER  vo_layer = 0;

    /* Check input param */
    if (vo_chn < 0 || vo_chn >= MAX_VO_CHN_NUM) {
        ERR_PRT("Input vo_chn:%d error!\n", vo_chn);
        return -1;
    }

    chn      = 0;
    vo_layer = HLAY(vo_chn, 0);
    ret = AW_MPI_VO_DisableChn(vo_layer, chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_VO_DisableChn vo_layer:%d chn:%d failed! ret:%d \n", vo_layer, chn, ret);
        return -1;
    }

    ret = AW_MPI_VO_DisableVideoLayer(vo_layer);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_VO_DisableVideoLayer fail! vo_layer:%d ret:%d\n", vo_layer, ret);
        return -1;
    }

    return ret;
}


/**
 * @brief Start vo channel,  actually is start vo_layer  .guixing
 * @param
 * - vo_chn               input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_vo_chn_start(int vo_chn)
{
    int       ret      = SUCCESS;
    VO_CHN    chn      = 0;
    VO_LAYER  vo_layer = 0;

    /* Check input param */
    if (vo_chn < 0 || vo_chn >= MAX_VO_CHN_NUM) {
        ERR_PRT("Input vo_chn:%d error!\n", vo_chn);
        return -1;
    }

    /* enable vo layer */
    vo_layer = HLAY(vo_chn, 0);
    chn      = 0;
    ret = AW_MPI_VO_StartChn(vo_layer, chn);
    if (ret) {
        ERR_PRT("Do AW_MPI_VO_StartChn vo_layer:%d  chn:%d fail! ret:%d\n", vo_layer, chn, ret);
        return -1;
    }

    return ret;
}


/**
 * @brief Stop vo channel,  actually is stop vo_layer  .guixing
 * @param
 * - vo_chn               input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_vo_chn_stop(int vo_chn)
{
    int       ret      = SUCCESS;
    VO_CHN    chn      = 0;
    VO_LAYER  vo_layer = 0;

    /* Check input param */
    if (vo_chn < 0 || vo_chn >= MAX_VO_CHN_NUM) {
        ERR_PRT("Input vo_chn:%d error!\n", vo_chn);
        return -1;
    }

    /* enable vo layer */
    vo_layer = HLAY(vo_chn, 0);
    chn      = 0;
    ret = AW_MPI_VO_StopChn(vo_layer, chn);
    if (ret) {
        ERR_PRT("Do AW_MPI_VO_StopChn vo_layer:%d  chn:%d fail! ret:%d\n", vo_layer, chn, ret);
        return -1;
    }

    return ret;
}


int mpp_comm_vo_clock_create(CLOCK_CHN clock_chn)
{
    int ret = 0;
    CLOCK_CHN_ATTR_S clock_attr;

    clock_attr.nWaitMask = 0;
    clock_attr.nWaitMask |= 1<<CLOCK_PORT_INDEX_VIDEO; /* to video render */
    ret = AW_MPI_CLOCK_CreateChn(clock_chn, &clock_attr);
    if (ERR_CLOCK_EXIST == ret) {
        ERR_PRT("clock channel[%d] is exist, find next!\n", clock_chn);
        return -1;
    } else if (SUCCESS != ret) {
        ERR_PRT("Create clock channel[%d] ret[0x%x] fail!\n", clock_chn, ret);
        return -1;
    }
    DB_PRT("Create clock channel[%d] success!\n", clock_chn);

    g_clock_cbInfo.cookie = (void*)NULL;
    g_clock_cbInfo.callback = (MPPCallbackFuncType)&mpp_comm_clock_callback;
    AW_MPI_CLOCK_RegisterCallback(clock_chn, &g_clock_cbInfo);

    return 0;
}


int mpp_comm_vo_clock_destroy(CLOCK_CHN clock_chn)
{
    int ret = 0;

    ret = AW_MPI_CLOCK_DestroyChn(clock_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_CLOCK_DestroyChn fail! clock_chn[%d] ret[0x%x]\n", clock_chn, ret);
        return -1;
    }

    return 0;
}


ERRORTYPE mpp_comm_vo_bind_clock(int clock_chn, int vo_chn)
{
    ERRORTYPE ret = SUCCESS;
    MPP_CHN_S VoChn, ClockChn;

    VoChn.mModId = MOD_ID_VOU;
    VoChn.mDevId = HLAY(vo_chn, 0);
    VoChn.mChnId = 0;

    ClockChn.mModId = MOD_ID_CLOCK;
    ClockChn.mDevId = 0;
    ClockChn.mChnId = clock_chn;

    ret = AW_MPI_SYS_Bind(&ClockChn, &VoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind clock bind vo fail! ret:%d\n", ret);
        return FAILURE;
    }

    return ret;
}


ERRORTYPE mpp_comm_vo_unbind_clock(int clock_chn, int vo_chn)
{
    ERRORTYPE ret = SUCCESS;
    MPP_CHN_S VoChn, ClockChn;

    VoChn.mModId = MOD_ID_VOU;
    VoChn.mDevId = HLAY(vo_chn, 0);
    VoChn.mChnId = 0;

    ClockChn.mModId = MOD_ID_CLOCK;
    ClockChn.mDevId = 0;
    ClockChn.mChnId = clock_chn;

    ret = AW_MPI_SYS_UnBind(&ClockChn, &VoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind clock unbind vo fail! ret:%d\n", ret);
        return FAILURE;
    }

    return ret;
}


