/******************************************************************************
  Copyright (C), 2001-2017, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : sample_ise.c
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2017/1/5
  Last Modified :
  Description   : mpp component implement
  Function List :
  History       :
******************************************************************************/

#define LOG_TAG "sample_ise"
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
#include "sample_ise.h"
#include "sample_ise_config.h"

#define Save_Picture           1
#define Load_Len_Parameter     0
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
} ISE_PortCap_S;

typedef struct awISE_pGroupCap_S {
    ISE_GRP ISE_Group;
    ISE_GROUP_ATTR_S pGrpAttr;
    ISE_PortCap_S PortCap_S[4];
} ISE_GroupCap_S;

typedef struct SampleISEFrameNode {
    VIDEO_FRAME_INFO_S mFrame;
    struct list_head mList;
} SampleISEFrameNode;

typedef struct SampleISEFrameManager {
    int PicId[2];
    struct list_head mIdleList_0; //SampleISEFrameNode
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
} SampleISEFrameManager;

typedef struct SampleISEParameter {
    int Process_Count;
    ISE_GroupCap_S pISEGroupCap[4];
    SampleISEFrameManager  mFrameManager;
    awPic_Cap_s    PictureCap;
} SampleISEParameter;

VIDEO_FRAME_INFO_S* SampleISEFrameManager_PrefetchFirstIdleFrame(void *pThiz)
{
    SampleISEFrameManager *pFrameManager = (SampleISEFrameManager*)pThiz;
    SampleISEFrameNode *pFirstNode_0,*pFirstNode_1;
    VIDEO_FRAME_INFO_S *pFrameInfo = NULL;
    pthread_mutex_lock(&pFrameManager->mLock);
    if(pFrameManager->PicId[0] == 1) {
        if(!list_empty(&pFrameManager->mIdleList_0)) {
            pFirstNode_0 = list_first_entry(&pFrameManager->mIdleList_0, SampleISEFrameNode, mList);
            pFrameInfo = &pFirstNode_0->mFrame;
        } else {
            pFrameInfo = NULL;
        }
        pFrameManager->PicId[0] = 0;
    }
    if(pFrameManager->PicId[1] == 1) {
        if(!list_empty(&pFrameManager->mIdleList_1)) {
            pFirstNode_1 = list_first_entry(&pFrameManager->mIdleList_1, SampleISEFrameNode, mList);
            pFrameInfo = &pFirstNode_1->mFrame;
        } else {
            pFrameInfo = NULL;
        }
        pFrameManager->PicId[1] = 0;
    }
    pthread_mutex_unlock(&pFrameManager->mLock);
    return pFrameInfo;
}

int SampleISEFrameManager_UseFrame(void *pThiz, VIDEO_FRAME_INFO_S *pFrame)
{
    int ret = 0;
    SampleISEFrameManager *pFrameManager = (SampleISEFrameManager*)pThiz;
    if(NULL == pFrame) {
        aloge("fatal error! pNode == NULL!");
        return -1;
    }
    pthread_mutex_lock(&pFrameManager->mLock);
    if(pFrameManager->PicId[0] == 1) {
        SampleISEFrameNode *pFirstNode = list_first_entry_or_null(&pFrameManager->mIdleList_0,
                                         SampleISEFrameNode, mList);
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
        SampleISEFrameNode *pFirstNode = list_first_entry_or_null(&pFrameManager->mIdleList_1,
                                         SampleISEFrameNode, mList);
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

int SampleISEFrameManager_ReleaseFrame(void *pThiz, unsigned int nFrameId)
{
    int ret = 0;
    SampleISEFrameManager *pFrameManager = (SampleISEFrameManager*)pThiz;
    pthread_mutex_lock(&pFrameManager->mLock);
    int bFindFlag = 0;
    SampleISEFrameNode *pEntry, *pTmp;
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

static ERRORTYPE SampleISECallbackWrapper(void *cookie,MPP_CHN_S *Port, MPP_EVENT_TYPE event, void *pEventData)
{
    ERRORTYPE ret = SUCCESS;
    SampleISEFrameManager *pContext = (SampleISEFrameManager*)cookie;
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

int initSampleISEFrameManager(SampleISEParameter *pISEParameter, int nFrameNum)
{
    int ret = 0;
    ret = pthread_mutex_init(&pISEParameter->mFrameManager.mLock, NULL);
    if(ret!=0) {
        aloge("fatal error! pthread mutex init fail!");
        return ret;
    }
    ret = pthread_mutex_init(&pISEParameter->mFrameManager.mWaitFrameLock, NULL);
    if(ret != 0) {
        aloge("fatal error! pthread mutex init fail!");
        return ret;
    }
    ret = cdx_sem_init(&pISEParameter->mFrameManager.mSemFrameCome, 0);
    if(ret != 0) {
        aloge("cdx sem init fail!");
        return ret;
    }
    Thread_EXIT = FALSE;
    INIT_LIST_HEAD(&pISEParameter->mFrameManager.mIdleList_0);
    INIT_LIST_HEAD(&pISEParameter->mFrameManager.mUsingList_0);
    INIT_LIST_HEAD(&pISEParameter->mFrameManager.mIdleList_1);
    INIT_LIST_HEAD(&pISEParameter->mFrameManager.mUsingList_1);

    FILE *fd[2];
    fd[0] = pISEParameter->PictureCap.Pic0_FilePath;
    fd[1] = pISEParameter->PictureCap.Pic1_FilePath;

    int width = 0, height = 0;
    width = pISEParameter->PictureCap.PicWidth;
    height = pISEParameter->PictureCap.PicHeight;
    int i = 0,j = 0;
    SampleISEFrameNode *pNode;
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
            pNode = (SampleISEFrameNode*)malloc(sizeof(SampleISEFrameNode));
            memset(pNode, 0, sizeof(SampleISEFrameNode));
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
                list_add_tail(&pNode->mList, &pISEParameter->mFrameManager.mIdleList_0);
                pISEParameter->mFrameManager.PicId[j] = 0;
            } else {
                list_add_tail(&pNode->mList, &pISEParameter->mFrameManager.mIdleList_1);
                pISEParameter->mFrameManager.PicId[j] = 0;
            }
        }
        fclose(fd[j]);
    }
    pISEParameter->mFrameManager.mNodeCnt = nFrameNum;
    pISEParameter->mFrameManager.PrefetchFirstIdleFrame = SampleISEFrameManager_PrefetchFirstIdleFrame;
    pISEParameter->mFrameManager.UseFrame = SampleISEFrameManager_UseFrame;
    pISEParameter->mFrameManager.ReleaseFrame = SampleISEFrameManager_ReleaseFrame;
    return 0;
}

int destroySampleISEFrameManager(SampleISEParameter *pISEParameter)
{
    SampleISEFrameManager *pISEList = NULL;
    SampleISEFrameNode *pEntry = NULL, *pTmp = NULL;
    pISEList = &pISEParameter->mFrameManager;
    if(!list_empty(&pISEList->mUsingList_0)) {
        aloge("fatal error! why using list is not empty");
        list_for_each_entry_safe(pEntry, pTmp, &pISEList->mUsingList_0, mList) {
            list_move_tail(&pEntry->mList, &pISEList->mIdleList_0);
        }
    }
    if(!list_empty(&pISEList->mUsingList_1)) {
        aloge("fatal error! why using list is not empty");
        list_for_each_entry_safe(pEntry, pTmp, &pISEList->mUsingList_1, mList) {
            list_move_tail(&pEntry->mList, &pISEList->mIdleList_1);
        }
    }
    int cnt = 0;
    struct list_head *pList;
    list_for_each(pList, &pISEList->mIdleList_0) {
        cnt++;
    }
    if(cnt != pISEList->mNodeCnt) {
        aloge("fatal error! frame count is not match [%d]!=[%d]", cnt,pISEList->mNodeCnt);
    }
    cnt = 0;
    list_for_each(pList, &pISEList->mIdleList_1) {
        cnt++;
    }
    if(cnt != pISEList->mNodeCnt) {
        aloge("fatal error! frame count is not match [%d]!=[%d]", cnt,pISEList->mNodeCnt);
    }
    cnt = 0;
    pEntry = NULL, pTmp = NULL;
    list_for_each_entry_safe(pEntry, pTmp, &pISEList->mIdleList_0, mList) {
        cnt++;
        ion_freeMem(pEntry->mFrame.VFrame.mpVirAddr[0]);
        list_del(&pEntry->mList);
        free(pEntry);
    }
    cnt = 0;
    pEntry = NULL, pTmp = NULL;
    list_for_each_entry_safe(pEntry, pTmp, &pISEList->mIdleList_1, mList) {
        cnt++;
        ion_freeMem(pEntry->mFrame.VFrame.mpVirAddr[0]);
        list_del(&pEntry->mList);
        free(pEntry);
    }
    pthread_mutex_destroy(&pISEList->mLock);
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

static int ParseCmdLine(int argc, char **argv, SampleISECmdLineParam *pCmdLinePara)
{
    alogd("sample_ISE path:[%s], arg number is [%d]", argv[0], argc);
    int ret = 0;
    int i=1;
    memset(pCmdLinePara, 0, sizeof(SampleISECmdLineParam));
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
                  "\t-path /home/sample_ISE.conf\n");
            ret = 1;
            break;
        } else {
            alogd("ignore invalid CmdLine param:[%s], type -h to get how to set parameter!", argv[i]);
        }
        i++;
    }
    return ret;
}

static ERRORTYPE loadSampleISEConfig(SampleISEConfig *pConfig, const char *conf_path)
{
    int ret;
    char *ptr = NULL;
    char *pStrPixelFormat = NULL,*EncoderType = NULL;
    int i = 0,j = 0,ISEPortNum = 0;
    CONFPARSER_S stConfParser;
    char name[256];
    double ratio = 0,ratio_16to9 = 0,ratio_4to3 = 0;
    ret = createConfParser(conf_path, &stConfParser);
    if(ret < 0) {
        aloge("load conf fail");
        return FAILURE;
    }
    memset(pConfig, 0, sizeof(SampleISEConfig));
    pConfig->AutoTestCount = GetConfParaInt(&stConfParser, SAMPLE_ISE_Auto_Test_Count, 0);
    pConfig->Process_Count = GetConfParaInt(&stConfParser, SAMPLE_ISE_Process_Count, 0);

    /* Get Picture parameter*/
    pConfig->PicConfig.PicFrameRate = GetConfParaInt(&stConfParser, SAMPLE_ISE_Pic_Frame_Rate, 0);
    pConfig->PicConfig.PicWidth = GetConfParaInt(&stConfParser, SAMPLE_ISE_Pic_Width, 0);
    pConfig->PicConfig.PicHeight = GetConfParaInt(&stConfParser, SAMPLE_ISE_Pic_Height, 0);
    pConfig->PicConfig.PicStride = GetConfParaInt(&stConfParser, SAMPLE_ISE_Pic_Stride, 0);
    alogd("pic_width = %d, pic_height = %d, pic_frame_rate = %d\n",
          pConfig->PicConfig.PicWidth,pConfig->PicConfig.PicHeight,pConfig->PicConfig.PicFrameRate);
    ptr = (char*)GetConfParaString(&stConfParser, SAMPLE_ISE_Pic0_File_Path, NULL);
    strncpy(pConfig->PicConfig.Pic0_FilePath, ptr, MAX_FILE_PATH_SIZE-1);
    pConfig->PicConfig.Pic0_FilePath[MAX_FILE_PATH_SIZE-1] = '\0';
    ptr = (char*)GetConfParaString(&stConfParser, SAMPLE_ISE_Pic1_File_Path, NULL);
    strncpy(pConfig->PicConfig.Pic1_FilePath, ptr, MAX_FILE_PATH_SIZE-1);
    pConfig->PicConfig.Pic1_FilePath[MAX_FILE_PATH_SIZE-1] = '\0';

    /* Get ISE parameter*/
    pConfig->ISEGroupConfig.ISEPortNum = GetConfParaInt(&stConfParser, SAMPLE_ISE_Port_Num, 0);
    ISEPortNum = pConfig->ISEGroupConfig.ISEPortNum;
    ptr = (char*)GetConfParaString(&stConfParser, SAMPLE_ISE_Output_File_Path, NULL);
    strncpy(pConfig->ISEGroupConfig.OutputFilePath, ptr, MAX_FILE_PATH_SIZE-1);
    pConfig->ISEGroupConfig.OutputFilePath[MAX_FILE_PATH_SIZE-1] = '\0';
    alogd("ise output_file_path = %s\n",pConfig->ISEGroupConfig.OutputFilePath);
    pConfig->ISEGroupConfig.ncam = 2;
    pConfig->ISEGroupConfig.hfov = GetConfParaInt(&stConfParser, SAMPLE_ISE_HFOV, 0);
    pConfig->ISEGroupConfig.wfov = GetConfParaInt(&stConfParser, SAMPLE_ISE_WFOV, 0);
    pConfig->ISEGroupConfig.ov = GetConfParaInt(&stConfParser, SAMPLE_ISE_OV, 0);
    pConfig->ISEGroupConfig.wfov_rev = pConfig->ISEGroupConfig.wfov;
    pConfig->ISEGroupConfig.pano_fov = GetConfParaInt(&stConfParser, SAMPLE_ISE_Pano_Fov, 0);
    pConfig->ISEGroupConfig.stre_en = 0;
    pConfig->ISEGroupConfig.stre_coeff = 1.12f;
    pConfig->ISEGroupConfig.offset_r2l = 0;
    pConfig->ISEGroupConfig.t_angle = 0.0f;
    /********************* variable init *********************/
    ratio = (double)pConfig->PicConfig.PicWidth / pConfig->PicConfig.PicHeight;
    ratio_16to9 = (double)16/9;
    ratio_4to3 = (double)4/3;
    if(ratio == ratio_16to9) {
        alogd("ratio 16To9,ratio = %lf",ratio);
        pConfig->ISEGroupConfig.Lens_Parameter_P0 = 45.0f;
        pConfig->ISEGroupConfig.Lens_Parameter_P1 = 180.0f - pConfig->ISEGroupConfig.Lens_Parameter_P0;
        // 161128 4kultrahd运动相机,1920*1080,16:9
        double cam_matr[3][3] = {{1.0801553927650632e+003, 0., 960},
            {0., 1.0801553927650632e+003, 540},
            {0., 0., 1.}
        };
        double dist[8] = {-3.7281828263384303e-001, 1.4872623555443068e-001,
                          5.0219487410607937e-004, 4.9114015903837594e-004,
                          -2.8820464576691112e-002, 0, 0, 0
                         };
        double cam_matr_prime[3][3] = {{800, 0., 960},
            {0., 800, 540},
            {0., 0., 1.}
        };

        // 根据所使用原图缩放比例对camera matrix进行缩放
        for (i = 0; i < 2; i++) {
            for (j = 0; j < 3; j++) {
                cam_matr[i][j] = cam_matr[i][j] / ((float)1080/pConfig->PicConfig.PicHeight);
                cam_matr_prime[i][j] = cam_matr_prime[i][j] / ((float)1080/pConfig->PicConfig.PicHeight);
            }
        }
        memcpy(pConfig->ISEGroupConfig.calib_matr,cam_matr,3 * 3 * sizeof(double));
        memcpy(pConfig->ISEGroupConfig.calib_matr_cv,cam_matr_prime,3 * 3 * sizeof(double));
        memcpy(pConfig->ISEGroupConfig.distort, dist, 8 * sizeof(double));
    } else if(ratio == ratio_4to3) {
        alogd("ratio 4To3,ratio = %lf",ratio);
        pConfig->ISEGroupConfig.Lens_Parameter_P0 = 51.5f;
        pConfig->ISEGroupConfig.Lens_Parameter_P1 = 180.0f - pConfig->ISEGroupConfig.Lens_Parameter_P0;
        // mindvision 2.2mm镜头参数,1280*960,4:3
        double cam_matr[3][3] = {{6.4083926311003916e+002, 0., 6.4046604117646211e+002},
            {0., 6.4083926311003916e+002, 4.8024084852398539e+002},
            {0., 0., 1.}
        };

        double dist[8] = {-1.1238957055436295e-001, -2.8526234113446611e-002, -1.5744176875224516e-003,
                          4.3263933494422564e-004, -1.8503663972417678e-002, 1.9027042284894091e-001,
                          -9.3597920343908084e-002, -3.2434503034753627e-002
                         };

        double cam_matr_prime[3][3] = {{3.7479101562500000e+002, 0., 6.40e+002},
            {0., 3.6949392700195312e+002, 4.80e+002},
            {0., 0., 1.}
        };
        for (i = 0; i < 2; i++) {
            for (j = 0; j < 3; j++) {
                cam_matr[i][j] = cam_matr[i][j] / ((float)960/pConfig->PicConfig.PicHeight);
                cam_matr_prime[i][j] = cam_matr_prime[i][j] / ((float)960/pConfig->PicConfig.PicHeight);
            }
        }
        memcpy(pConfig->ISEGroupConfig.calib_matr,cam_matr,3 * 3 * sizeof(double));
        memcpy(pConfig->ISEGroupConfig.calib_matr_cv,cam_matr_prime,3 * 3 * sizeof(double));
        memcpy(pConfig->ISEGroupConfig.distort, dist, 8 * sizeof(double));
    }
    alogd("ISE Group Parameter:port_num = %d,ncam = %d,Lens_Parameter_P0 = %f,Lens_Parameter_P1 = %f"
          "ov = %d, stre_en = %d",pConfig->ISEGroupConfig.ISEPortNum,pConfig->ISEGroupConfig.ncam,
          pConfig->ISEGroupConfig.Lens_Parameter_P0,pConfig->ISEGroupConfig.Lens_Parameter_P1,
          pConfig->ISEGroupConfig.ov,pConfig->ISEGroupConfig.stre_en);
    alogd("stre_coeff = %f,offset_r2l = %d,pano_fov = %f,t_angle = %f,hfov = %f,wfov = %f,wfov_rev = %f",
          pConfig->ISEGroupConfig.stre_coeff,pConfig->ISEGroupConfig.offset_r2l,
          pConfig->ISEGroupConfig.pano_fov,pConfig->ISEGroupConfig.t_angle,
          pConfig->ISEGroupConfig.hfov,pConfig->ISEGroupConfig.wfov,pConfig->ISEGroupConfig.wfov_rev);

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
    destroyConfParser(&stConfParser);
    return SUCCESS;
}

static void *Loop_SendImageFileThread(void *pArg)
{
    alogd("Start Loop_SendImageFileThread");
    SampleISEParameter *pCap = (SampleISEParameter *)pArg;
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
    alogd("Count = %d",pCap->Process_Count);
    while((i != pCap->Process_Count) || (pCap->Process_Count == -1)) {
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
        usleep(40000);
    }
    Thread_EXIT = TRUE;
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
    while(1) {

        if(Thread_EXIT == TRUE) {
            alogd("Thread_EXIT is True!\n");
            break;
        }
        if ((ret = AW_MPI_ISE_GetData(ISEGroup, ISEPort, &ISE_Frame_buffer, s32MilliSec)) < 0) {
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
            sprintf(filename,"/%s/ise_ch%d_%d.yuv",name,ISEPort,i);
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

    return NULL;
}

void Sample_ISE_HELP()
{
    alogd("Run sample_ISE command: ./sample_ISE -path ./sample_ISE.conf\r\n");
}

int main(int argc, char *argv[])
{
    int ret = 0, i = 0, j = 0, count = 0;
    int result = 0;
    int AutoTestCount = 0;
    alogd("Sample ISE buile time = %s, %s.\r\n", __DATE__, __TIME__);
    if(argc != 3) {
        Sample_ISE_HELP();
        exit(0);
    }

    SampleISEParameter ISEeyePara;
    SampleISEConfparser stContext;

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
        pConfigFilePath = DEFAULT_SAMPLE_ISE_CONF_PATH;
    }
    //parse config file.
    if(loadSampleISEConfig(&stContext.mConfigPara, pConfigFilePath) != SUCCESS) {
        aloge("fatal error! no config file or parse conf file fail");
        result = -1;
        goto _exit;
    }
    AutoTestCount = stContext.mConfigPara.AutoTestCount;
    ISEeyePara.Process_Count = stContext.mConfigPara.Process_Count;

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
        ISEeyePara.PictureCap.PicWidth = stContext.mConfigPara.PicConfig.PicWidth;
        ISEeyePara.PictureCap.PicHeight = stContext.mConfigPara.PicConfig.PicHeight;
        ISEeyePara.PictureCap.PicStride = stContext.mConfigPara.PicConfig.PicStride;
        ISEeyePara.PictureCap.PicFrameRate = ((float)1/stContext.mConfigPara.PicConfig.PicFrameRate) * 1000000;
        ISEeyePara.PictureCap.Pic0_FilePath = fopen(stContext.mConfigPara.PicConfig.Pic0_FilePath,"rb");
        ISEeyePara.PictureCap.Pic1_FilePath = fopen(stContext.mConfigPara.PicConfig.Pic1_FilePath,"rb");
        ret = initSampleISEFrameManager(&ISEeyePara, 10);
        if(ret < 0) {
            aloge("Init FrameManager failed!");
            goto destory_framemanager;
        }
        int ISEPortNum = 0;
        ISEPortNum = stContext.mConfigPara.ISEGroupConfig.ISEPortNum;
        ISE_PortCap_S *pISEPortCap;

#if  Load_Len_Parameter
        pISEPortCap = &ISEeyePara.pISEGroupCap[0].PortCap_S[0];
        FILE *sti_cfg_fd = NULL;
        sti_cfg_fd = fopen("./sti_cfg.bin","rb+");
        fread(&pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.p0,sizeof(float),1,sti_cfg_fd);
        fread(&pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.p1,sizeof(float),1,sti_cfg_fd);
        fread(&pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.hfov,sizeof(float),1,sti_cfg_fd);
        fread(&pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.wfov,sizeof(float),1,sti_cfg_fd);
        fread(&pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.wfov_rev,sizeof(float),1,sti_cfg_fd);
        alogd("load sti len parameter:p0 = %f, p1 = %f, hfov = %f, wfov  = %f, wfov_rev = %f",
              pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.p0,
              pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.p1,
              pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.hfov,
              pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.wfov,
              pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.wfov_rev);
#endif

        for(j = 0; j < 1; j++) {
            /*Set ISE Group Attribute*/
            memset(&ISEeyePara.pISEGroupCap[j], 0, sizeof(ISE_GroupCap_S));
            ISEeyePara.pISEGroupCap[j].ISE_Group = j;
            ISEeyePara.pISEGroupCap[j].pGrpAttr.iseMode = ISEMODE_TWO_ISE;
            ret = aw_isegroup_creat(ISEeyePara.pISEGroupCap[j].ISE_Group, &ISEeyePara.pISEGroupCap[j].pGrpAttr);
            if(ret < 0) {
                aloge("ISE Group %d creat failed",ISEeyePara.pISEGroupCap[j].ISE_Group);
                goto destory_isegroup;
            }
            for(i = 0; i< ISEPortNum; i++) {
                memset(&ISEeyePara.pISEGroupCap[j].PortCap_S[i], 0, sizeof(ISE_PortCap_S));
                ISEeyePara.pISEGroupCap[j].PortCap_S[i].ISE_Group = j;
                ISEeyePara.pISEGroupCap[j].PortCap_S[i].ISE_Port = i;
                ISEeyePara.pISEGroupCap[j].PortCap_S[i].thread_id = i;
                ISEeyePara.pISEGroupCap[j].PortCap_S[i].s32MilliSec = 4000;
                ISEeyePara.pISEGroupCap[j].PortCap_S[i].OutputFilePath =  stContext.mConfigPara.ISEGroupConfig.OutputFilePath;
                pISEPortCap = &ISEeyePara.pISEGroupCap[j].PortCap_S[i];
                /*Set ISE Port Attribute*/
                if(i == 0) { //fish arttr
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.ncam = 2;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.in_w = stContext.mConfigPara.PicConfig.PicWidth;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.in_h = stContext.mConfigPara.PicConfig.PicHeight;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.pano_w = stContext.mConfigPara.ISEPortConfig[i].ISEWidth;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.pano_h = stContext.mConfigPara.ISEPortConfig[i].ISEHeight;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.p0  = stContext.mConfigPara.ISEGroupConfig.Lens_Parameter_P0;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.p1  = stContext.mConfigPara.ISEGroupConfig.Lens_Parameter_P1;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.ov = stContext.mConfigPara.ISEGroupConfig.ov;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.yuv_type = 0;
                    memcpy(pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.calib_matr, stContext.mConfigPara.ISEGroupConfig.calib_matr,
                           3 * 3 * sizeof(double));
                    memcpy(pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.calib_matr_cv, stContext.mConfigPara.ISEGroupConfig.calib_matr_cv,
                           3 * 3 * sizeof(double));
                    memcpy(pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.distort, stContext.mConfigPara.ISEGroupConfig.distort,
                           8 * sizeof(double));
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.stre_en = stContext.mConfigPara.ISEGroupConfig.stre_en;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.stre_coeff = stContext.mConfigPara.ISEGroupConfig.stre_coeff;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.offset_r2l = stContext.mConfigPara.ISEGroupConfig.offset_r2l;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.pano_fov = stContext.mConfigPara.ISEGroupConfig.pano_fov;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.t_angle = stContext.mConfigPara.ISEGroupConfig.t_angle;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.hfov = stContext.mConfigPara.ISEGroupConfig.hfov;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.wfov = stContext.mConfigPara.ISEGroupConfig.wfov;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.wfov_rev = stContext.mConfigPara.ISEGroupConfig.wfov_rev;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.in_luma_pitch = stContext.mConfigPara.PicConfig.PicStride;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.in_chroma_pitch = stContext.mConfigPara.PicConfig.PicStride;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.pano_luma_pitch = stContext.mConfigPara.ISEPortConfig[i].ISEStride;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_cfg.pano_chroma_pitch = stContext.mConfigPara.ISEPortConfig[i].ISEStride;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_proccfg.pano_flip = stContext.mConfigPara.ISEPortConfig[i].flip_enable;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_proccfg.pano_mirr = stContext.mConfigPara.ISEPortConfig[i].mirror_enable;
                } else if(i>= 1 || i <= 3) {
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_proccfg.scalar_en[i-1] = 1;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_proccfg.scalar_w[i-1] =  stContext.mConfigPara.ISEPortConfig[i].ISEWidth;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_proccfg.scalar_h[i-1] =  stContext.mConfigPara.ISEPortConfig[i].ISEHeight;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_proccfg.scalar_flip[i-1] =  stContext.mConfigPara.ISEPortConfig[i].flip_enable;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_proccfg.scalar_mirr[i-1] =  stContext.mConfigPara.ISEPortConfig[i].mirror_enable;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_proccfg.scalar_luma_pitch[i-1] =  stContext.mConfigPara.ISEPortConfig[i].ISEStride;
                    pISEPortCap->PortAttr.mode_attr.mIse.ise_proccfg.scalar_chroma_pitch[i-1] =  stContext.mConfigPara.ISEPortConfig[i].ISEStride;
                }
                ret = aw_iseport_creat(ISEeyePara.pISEGroupCap[j].ISE_Group, ISEeyePara.pISEGroupCap[j].PortCap_S[i].ISE_Port,
                                       &pISEPortCap->PortAttr);
                if(ret < 0) {
                    aloge("ISE Port%d creat failed",ISEeyePara.pISEGroupCap[j].PortCap_S[i].ISE_Port);
                    goto destory_iseport;
                }
                ISEeyePara.pISEGroupCap[j].PortCap_S[i].width = stContext.mConfigPara.ISEPortConfig[i].ISEStride;
                ISEeyePara.pISEGroupCap[j].PortCap_S[i].height = stContext.mConfigPara.ISEPortConfig[i].ISEHeight;
            }
        }

        MPPCallbackInfo cbInfo;
        cbInfo.cookie = (void*)&ISEeyePara.mFrameManager;
        cbInfo.callback = (MPPCallbackFuncType)&SampleISECallbackWrapper;
        ret = AW_MPI_ISE_RegisterCallback(ISEeyePara.pISEGroupCap[0].ISE_Group, &cbInfo);
        if(ret != SUCCESS) {
            aloge("ISE Register Callback error!");
            result = -1;
            goto _exit;
        }

        ret = pthread_create(&ISEeyePara.PictureCap.thread_id, NULL, Loop_SendImageFileThread, (void *)&ISEeyePara);
        if (ret < 0) {
            aloge("Loop_SendImageFileThread create failed");
            result = -1;
            goto _exit;
        }
        for(i = 0; i< ISEPortNum; i++) {
            pthread_create(&ISEeyePara.pISEGroupCap[0].PortCap_S[i].thread_id, NULL,
                           Loop_GetIseData, (void *)&ISEeyePara.pISEGroupCap[0].PortCap_S[i]);
        }

        for (i = 0; i < 1; i++) {
            pthread_join(ISEeyePara.PictureCap.thread_id, NULL);
        }

        for(i = 0; i < ISEPortNum; i++) {
            pthread_join(ISEeyePara.pISEGroupCap[0].PortCap_S[i].thread_id, NULL);
        }

        ret = AW_MPI_ISE_Stop(ISEeyePara.pISEGroupCap[0].ISE_Group);
        if(ret < 0) {
            aloge("ise stop error!\n");
            result = -1;
            goto _exit;
        }

destory_iseport:
        for(i = 0; i < ISEPortNum; i++) {
            ret = aw_iseport_destory(ISEeyePara.pISEGroupCap[0].ISE_Group,
                                     ISEeyePara.pISEGroupCap[0].PortCap_S[i].ISE_Port);
            if(ret < 0) {
                aloge("ISE Port%d distory error!",ISEeyePara.pISEGroupCap[0].PortCap_S[i].ISE_Port);
                result = -1;
                goto _exit;
            }
        }
destory_isegroup:
        ret = aw_isegroup_destory(ISEeyePara.pISEGroupCap[0].ISE_Group);
        if(ret < 0) {
            aloge("ISE Destroy Group%d error!",ISEeyePara.pISEGroupCap[0].ISE_Group);
            result = -1;
            goto _exit;
        }
destory_framemanager:
        destroySampleISEFrameManager(&ISEeyePara);
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
    alogd("sample_ise exit!\n");
    return 0;

_exit:
    return 0;
}

