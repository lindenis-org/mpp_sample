/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_menu_vi.c
 * @brief 该目录是对mpp中的 VI 模块进行菜单设置项控制.
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-05-28
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
#include "mpp_menu.h"
//#include "mpi_vi_common.h"


/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
static int mpp_menu_vi_save_yuv(void *pData, char *pTitle);
static int mpp_menu_vi_mirror_set(void *pData, char *pTitle);
static int mpp_menu_vi_mirror_get(void *pData, char *pTitle);
static int mpp_menu_vi_flip_set(void *pData, char *pTitle);
static int mpp_menu_vi_flip_get(void *pData, char *pTitle);

/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static MENU_INODE g_vi_menu_list[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"VI Save yuv data (path:/tmp/)",   mpp_menu_vi_save_yuv,    NULL, NULL},
    {(char *)"VI mirror set",                   mpp_menu_vi_mirror_set,  NULL, NULL},
    {(char *)"VI mirror get",                   mpp_menu_vi_mirror_get,  NULL, NULL},
    {(char *)"VI flip set",                     mpp_menu_vi_flip_set,    NULL, NULL},
    {(char *)"VI flip get",                     mpp_menu_vi_flip_get,    NULL, NULL},
    {(char *)"Previous Step (Quit VI setting)", ExitCurrentMenuLevel,    NULL, NULL},
    {NULL, NULL, NULL, NULL},
};


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
int mpp_menu_vi_get(PMENU_INODE *pmenu_list_vi)
{
    if (NULL == pmenu_list_vi) {
        DB_PRT("Input pmenu_list_vi is NULL!\n");
        return -1;
    }

    *pmenu_list_vi = g_vi_menu_list;

    return 0;
}


static int mpp_menu_vi_save_yuv(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  chn      = 0;
    int  device   = 0;
    char str[256] = {0};

    while (1) {
        printf("\n\n***************** Save VI yuv file **************************\n");
        printf(" Please Input VI device id: 0~7 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        device = atoi(str);
        if (device < 0 || device > 7) {
            printf(" Input VI device id:%d error!\n", device);
            continue;
        }

        printf("\n Please Input VI_DEV:%d channel id: 0~7 or (q-Quit): ", device);
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        chn = atoi(str);
        if (chn < 0 || chn > 7) {
            printf(" Input VI chn id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VI_Debug_StoreFrame(device, chn, "/tmp/");
        if (ret) {
            ERR_PRT(" Do AW_MPI_VI_Debug_StoreFrame vi_dev:%d vi_chn:%d save_path:%s fail! ret:0x%x\n", device, chn, "/tmp/", ret);
        } else {
            DB_PRT(" Do AW_MPI_VI_Debug_StoreFrame vi_dev:%d vi_chn:%d save_path:%s success! ret:0x%x\n", device, chn, "/tmp/", ret);
        }

        return ret;
    }

    return 0;
}

static int mpp_menu_vi_mirror_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  device    = 0;
    int  val       = 0;
    char str[256]  = {0};
    while(1) {
        printf("\n\n************* Set mirror device id **********************\n");
        printf(" Please Input VI device id: 0~7 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        device = atoi(str);
        if (device < 0 || device > 7) {
            printf(" Input VI device id:%d error!\n", device);
            continue;
        }
        printf("you chose device id : %d \n",device);

        while (1) {
            printf("\n**************** Setting vi mirror *************************\n");
            printf("[0]-Disable\n");
            printf("[1]-Enable\n");
            printf(" Please Input  mirror value [0] or [1] or (q-Quit): ");

            memset(str, 0, sizeof(str));
            gets(str);
            printf("\n");
            if (0 == str[0]) {
                continue;
            }
            if ('q' == str[0]) {
                return 0;
            }

            ret = is_digit_str(str);
            if (ret) {
                printf(" Input %s error.\n\n", str);
                continue;
            }

            val = atoi(str);
            if (0 == val || val == 1) {
                ret = AW_MPI_VI_SetVippMirror(device, val);
                if (0 == ret) {
                    DB_PRT("Do AW_MPI_VI_SetVippMirror succeed isp_dev:%d  value:%d  ret:%d \n",
                           device, val, ret);
                    return 0;
                } else {
                    ERR_PRT("Do AW_MPI_VI_SetVippMirror fail isp_dev:%d  value:%d  ret:%d \n",
                            device, val, ret);
                    return -1;
                }
            } else {
                printf(" Input mirror value:%d error!\n", val);
                continue;
            }
        }
    }
}

static int mpp_menu_vi_mirror_get(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  device    = 0;
    int  val       = 0;
    char str[256]  = {0};

    while(1) {
        printf("\n\n************* Set mirror device id **********************\n");
        printf(" Please Input VI device id: 0~7 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        device = atoi(str);
        if (device < 0 || device > 7) {
            printf(" Input VI device id:%d error!\n", device);
            continue;
        }
        printf("you chose device id : %d \n",device);

        printf("\n**************** Get Vipp mirror *************************\n");
        ret = AW_MPI_VI_GetVippMirror(device, &val);
        if (0 == ret) {
            DB_PRT("Do AW_MPI_VI_GetVippMirror succeed isp_dev:%d  (value:%d)  ret:%d \n",
                   device, val, ret);
            if (1 == val) {
                printf("Mirror status : Enable\n");
            } else {
                printf("Mirror status : Disable\n");
            }
            return 0;
        } else {
            ERR_PRT("Do AW_MPI_ISP_GetMirror fail isp_dev:%d  (value:%d)  ret:%d \n",
                    device, val, ret);
            return -1;
        }
    }
}

static int mpp_menu_vi_flip_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  device    = 0;
    int  val       = 0;
    char str[256]  = {0};

    while(1) {
        printf("\n\n************* Set flip device id **********************\n");
        printf(" Please Input VI device id: 0~7 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        device = atoi(str);
        if (device < 0 || device > 7) {
            printf(" Input VI device id:%d error!\n", device);
            continue;
        }
        printf("you chose device id : %d \n",device);

        while (1) {
            printf("\n**************** Setting Vipp flip *************************\n");
            printf("[0]-Disable\n");
            printf("[1]-Enable\n");
            printf(" Please Input  flip value [0] or [1] or (q-Quit): ");

            memset(str, 0, sizeof(str));
            gets(str);
            printf("\n");
            if (0 == str[0]) {
                continue;
            }
            if ('q' == str[0]) {
                return 0;
            }

            ret = is_digit_str(str);
            if (ret) {
                printf(" Input %s error.\n\n", str);
                continue;
            }

            val = atoi(str);
            if (0 == val || val == 1) {
                ret = AW_MPI_VI_SetVippFlip(device, val);
                if (0 == ret) {
                    DB_PRT("Do AW_MPI_VI_SetVippFlip succeed isp_dev:%d  value:%d  ret:%d \n",
                           device, val, ret);
                    return 0;
                } else {
                    ERR_PRT("Do AW_MPI_VI_SetVippFlip fail isp_dev:%d  value:%d  ret:%d \n",
                            device, val, ret);
                    return -1;
                }
            } else {
                printf(" Input flip value:%d error!\n", val);
                continue;
            }
        }
    }
}

static int mpp_menu_vi_flip_get(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  device    = 0;
    int  val       = 0;
    char str[256]  = {0};

    while(1) {
        printf("\n\n************* Set flip device id **********************\n");
        printf(" Please Input VI device id: 0~7 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        device = atoi(str);
        if (device < 0 || device > 7) {
            printf(" Input VI device id:%d error!\n", device);
            continue;
        }
        printf("you chose device id : %d \n",device);

        printf("\n**************** Get Vipp mirror *************************\n");
        ret = AW_MPI_VI_GetVippFlip(device, &val);
        if (0 == ret) {
            DB_PRT("Do AW_MPI_VI_GetVippFlip succeed isp_dev:%d  (value:%d)  ret:%d \n",
                   device, val, ret);
            if (1 == val) {
                printf("Flip status : Enable\n");
            } else {
                printf("Flip status : Disable\n");
            }
            return 0;
        } else {
            ERR_PRT("Do AW_MPI_VI_GetVippFlip fail isp_dev:%d  (value:%d)  ret:%d \n",
                    device, val, ret);
            return -1;
        }
    }
}

