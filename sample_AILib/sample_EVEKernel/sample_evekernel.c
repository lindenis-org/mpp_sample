#define LOG_NDEBUG 0
#define LOG_TAG "sample_evekernel"

#include "plat_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "media/mpi_sys.h"
#include "mm_comm_video.h"
#include "ion_memmanager.h"

#include <confparser.h>

#include "aw_ai_eve_kernel_interface.h"


//#define TIME_TEST
#ifdef TIME_TEST
#include "SystemBase.h"
#endif

//loader yuv source data from file by call eve_yuv lib interface
//#define USE_EVE_YUV_LOADER


#define MAX_FILE_PATH_LEN  (256)

#define EVE_SRC_DATA_FILE_STR   "src_data_file"
#define EVE_SRC_DATA_WIDTH   "src_w"
#define EVE_SRC_DATA_HEIGHT   "src_h"
#define EVE_SRC_DATA_PIXFMT   "src_pixfmt"
#define EVE_TEST_FRAME_RATE  "test_frame_rate"
#define EVE_TEST_FRAME_NUM  "test_frame_num"
#define EVE_RESULT_OUT_FILE  "result_out_file"


#define DEFAULT_SRC_DATA_FILE "/mnt/extsd/sample_eve/data.yuv"
#define DEFAULT_SRC_DATA_W 640
#define DEFAULT_SRC_DATA_H 360


typedef struct EVE_CONF_INFO_S {
    char srcDataPath[MAX_FILE_PATH_LEN];
    int src_w;
    int src_h;
    PIXEL_FORMAT_E pixformat;

    int frame_rate;
    int get_frame_num;
    char resultOutPath[MAX_FILE_PATH_LEN];
} EVE_CONF_INFO_S;

typedef struct EVE_CMD_LINE_PARAM_S {
    char mConfigFilePath[MAX_FILE_PATH_LEN];
} EVE_CMD_LINE_PARAM_S;

typedef struct EVE_TEST_DATA {
    EVE_CMD_LINE_PARAM_S mCmdLinePara;
    EVE_CONF_INFO_S mConfInfo;
} EVE_TEST_DATA;


static EVE_TEST_DATA *pEveTestData = NULL;


static int initEveTestData()
{
    if (NULL != pEveTestData) {
        alogw("has been inited!");
        return 0;
    }

    pEveTestData = (EVE_TEST_DATA *)malloc(sizeof(EVE_TEST_DATA));
    if (NULL == pEveTestData) {
        aloge("malloc eve test data mem fail!");
        return -1;
    }
    memset(pEveTestData, 0, sizeof(EVE_TEST_DATA));

    return 0;
}

static int parseCmdLine(EVE_CMD_LINE_PARAM_S *pCmdLinePara, int argc, char** argv)
{
    int ret = -1;

    while (*argv) {
        if (!strcmp(*argv, "-path")) {
            argv++;
            if (*argv) {
                ret = 0;
                if (strlen(*argv) >= MAX_FILE_PATH_LEN) {
                    aloge("fatal error! file path[%s] too long:!", *argv);
                }

                strncpy(pCmdLinePara->mConfigFilePath, *argv, MAX_FILE_PATH_LEN-1);
                pCmdLinePara->mConfigFilePath[MAX_FILE_PATH_LEN-1] = '\0';
            }
        } else if(!strcmp(*argv, "-h")) {
            printf("CmdLine param:\n"
                   "\t-path /home/sample_eve.conf\n");
            break;
        } else if (*argv) {
            argv++;
        }
    }

    return ret;
}

static ERRORTYPE loadConfigPara(EVE_CONF_INFO_S *pConfInfo, char *conf_path)
{
    int ret;
    char *ptr;
    CONFPARSER_S mConf;

    ret = createConfParser(conf_path, &mConf);
    if (ret < 0) {
        aloge("load conf fail");
        return FAILURE;
    }

    ptr = (char *)GetConfParaString(&mConf, EVE_SRC_DATA_FILE_STR, NULL);
    if (NULL != ptr) {
        strcpy(pConfInfo->srcDataPath, ptr);
    }
    pConfInfo->src_w = GetConfParaInt(&mConf, EVE_SRC_DATA_WIDTH, 0);
    pConfInfo->src_h = GetConfParaInt(&mConf, EVE_SRC_DATA_HEIGHT, 0);
    pConfInfo->pixformat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
#if 0  //fix at yuv 420p
    ptr = (char *)GetConfParaString(&mConf, EVE_SRC_DATA_PIXFMT, NULL);
    if (NULL != ptr) {
        if (!strcmp(ptr, "yu12")) {
            pConfInfo->pixformat = MM_PIXEL_FORMAT_YUV_PLANAR_420;
        } else if (!strcmp(ptr, "yu21")) {
            pConfInfo->pixformat = MM_PIXEL_FORMAT_YVU_PLANAR_420;
        } else if (!strcmp(ptr, "nv21")) {
            pConfInfo->pixformat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
        } else if (!strcmp(ptr, "nv12")) {
            pConfInfo->pixformat = MM_PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        } else {
            aloge("not support the pixfmt!!");
        }
    }
#endif
    pConfInfo->frame_rate = GetConfParaInt(&mConf, EVE_TEST_FRAME_RATE, 0);
    pConfInfo->get_frame_num = GetConfParaInt(&mConf, EVE_TEST_FRAME_NUM, 0);

    ptr = (char *)GetConfParaString(&mConf, EVE_RESULT_OUT_FILE, NULL);
    if (NULL != ptr) {
        strcpy(pConfInfo->resultOutPath, ptr);
    }

    alogd("src_file=%s, src_w=%d, src_h=%d, src_pix_fmt=%d, test_frame_rate=%d, out_file=%s",\
          pConfInfo->srcDataPath, pConfInfo->src_w, \
          pConfInfo->src_h, pConfInfo->pixformat, pConfInfo->frame_rate, pConfInfo->resultOutPath);

    destroyConfParser(&mConf);

    return SUCCESS;
}

static int test_MemOpen(void)
{
    return ion_memOpen();
}

static int test_MemClose(void)
{
    return ion_memClose();
}

static unsigned char* test_allocMem(unsigned int size)
{
    return ion_allocMem(size);
}

static int test_freeMem(void *vir_ptr)
{
    return ion_freeMem(vir_ptr);
}

static unsigned int test_getPhyAddrByVirAddr(void *vir_ptr)
{
    return ion_getMemPhyAddr(vir_ptr);
}

int test_flushCache(void *vir_ptr, unsigned int size)
{
    return ion_flushCache(vir_ptr, size);
}


static void EveCallBackFunc(void *pData)
{
    EVE_TEST_DATA *pEveTestData = (EVE_TEST_DATA *)pData;
    alogd("[Dma Callback]: start Run!");
    return;
}

static int createEVE(EVE_TEST_DATA *pEveTestData)
{
    AW_S32 status;
    AW_AI_EVE_CTRL_S sEVECtrl;

    //initilize
    //sEVECtrl.addrInputType = AW_AI_EVE_ADDR_INPUT_PHY;
    sEVECtrl.addrInputType = AW_AI_EVE_ADDR_INPUT_VIR;
    sEVECtrl.scale_factor = 1;
    //sEVECtrl.maxPassStage = 13;
    sEVECtrl.mScanStageNo = 10;
    sEVECtrl.xStep0 = 1;
    sEVECtrl.xStep1 = 3;
    sEVECtrl.yStep = 4;
    sEVECtrl.mRltNum = AW_AI_EVE_MAX_RESULT_NUM;
    sEVECtrl.mMidRltNum = 0;
    //sEVECtrl.mMidRltLayel = 15;
    sEVECtrl.mMidRltStageNo = 10;
    sEVECtrl.rltType = AW_AI_EVE_RLT_OUTPUT_DETAIL;

    sEVECtrl.mDmaOut.s16Width = pEveTestData->mConfInfo.src_w;
    sEVECtrl.mDmaOut.s16Height = pEveTestData->mConfInfo.src_h;
    sEVECtrl.mPyramidLowestLayel.s16Width = pEveTestData->mConfInfo.src_w;
    sEVECtrl.mPyramidLowestLayel.s16Height = pEveTestData->mConfInfo.src_h;
    sEVECtrl.dmaSrcSize.s16Width = pEveTestData->mConfInfo.src_w;
    sEVECtrl.dmaSrcSize.s16Height = pEveTestData->mConfInfo.src_h;
    sEVECtrl.dmaDesSize.s16Width = sEVECtrl.dmaSrcSize.s16Width;
    sEVECtrl.dmaDesSize.s16Height = sEVECtrl.dmaSrcSize.s16Height;
    sEVECtrl.dmaRoi.s16X = 0;
    sEVECtrl.dmaRoi.s16Y = 0;
    sEVECtrl.dmaRoi.s16Width = sEVECtrl.dmaDesSize.s16Width;
    sEVECtrl.dmaRoi.s16Height = sEVECtrl.dmaDesSize.s16Height;

    AW_S8 *awKeyNew = "1111111111111111"; //key
    sEVECtrl.classifierNum = 8;
    sEVECtrl.classifierPath[0].path = (AW_S8*)"./classifier/frontface.ld";
    sEVECtrl.classifierPath[0].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[1].path = (AW_S8*)"./classifier/fullprofleftface.ld";
    sEVECtrl.classifierPath[1].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[2].path = (AW_S8*)"./classifier/fullprofrightface.ld";
    sEVECtrl.classifierPath[2].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[3].path = (AW_S8*)"./classifier/halfdownface.ld";
    sEVECtrl.classifierPath[3].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[4].path = (AW_S8*)"./classifier/profileface.ld";
    sEVECtrl.classifierPath[4].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[5].path = (AW_S8*)"./classifier/rotleftface.ld";
    sEVECtrl.classifierPath[5].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[6].path = (AW_S8*)"./classifier/rotrightface.ld";
    sEVECtrl.classifierPath[6].key  = (AW_U8*)awKeyNew;
    sEVECtrl.classifierPath[7].path = (AW_S8*)"./classifier/smallface.ld";
    sEVECtrl.classifierPath[7].key  = (AW_U8*)awKeyNew;

    sEVECtrl.dma_pUsr = pEveTestData;
    sEVECtrl.dmaCallBackFunc = &EveCallBackFunc;
    if (AW_AI_EVE_Kernel_Init(&sEVECtrl) == AW_STATUS_ERROR) {
        printf("Error! AW_AI_EVE_Kernel_Init\n");
        AW_AI_EVE_Kernel_UnInit();
        return -1;
    } else {
        printf("AW_AI_EVE_Kernel_Init finish!\n");
    }

    AW_AI_EVE_Kernel_SetDMAMode(AW_AI_EVE_DMA_EXECUTE_SYNC);

    return 0;
}

static void userFunc(AW_AI_EVE_RESULTS* rlt, AW_AI_EVE_MIDRESULTS* midRlt, void* pUsr )
{
    AW_U32 i;

    printf("<rltNum>: %d",rlt->total_rltNum);
    for (i = 0; i < rlt->total_rltNum; i++) {
        printf("[%d, %d, %d, %d]\n", rlt->Results[i].s16X, rlt->Results[i].s16Y, rlt->Results[i].s16Width, rlt->Results[i].s16Height);
    }
}

static int EVEProc(EVE_TEST_DATA *pEveTestData)
{
    int ret = -1;
    int i, size;
    int frameNum = 0;
    AW_STATUS_E status;
    AW_S64 timestamp = 0;
    AW_PVOID virPtr_y=NULL, virPtr_c=NULL;
    AW_IMAGE_S image = {0};
    FILE *src_fd = NULL;
    FILE *result_out_fd = NULL;
    char reslutStr[256] = {0};
    AW_U32 sUsr = 1;
    src_fd = fopen(pEveTestData->mConfInfo.srcDataPath, "rb");
    if (NULL == src_fd) {
        aloge("open src data file fail!!!");
        goto error_out_0;
    }

    result_out_fd = fopen(pEveTestData->mConfInfo.resultOutPath, "wb");
    if (NULL == result_out_fd) {
        aloge("create result out file fail!!!");
    }

    if (test_MemOpen() != 0) {
        aloge("open ion mem fail!!!");
        goto error_out_1;
    }

    size = pEveTestData->mConfInfo.src_w * pEveTestData->mConfInfo.src_h;
    virPtr_y = (AW_PVOID)test_allocMem(size);
    if (NULL == virPtr_y) {
        aloge("alloc y mem buffer fail!");
        goto error_out_2;
    }
    virPtr_c = (AW_PVOID)test_allocMem(size/2);
    if (NULL == virPtr_c) {
        aloge("alloc uv mem buffer fail!");
        goto error_out_3;
    }

    int read_len1, read_len2;
    fseek(src_fd, 0, SEEK_SET);
    if (NULL != result_out_fd) {
        fseek(result_out_fd, 0, SEEK_SET);
    }

    image.mWidth = pEveTestData->mConfInfo.src_w;
    image.mHeight = pEveTestData->mConfInfo.src_h;
    //image.mPixelFormat = MM_PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    image.mPixelFormat = pEveTestData->mConfInfo.pixformat;
    image.mpVirAddr[0] = virPtr_y;
    image.mpVirAddr[1] = virPtr_c;
    image.mPhyAddr[0] = test_getPhyAddrByVirAddr(virPtr_y);
    image.mPhyAddr[1] = test_getPhyAddrByVirAddr(virPtr_c);
    //image.mStride[0] = image.mWidth;
    //image.mStride[1] = image.mWidth;
    while (1) {
        read_len1 = fread(virPtr_y, 1, size, src_fd);
        read_len2 = fread(virPtr_c, 1, size/2, src_fd);

        if (!((read_len1==size) && (read_len2==size/2))) {
            alogw("eof! break out!");
            break;
        }

        if (frameNum >= pEveTestData->mConfInfo.get_frame_num) {
            alogd("has get[%d] frame, break out", frameNum);
            break;
        }

        test_flushCache(image.mpVirAddr[0], read_len1);
        test_flushCache(image.mpVirAddr[1], read_len2);

        status = AW_AI_EVE_Kernel_SetSrcAddr((void *)(image.mpVirAddr[0]));
        //AW_AI_EVE_Kernel_SetSrcAddr((void *)image.mPhyAddr[0]);

#ifdef TIME_TEST
        int64_t nStartTm = CDX_GetSysTimeUsMonotonic();//start time
#endif
        status = AW_AI_EVE_Kernel_Detect(&userFunc, &sUsr);
        if (AW_STATUS_ERROR == status) {
            aloge("AW_AI_EVE_Kernel_Detect failure!\n");
            break;
        }
#ifdef TIME_TEST
        int64_t nEndTm = CDX_GetSysTimeUsMonotonic();//end time
        alogd("frame[%d] handle time[%lld]", frameNum, nEndTm-nStartTm);
#endif

        timestamp += 1*1000 / pEveTestData->mConfInfo.frame_rate;
        frameNum++;
    }

    ret = 0;

    if (NULL != virPtr_c) {
        test_freeMem(virPtr_c);
    }
error_out_3:
    if (NULL != virPtr_y) {
        test_freeMem(virPtr_y);
    }
error_out_2:
    test_MemClose();
error_out_1:
    if (NULL != result_out_fd) {
        fclose(result_out_fd);
    }
    if (NULL != src_fd) {
        fclose(src_fd);
    }
error_out_0:
    //release
    status = AW_AI_EVE_Kernel_UnInit();
    if (AW_STATUS_ERROR == status) {
        aloge("AW_AI_EVE_Kernel_UnInit failure!");
    }
    return ret;
}

static unsigned int get_file_size(const char *path)
{
    unsigned long filesize = 0;
    struct stat statInfo;

    if (stat(path, &statInfo) < 0) {
        return filesize;
    } else {
        filesize = statInfo.st_size;
    }
    return filesize;
}


int main(int argc, char *argv[])
{
    int ret;
    EVE_TEST_DATA *pEveTest;

    alogd("sample_evekernel running!\n");
    if (initEveTestData() != 0) {
        aloge("init error!\n");
        return -1;
    }

    pEveTest = pEveTestData;
    if (parseCmdLine(&pEveTest->mCmdLinePara, argc, argv) != 0) {
        aloge( "parameter error!\n" );
        free(pEveTest);
        return -1;
    }

    ret = loadConfigPara(&pEveTest->mConfInfo, pEveTest->mCmdLinePara.mConfigFilePath);
    if (SUCCESS != ret) {
        aloge("warning get conf param fail!");
    }

    MPP_SYS_CONF_S mSysConf;
    memset(&mSysConf,0,sizeof(MPP_SYS_CONF_S));
    mSysConf.nAlignWidth = 32;
    AW_MPI_SYS_SetConf(&mSysConf);
    ret = AW_MPI_SYS_Init();
    if (ret < 0) {
        aloge("sys init failed");
        return -1;
    }

    ret = createEVE(pEveTest);
    if (ret != 0) {
        aloge("create eve fail! exit!");
        return -1;
    }

    ret = EVEProc(pEveTest);

    free(pEveTest);
    if (ret == 0) {
        printf("sample_evekernel exit!\n");
    }

    return ret;
}
