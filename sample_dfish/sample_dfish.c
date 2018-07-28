/******************************************************************************
  Copyright (C), 2001-2017, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : sample_dfish.c
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2017/1/5
  Last Modified :
  Description   : mpp component implement
  Function List :
  History       :
******************************************************************************/

#define LOG_TAG "sample_dfish"
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cdx_list.h>
#include <utils/plat_log.h>
#include "media/mm_comm_ise.h"
#include "media/mpi_ise.h"
#include "media/mpi_sys.h"
#include "mpi_ise_common.h"
#include "ion_memmanager.h"
#include "sc_interface.h"
#include "memoryAdapter.h"
#include <tsemaphore.h>

#include <confparser.h>
#include "sample_dfish.h"
#include "sample_dfish_config.h"

#define Save_Picture             1
#define TakePicture_By_Soft      0
#define TakePicture_By_Hardware  1
#define FEATHER                  64
#define TRANS_WIDTH              64
#define FINDPATH_WIDTH           96
#define PYR_LEVEL                5
#define CHANGE_FOCUS             0.905f
#define FINDPATH_ENABLE          true
#define INV_SCALE                2.0f
// 相机参数
#define TEST_P0                  1998.993f
#define TEST_CX0                 1943.624f
#define TEST_CY0                 1980.219f
#define TEST_P1                  2012.214f
#define TEST_CX1                 1938.909f
#define TEST_CY1                 1971.121f
BOOL Thread_EXIT;
typedef struct awPic_Cap_s {
    int PicWidth;
    int PicHeight;
    int PicStride;
    int PicFrameRate;
    FILE* Pic0_FilePath;
    FILE* Pic1_FilePath;
    pthread_t thread_id;
} awPic_Cap_s;

typedef struct awISE_PortCap_S {
    ISE_GRP  ISE_Group;
    ISE_CHN  ISE_Port;
    int width;
    int height;
    char *OutputFilePath;
    ISE_CHN_ATTR_S PortAttr;
    AW_S32   s32MilliSec;
    pthread_t thread_id;
    int Process_Count;
} ISE_PortCap_S;

typedef struct awISE_pGroupCap_S {
    ISE_GRP ISE_Group;
    ISE_GROUP_ATTR_S pGrpAttr;
    ISE_PortCap_S PortCap_S[4];
} ISE_GroupCap_S;

typedef struct SampleDfishFrameNode {
    VIDEO_FRAME_INFO_S mFrame;
    struct list_head mList;
} SampleDfishFrameNode;

typedef struct SampleDfishFrameManager {
    int PicId[2];
    struct list_head mIdleList_0; //SampleDfishFrameNode
    struct list_head mUsingList_0;
    struct list_head mIdleList_1;
    struct list_head mUsingList_1;
    pthread_mutex_t mWaitFrameLock;
    int mbWaitFrameFlag;
    cdx_sem_t mSemFrameCome;
    int mNodeCnt;
    pthread_mutex_t mLock;
    VIDEO_FRAME_INFO_S* (*PrefetchFirstIdleFrame)(void *pThiz);
    int (*UseFrame)(void *pThiz, VIDEO_FRAME_INFO_S *pFrame);
    int (*ReleaseFrame)(void *pThiz, unsigned int nFrameId);
} SampleDfishFrameManager;

typedef struct SampleDfishParameter {
    ISE_GroupCap_S pISEGroupCap[4];
    SampleDfishFrameManager  mFrameManager;
    awPic_Cap_s    PictureCap;
} SampleDfishParameter;

VIDEO_FRAME_INFO_S* SampleDfishFrameManager_PrefetchFirstIdleFrame(void *pThiz)
{
    SampleDfishFrameManager *pFrameManager = (SampleDfishFrameManager*)pThiz;
    SampleDfishFrameNode *pFirstNode_0,*pFirstNode_1;
    VIDEO_FRAME_INFO_S *pFrameInfo = NULL;
    pthread_mutex_lock(&pFrameManager->mLock);
    if(pFrameManager->PicId[0] == 1) {
        if(!list_empty(&pFrameManager->mIdleList_0)) {
            pFirstNode_0 = list_first_entry(&pFrameManager->mIdleList_0, SampleDfishFrameNode, mList);
            pFrameInfo = &pFirstNode_0->mFrame;
        } else {
            pFrameInfo = NULL;
        }
        pFrameManager->PicId[0] = 0;
    }
    if(pFrameManager->PicId[1] == 1) {
        if(!list_empty(&pFrameManager->mIdleList_1)) {
            pFirstNode_1 = list_first_entry(&pFrameManager->mIdleList_1, SampleDfishFrameNode, mList);
            pFrameInfo = &pFirstNode_1->mFrame;
        } else {
            pFrameInfo = NULL;
        }
        pFrameManager->PicId[1] = 0;
    }
    pthread_mutex_unlock(&pFrameManager->mLock);
    return pFrameInfo;
}

int SampleDfishFrameManager_UseFrame(void *pThiz, VIDEO_FRAME_INFO_S *pFrame)
{
    int ret = 0;
    SampleDfishFrameManager *pFrameManager = (SampleDfishFrameManager*)pThiz;
    if(NULL == pFrame) {
        aloge("fatal error! pNode == NULL!");
        return -1;
    }
    pthread_mutex_lock(&pFrameManager->mLock);
    if(pFrameManager->PicId[0] == 1) {
        SampleDfishFrameNode *pFirstNode = list_first_entry_or_null(&pFrameManager->mIdleList_0,
                                           SampleDfishFrameNode, mList);
        if(pFirstNode) {
            if(&pFirstNode->mFrame == pFrame) {
                list_move_tail(&pFirstNode->mList, &pFrameManager->mUsingList_0);
                pFrameManager->PicId[0] = 0;
            } else {
                aloge("fatal error! node is not match [%p]!=[%p]", pFrame, &pFirstNode->mFrame);
                ret = -1;
                pFrameManager->PicId[0] = 0;
            }
        } else {
            aloge("fatal error! idle list_0 is empty");
            ret = -1;
            pFrameManager->PicId[0] = 0;
        }
    }
    if(pFrameManager->PicId[1] == 1) {
        SampleDfishFrameNode *pFirstNode = list_first_entry_or_null(&pFrameManager->mIdleList_1,
                                           SampleDfishFrameNode, mList);
        if(pFirstNode) {
            if(&pFirstNode->mFrame == pFrame) {
                list_move_tail(&pFirstNode->mList, &pFrameManager->mUsingList_1);
                pFrameManager->PicId[1] = 0;
            } else {
                aloge("fatal error! node is not match [%p]!=[%p]", pFrame, &pFirstNode->mFrame);
                ret = -1;
                pFrameManager->PicId[1] = 0;
            }
        } else {
            aloge("fatal error! idle list_1 is empty");
            ret = -1;
            pFrameManager->PicId[1] = 0;
        }
    }
    pthread_mutex_unlock(&pFrameManager->mLock);
    return ret;
}

int SampleDfishFrameManager_ReleaseFrame(void *pThiz, unsigned int nFrameId)
{
    int ret = 0;
    SampleDfishFrameManager *pFrameManager = (SampleDfishFrameManager*)pThiz;
    pthread_mutex_lock(&pFrameManager->mLock);
    int bFindFlag = 0;
    SampleDfishFrameNode *pEntry, *pTmp;
    if(pFrameManager->PicId[0] == 1) {
        list_for_each_entry_safe(pEntry, pTmp, &pFrameManager->mUsingList_0, mList) {
            if(pEntry->mFrame.mId == nFrameId) {
                list_move_tail(&pEntry->mList, &pFrameManager->mIdleList_0);
                bFindFlag = 1;
                pFrameManager->PicId[0] = 0;
                break;
            }
        }
    }
    pEntry = NULL,pTmp = NULL;
    if(pFrameManager->PicId[1] == 1) {
        list_for_each_entry_safe(pEntry, pTmp, &pFrameManager->mUsingList_1, mList) {
            if(pEntry->mFrame.mId == nFrameId) {
                list_move_tail(&pEntry->mList, &pFrameManager->mIdleList_1);
                bFindFlag = 1;
                pFrameManager->PicId[1] = 0;
                break;
            }
        }
    }
    if(0 == bFindFlag) {
        ret = -1;
        if(pFrameManager->PicId[0] == 1) {
            aloge("fatal error! frameId[%d] is not find,picid = %d", nFrameId,pFrameManager->PicId[0]);
            pFrameManager->PicId[0] = 0;
        }
        if(pFrameManager->PicId[1] == 1) {
            aloge("fatal error! frameId[%d] is not find,picid = %d", nFrameId,pFrameManager->PicId[1]);
            pFrameManager->PicId[1] = 0;
        }
    }
    pthread_mutex_unlock(&pFrameManager->mLock);
    return ret;
}

static ERRORTYPE SampleDfishCallbackWrapper(void *cookie,MPP_CHN_S *Port, MPP_EVENT_TYPE event, void *pEventData)
{
    ERRORTYPE ret = SUCCESS;
    SampleDfishFrameManager *pContext = (SampleDfishFrameManager*)cookie;
    if(MOD_ID_ISE == Port->mModId) {
        switch(event) {
        case MPP_EVENT_RELEASE_ISE_VIDEO_BUFFER0: {
            VIDEO_FRAME_INFO_S *pVideoFrameInfo = (VIDEO_FRAME_INFO_S*)pEventData;
            pContext->PicId[0] = 1;
            pContext->ReleaseFrame(pContext, pVideoFrameInfo->mId);
            pthread_mutex_lock(&pContext->mWaitFrameLock);
            if(pContext->mbWaitFrameFlag) {
                pContext->mbWaitFrameFlag = 0;
                cdx_sem_up(&pContext->mSemFrameCome);
            }
            pthread_mutex_unlock(&pContext->mWaitFrameLock);
            break;
        }
        case MPP_EVENT_RELEASE_ISE_VIDEO_BUFFER1: {
            VIDEO_FRAME_INFO_S *pVideoFrameInfo = (VIDEO_FRAME_INFO_S*)pEventData;
            pContext->PicId[1] = 1;
            pContext->ReleaseFrame(pContext, pVideoFrameInfo->mId);
            pthread_mutex_lock(&pContext->mWaitFrameLock);
            if(pContext->mbWaitFrameFlag) {
                pContext->mbWaitFrameFlag = 0;
                cdx_sem_up(&pContext->mSemFrameCome);
            }
            pthread_mutex_unlock(&pContext->mWaitFrameLock);
            break;
        }
        default: {
            printf("fatal error! unknown event[0x%x] from channel[0x%x][0x%x][0x%x]!",
                   event, Port->mModId, Port->mDevId, Port->mChnId);
            ret = FALSE;
            break;
        }
        }
    }
    return ret;
}

int initSampleDfishFrameManager(SampleDfishParameter *pDfishParameter, int nFrameNum)
{
    int ret = 0;
    ret = pthread_mutex_init(&pDfishParameter->mFrameManager.mLock, NULL);
    if(ret!=0) {
        aloge("fatal error! pthread mutex init fail!");
        return ret;
    }
    ret = pthread_mutex_init(&pDfishParameter->mFrameManager.mWaitFrameLock, NULL);
    if(ret != 0) {
        aloge("fatal error! pthread mutex init fail!");
        return ret;
    }
    ret = cdx_sem_init(&pDfishParameter->mFrameManager.mSemFrameCome, 0);
    if(ret != 0) {
        aloge("cdx sem init fail!");
        return ret;
    }
    Thread_EXIT = FALSE;
    INIT_LIST_HEAD(&pDfishParameter->mFrameManager.mIdleList_0);
    INIT_LIST_HEAD(&pDfishParameter->mFrameManager.mUsingList_0);
    INIT_LIST_HEAD(&pDfishParameter->mFrameManager.mIdleList_1);
    INIT_LIST_HEAD(&pDfishParameter->mFrameManager.mUsingList_1);

    FILE *fd[2];
    fd[0] = pDfishParameter->PictureCap.Pic0_FilePath;
    fd[1] = pDfishParameter->PictureCap.Pic1_FilePath;

    int width = 0, height = 0;
    width = pDfishParameter->PictureCap.PicStride;
    height = pDfishParameter->PictureCap.PicHeight;
    int i = 0,j = 0;
    SampleDfishFrameNode *pNode;
    unsigned int uPhyAddr;
    void *pVirtAddr;
    unsigned int block_size = 0;
    unsigned int read_size = 0;
    ret = ion_memOpen();
    if (ret != 0) {
        aloge("Open ion failed!");
        return ret;
    }
    for(j=0; j<2; j++) {
        for(i=0; i<nFrameNum; i++) {
            pNode = (SampleDfishFrameNode*)malloc(sizeof(SampleDfishFrameNode));
            memset(pNode, 0, sizeof(SampleDfishFrameNode));
            pNode->mFrame.mId = i;
            pNode->mFrame.VFrame.mpVirAddr[0] = ion_allocMem(width * height *1.5);
            if (pNode->mFrame.VFrame.mpVirAddr[0] == NULL) {
                aloge("allocMem error!");
                return -1;
            }
            memset(pNode->mFrame.VFrame.mpVirAddr[0], 0x0, width * height * 1.5);
            pNode->mFrame.VFrame.mPhyAddr[0] = (unsigned int)ion_getMemPhyAddr(pNode->mFrame.VFrame.mpVirAddr[0]);
            pNode->mFrame.VFrame.mpVirAddr[1] = pNode->mFrame.VFrame.mpVirAddr[0] + width * height;
            pNode->mFrame.VFrame.mPhyAddr[1] = pNode->mFrame.VFrame.mPhyAddr[0] + width * height;
            block_size = width * height * sizeof(unsigned char);
            read_size = fread(pNode->mFrame.VFrame.mpVirAddr[0], 1 ,block_size, fd[j]);
            if (read_size < 0) {
                aloge("read yuv file fail\n");
                fclose(fd[j]);
                fd[j] = NULL;
                return -1;
            }
            block_size =  width * height * sizeof(unsigned char) / 2;
            read_size = fread(pNode->mFrame.VFrame.mpVirAddr[1], 1, block_size, fd[j]);
            if (read_size < 0) {
                aloge("read yuv file fail\n");
                fclose(fd[j]);
                fd[j] = NULL;
                return -1;
            }
            ion_flushCache(pNode->mFrame.VFrame.mpVirAddr[0],width * height *1.5);
            pNode->mFrame.VFrame.mWidth = width;
            pNode->mFrame.VFrame.mHeight = height;
            if(j == 0) {
                list_add_tail(&pNode->mList, &pDfishParameter->mFrameManager.mIdleList_0);
                pDfishParameter->mFrameManager.PicId[j] = 0;
            } else {
                list_add_tail(&pNode->mList, &pDfishParameter->mFrameManager.mIdleList_1);
                pDfishParameter->mFrameManager.PicId[j] = 0;
            }
        }
        fclose(fd[j]);
    }
    pDfishParameter->mFrameManager.mNodeCnt = nFrameNum;
    pDfishParameter->mFrameManager.PrefetchFirstIdleFrame = SampleDfishFrameManager_PrefetchFirstIdleFrame;
    pDfishParameter->mFrameManager.UseFrame = SampleDfishFrameManager_UseFrame;
    pDfishParameter->mFrameManager.ReleaseFrame = SampleDfishFrameManager_ReleaseFrame;
    return 0;
}

int destroySampleDfishFrameManager(SampleDfishParameter *pDfishParameter)
{
    SampleDfishFrameManager *pDfishList = NULL;
    SampleDfishFrameNode *pEntry = NULL, *pTmp = NULL;
    pDfishList = &pDfishParameter->mFrameManager;
    if(!list_empty(&pDfishList->mUsingList_0)) {
        aloge("fatal error! why using list is not empty");
        list_for_each_entry_safe(pEntry, pTmp, &pDfishList->mUsingList_0, mList) {
            list_move_tail(&pEntry->mList, &pDfishList->mIdleList_0);
        }
    }
    if(!list_empty(&pDfishList->mUsingList_1)) {
        aloge("fatal error! why using list is not empty");
        list_for_each_entry_safe(pEntry, pTmp, &pDfishList->mUsingList_1, mList) {
            list_move_tail(&pEntry->mList, &pDfishList->mIdleList_1);
        }
    }
    int cnt = 0;
    struct list_head *pList;
    list_for_each(pList, &pDfishList->mIdleList_0) {
        cnt++;
    }
    if(cnt != pDfishList->mNodeCnt) {
        aloge("fatal error! frame count is not match [%d]!=[%d]", cnt,pDfishList->mNodeCnt);
    }
    cnt = 0;
    list_for_each(pList, &pDfishList->mIdleList_1) {
        cnt++;
    }
    if(cnt != pDfishList->mNodeCnt) {
        aloge("fatal error! frame count is not match [%d]!=[%d]", cnt,pDfishList->mNodeCnt);
    }
    cnt = 0;
    pEntry = NULL, pTmp = NULL;
    list_for_each_entry_safe(pEntry, pTmp, &pDfishList->mIdleList_0, mList) {
        cnt++;
        ion_freeMem(pEntry->mFrame.VFrame.mpVirAddr[0]);
        list_del(&pEntry->mList);
        free(pEntry);
    }
    cnt = 0;
    pEntry = NULL, pTmp = NULL;
    list_for_each_entry_safe(pEntry, pTmp, &pDfishList->mIdleList_1, mList) {
        cnt++;
        ion_freeMem(pEntry->mFrame.VFrame.mpVirAddr[0]);
        list_del(&pEntry->mList);
        free(pEntry);
    }
    pthread_mutex_destroy(&pDfishList->mLock);
    return 0;
}

/*MPI ise*/
int aw_iseport_creat(ISE_GRP IseGrp, ISE_CHN IsePort, ISE_CHN_ATTR_S *PortAttr)
{
    int ret = -1;
    ret = AW_MPI_ISE_CreatePort(IseGrp, IsePort, PortAttr);
    if(ret < 0) {
        aloge("Create ISE Port failed,IseGrp= %d,IsePort = %d",IseGrp,IsePort);
        return ret ;
    }
    ret = AW_MPI_ISE_SetPortAttr(IseGrp,IsePort,PortAttr);
    if(ret < 0) {
        aloge("Set ISE Port Attr failed,IseGrp= %d,IsePort = %d",IseGrp,IsePort);
        return ret ;
    }
    return 0;
}

int aw_iseport_destory(ISE_GRP IseGrp, ISE_CHN IsePort)
{
    int ret = -1;
    ret = AW_MPI_ISE_DestroyPort(IseGrp,IsePort);
    if(ret < 0) {
        aloge("Destory ISE Port failed, IseGrp= %d,IsePort = %d",IseGrp,IsePort);
        return ret ;
    }
    return 0;
}

int aw_isegroup_creat(ISE_GRP IseGrp, ISE_GROUP_ATTR_S *pGrpAttr)
{
    int ret = -1;
    ret = AW_MPI_ISE_CreateGroup(IseGrp, pGrpAttr);
    if(ret < 0) {
        aloge("Create ISE Group failed, IseGrp= %d",IseGrp);
        return ret ;
    }
    ret = AW_MPI_ISE_SetGrpAttr(IseGrp, pGrpAttr);
    if(ret < 0) {
        aloge("Set ISE GrpAttr failed, IseGrp= %d",IseGrp);
        return ret ;
    }
    return 0;
}

int aw_isegroup_destory(ISE_GRP IseGrp)
{
    int ret = -1;
    ret = AW_MPI_ISE_DestroyGroup(IseGrp);
    if(ret < 0) {
        aloge("Destroy ISE Group failed, IseGrp= %d",IseGrp);
        return ret ;
    }
    return 0;
}

static int ParseCmdLine(int argc, char **argv, SampleDfishCmdLineParam *pCmdLinePara)
{
    alogd("sample_dfish path:[%s], arg number is [%d]", argv[0], argc);
    int ret = 0;
    int i=1;
    memset(pCmdLinePara, 0, sizeof(SampleDfishCmdLineParam));
    while(i < argc) {
        if(!strcmp(argv[i], "-path")) {
            if(++i >= argc) {
                aloge("fatal error! use -h to learn how to set parameter!!!");
                ret = -1;
                break;
            }
            if(strlen(argv[i]) >= MAX_FILE_PATH_SIZE) {
                aloge("fatal error! file path[%s] too long: [%d]>=[%d]!",
                      argv[i], strlen(argv[i]), MAX_FILE_PATH_SIZE);
            }
            strncpy(pCmdLinePara->mConfigFilePath, argv[i], MAX_FILE_PATH_SIZE-1);
            pCmdLinePara->mConfigFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
        } else if(!strcmp(argv[i], "-h")) {
            alogd("CmdLine param:\n"
                  "\t-path /home/sample_dfish.conf\n");
            ret = 1;
            break;
        } else {
            alogd("ignore invalid CmdLine param:[%s], type -h to get how to set parameter!", argv[i]);
        }
        i++;
    }
    return ret;
}

static ERRORTYPE loadSampleDfishConfig(SampleDfishConfig *pConfig, const char *conf_path)
{
    int ret;
    char *ptr = NULL;
    char *pStrPixelFormat = NULL,*EncoderType = NULL;
    int i = 0,ISEPortNum = 0;
    CONFPARSER_S stConfParser;
    char name[256];
    ret = createConfParser(conf_path, &stConfParser);
    if(ret < 0) {
        aloge("load conf fail");
        return FAILURE;
    }
    memset(pConfig, 0, sizeof(SampleDfishConfig));
    pConfig->AutoTestCount = GetConfParaInt(&stConfParser, SAMPLE_DFISH_Auto_Test_Count, 0);
    pConfig->Process_Count = GetConfParaInt(&stConfParser, SAMPLE_DFISH_Process_Count, 0);

    /* Get Picture parameter*/
    pConfig->PicConfig.PicFrameRate = GetConfParaInt(&stConfParser, SAMPLE_Dfish_Pic_Frame_Rate, 0);
    pConfig->PicConfig.PicWidth = GetConfParaInt(&stConfParser, SAMPLE_Dfish_Pic_Width, 0);
    pConfig->PicConfig.PicHeight = GetConfParaInt(&stConfParser, SAMPLE_Dfish_Pic_Height, 0);
    pConfig->PicConfig.PicStride = GetConfParaInt(&stConfParser, SAMPLE_Dfish_Pic_Stride, 0);
    alogd("pic_width = %d, pic_height = %d, pic_frame_rate = %d\n",
          pConfig->PicConfig.PicWidth,pConfig->PicConfig.PicHeight,pConfig->PicConfig.PicFrameRate);
    ptr = (char*)GetConfParaString(&stConfParser, SAMPLE_Dfish_Pic0_File_Path, NULL);
    strncpy(pConfig->PicConfig.Pic0_FilePath, ptr, MAX_FILE_PATH_SIZE-1);
    pConfig->PicConfig.Pic0_FilePath[MAX_FILE_PATH_SIZE-1] = '\0';
    ptr = (char*)GetConfParaString(&stConfParser, SAMPLE_Dfish_Pic1_File_Path, NULL);
    strncpy(pConfig->PicConfig.Pic1_FilePath, ptr, MAX_FILE_PATH_SIZE-1);
    pConfig->PicConfig.Pic1_FilePath[MAX_FILE_PATH_SIZE-1] = '\0';

    /* Get ISE parameter*/
    pConfig->ISEGroupConfig.ISEPortNum = GetConfParaInt(&stConfParser, SAMPLE_Dfish_ISE_Port_Num, 0);
    ISEPortNum = pConfig->ISEGroupConfig.ISEPortNum;
    ptr = (char*)GetConfParaString(&stConfParser, SAMPLE_Dfish_ISE_Output_File_Path, NULL);
    strncpy(pConfig->ISEGroupConfig.OutputFilePath, ptr, MAX_FILE_PATH_SIZE-1);
    pConfig->ISEGroupConfig.OutputFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
    alogd("ise output_file_path = %s\n",pConfig->ISEGroupConfig.OutputFilePath);
    float calib_matr[3][3] = {{1,0,0},
        {0, 1, 0},
        {0, 0, 1}
    };
#if 0
    float calib_matr[3][3] = { {0.999493,  0.0303746,  -0.00959498},
        { -0.0303818,  0.999538,  -0.000608169},
        {0.00957208,  0.000899373,  0.999954}
    };
#endif
    //picture size 1280*1280
    if(pConfig->PicConfig.PicWidth == 1280) {
        pConfig->ISEGroupConfig.Lens_Parameter_P0 = 647.485*2/3.14*0.922;
        pConfig->ISEGroupConfig.Lens_Parameter_Cx0 = 643.1512;
        pConfig->ISEGroupConfig.Lens_Parameter_Cy0 = 639.1745;
        pConfig->ISEGroupConfig.Lens_Parameter_P1 = 652.948*2/3.14*0.922;
        pConfig->ISEGroupConfig.Lens_Parameter_Cx1 = 644.6785;
        pConfig->ISEGroupConfig.Lens_Parameter_Cy1 = 635.2576;
    } else {
        pConfig->ISEGroupConfig.Lens_Parameter_P0 = pConfig->PicConfig.PicWidth/3.1415;
        pConfig->ISEGroupConfig.Lens_Parameter_Cx0 = pConfig->PicConfig.PicWidth/2;
        pConfig->ISEGroupConfig.Lens_Parameter_Cy0 = pConfig->PicConfig.PicHeight/2;
        pConfig->ISEGroupConfig.Lens_Parameter_P1 = pConfig->PicConfig.PicWidth/3.1415;
        pConfig->ISEGroupConfig.Lens_Parameter_Cx1 = pConfig->PicConfig.PicWidth/2;
        pConfig->ISEGroupConfig.Lens_Parameter_Cy1 = pConfig->PicConfig.PicHeight/2;
#if 0
        pConfig->ISEGroupConfig.Lens_Parameter_P0 = TEST_P0 *2 /3.1415;
        pConfig->ISEGroupConfig.Lens_Parameter_Cx0 = TEST_CX0;
        pConfig->ISEGroupConfig.Lens_Parameter_Cy0 = TEST_CY0;
        pConfig->ISEGroupConfig.Lens_Parameter_P1 = TEST_P1*2 /3.1415;
        pConfig->ISEGroupConfig.Lens_Parameter_Cx1 = TEST_CX1;
        pConfig->ISEGroupConfig.Lens_Parameter_Cy1 = TEST_CY1;
#endif
    }
    for(int ii = 0; ii < 3; ii++) {
        for(int jj = 0; jj < 3; jj++) {
            pConfig->ISEGroupConfig.calib_matr[ii][jj] = calib_matr[ii][jj];
        }
    }

    /*ISE Port parameter*/
    for(i = 0; i < ISEPortNum; i++) {
        snprintf(name, 256, "ise_port%d_width", i);
        pConfig->ISEPortConfig[i].ISEWidth = GetConfParaInt(&stConfParser, name, 0);
        snprintf(name, 256, "ise_port%d_height", i);
        pConfig->ISEPortConfig[i].ISEHeight = GetConfParaInt(&stConfParser, name, 0);
        snprintf(name, 256, "ise_port%d_stride", i);
        pConfig->ISEPortConfig[i].ISEStride = GetConfParaInt(&stConfParser, name, 0);
        snprintf(name, 256, "ise_port%d_flip_enable", i);
        pConfig->ISEPortConfig[i].flip_enable = GetConfParaInt(&stConfParser, name, 0);
        snprintf(name, 256, "ise_port%d_mirror_enable", i);
        pConfig->ISEPortConfig[i].mirror_enable = GetConfParaInt(&stConfParser, name, 0);
        alogd("ISE Port%d Parameter:ISE_Width = %d,ISE_Height = %d,ISE_Stride = %d,"
              "ISE_flip_enable = %d,mirror_enable = %d\n",i,pConfig->ISEPortConfig[i].ISEWidth,
              pConfig->ISEPortConfig[i].ISEHeight,pConfig->ISEPortConfig[i].ISEStride,
              pConfig->ISEPortConfig[i].flip_enable,pConfig->ISEPortConfig[i].mirror_enable);
    }
    alogd("Lens Parameter:P0 = %f,P1= %f,Cx0  = %f,Cx1 = %f,Cy0 = %f,Cy1 = %f",
          pConfig->ISEGroupConfig.Lens_Parameter_P0,pConfig->ISEGroupConfig.Lens_Parameter_P1,
          pConfig->ISEGroupConfig.Lens_Parameter_Cx0,pConfig->ISEGroupConfig.Lens_Parameter_Cx1,
          pConfig->ISEGroupConfig.Lens_Parameter_Cy0,pConfig->ISEGroupConfig.Lens_Parameter_Cy1);
    destroyConfParser(&stConfParser);
    return SUCCESS;
}

static void *Loop_SendImageFileThread(void *pArg)
{
    alogd("Start Loop_SendImageFileThread");
    SampleDfishParameter *pCap = (SampleDfishParameter *)pArg;
    ISE_GRP ISEGroup = pCap->pISEGroupCap[0].ISE_Group;
    int ret = 0;
    int i = 0;
    int s32MilliSec = -1;
    int framerate = pCap->PictureCap.PicFrameRate;
    VIDEO_FRAME_INFO_S *stUserFrame0 = NULL;
    VIDEO_FRAME_INFO_S *stUserFrame1 = NULL;
    ret = AW_MPI_ISE_Start(ISEGroup);
    if(ret < 0) {
        aloge("ise start error!\n");
        return (void*)ret;
    }
    int sleep_time = 0;
    while(1) {
        if(Thread_EXIT == TRUE) {
            aloge("Thread_EXIT is True!\n");
            break;
        }
        //request idle frame
        pCap->mFrameManager.PicId[0] = 1;
        stUserFrame0 = pCap->mFrameManager.PrefetchFirstIdleFrame(&pCap->mFrameManager);
        if(NULL == stUserFrame0) {
            pthread_mutex_lock(&pCap->mFrameManager.mWaitFrameLock);
            pCap->mFrameManager.PicId[0] = 1;
            stUserFrame0 = pCap->mFrameManager.PrefetchFirstIdleFrame(&pCap->mFrameManager);
            if(stUserFrame0 != NULL) {
                pthread_mutex_unlock(&pCap->mFrameManager.mWaitFrameLock);
            } else {
                pCap->mFrameManager.mbWaitFrameFlag = 1;
                pthread_mutex_unlock(&pCap->mFrameManager.mWaitFrameLock);
                cdx_sem_down_timedwait(&pCap->mFrameManager.mSemFrameCome, 500);
                continue;
            }
        }
        pCap->mFrameManager.PicId[0] = 1;
        pCap->mFrameManager.UseFrame(&pCap->mFrameManager, stUserFrame0);

        pCap->mFrameManager.PicId[1] = 1;
        stUserFrame1 = pCap->mFrameManager.PrefetchFirstIdleFrame(&pCap->mFrameManager);
        if(NULL == stUserFrame1) {
            pthread_mutex_lock(&pCap->mFrameManager.mWaitFrameLock);
            pCap->mFrameManager.PicId[1] = 1;
            stUserFrame1 = pCap->mFrameManager.PrefetchFirstIdleFrame(&pCap->mFrameManager);
            if(stUserFrame1 != NULL) {
                pthread_mutex_unlock(&pCap->mFrameManager.mWaitFrameLock);
            } else {
                pCap->mFrameManager.mbWaitFrameFlag = 1;
                pthread_mutex_unlock(&pCap->mFrameManager.mWaitFrameLock);
                cdx_sem_down_timedwait(&pCap->mFrameManager.mSemFrameCome, 500);
                continue;
            }
        }
        pCap->mFrameManager.PicId[1] = 1;
        pCap->mFrameManager.UseFrame(&pCap->mFrameManager, stUserFrame1);

        ret = AW_MPI_ISE_SendPic(ISEGroup, stUserFrame0, stUserFrame1, s32MilliSec);
        if(ret != SUCCESS) {
            alogd("impossible, send frameId[%d] fail?", stUserFrame0->mId);
            pCap->mFrameManager.PicId[0] = 1;
            pCap->mFrameManager.ReleaseFrame(&pCap->mFrameManager, stUserFrame0->mId);
            pCap->mFrameManager.PicId[1] = 1;
            pCap->mFrameManager.ReleaseFrame(&pCap->mFrameManager, stUserFrame0->mId);
            continue;
        }
        i++;
#if TakePicture_By_Hardware
        sleep_time = 40000;
#elif TakePicture_By_Soft
        sleep_time = 142857;
#else
        sleep_time = 40000;
#endif
        usleep(sleep_time);
    }
    return NULL;
}

static void *Loop_GetIseData(void *pArg)
{
    alogd("Loop Start %s.\r\n", __func__);
    int i = 0,ret = 0,j =0;
    int width = 0,height = 0;
    ISE_CHN_ATTR_S ISEPortAttr;
    ISE_PortCap_S *pCap = (ISE_PortCap_S *)pArg;
    ISE_GRP ISEGroup = pCap->ISE_Group;
    ISE_CHN ISEPort = pCap->ISE_Port;
    pthread_t thread_id = pCap->thread_id;
    int s32MilliSec = pCap->s32MilliSec;
    VIDEO_FRAME_INFO_S ISE_Frame_buffer;
    // YUV Out
    width = pCap->width;
    height = pCap->height;
    char *name = pCap->OutputFilePath;
    alogd("Count = %d",pCap->Process_Count);
    while((i != pCap->Process_Count) || (pCap->Process_Count == -1)) {
        if ((ret = AW_MPI_ISE_GetData(ISEGroup, ISEPort, &ISE_Frame_buffer, -1)) < 0) {
            aloge("ISE Port%d get data failed!\n",ISEPort);
            continue ;
        }
        j++;
        if (j % 30 == 0) {
            time_t now;
            struct tm *timenow;
            time(&now);
            timenow = localtime(&now);
            alogd("Cap threadid = 0x%lx,port = %d; local time is %s\r\n",
                  thread_id, ISEPort, asctime(timenow));
        }
#if Save_Picture
        if(i % 100 == 0) {
            char filename[125];
            sprintf(filename,"/%s/dfish_ch%d_%d.yuv",name,ISEPort,i);
            FILE *fd = NULL;
            fd = fopen(filename,"wb+");
            fwrite(ISE_Frame_buffer.VFrame.mpVirAddr[0], width * height, 1, fd);
            fwrite(ISE_Frame_buffer.VFrame.mpVirAddr[1], ((width * height)>>1), 1, fd);
            fclose(fd);
        }
#endif
        AW_MPI_ISE_ReleaseData(ISEGroup, ISEPort, &ISE_Frame_buffer);
        i++;
    }
    Thread_EXIT = TRUE;
    return NULL;
}

void Sample_Dfish_HELP()
{
    alogd("Run sample_dfish command: ./sample_dfish -path ./sample_dfish.conf\r\n");
}

int main(int argc, char *argv[])
{
    int ret = 0, i = 0, j = 0, count = 0;
    int result = 0;
    int AutoTestCount = 0;
    alogd("Sample dfish buile time = %s, %s.\r\n", __DATE__, __TIME__);
    if(argc != 3) {
        Sample_Dfish_HELP();
        exit(0);
    }

    SampleDfishParameter DfisheyePara;
    SampleDfishConfparser stContext;

    //parse command line param,read sample_virvi2fish2vo.conf
    if(ParseCmdLine(argc, argv, &stContext.mCmdLinePara) != 0) {
        aloge("fatal error! command line param is wrong, exit!");
        result = -1;
        goto _exit;
    }
    char *pConfigFilePath;
    if(strlen(stContext.mCmdLinePara.mConfigFilePath) > 0) {
        pConfigFilePath = stContext.mCmdLinePara.mConfigFilePath;
    } else {
        pConfigFilePath = DEFAULT_SAMPLE_DFISH_CONF_PATH;
    }
    //parse config file.
    if(loadSampleDfishConfig(&stContext.mConfigPara, pConfigFilePath) != SUCCESS) {
        aloge("fatal error! no config file or parse conf file fail");
        result = -1;
        goto _exit;
    }
    AutoTestCount = stContext.mConfigPara.AutoTestCount;

    while (count != AutoTestCount) {
        /* start mpp systerm */
        MPP_SYS_CONF_S mSysConf;
        memset(&mSysConf,0,sizeof(MPP_SYS_CONF_S));
        mSysConf.nAlignWidth = 32;
        AW_MPI_SYS_SetConf(&mSysConf);
        ret = AW_MPI_SYS_Init();
        if (ret < 0) {
            aloge("sys init failed");
            result = -1;
            goto sys_exit;
        }

        //init frame manager
        DfisheyePara.PictureCap.PicWidth = stContext.mConfigPara.PicConfig.PicWidth;
        DfisheyePara.PictureCap.PicHeight = stContext.mConfigPara.PicConfig.PicHeight;
        DfisheyePara.PictureCap.PicStride = stContext.mConfigPara.PicConfig.PicStride;
        DfisheyePara.PictureCap.PicFrameRate = ((float)1/stContext.mConfigPara.PicConfig.PicFrameRate) * 1000000;
        DfisheyePara.PictureCap.Pic0_FilePath = fopen(stContext.mConfigPara.PicConfig.Pic0_FilePath,"rb");
        DfisheyePara.PictureCap.Pic1_FilePath = fopen(stContext.mConfigPara.PicConfig.Pic1_FilePath,"rb");
#if  TakePicture_By_Soft || TakePicture_By_Hardware
        int nFrameNum = 2;
#else
        int nFrameNum = 5;
#endif
        ret = initSampleDfishFrameManager(&DfisheyePara, nFrameNum);
        if(ret < 0) {
            aloge("Init FrameManager failed!");
            goto destory_framemanager;
        }

        int ISEPortNum = 0;
        ISEPortNum = stContext.mConfigPara.ISEGroupConfig.ISEPortNum;
        ISE_PortCap_S *pISEPortCap;
        for(j = 0; j < 1; j++) {
            /*Set ISE Group Attribute*/
            memset(&DfisheyePara.pISEGroupCap[j], 0, sizeof(ISE_GroupCap_S));
            DfisheyePara.pISEGroupCap[j].ISE_Group = j;
            DfisheyePara.pISEGroupCap[j].pGrpAttr.iseMode = ISEMODE_TWO_FISHEYE;

            ret = aw_isegroup_creat(DfisheyePara.pISEGroupCap[j].ISE_Group, &DfisheyePara.pISEGroupCap[j].pGrpAttr);
            if(ret < 0) {
                aloge("ISE Group %d creat failed",DfisheyePara.pISEGroupCap[j].ISE_Group);
                goto destory_isegroup;
            }
            for(i = 0; i< ISEPortNum; i++) {
                memset(&DfisheyePara.pISEGroupCap[j].PortCap_S[i], 0, sizeof(ISE_PortCap_S));
                DfisheyePara.pISEGroupCap[j].PortCap_S[i].ISE_Group = j;
                DfisheyePara.pISEGroupCap[j].PortCap_S[i].ISE_Port = i;
                DfisheyePara.pISEGroupCap[j].PortCap_S[i].thread_id = i;
                DfisheyePara.pISEGroupCap[j].PortCap_S[i].s32MilliSec = 4000;
                DfisheyePara.pISEGroupCap[j].PortCap_S[i].OutputFilePath =  stContext.mConfigPara.ISEGroupConfig.OutputFilePath;
                pISEPortCap = &DfisheyePara.pISEGroupCap[j].PortCap_S[i];
                pISEPortCap->Process_Count = stContext.mConfigPara.Process_Count;
                /*Set ISE Port Attribute*/
                if(i == 0) { //dfish arttr
#if TakePicture_By_Soft || TakePicture_By_Hardware
                    pISEPortCap->PortAttr.buffer_num = 2;
#else
                    pISEPortCap->PortAttr.buffer_num = 5;
#endif

#if TakePicture_By_Soft
                    pISEPortCap->PortAttr.mode_attr.mDFish.handle_mode = HANDLE_BY_SOFT;
#else
                    pISEPortCap->PortAttr.mode_attr.mDFish.handle_mode = HANDLE_BY_HARDWARE;
#endif
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.in_h = stContext.mConfigPara.PicConfig.PicHeight;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.in_w = stContext.mConfigPara.PicConfig.PicWidth;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.p0  = stContext.mConfigPara.ISEGroupConfig.Lens_Parameter_P0;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.cx0 = stContext.mConfigPara.ISEGroupConfig.Lens_Parameter_Cx0;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.cy0 = stContext.mConfigPara.ISEGroupConfig.Lens_Parameter_Cy0;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.p1  = stContext.mConfigPara.ISEGroupConfig.Lens_Parameter_P1;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.cx1 = stContext.mConfigPara.ISEGroupConfig.Lens_Parameter_Cx1;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.cy1 = stContext.mConfigPara.ISEGroupConfig.Lens_Parameter_Cy1;
                    for(int ii = 0; ii < 3; ii++) {
                        for(int jj = 0; jj < 3; jj++) {
                            pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.calib_matr[ii][jj] = stContext.mConfigPara.ISEGroupConfig.calib_matr[ii][jj];
                        }
                    }
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.in_yuv_type = 0; // YUV420
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.out_yuv_type = 0; // YUV420
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.in_luma_pitch = stContext.mConfigPara.PicConfig.PicStride;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.in_chroma_pitch = stContext.mConfigPara.PicConfig.PicStride;
#if TakePicture_By_Soft
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.feather = FEATHER;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.trans_width = TRANS_WIDTH;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.findpath_width = FINDPATH_WIDTH;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.pyr_level = PYR_LEVEL;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.findpath_enable = FINDPATH_ENABLE;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.change_focus = CHANGE_FOCUS;
                    pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.inv_scale = INV_SCALE;
#endif
                }
                pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.out_en[i] = 1;
                pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.out_w[i] =  stContext.mConfigPara.ISEPortConfig[i].ISEWidth;
                pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.out_h[i] =  stContext.mConfigPara.ISEPortConfig[i].ISEHeight;
                pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.out_flip[i] =  stContext.mConfigPara.ISEPortConfig[i].flip_enable;
                pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.out_mirror[i] =  stContext.mConfigPara.ISEPortConfig[i].mirror_enable;
                pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.out_luma_pitch[i] =  stContext.mConfigPara.ISEPortConfig[i].ISEStride;
                pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.out_chroma_pitch[i] =  stContext.mConfigPara.ISEPortConfig[i].ISEStride;
                ret = aw_iseport_creat(DfisheyePara.pISEGroupCap[j].ISE_Group, DfisheyePara.pISEGroupCap[j].PortCap_S[i].ISE_Port,
                                       &pISEPortCap->PortAttr);
                if(ret < 0) {
                    aloge("ISE Port%d creat failed",DfisheyePara.pISEGroupCap[j].PortCap_S[i].ISE_Port);
                    goto destory_iseport;
                }
                DfisheyePara.pISEGroupCap[j].PortCap_S[i].width = stContext.mConfigPara.ISEPortConfig[i].ISEStride;
                DfisheyePara.pISEGroupCap[j].PortCap_S[i].height = pISEPortCap->PortAttr.mode_attr.mDFish.ise_cfg.out_h[i];
            }
        }

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)&DfisheyePara.mFrameManager;
        cbInfo.callback = (MPPCallbackFuncType)&SampleDfishCallbackWrapper;
        ret = AW_MPI_ISE_RegisterCallback(DfisheyePara.pISEGroupCap[0].ISE_Group, &cbInfo);
        if(ret != SUCCESS) {
            aloge("Dfish Register Callback error!");
            result = -1;
            goto _exit;
        }

        ret = pthread_create(&DfisheyePara.PictureCap.thread_id, NULL, Loop_SendImageFileThread, (void *)&DfisheyePara);
        if (ret < 0) {
            aloge("Loop_SendImageFileThread create failed");
            result = -1;
            goto _exit;
        }
        for(i = 0; i< ISEPortNum; i++) {
            pthread_create(&DfisheyePara.pISEGroupCap[0].PortCap_S[i].thread_id, NULL,
                           Loop_GetIseData, (void *)&DfisheyePara.pISEGroupCap[0].PortCap_S[i]);
        }

        for (i = 0; i < 1; i++) {
            pthread_join(DfisheyePara.PictureCap.thread_id, NULL);
        }

        for(i = 0; i < ISEPortNum; i++) {
            pthread_join(DfisheyePara.pISEGroupCap[0].PortCap_S[i].thread_id, NULL);
        }

        ret = AW_MPI_ISE_Stop(DfisheyePara.pISEGroupCap[0].ISE_Group);
        if(ret < 0) {
            aloge("ise stop error!\n");
            result = -1;
            goto _exit;
        }

destory_iseport:
        for(i = 0; i < ISEPortNum; i++) {
            ret = aw_iseport_destory(DfisheyePara.pISEGroupCap[0].ISE_Group,
                                     DfisheyePara.pISEGroupCap[0].PortCap_S[i].ISE_Port);
            if(ret < 0) {
                aloge("ISE Port%d distory error!",DfisheyePara.pISEGroupCap[0].PortCap_S[i].ISE_Port);
                result = -1;
                goto _exit;
            }
        }

destory_isegroup:
        ret = aw_isegroup_destory(DfisheyePara.pISEGroupCap[0].ISE_Group);
        if(ret < 0) {
            aloge("ISE Destroy Group%d error!",DfisheyePara.pISEGroupCap[0].ISE_Group);
            result = -1;
            goto _exit;
        }
destory_framemanager:
        destroySampleDfishFrameManager(&DfisheyePara);
        ret = ion_memClose();
        if (ret != 0) {
            aloge("Close ion failed!");
        }

sys_exit:
        /* exit mpp systerm */
        ret = AW_MPI_SYS_Exit();
        if (ret < 0) {
            aloge("sys exit failed!");
            result = -1;
            goto _exit;
        }
        alogd("======================================.\r\n");
        alogd("Auto Test count end: %d. (MaxCount==1000).\r\n", count);
        alogd("======================================.\r\n");
        count ++;
    }
    alogd("sample_dfish exit!\n");
    return 0;

_exit:
    return 0;
}

