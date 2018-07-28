
//#define LOG_NDEBUG 0
#define LOG_TAG "sample_audioeffect"
#include <utils/plat_log.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>


#include <mm_common.h>
#include <mpi_sys.h>
#include <mpi_ai.h>
#include <mpi_ao.h>

#include <confparser.h>

#include "sample_audioeffect_config.h"
#include "sample_audioeffect.h"

#include <cdx_list.h>

static int ParseCmdLine(int argc, char **argv, SampleAudioeffectCmdLineParam *pCmdLinePara)
{
    alogd("sample_audioeffect path:[%s], arg number is [%d]", argv[0], argc);
    int ret = 0;
    int i=1;
    memset(pCmdLinePara, 0, sizeof(SampleAudioeffectCmdLineParam));
    while(i < argc) {
        if(!strcmp(argv[i], "-path")) {
            if(++i >= argc) {
                aloge("fatal error! use -h to learn how to set parameter!!!");
                ret = -1;
                break;
            }
            if(strlen(argv[i]) >= MAX_FILE_PATH_SIZE) {
                aloge("fatal error! file path[%s] too long: [%d]>=[%d]!", argv[i], strlen(argv[i]), MAX_FILE_PATH_SIZE);
            }
            strncpy(pCmdLinePara->mConfigFilePath, argv[i], MAX_FILE_PATH_SIZE-1);
            pCmdLinePara->mConfigFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
        } else if(!strcmp(argv[i], "-h")) {
            alogd("CmdLine param:\n"
                  "\t-path /home/sample_audioeffect.conf\n");
            ret = 1;
            break;
        } else {
            alogd("ignore invalid CmdLine param:[%s], type -h to get how to set parameter!", argv[i]);
        }
        i++;
    }
    return ret;
}

static ERRORTYPE loadSampleAudioeffectConfig(SampleAudioeffectConfig *pConfig, const char *conf_path)
{
    int ret;
    char *ptr;
    CONFPARSER_S stConfParser;

    ret = createConfParser(conf_path, &stConfParser);
    if(ret < 0) {
        aloge("load conf fail");
        return FAILURE;
    }
    memset(pConfig, 0, sizeof(SampleAudioeffectConfig));
    pConfig->mSampleRate = GetConfParaInt(&stConfParser, SAMPLE_AUDIOEFFECT_PCM_SAMPLE_RATE, 0);
    pConfig->mBitWidth = GetConfParaInt(&stConfParser, SAMPLE_AUDIOEFFECT_PCM_BIT_WIDTH, 0);
    pConfig->mChannelCnt = GetConfParaInt(&stConfParser, SAMPLE_AUDIOEFFECT_PCM_CHANNEL_CNT, 0);
    alogd("parse info: mSampleRate[%d], mBitWidth[%d], mChannelCnt[%d]",
          pConfig->mSampleRate, pConfig->mBitWidth, pConfig->mChannelCnt);
    destroyConfParser(&stConfParser);

    return SUCCESS;
}

void config_AIO_ATTR_S_by_SampleAudioeffectConfig(AIO_ATTR_S *dst, SampleAudioeffectConfig *src)
{
    memset(dst, 0, sizeof(AIO_ATTR_S));
    dst->u32ChnCnt = src->mChannelCnt;
    dst->enSamplerate = (AUDIO_SAMPLE_RATE_E)src->mSampleRate;
    dst->enBitwidth = (AUDIO_BIT_WIDTH_E)(src->mBitWidth/8-1);
}

static SampleAudioeffectContext *gp_Ctx;

void signalHandler(int signo)
{
    alogd("detect signal, ready to finish test...");
    if (gp_Ctx != NULL) {
        gp_Ctx->mOverFlag = TRUE;
    } else {
        aloge("Are you kidding me?! you must kill it by youself!");
    }
}


int main(int argc, char *argv[])
{
    int result = 0;
    alogd("Hello, sample_audioeffect!");
    SampleAudioeffectContext stContext;
    memset(&stContext, 0, sizeof(SampleAudioeffectContext));
    gp_Ctx = &stContext;

    //parse command line param
    if(ParseCmdLine(argc, argv, &stContext.mCmdLinePara) != 0) {
        //aloge("fatal error! command line param is wrong, exit!");
        result = -1;
        goto _exit;
    }
    char *pConfigFilePath;
    if(strlen(stContext.mCmdLinePara.mConfigFilePath) > 0) {
        pConfigFilePath = stContext.mCmdLinePara.mConfigFilePath;
    } else {
        pConfigFilePath = DEFAULT_SAMPLE_AUDIOEFFECT_CONF_PATH;
    }

    //parse config file.
    if(loadSampleAudioeffectConfig(&stContext.mConfigPara, pConfigFilePath) != SUCCESS) {
        aloge("fatal error! no config file or parse conf file fail!");
        result = -1;
        goto _exit;
    }

    //init mpp system
    stContext.mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&stContext.mSysConf);
    AW_MPI_SYS_Init();

    //enable ai&ao dev
    stContext.mAIODev = 0;
    config_AIO_ATTR_S_by_SampleAudioeffectConfig(&stContext.mAIOAttr, &stContext.mConfigPara);
    AW_MPI_AI_SetPubAttr(stContext.mAIODev, &stContext.mAIOAttr);
    AW_MPI_AO_SetPubAttr(stContext.mAIODev, &stContext.mAIOAttr);
    AW_MPI_AI_Enable(stContext.mAIODev);
    AW_MPI_AO_Enable(stContext.mAIODev);

    //create ai channel.
    ERRORTYPE ret;
    BOOL bSuccessFlag = FALSE;
    stContext.mAIChn = 0;
    while(stContext.mAIChn < AIO_MAX_CHN_NUM) {
        ret = AW_MPI_AI_CreateChn(stContext.mAIODev, stContext.mAIChn);
        if(SUCCESS == ret) {
            bSuccessFlag = TRUE;
            alogd("create ai channel[%d] success!", stContext.mAIChn);
            break;
        } else if (ERR_AI_EXIST == ret) {
            alogd("ai channel[%d] exist, find next!", stContext.mAIChn);
            stContext.mAIChn++;
        } else if(ERR_AI_NOT_ENABLED == ret) {
            aloge("audio_hw_ai not started!");
            break;
        } else {
            aloge("create ai channel[%d] fail! ret[0x%x]!", stContext.mAIChn, ret);
            break;
        }
    }
    if(FALSE == bSuccessFlag) {
        stContext.mAIChn = MM_INVALID_CHN;
        aloge("fatal error! create ai channel fail!");
        goto _exit;
    }

    //create ao channel.
    bSuccessFlag = FALSE;
    stContext.mAOChn = 0;
    while(stContext.mAOChn < AIO_MAX_CHN_NUM) {
        ret = AW_MPI_AO_EnableChn(stContext.mAIODev, stContext.mAOChn);
        if(SUCCESS == ret) {
            bSuccessFlag = TRUE;
            alogd("create ao channel[%d] success!", stContext.mAOChn);
            break;
        } else if (ERR_AO_EXIST == ret) {
            alogd("ao channel[%d] exist, find next!", stContext.mAOChn);
            stContext.mAOChn++;
        } else if(ERR_AO_NOT_ENABLED == ret) {
            aloge("audio_hw_ao not started!");
            break;
        } else {
            aloge("create ao channel[%d] fail! ret[0x%x]!", stContext.mAOChn, ret);
            break;
        }
    }
    if(FALSE == bSuccessFlag) {
        stContext.mAOChn = MM_INVALID_CHN;
        aloge("fatal error! create ao channel fail!");
        goto _exit;
    }

    //bind ai and ao
    MPP_CHN_S AIChn = {MOD_ID_AI, stContext.mAIODev, stContext.mAIChn};
    MPP_CHN_S AOChn = {MOD_ID_AO, stContext.mAIODev, stContext.mAOChn};
    AW_MPI_SYS_Bind(&AIChn, &AOChn);
    //AW_MPI_SYS_Bind(&AOChn, &AIChn);

    //start ai chn.
    AW_MPI_AI_EnableChn(stContext.mAIODev, stContext.mAIChn);
    //start ao chn.
    AW_MPI_AO_StartChn(stContext.mAIODev, stContext.mAOChn);

    alogd("###############[ sample test goal: ai send pcm to ao through audio effectLib process]###############");
    alogd("###############[ you can finish test by ctrl+c                                      ]###############");
    if (signal(SIGINT, signalHandler) == SIG_ERR)
        aloge("register signal fail!");

    alogd("-----------key HELP for promote!-----------");

    while (!stContext.mOverFlag) {
        char buf[64] = {0};
        fgets(buf, 64, stdin);
        if (!strncmp(buf, "help", 4)) {
            alogd("\t 0 0    - get capture vol");
            alogd("\t 1 0 xx - set capture vol");
            alogd("\t 0 1    - get playback vol");
            alogd("\t 1 1 xx - set playback vol");
            alogd("\t 2 0/1(usr/default) xxx(max_suppression:-30~-12) yy(overlap_percnt:0/1) zz(nonstat:0~3) - set rnr(nosc)");
            alogd("\t 3  - disable rnr");
            alogd("\t 4 0/1 xx(tl:-25~0) yy(mg:0~15) zz(mg:0~15) xx(at:1~1000) yy(rt:1~10000) zz(nt:-60~-35) - set drc(agc)");
            alogd("\t 5  - disable drc");
            alogd("\t 6 xx(preamp:-20~20) - set gain");
            alogd("\t 7  - disable gain");
            alogd("\t 8 use default para - set aec");
            alogd("\t 9  - disable aec");
            alogd("\t a use default para - set eq");
            alogd("\t b  - disable eq");
            alogd("\t c user key value - set resample");
            alogd("\t d  - disable resample");
        } else if (!strncmp(buf, "exit", 4)) {
            break;
        } else if (strlen(buf) == 0) {
            continue;
        } else if (buf[0] == '0') {
            int vol;
            if (atoi(buf+2) == 0) {
                AW_MPI_AI_GetDevVolume(stContext.mAIODev, &vol);
                alogd("get capture vol: %d", vol);
            } else if (atoi(buf+2) == 1) {
                AW_MPI_AO_GetDevVolume(stContext.mAIODev, &vol);
                alogd("get playback vol: %d", vol);
            } else {
                aloge("error input cmd: %s", buf);
            }
        } else if (buf[0] == '1') {
            int vol = atoi(buf+4);
            if (atoi(buf+2) == 0) {
                AW_MPI_AI_SetDevVolume(stContext.mAIODev, vol);
                alogd("set capture vol: %d", vol);
            } else if (atoi(buf+2) == 1) {
                AW_MPI_AO_SetDevVolume(stContext.mAIODev, vol);
                alogd("set playback vol: %d", vol);
            } else {
                aloge("error input cmd: %s", buf);
            }
        } else if (buf[0] == '2') {
            int sup, overlap, nonstat;
            if (atoi(buf+2) == 0) {
                sup = atoi(buf+4);
                overlap = atoi(buf+8);
                nonstat = atoi(buf+10);
            } else {
                sup = -12;
                overlap = 1;
                nonstat = 3;
            }
            alogd("RNR: mode-%d, max_suppression-%d, overlap_percnt-%d, nonstat-%d", atoi(buf+2), sup, overlap, nonstat);
            AI_RNR_CONFIG_S conf;
            memset(&conf, 0, sizeof(conf));
            conf.sMaxNoiseSuppression = sup;
            conf.sOverlapPercent = overlap;
            conf.sNonstat = nonstat;
            AI_VQE_CONFIG_S vqe_conf;
            memset(&vqe_conf, 0, sizeof(vqe_conf));
            vqe_conf.stRnrCfg = conf;
            vqe_conf.bRnrOpen = 1;
            AW_MPI_AI_SetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            AW_MPI_AI_EnableVqe(stContext.mAIODev, stContext.mAIChn);
        } else if (buf[0] == '3') {
            alogd("RNR disable");
            AI_VQE_CONFIG_S vqe_conf;
            AW_MPI_AI_GetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            vqe_conf.bRnrOpen = 0;
            AW_MPI_AI_SetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            //AW_MPI_AI_DisableVqe(stContext.mAIODev, stContext.mAIChn);
        } else if (buf[0] == '4') {
            int target_level, max_gain, min_gain, attack_time, release_time, noise_threshold;
            if (atoi(buf+2) == 0) {
                target_level = atoi(buf+4);
                max_gain = atoi(buf+8);
                min_gain = atoi(buf+11);
                attack_time = atoi(buf+14);
                release_time = atoi(buf+19);
                noise_threshold = atoi(buf+25);
            } else {
                target_level = -9;
                max_gain = 6;
                min_gain = -9;
                attack_time = 1;
                release_time = 100;
                noise_threshold = -45;
            }
            alogd("DRC: mode-%d, target_level-%d, max_gain-%d, min_gain-%d, attack_time-%d, release_time-%d, noise_threshold-%d",
                  atoi(buf+2), target_level, max_gain, min_gain, attack_time, release_time, noise_threshold);
            AI_DRC_CONFIG_S conf;
            conf.sTargetLevel = target_level;
            conf.sMaxGain = max_gain;
            conf.sMinGain = min_gain;
            conf.sAttackTime = attack_time;
            conf.sReleaseTime = release_time;
            conf.sNoiseThreshold = noise_threshold;
            AI_VQE_CONFIG_S vqe_conf;
            memset(&vqe_conf, 0, sizeof(vqe_conf));
            vqe_conf.stDrcCfg = conf;
            vqe_conf.bDrcOpen = 1;
            AW_MPI_AI_SetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            AW_MPI_AI_EnableVqe(stContext.mAIODev, stContext.mAIChn);
        } else if (buf[0] == '5') {
            alogd("DRC disable");
            AI_VQE_CONFIG_S vqe_conf;
            AW_MPI_AI_GetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            vqe_conf.bDrcOpen = 0;
            AW_MPI_AI_SetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            //AW_MPI_AI_DisableVqe(stContext.mAIODev, stContext.mAIChn);
        } else if (buf[0] == '6') {
            int preamp;
            if (atoi(buf+2) == 0) {
                preamp = atoi(buf+4);
            } else {
                preamp = 20;
            }

            AUDIO_GAIN_CONFIG_S gain_conf;
            memset(&gain_conf, 0, sizeof(gain_conf));
            alogd("GAIN: mode-%d, gain-%d", atoi(buf+2), preamp);
            gain_conf.s8GainValue = preamp;
            AO_VQE_CONFIG_S vqe_conf;
            memset(&vqe_conf, 0, sizeof(vqe_conf));
            vqe_conf.stGainCfg = gain_conf;
            vqe_conf.bGainOpen = 1;
            AW_MPI_AO_SetVqeAttr(stContext.mAIODev, stContext.mAOChn, &vqe_conf);
            AW_MPI_AO_EnableVqe(stContext.mAIODev, stContext.mAOChn);
        } else if (buf[0] == '7') {
            alogd("GAIN disable");
            AO_VQE_CONFIG_S vqe_conf;
            AW_MPI_AO_GetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            vqe_conf.bGainOpen = 0;
            AW_MPI_AO_SetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            //AW_MPI_AO_DisableVqe(stContext.mAIODev, stContext.mAOChn);
        } else if (buf[0] == '8') {
            alogd("AEC: use default param. this item should be lastly tested!");
            AI_VQE_CONFIG_S vqe_conf;
            memset(&vqe_conf, 0, sizeof(vqe_conf));
            vqe_conf.bAecOpen = 1;
            AW_MPI_AI_SetVqeAttr(stContext.mAIODev, stContext.mAOChn, &vqe_conf);
            AW_MPI_AI_EnableVqe(stContext.mAIODev, stContext.mAOChn);
        } else if (buf[0] == '9') {
            alogd("AEC disable");
            AI_VQE_CONFIG_S vqe_conf;
            AW_MPI_AI_GetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            vqe_conf.bAecOpen = 0;
            AW_MPI_AI_SetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            //AW_MPI_AI_DisableVqe(stContext.mAIODev, stContext.mAIChn);
        } else if (buf[0] == 'a') {
            alogd("EQ: use default param. It can be tested in AI & AO, but this only tested in AI!");
            AI_VQE_CONFIG_S vqe_conf;
            memset(&vqe_conf, 0, sizeof(vqe_conf));
            vqe_conf.bEqOpen = 1;
            AW_MPI_AI_SetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            AW_MPI_AI_EnableVqe(stContext.mAIODev, stContext.mAIChn);
        } else if (buf[0] == 'b') {
            alogd("EQ disable");
            AI_VQE_CONFIG_S vqe_conf;
            AW_MPI_AI_GetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            vqe_conf.bEqOpen = 0;
            AW_MPI_AI_SetVqeAttr(stContext.mAIODev, stContext.mAIChn, &vqe_conf);
            //AW_MPI_AI_DisableVqe(stContext.mAIODev, stContext.mAIChn);
        } else if (buf[0] == 'c') {
            alogd("RESAMPLE: usr key dst sample_rate! wait to key...");
            char key_buf[64] = {0};
            fgets(key_buf, 64, stdin);
            int dst_samplerate = atoi(key_buf);
            AO_VQE_CONFIG_S vqe_conf;
            memset(&vqe_conf, 0, sizeof(vqe_conf));
            vqe_conf.s32WorkSampleRate = dst_samplerate;
            alogd("you keyed dst samplerate: %d, open resample!", dst_samplerate);
            AW_MPI_AO_SetVqeAttr(stContext.mAIODev, stContext.mAOChn, &vqe_conf);
            AW_MPI_AO_EnableVqe(stContext.mAIODev, stContext.mAOChn);
        } else if (buf[0] == 'd') {
            alogd("RESAMPLE disable!");
            AO_VQE_CONFIG_S vqe_conf;
            AW_MPI_AO_GetVqeAttr(stContext.mAIODev, stContext.mAOChn, &vqe_conf);
            vqe_conf.s32WorkSampleRate = 0;     //0 means disable it
            AW_MPI_AO_SetVqeAttr(stContext.mAIODev, stContext.mAOChn, &vqe_conf);
            //AW_MPI_AO_DisableVqe(stContext.mAIODev, stContext.mAOChn);
        } else {
            alogd("unknown cmd: %s", buf);
        }
    }

_deinit_ao:
    //stop and reset ao chn, destroy ao dev
    AW_MPI_AO_StopChn(stContext.mAIODev, stContext.mAOChn);
    AW_MPI_AO_DisableChn(stContext.mAIODev, stContext.mAOChn);

_deinit_ai:
    //stop ai chn
    AW_MPI_AI_DisableChn(stContext.mAIODev, stContext.mAIChn);
    //reset ai chn and destroy ai dev
    AW_MPI_AI_ResetChn(stContext.mAIODev, stContext.mAIChn);
    AW_MPI_AI_DestroyChn(stContext.mAIODev, stContext.mAIChn);

    stContext.mAIODev = MM_INVALID_DEV;
    stContext.mAOChn = MM_INVALID_CHN;
    stContext.mAIChn = MM_INVALID_CHN;

    //exit mpp system
    AW_MPI_SYS_Exit();

_exit:
    alogd("Goodbye, sample_audioeffect!");
    return result;
}
