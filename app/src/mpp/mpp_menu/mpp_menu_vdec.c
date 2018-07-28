/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_menu_vdec.c
 * @brief 该目录是对mpp中的 VDEC 模块进行菜单设置项控制.
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
static int mpp_menu_vdec_rotate_get(void *pData, char *pTitle);
static int mpp_menu_vdec_pause(void *pData, char *pTitle);
static int mpp_menu_vdec_resume(void *pData, char *pTitle);


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static MENU_INODE g_vdec_menu_list[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"VDEC Get Rotate ",         mpp_menu_vdec_rotate_get,  NULL, NULL},
    {(char *)"VDEC Chn Pause  (Chn:0)",  mpp_menu_vdec_pause,       NULL, NULL},
    {(char *)"VDEC Chn Resume (Chn:0)",  mpp_menu_vdec_resume,      NULL, NULL},
    {(char *)"Previous Step (Quit VDEC setting)", ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
int mpp_menu_vdec_get(PMENU_INODE *pmenu_list_vdec)
{
    if (NULL == pmenu_list_vdec) {
        DB_PRT("Input pmenu_list_vdec is NULL!\n");
        return -1;
    }

    *pmenu_list_vdec = g_vdec_menu_list;

    return 0;
}


int mpp_menu_vdec_rotate_set(void *pData, char *pTitle)
{
    int  index    = 0;
    char str[256] = {0};
    ROTATE_E *p_rotate = NULL;

    if (NULL == pData) {
        ERR_PRT("Input pData is NULL\n");
        return -1;
    }

    p_rotate = (ROTATE_E *)pData;
    while (1) {
        printf("\n\n***************** Set VDEC rotate type **************************\n");
        printf(" [0]:ROTATE_NONE [default] \n");
        printf(" [1]:ROTATE_90  \n");
        printf(" [2]:ROTATE_180 \n");
        printf(" [3]:ROTATE_270 \n");
        printf(" Please choose rotate or (q-Quit): \n");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;
        index = is_digit_str(str);
        if (index) {
            printf(" Input %s error.\n\n", str);
            continue;
        }
        index = atoi(str);
        if (index < 0 || index > 3) {
            printf(" Input Vdec rotate type:%d error!\n", index);
            continue;
        }

        switch(index) {
        case 0:
            *p_rotate = ROTATE_NONE;
            break;
        case 1:
            *p_rotate = ROTATE_90;
            break;
        case 2:
            *p_rotate = ROTATE_180;
            break;
        case 3:
            *p_rotate = ROTATE_270;
            break;
        default:
            break;
        }
        return 0;
    }

    return 0;
}


static int mpp_menu_vdec_rotate_get(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  chn      = 0;
    char str[256] = {0};
    ROTATE_E rotate;

    while (1) {
        printf("\n\n***************** Get Vdec rotate type **************************\n");
        printf(" Please Input Vdec channel id 0~15 or (q-Quit): ");
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
        if (chn < 0 || chn > 15) {
            printf(" Input Vdec channel id:%d error!\n", chn);
            continue;
        }

        ret = AW_MPI_VDEC_GetRotate(chn, &rotate);
        if (ret) {
            ERR_PRT(" Do AW_MPI_VDEC_SetRotate vdec_chn:%d rotate:[%d] fail! ret:0x%x\n", chn, rotate, ret);
        } else {
            DB_PRT(" Do AW_MPI_VDEC_SetRotate vdec_chn:%d rotate:[%d] success! ret:0x%x\n", chn, rotate, ret);
        }

        return ret;
    }

    return 0;
}


static int mpp_menu_vdec_pause(void *pData, char *pTitle)
{
    int ret = 0;
    int chn = 0;

    printf("\n\n***************** Pause VDEC Chn:0 **************************\n");

    ret = AW_MPI_VDEC_Pause(chn);
    if (ret) {
        ERR_PRT(" Do AW_MPI_VDEC_Pause vdec_chn:%d fail! ret:0x%x\n", chn, ret);
    } else {
        DB_PRT(" Do AW_MPI_VDEC_Pause vdec_chn:%d success! ret:0x%x\n", chn, ret);
    }

    return ret;
}


static int mpp_menu_vdec_resume(void *pData, char *pTitle)
{
    int ret = 0;
    int chn = 0;

    printf("\n\n***************** Resume VDEC Chn:0 **************************\n");

    ret = AW_MPI_VDEC_Resume(chn);
    if (ret) {
        ERR_PRT(" Do AW_MPI_VDEC_Resume vdec_chn:%d fail! ret:0x%x\n", chn, ret);
    } else {
        DB_PRT(" Do AW_MPI_VDEC_Resume vdec_chn:%d success! ret:0x%x\n", chn, ret);
    }

    return ret;
}

