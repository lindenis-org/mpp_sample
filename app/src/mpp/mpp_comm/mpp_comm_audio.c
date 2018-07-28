/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_comm_audio.c
 * @brief 该目录是对mpp中 Audio 模块的公共操作,参数设置和获取类型进行简单抽象
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
/* None */


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
static int g_audio_fd[DEMUX_MAX_CHN_NUM] = {-1};


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/

static int audio_get_bitwidth_by_type(AUDIO_BIT_WIDTH_E type, int *bit_width)
{
    if (NULL == bit_width) {
        ERR_PRT("Input bit_width is null!\n");
        return -1;
    }

    switch(type) {
    case AUDIO_BIT_WIDTH_8:
        *bit_width = 8;
        break;
    case AUDIO_BIT_WIDTH_16:
        *bit_width = 16;
        break;
    case AUDIO_BIT_WIDTH_24:
        *bit_width = 24;
        break;
    case AUDIO_BIT_WIDTH_32:
        *bit_width = 32;
        break;

    default:
        ERR_PRT("Input bit_width_type:%d is not support!\n", type);
        *bit_width = -1;
        return -1;
        break;
    }

    return 0;
}


static int audio_get_samplerate_by_type(AUDIO_SAMPLE_RATE_E type, int *sample_rate)
{
    if (NULL == sample_rate) {
        ERR_PRT("Input sample_rate is null!\n");
        return -1;
    }

    switch(type) {
    case AUDIO_SAMPLE_RATE_8000:
        *sample_rate = 8000;
        break;
    case AUDIO_SAMPLE_RATE_16000:
        *sample_rate = 16000;
        break;
    case AUDIO_SAMPLE_RATE_22050:
        *sample_rate = 22050;
        break;
    case AUDIO_SAMPLE_RATE_24000:
        *sample_rate = 24000;
        break;
    case AUDIO_SAMPLE_RATE_32000:
        *sample_rate = 32000;
        break;
    case AUDIO_SAMPLE_RATE_44100:
        *sample_rate = 44100;
        break;
    case AUDIO_SAMPLE_RATE_48000:
        *sample_rate = 48000;
        break;

    default:
        ERR_PRT("Input sample_rate_type:%d is not support!\n", type);
        *sample_rate = -1;
        return -1;
        break;
    }

    return 0;
}


int mpp_comm_get_audio_cfg(AUDIO_CFG_TYPE_E audio_type, AUDIO_CFG_S *audio_cfg)
{
    if (NULL == audio_cfg) {
        ERR_PRT("Input audio_cfg is null!\n");
        return -1;
    }

    switch(audio_type) {
    case AUDIO_AAC_16BIT_8K_MONO:
        audio_cfg->bit_width   = AUDIO_BIT_WIDTH_16;
        audio_cfg->sample_rate  = AUDIO_SAMPLE_RATE_8000;
        audio_cfg->chn_num      = 1;
        audio_cfg->sample_frame = 1024;
        audio_cfg->codec_type   = PT_AAC;
        audio_cfg->audio_mode   = AUDIO_SOUND_MODE_MONO;
        break;
    case AUDIO_AAC_16BIT_16K_MONO:
        audio_cfg->bit_width   = AUDIO_BIT_WIDTH_16;
        audio_cfg->sample_rate  = AUDIO_SAMPLE_RATE_16000;
        audio_cfg->chn_num      = 1;
        audio_cfg->sample_frame = 1024;
        audio_cfg->codec_type   = PT_AAC;
        audio_cfg->audio_mode   = AUDIO_SOUND_MODE_MONO;
        break;

    case AUDIO_ADPCM_16BIT_8K_MONO:
        audio_cfg->bit_width   = AUDIO_BIT_WIDTH_16;
        audio_cfg->sample_rate  = AUDIO_SAMPLE_RATE_8000;
        audio_cfg->chn_num      = 1;
        audio_cfg->sample_frame = 1024;
        audio_cfg->codec_type   = PT_ADPCMA;
        audio_cfg->audio_mode   = AUDIO_SOUND_MODE_MONO;
        break;
    case AUDIO_ADPCM_16BIT_16K_MONO:
        audio_cfg->bit_width   = AUDIO_BIT_WIDTH_16;
        audio_cfg->sample_rate  = AUDIO_SAMPLE_RATE_16000;
        audio_cfg->chn_num      = 1;
        audio_cfg->sample_frame = 1024;
        audio_cfg->codec_type   = PT_ADPCMA;
        audio_cfg->audio_mode   = AUDIO_SOUND_MODE_MONO;
        break;

    case AUDIO_MP3_16BIT_8K_MONO:
        audio_cfg->bit_width   = AUDIO_BIT_WIDTH_16;
        audio_cfg->sample_rate  = AUDIO_SAMPLE_RATE_8000;
        audio_cfg->chn_num      = 1;
        audio_cfg->sample_frame = 1024;
        audio_cfg->codec_type   = PT_MP3;
        audio_cfg->audio_mode   = AUDIO_SOUND_MODE_MONO;
        break;
    case AUDIO_MP3_16BIT_16K_MONO:
        audio_cfg->bit_width   = AUDIO_BIT_WIDTH_16;
        audio_cfg->sample_rate  = AUDIO_SAMPLE_RATE_16000;
        audio_cfg->chn_num      = 1;
        audio_cfg->sample_frame = 1024;
        audio_cfg->codec_type   = PT_MP3;
        audio_cfg->audio_mode   = AUDIO_SOUND_MODE_MONO;
        break;

    case AUDIO_G711A_16BIT_8K_MONO:
        audio_cfg->bit_width   = AUDIO_BIT_WIDTH_16;
        audio_cfg->sample_rate  = AUDIO_SAMPLE_RATE_8000;
        audio_cfg->chn_num      = 1;
        audio_cfg->sample_frame = 1024;
        audio_cfg->codec_type   = PT_G711A;
        audio_cfg->audio_mode   = AUDIO_SOUND_MODE_MONO;
        break;
    case AUDIO_G711A_16BIT_16K_MONO:
        audio_cfg->bit_width   = AUDIO_BIT_WIDTH_16;
        audio_cfg->sample_rate  = AUDIO_SAMPLE_RATE_16000;
        audio_cfg->chn_num      = 1;
        audio_cfg->sample_frame = 1024;
        audio_cfg->codec_type   = PT_G711A;
        audio_cfg->audio_mode   = AUDIO_SOUND_MODE_MONO;
        break;

    default:
        ERR_PRT("Input audio_type:%d is not support!\n", audio_type);
        return -1;
        break;
    }

    return 0;
}


/**
 * @brief  Create ai channel
 * @param
 * - ai_chn            input
 * - audio_cfg      input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_ai_create(AI_CHN ai_chn, AUDIO_CFG_S *audio_cfg)
{
    int  ret = 0;
    AIO_ATTR_S audio_attr;


    if (ai_chn < 0 || ai_chn >= AIO_MAX_CHN_NUM) {
        ERR_PRT("Input ai_chn:%d is error! 0~%d \n", ai_chn, AIO_MAX_CHN_NUM);
        return -1;
    }

    if (NULL == audio_cfg) {
        ERR_PRT("Input audio_cfg is null!\n");
        return -1;
    }

    memset(&audio_attr, 0, sizeof(AIO_ATTR_S));
    audio_attr.u32ChnCnt    = audio_cfg->chn_num;
    audio_attr.enSamplerate = audio_cfg->sample_rate;
    audio_attr.enBitwidth   = audio_cfg->bit_width;
    audio_attr.enSoundmode  = audio_cfg->audio_mode;

    /* Setup 1.  Enable audio_hw_ai and setting ai attr */
    ret = AW_MPI_AI_SetPubAttr(AI_DEV_0, &audio_attr);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_AI_SetPubAttr ai_dev:%d fail! ret:0x%x\n", AI_DEV_0, ret);
        return -1;
    }

    /* Setup 2.  Create Ai channel.  */
    ret = AW_MPI_AI_CreateChn(AI_DEV_0, ai_chn);
    if (ERR_AI_EXIST == ret) {
        ERR_PRT("Ai chn[%d] is exist, find next!\n", ai_chn);
        return -1;
    } else if (ERR_AI_NOT_ENABLED == ret) {
        ERR_PRT("audio_hw_ai ai_dev:%d not started!", AI_DEV_0);
        return -1;
    } else if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_AI_CreateChn ai_chn:%d fail! ret:0x%x\n", ai_chn, ret);
        return -1;
    }
    DB_PRT("Create ai_chn:%d success!\n", ai_chn);

    return 0;
}

int mpp_comm_ai_destroy(AI_CHN ai_chn)
{
    int ret = 0;

    ret = AW_MPI_AI_ResetChn(AI_DEV_0, ai_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_AI_ResetChn ai_chn:%d fail! ret:0x%x\n", ai_chn, ret);
        //return -1;
    }

    ret = AW_MPI_AI_DestroyChn(AI_DEV_0, ai_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_AI_DestroyChn ai_chn:%d fail! ret:0x%x\n", ai_chn, ret);
        //return -1;
    }

    return ret;
}


int mpp_comm_ai_bind_ao(int ai_chn, int ao_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S AiChn, AoChn;

    AiChn.mModId = MOD_ID_AI;
    AiChn.mDevId = 0;
    AiChn.mChnId = ai_chn;

    AoChn.mModId = MOD_ID_AO;
    AoChn.mDevId = 0;
    AoChn.mChnId = ao_chn;

    ret = AW_MPI_SYS_Bind(&AiChn, &AoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind ai:%d bind ao:%d fail! ret:0x%x\n", ai_chn, ao_chn, ret);
        return FAILURE;
    }

    return ret;
}

int mpp_comm_ai_unbind_ao(int ai_chn, int ao_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S AiChn, AoChn;

    AiChn.mModId = MOD_ID_AI;
    AiChn.mDevId = 0;
    AiChn.mChnId = ai_chn;

    AoChn.mModId = MOD_ID_AO;
    AoChn.mDevId = 0;
    AoChn.mChnId = ao_chn;

    ret = AW_MPI_SYS_UnBind(&AiChn, &AoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind ai:%d unbind ao:%d fail! ret:0x%x\n", ai_chn, ao_chn, ret);
        return FAILURE;
    }

    return ret;
}


int mpp_comm_ai_bind_aenc(int ai_chn, int aenc_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S AiChn, AencChn;

    AiChn.mModId = MOD_ID_AI;
    AiChn.mDevId = 0;
    AiChn.mChnId = ai_chn;

    AencChn.mModId = MOD_ID_AENC;
    AencChn.mDevId = 0;
    AencChn.mChnId = aenc_chn;

    ret = AW_MPI_SYS_Bind(&AiChn, &AencChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind ai:%d bind aenc:%d fail! ret:0x%x\n", ai_chn, aenc_chn, ret);
        return FAILURE;
    }

    return ret;
}

int mpp_comm_ai_unbind_aenc(int ai_chn, int aenc_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S AiChn, AencChn;

    AiChn.mModId = MOD_ID_AI;
    AiChn.mDevId = 0;
    AiChn.mChnId = ai_chn;

    AencChn.mModId = MOD_ID_AENC;
    AencChn.mDevId = 0;
    AencChn.mChnId = aenc_chn;

    ret = AW_MPI_SYS_UnBind(&AiChn, &AencChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind ai:%d bind aenc:%d fail! ret:0x%x\n", ai_chn, aenc_chn, ret);
        return FAILURE;
    }

    return ret;
}


/**
 * @brief  Create ao channel   .guixing
 * @param
 * - ao_chn           input
 * - audio_cfg       input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_ao_create(AO_CHN ao_chn, AUDIO_CFG_S *audio_cfg, MPPCallbackInfo *cb_info)
{
    int  ret = 0;
    AIO_ATTR_S audio_attr;


    if (ao_chn < 0 || ao_chn >= AIO_MAX_CHN_NUM) {
        ERR_PRT("Input ao_chn:%d is error! 0~%d \n", ao_chn, AIO_MAX_CHN_NUM);
        return -1;
    }

    if (NULL == audio_cfg) {
        ERR_PRT("Input audio_cfg is null!\n");
        return -1;
    }

    memset(&audio_attr, 0, sizeof(AIO_ATTR_S));
    audio_attr.u32ChnCnt    = audio_cfg->chn_num;
    audio_attr.enSamplerate = audio_cfg->sample_rate;
    audio_attr.enBitwidth   = audio_cfg->bit_width;
    audio_attr.enSoundmode  = audio_cfg->audio_mode;

    /* Setup 1.  Enable audio_hw_ao and setting ao attr */
    ret = AW_MPI_AO_SetPubAttr(AO_DEV_0, &audio_attr);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_AO_SetPubAttr ao_dev:%d fail! ret:0x%x\n", AO_DEV_0, ret);
        return -1;
    }

    ret = AW_MPI_AO_Enable(AO_DEV_0);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_AO_Enable ao_dev:%d fail! ret:0x%x\n", AO_DEV_0, ret);
        return -1;
    }

    /* Setup 2.  Create Ao channel.  */
    ret = AW_MPI_AO_EnableChn(AO_DEV_0, ao_chn);
    if (ERR_AO_EXIST == ret) {
        ERR_PRT("Ao chn[%d] is exist, find next!\n", ao_chn);
        return -1;
    } else if (ERR_AO_NOT_ENABLED == ret) {
        ERR_PRT("audio_hw_ao ao_dev:%d not started!", AO_DEV_0);
        return -1;
    } else if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_AO_EnableChn ao_chn:%d fail! ret:0x%x\n", ao_chn, ret);
        return -1;
    }
    DB_PRT("Create ao_chn:%d success!\n", ao_chn);

    if (NULL != cb_info) {
        AW_MPI_AO_RegisterCallback(AO_DEV_0, ao_chn, cb_info);
    }

    return 0;
}

int mpp_comm_ao_destroy(AO_CHN ao_chn)
{
    int ret = 0;

    ret = AW_MPI_AO_DisableChn(AO_DEV_0, ao_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_AO_DisableChn ao_chn:%d fail! ret:0x%x\n", ao_chn, ret);
        return -1;
    }

    return ret;
}


int mpp_comm_ao_clock_create(CLOCK_CHN clock_chn)
{
    int ret = 0;
    CLOCK_CHN_ATTR_S clock_attr;

    clock_attr.nWaitMask = 0;
    clock_attr.nWaitMask |= 1<<CLOCK_PORT_INDEX_AUDIO; /* to video render */
    ret = AW_MPI_CLOCK_CreateChn(clock_chn, &clock_attr);
    if (ERR_CLOCK_EXIST == ret) {
        ERR_PRT("clock channel[%d] is exist, find next!\n", clock_chn);
        return -1;
    } else if (SUCCESS != ret) {
        ERR_PRT("Create clock channel[%d] ret[0x%x] fail!\n", clock_chn, ret);
        return -1;
    }
    DB_PRT("Create clock channel[%d] success!\n", clock_chn);

    return 0;
}


int mpp_comm_ao_clock_destroy(CLOCK_CHN clock_chn)
{
    int ret = 0;

    ret = AW_MPI_CLOCK_DestroyChn(clock_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_CLOCK_DestroyChn fail! clock_chn[%d] ret[0x%x]\n", clock_chn, ret);
        return -1;
    }

    return 0;
}


int mpp_comm_ao_bind_clock(int clock_chn, int ao_chn)
{
    ERRORTYPE ret = SUCCESS;
    MPP_CHN_S AoChn, ClockChn;

    AoChn.mModId = MOD_ID_AO;
    AoChn.mDevId = 0;
    AoChn.mChnId = ao_chn;

    ClockChn.mModId = MOD_ID_CLOCK;
    ClockChn.mDevId = 0;
    ClockChn.mChnId = clock_chn;

    ret = AW_MPI_SYS_Bind(&ClockChn, &AoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind clock:%d bind ao:%d fail! ret:%d\n", clock_chn, ao_chn, ret);
        return FAILURE;
    }

    return ret;
}

int mpp_comm_ao_unbind_clock(int clock_chn, int ao_chn)
{
    ERRORTYPE ret = SUCCESS;
    MPP_CHN_S AoChn, ClockChn;

    AoChn.mModId = MOD_ID_AO;
    AoChn.mDevId = 0;
    AoChn.mChnId = ao_chn;

    ClockChn.mModId = MOD_ID_CLOCK;
    ClockChn.mDevId = 0;
    ClockChn.mChnId = clock_chn;

    ret = AW_MPI_SYS_UnBind(&ClockChn, &AoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind clock:%d unbind ao:%d fail! ret:%d\n", clock_chn, ao_chn, ret);
        return FAILURE;
    }

    return ret;
}


/**
 * @brief  Create aenc channel
 * @param
 * - ai_chn            input
 * - audio_cfg      input
 * @return
 *  - SUCCESS 0
 *  - FAIL   -1
 */
int mpp_comm_aenc_create(AENC_CHN aenc_chn, AUDIO_CFG_S *audio_cfg)
{
    int  ret         = 0;
    int  bit_width   = 0;
    int  sample_rate = 0;
    AENC_CHN_ATTR_S audio_attr;

    if (aenc_chn < 0 || aenc_chn >= AENC_MAX_CHN_NUM) {
        ERR_PRT("Input aenc_chn:%d is error! 0~%d \n", aenc_chn, AENC_MAX_CHN_NUM);
        return -1;
    }

    if (NULL == audio_cfg) {
        ERR_PRT("Input audio_cfg is null!\n");
        return -1;
    }

    memset(&audio_attr, 0, sizeof(AENC_CHN_ATTR_S));
    audio_get_bitwidth_by_type(audio_cfg->bit_width, &bit_width);
    audio_get_samplerate_by_type(audio_cfg->sample_rate, &sample_rate);
    audio_attr.AeAttr.Type            = audio_cfg->codec_type;
    audio_attr.AeAttr.channels        = audio_cfg->chn_num;
    audio_attr.AeAttr.bitsPerSample   = bit_width;
    audio_attr.AeAttr.sampleRate      = sample_rate;
    audio_attr.AeAttr.attachAACHeader = 1;

    /* Setup 2.  Create Aenc channel.  */
    ret = AW_MPI_AENC_CreateChn(aenc_chn, &audio_attr);
    if (ERR_AENC_EXIST == ret) {
        ERR_PRT("Aenc chn[%d] is exist, find next!\n", aenc_chn);
        return -1;
    } else if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_AENC_CreateChn aenc_chn:%d fail! ret:0x%x\n", aenc_chn, ret);
        return -1;
    }
    DB_PRT("Create aenc_chn:%d success!\n", aenc_chn);

    return 0;
}


int mpp_comm_aenc_destroy(AENC_CHN aenc_chn)
{
    int ret = 0;

    ret = AW_MPI_AENC_ResetChn(aenc_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_AENC_ResetChn aenc_chn:%d fail! ret:0x%x\n", aenc_chn, ret);
        //return -1;
    }

    ret = AW_MPI_AENC_DestroyChn(aenc_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_AENC_DestroyChn aenc_chn:%d fail! ret:0x%x\n", aenc_chn, ret);
        //return -1;
    }

    return ret;
}


int mpp_comm_adec_create(ADEC_CHN adec_chn, AUDIO_CFG_S *audio_cfg, MPPCallbackInfo *cb_info)
{
    int  ret         = 0;
    int  bit_width   = 0;
    int  sample_rate = 0;
    ADEC_CHN_ATTR_S audio_attr;

    if (adec_chn < 0 || adec_chn >= ADEC_MAX_CHN_NUM) {
        ERR_PRT("Input adec_chn:%d is error! 0~%d \n", adec_chn, ADEC_MAX_CHN_NUM);
        return -1;
    }

    if (NULL == audio_cfg || NULL == cb_info) {
        ERR_PRT("Input audio_cfg or cb_info is null!\n");
        return -1;
    }

    /* Setup 1.  Config Adec channel attr  */
    memset(&audio_attr, 0, sizeof(ADEC_CHN_ATTR_S));
    audio_get_bitwidth_by_type(audio_cfg->bit_width, &bit_width);
    audio_get_samplerate_by_type(audio_cfg->sample_rate, &sample_rate);
    audio_attr.mType            = audio_cfg->codec_type;
    audio_attr.sampleRate       = sample_rate;
    audio_attr.channels         = audio_cfg->chn_num;
    audio_attr.bitsPerSample    = audio_cfg->sample_frame;
    //audio_attr.bitRate          = bit_width;
    //audio_attr.attachAACHeader  = 0;

    /* Setup 2.  Create Adec channel.  */
    ret = AW_MPI_ADEC_CreateChn(adec_chn, &audio_attr);
    if (ERR_AENC_EXIST == ret) {
        ERR_PRT("Adec chn[%d] is exist, find next!\n", adec_chn);
        return -1;
    } else if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_ADEC_CreateChn adec_chn:%d fail! ret:0x%x\n", adec_chn, ret);
        return -1;
    }
    DB_PRT("Create adec_chn:%d success!\n", adec_chn);

    /* Setup 3.  Register Adec process call back.  */
    AW_MPI_ADEC_RegisterCallback(adec_chn, cb_info);

    return 0;
}

int mpp_comm_adec_destroy(ADEC_CHN adec_chn)
{
    int ret = 0;

    ret = AW_MPI_ADEC_DestroyChn(adec_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_ADEC_DestroyChn adec_chn:%d fail! ret:0x%x\n", adec_chn, ret);
        return -1;
    }

    return ret;
}


int mpp_comm_adec_bind_ao(int adec_chn, int ao_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S AdecChn, AoChn;

    AdecChn.mModId = MOD_ID_ADEC;
    AdecChn.mDevId = 0;
    AdecChn.mChnId = adec_chn;

    AoChn.mModId = MOD_ID_AO;
    AoChn.mDevId = 0;
    AoChn.mChnId = ao_chn;

    ret = AW_MPI_SYS_Bind(&AdecChn, &AoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_Bind adec_chn:%d bind ao:%d fail! ret:0x%x\n", adec_chn, ao_chn, ret);
        return FAILURE;
    }

    return ret;
}

int mpp_comm_adec_unbind_ao(int adec_chn, int ao_chn)
{
    int       ret = SUCCESS;
    MPP_CHN_S AdecChn, AoChn;

    AdecChn.mModId = MOD_ID_ADEC;
    AdecChn.mDevId = 0;
    AdecChn.mChnId = adec_chn;

    AoChn.mModId = MOD_ID_AO;
    AoChn.mDevId = 0;
    AoChn.mChnId = ao_chn;

    ret = AW_MPI_SYS_UnBind(&AdecChn, &AoChn);
    if (ret) {
        ERR_PRT("Do AW_MPI_SYS_UnBind adec_chn:%d unbind ao:%d fail! ret:0x%x\n", adec_chn, ao_chn, ret);
        return FAILURE;
    }

    return ret;
}

int mpp_comm_audio_demux_create(DEMUX_CHN demux_chn, const char *file_path, MPPCallbackInfo *cb_info)
{
    int  ret = 0;
    DEMUX_CHN_ATTR_S demux_attr;

    if (demux_chn < 0 || demux_chn >= DEMUX_MAX_CHN_NUM) {
        ERR_PRT("Input demux_chn:%d is error! 0~%d \n", demux_chn, DEMUX_MAX_CHN_NUM);
        return -1;
    }

    if (NULL == cb_info || NULL == file_path) {
        ERR_PRT("Input cb_info or file_path is null!\n");
        return -1;
    }

    if (g_audio_fd[demux_chn] > 0) {
        ERR_PRT("The demux chn:%d file haved opened! error!\n", demux_chn);
        return -1;
    }

    g_audio_fd[demux_chn] = open(file_path, O_RDONLY);
    if (g_audio_fd[demux_chn] <= 0) {
        ERR_PRT("Open %s file error! ret:%d errno[%d] errinfo[%s] \n",
                file_path, g_audio_fd[demux_chn], errno, strerror(errno));
        return -1;
    }

    /* Step 0. Config demux channel attr */
    memset(&demux_attr, 0, sizeof(DEMUX_CHN_ATTR_S));
    demux_attr.mStreamType = STREAMTYPE_LOCALFILE;
    demux_attr.mSourceType = SOURCETYPE_FD;
    demux_attr.mSourceUrl  = NULL;
    demux_attr.mFd         = g_audio_fd[demux_chn];
    demux_attr.mDemuxDisableTrack = DEMUX_DISABLE_SUBTITLE_TRACK;

    /* Step 1. Create demux channel */
    ret = AW_MPI_DEMUX_CreateChn(demux_chn, &demux_attr);
    if (ERR_DEMUX_EXIST == ret) {
        ERR_PRT("Demux chn[%d] is exist, find next!\n", demux_chn);
        return -1;
    } else if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_DEMUX_CreateChn demux_chn:%d fail! ret:0x%x\n", demux_chn, ret);
        return -1;
    }
    DB_PRT("Create demux_chn:%d success!\n", demux_chn);

    /* Step 2. Register demux call back function for control demux decode file. */
    ret = AW_MPI_DEMUX_RegisterCallback(demux_chn, cb_info);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_DEMUX_RegisterCallback demux_chn:%d fail! ret:0x%x\n", demux_chn, ret);
        return -1;
    }

    /* Step 3. Get demux file information, for setting vdec model. */
    DEMUX_MEDIA_INFO_S media_info;
    ret = AW_MPI_DEMUX_GetMediaInfo(demux_chn, &media_info);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_DEMUX_GetMediaInfo fail! demux_chn:%d  ret:0x%x\n", demux_chn, ret);
        return -1;
    }

    if ((media_info.mVideoNum >0 && media_info.mVideoIndex >= media_info.mVideoNum)
        || (media_info.mAudioNum >0 && media_info.mAudioIndex >= media_info.mAudioNum)
        || (media_info.mSubtitleNum >0 && media_info.mSubtitleIndex >= media_info.mSubtitleNum)) {
        ERR_PRT("fatal error, trackIndex wrong! [%d][%d],[%d][%d],[%d][%d]",
                media_info.mVideoNum, media_info.mVideoIndex, media_info.mAudioNum,
                media_info.mAudioIndex, media_info.mSubtitleNum, media_info.mSubtitleIndex);
        return -1;
    }

    return 0;
}

int mpp_comm_audio_demux_destroy(DEMUX_CHN demux_chn)
{
    int  ret = 0;

    if (demux_chn < 0 || demux_chn >= DEMUX_MAX_CHN_NUM) {
        ERR_PRT("Input demux_chn:%d is error! 0~%d \n", demux_chn, DEMUX_MAX_CHN_NUM);
        return -1;
    }

    ret = AW_MPI_DEMUX_DestroyChn(demux_chn);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_DEMUX_DestroyChn fail! demux_chn:%d  ret:0x%x \n", demux_chn, ret);
        return -1;
    }

    if (g_audio_fd[demux_chn] > 0) {
        close(g_audio_fd[demux_chn]);
        g_audio_fd[demux_chn] = -1;
    }

    return 0;
}


ERRORTYPE mpp_comm_audio_demux_callbcak(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
    int ret = 0;
    DEMUX_CB_PARAM_S *pdemux_param = NULL;

    if (NULL != cookie) {
        pdemux_param = (DEMUX_CB_PARAM_S *)cookie;
    } else {
        return SUCCESS;
    }

    if (NULL != pChn) {
        DB_PRT(" ===> pChn->mModId:%d\n", pChn->mModId);
    }

    if (pChn->mModId == MOD_ID_DEMUX) {
        switch (event) {
        case MPP_EVENT_NOTIFY_EOF:
            DB_PRT("demux get EOF flag......\n");
            ret = AW_MPI_ADEC_SetStreamEof(pdemux_param->adec_chn, 1);
            if (SUCCESS != ret) {
                ERR_PRT("Do AW_MPI_ADEC_SetStreamEof fail! adec_chn:%d  ret:0x%x \n", pdemux_param->adec_chn, ret);
            }
            break;
        default:
            break;
        }
    } else if (pChn->mModId == MOD_ID_ADEC) {
        switch (event) {
        case MPP_EVENT_NOTIFY_EOF:
            DB_PRT("adec get EOF flag......\n");
            AW_MPI_AO_SetStreamEof(AO_DEV_0, pdemux_param->ao_chn, 1, 1);
            if (SUCCESS != ret) {
                ERR_PRT("Do AW_MPI_AO_SetStreamEof fail! ao_chn:%d  ret:0x%x \n", pdemux_param->ao_chn, ret);
            }
            break;
        default:
            break;
        }
    } else if (pChn->mModId == MOD_ID_AO) {
        switch (event) {
        case MPP_EVENT_NOTIFY_EOF:
            DB_PRT("ao get EOF flag......\n");
            pdemux_param->run_flag = 0;
            break;
        default:
            break;
        }
    }

    return SUCCESS;
}

