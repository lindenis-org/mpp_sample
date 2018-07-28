
#ifndef _SAMPLE_AUDIOEFFECT_H_
#define _SAMPLE_AUDIOEFFECT_H_

#include <plat_type.h>

#define MAX_FILE_PATH_SIZE (256)

typedef struct SampleAudioeffectCmdLineParam {
    char mConfigFilePath[MAX_FILE_PATH_SIZE];
} SampleAudioeffectCmdLineParam;

typedef struct SampleAudioeffectConfig {
    int mSampleRate;
    int mChannelCnt;
    int mBitWidth;
} SampleAudioeffectConfig;

typedef struct pcmFrameNode {
    AUDIO_FRAME_S frame;
    struct list_head mList;
} pcmFrameNode;

typedef struct SampleAudioeffectContext {
    SampleAudioeffectCmdLineParam mCmdLinePara;
    SampleAudioeffectConfig mConfigPara;

    MPP_SYS_CONF_S mSysConf;
    AUDIO_DEV mAIODev;
    AI_CHN mAIChn;
    AO_CHN mAOChn;
    AIO_ATTR_S mAIOAttr;

    pthread_mutex_t mPcmlock;
    struct list_head mPcmIdleList;   // AUDIO_FRAME_INFO_S
    struct list_head mPcmUsingList;   // AUDIO_FRAME_INFO_S

    BOOL mOverFlag;
} SampleAudioeffectContext;

#endif  /* _SAMPLE_AUDIOEFFECT_H_ */

