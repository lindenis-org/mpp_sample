/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_menu_audio.c
 * @brief 该目录是对mpp中的 AUDIO 模块进行菜单设置项控制.
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
static int mpp_menu_ai_volume_set(void *pData, char *pTitle);
static int mpp_menu_ai_volume_get(void *pData, char *pTitle);
static int mpp_menu_ai_mute_set(void *pData, char *pTitle);
static int mpp_menu_ai_mute_get(void *pData, char *pTitle);
static int mpp_menu_ao_volume_set(void *pData, char *pTitle);
static int mpp_menu_ao_volume_get(void *pData, char *pTitle);
static int mpp_menu_ao_mute_set(void *pData, char *pTitle);
static int mpp_menu_ao_mute_get(void *pData, char *pTitle);
static int mpp_menu_ao_chn_pause(void *pData, char *pTitle);
static int mpp_menu_ao_chn_resume(void *pData, char *pTitle);


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static MENU_INODE g_audio_menu_list[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"AI  set Volume ",     mpp_menu_ai_volume_set,  NULL, NULL},
    {(char *)"AI  Get Volume ",     mpp_menu_ai_volume_get,  NULL, NULL},
    {(char *)"AI  set Mute ",       mpp_menu_ai_mute_set,    NULL, NULL},
    {(char *)"AI  Get Mute ",       mpp_menu_ai_mute_get,    NULL, NULL},
    {(char *)"AO  set Volume ",     mpp_menu_ao_volume_set,  NULL, NULL},
    {(char *)"AO  Get Volume ",     mpp_menu_ao_volume_get,  NULL, NULL},
    {(char *)"AO  set Mute ",       mpp_menu_ao_mute_set,    NULL, NULL},
    {(char *)"AO  Get Mute ",       mpp_menu_ao_mute_get,    NULL, NULL},
    {(char *)"AO  set Pause  (ao_chn:0)",  mpp_menu_ao_chn_pause,   NULL, NULL},
    {(char *)"AO  set Resume (ao_chn:0)",  mpp_menu_ao_chn_resume,  NULL, NULL},
    {(char *)"Previous Step (Quit Audio setting)", ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
int mpp_menu_audio_get(PMENU_INODE *pmenu_list_audio)
{
    if (NULL == pmenu_list_audio) {
        DB_PRT("Input pmenu_list_audio is NULL!\n");
        return -1;
    }

    *pmenu_list_audio = g_audio_menu_list;

    return 0;
}


static int mpp_menu_ai_volume_set(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  val      = 0;
    char str[256] = {0};

    while (1) {
        printf("\n\n***************** Set AI volume **************************\n");

        printf("\n Please Input volume val 0~100 or (q-Quit): ");
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
        val = atoi(str);
        if (val > 100) {
            printf(" Input volume val:%d error!\n", val);
            continue;
        }

        ret = AW_MPI_AI_SetDevVolume(0, val);
        if (ret) {
            ERR_PRT(" Do AW_MPI_AI_SetVolume ai_dev:%d Volume:[%d] fail! ret:0x%x\n", 0, val, ret);
        } else {
            DB_PRT(" Do AW_MPI_AI_SetVolume ai_dev:%d Volume:[%d] success! ret:0x%x\n", 0, val, ret);
        }
        return ret;
    }

    return 0;
}

static int mpp_menu_ai_volume_get(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  val      = 0;
    char str[256] = {0};

    printf("\n\n***************** Get AI volume **************************\n");

    ret = AW_MPI_AI_GetDevVolume(0, &val);
    if (ret) {
        ERR_PRT(" Do AW_MPI_AI_GetVolume ai_dev:%d Volume:[%d] fail! ret:0x%x\n", 0, val, ret);
    } else {
        DB_PRT(" Do AW_MPI_AI_GetVolume ai_dev:%d Volume:[%d] success! ret:0x%x\n", 0, val, ret);
    }

    return ret;
}


static int mpp_menu_ai_mute_set(void *pData, char *pTitle)
{
    int  ret   = 0;
    int  index = 0;
    char str[256] = {0};
    BOOL bEnable = FALSE;

    while (1) {
        printf("\n\n***************** Set AI Mute **************************\n");
        printf(" [0]:Disable Mute [default] \n");
        printf(" [1]:Enable  Mute \n");
        printf(" Please choose Mute mode or (q-Quit): \n");
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

        if (0 == index) {
            bEnable = FALSE;
        } else {
            bEnable = TRUE;
        }

        ret = AW_MPI_AI_SetDevMute(0, bEnable);
        if (ret) {
            ERR_PRT(" Do AW_MPI_AI_SetMute ai_dev:%d enalbe_flag:%d fail! ret:0x%x\n", 0, bEnable, ret);
        } else {
            DB_PRT(" Do AW_MPI_AI_SetMute ai_dev:%d enalbe_flag:%d success! ret:0x%x\n", 0, bEnable, ret);
        }

        return ret;
    }

    return 0;
}

static int mpp_menu_ai_mute_get(void *pData, char *pTitle)
{
    int ret = 0;
    BOOL bEnable = FALSE;

    printf("\n\n***************** Get AI Mute **************************\n");

    ret = AW_MPI_AI_GetDevMute(0, &bEnable);
    if (ret) {
        ERR_PRT(" Do AW_MPI_AO_GetMute ai_dev:%d enalbe_flag:[%d] fail! ret:0x%x\n", 0, bEnable, ret);
    } else {
        DB_PRT(" Do AW_MPI_AO_GetMute ai_dev:%d enalbe_flag:[%d] success! ret:0x%x\n", 0, bEnable, ret);
    }

    return ret;
}

static int mpp_menu_ao_volume_set(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  val      = 0;
    char str[256] = {0};

    while (1) {
        printf("\n\n***************** Set AO volume **************************\n");

        printf("\n Please Input volume val 0~100 or (q-Quit): ");
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
        val = atoi(str);
        if (val < 0 || val > 100) {
            printf(" Input volume val:%d error!\n", val);
            continue;
        }

        ret = AW_MPI_AO_SetDevVolume(0, val);
        if (ret) {
            ERR_PRT(" Do AW_MPI_AO_SetVolume ai_dev:%d Volume:[%d] fail! ret:0x%x\n", 0, val, ret);
        } else {
            DB_PRT(" Do AW_MPI_AO_SetVolume ai_dev:%d Volume:[%d] success! ret:0x%x\n", 0, val, ret);
        }
        return ret;
    }

    return 0;
}

static int mpp_menu_ao_volume_get(void *pData, char *pTitle)
{
    int ret = 0;
    int val = 0;

    printf("\n\n***************** Get AO volume **************************\n");

    ret = AW_MPI_AO_GetDevVolume(0, &val);
    if (ret) {
        ERR_PRT(" Do AW_MPI_AO_GetVolume ao_dev:%d Volume:[%d] fail! ret:0x%x\n", 0, val, ret);
    } else {
        DB_PRT(" Do AW_MPI_AO_GetVolume ao_dev:%d Volume:[%d] success! ret:0x%x\n", 0, val, ret);
    }

    return ret;
}


static int mpp_menu_ao_mute_set(void *pData, char *pTitle)
{
    int  ret   = 0;
    int  index = 0;
    char str[256] = {0};
    BOOL bEnable = FALSE;

    while (1) {
        printf("\n\n***************** Set AO Mute **************************\n");
        printf(" [0]:Disable Mute [default] \n");
        printf(" [1]:Enable  Mute \n");
        printf(" Please choose Mute mode or (q-Quit): \n");
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

        if (0 == index) {
            bEnable = FALSE;
        } else {
            bEnable = TRUE;
        }

        ret = AW_MPI_AO_SetDevMute(0, bEnable, NULL);
        if (ret) {
            ERR_PRT(" Do AW_MPI_AO_SetMute ao_dev:%d enalbe_flag:%d fail! ret:0x%x\n", 0, bEnable, ret);
        } else {
            DB_PRT(" Do AW_MPI_AO_SetMute ao_dev:%d enalbe_flag:%d success! ret:0x%x\n", 0, bEnable, ret);
        }

        return ret;
    }

    return 0;
}

static int mpp_menu_ao_mute_get(void *pData, char *pTitle)
{
    int ret = 0;
    BOOL bEnable = FALSE;

    printf("\n\n***************** Get AO Mute **************************\n");

    ret = AW_MPI_AO_GetDevMute(0, &bEnable, NULL);
    if (ret) {
        ERR_PRT(" Do AW_MPI_AO_GetMute ao_dev:%d enalbe_flag:[%d] fail! ret:0x%x\n", 0, bEnable, ret);
    } else {
        DB_PRT(" Do AW_MPI_AO_GetMute ao_dev:%d enalbe_flag:[%d] success! ret:0x%x\n", 0, bEnable, ret);
    }

    return ret;
}


static int mpp_menu_ao_chn_pause(void *pData, char *pTitle)
{
    int ret = 0;

    printf("\n\n***************** Pause AO channel **************************\n");

    ret = AW_MPI_AO_PauseChn(0, 0);
    if (ret) {
        ERR_PRT(" Do AW_MPI_AO_PauseChn ao_dev:%d ao_chn:[%d] fail! ret:0x%x\n", 0, 0, ret);
    } else {
        DB_PRT(" Do AW_MPI_AO_PauseChn ao_dev:%d ao_chn:[%d] success! ret:0x%x\n", 0, 0, ret);
    }

    return ret;
}

static int mpp_menu_ao_chn_resume(void *pData, char *pTitle)
{
    int ret = 0;

    printf("\n\n***************** Resume AO channel **************************\n");

    ret = AW_MPI_AO_ResumeChn(0, 0);
    if (ret) {
        ERR_PRT(" Do AW_MPI_AO_ResumeChn ao_dev:%d ao_chn:[%d] fail! ret:0x%x\n", 0, 0, ret);
    } else {
        DB_PRT(" Do AW_MPI_AO_ResumeChn ao_dev:%d ao_chn:[%d] success! ret:0x%x\n", 0, 0, ret);
    }

    return ret;
}

