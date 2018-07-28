/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_comm_sys.c
 * @brief 该目录是对sys模块的公共操作,参数设置和获取类型进行简单抽象
 *        封装,以达到提高使用率和减少工作量的目的.
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-04-14
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
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/

ERRORTYPE mpp_comm_sys_init()
{
    ERRORTYPE ret = 0;
    MPP_SYS_CONF_S mSysConf;

    memset(&mSysConf, 0, sizeof(MPP_SYS_CONF_S));
    mSysConf.nAlignWidth = MPP_COMM_SYS_ALIGN_WIDTH;
    ret = AW_MPI_SYS_SetConf(&mSysConf);
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_SYS_SetConf fail! ret:%d \n", ret);
        return -1;
    }

    ret = AW_MPI_SYS_Init();
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_SYS_Init fail! ret:%d \n", ret);
        return -1;
    }

    DB_PRT("Do sys init success!\n");

    return 0;
}

ERRORTYPE mpp_comm_sys_exit()
{
    ERRORTYPE ret = 0;

    ret = AW_MPI_SYS_Exit();
    if (SUCCESS != ret) {
        ERR_PRT("Do AW_MPI_SYS_Init fail! ret:%d \n", ret);
        return -1;
    }

    DB_PRT("The sys exit!\n");

    return 0;
}

