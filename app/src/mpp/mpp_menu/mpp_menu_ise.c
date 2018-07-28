/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_menu_ise.c
 * @brief 该目录是对mpp中的 ISE 模块进行菜单设置项控制.
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
/* None */


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static MENU_INODE g_ise_menu_list[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"Previous Step (Quit ISE setting)", ExitCurrentMenuLevel,  NULL, NULL},
    {NULL, NULL, NULL, NULL},
};


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
int mpp_menu_ise_get(PMENU_INODE *pmenu_list_ise)
{
    if (NULL == pmenu_list_ise) {
        DB_PRT("Input pmenu_list_ise is NULL!\n");
        return -1;
    }

    *pmenu_list_ise = g_ise_menu_list;

    return 0;
}


int mpp_menu_ise_mode_select(void *pData, char *pTitle)
{
    int  index    = 0;
    char str[256] = {0};
    MPP_MENU_ISE_CFG_E *p_cfg_type = NULL;

    if (NULL == pData) {
        ERR_PRT("Input pData is NULL\n");
        return -1;
    }

    p_cfg_type = (MPP_MENU_ISE_CFG_E *)pData;

    while (1) {
        printf("\n***************** Choice ISE mode **************************\n");
        printf(" [0]:ISE one fish_180 out venc:2048x2048\n");
        printf(" [1]:ISE one fish_360 out venc:4096x1024 [default]\n");
        printf(" [2]:ISE one fish 4PTZ+bottom out venc:4xChn 1024x1024\n");
        printf(" [3]:ISE two fish_360 out venc:3840x1920\n");
        printf(" [4]:ISE two ise_90   out venc:3840x1080\n");
        printf(" Please choose ISE mode ID 0~4 or (q-Quit): ");
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
        if (index < 0 || index > 4) {
            printf(" Input ISE Mode id:%d error!\n", index);
            continue;
        }

        switch(index) {
        case 0:
            *p_cfg_type = MENU_ISE_FISH_180_MODE_2048x2048;
            break;
        case 1:
            *p_cfg_type = MENU_ISE_FISH_360_MODE_4096x1024;
            break;
        case 2:
            *p_cfg_type = MENU_ISE_FISH_BOTTOM_4PTZ_MODE_1024x1024;
            break;
        case 3:
            *p_cfg_type = MENU_ISE_TWO_FISH_360_MODE_3840x1920;
            break;
        case 4:
            *p_cfg_type = MENU_ISE_TWO_ISE_90_MODE_3840x1080;
            break;

        default:
            break;
        }

        return 0;
    }

    return 0;
}

