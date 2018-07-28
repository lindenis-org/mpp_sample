/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_menu_vo.c
 * @brief 该目录是对mpp中的 VO 模块进行菜单设置项控制.
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
static int mpp_menu_vo_change_display(void *pData, char *pTitle);
static int mpp_menu_vo_show_layer(void *pData, char *pTitle);
static int mpp_menu_vo_hide_layer(void *pData, char *pTitle);
static int mpp_menu_vo_switch_to_lcd(void *pData, char *pTitle);
static int mpp_menu_vo_switch_to_hdmi(void *pData, char *pTitle);
static int mpp_menu_vo_switch_to_cvbs(void *pData, char *pTitle);
static int mpp_menu_vo_show_layer(void *pData, char *pTitle);
static int mpp_menu_vo_hide_layer(void *pData, char *pTitle);
static int mpp_menu_vo_pause_layer(void *pData, char *pTitle);
static int mpp_menu_vo_resume_layer(void *pData, char *pTitle);
static int mpp_menu_vo_dispBuff_num_set(void *pData, char *pTitle);
static int mpp_menu_vo_dispBuff_num_get(void *pData, char *pTitle);

/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static MENU_INODE g_vo_menu_list[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"VO Change display (LCD/HDMI/TV-OUT)",   mpp_menu_vo_change_display,  NULL, NULL},
    {(char *)"VO show Layer",         mpp_menu_vo_show_layer,       NULL, NULL},
    {(char *)"VO Hide Layer",         mpp_menu_vo_hide_layer,       NULL, NULL},
    {(char *)"VO Pause Layer",        mpp_menu_vo_pause_layer,      NULL, NULL},
    {(char *)"VO Resume Layer",       mpp_menu_vo_resume_layer,     NULL, NULL},
    {(char *)"VO set chnDispBuffNum", mpp_menu_vo_dispBuff_num_set, NULL, NULL},
    {(char *)"VO get chnDispBuffNum", mpp_menu_vo_dispBuff_num_get, NULL, NULL},
    {(char *)"Previous Step (Quit VO setting)", ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
int mpp_menu_vo_get(PMENU_INODE *pmenu_list_vo)
{
    if (NULL == pmenu_list_vo) {
        DB_PRT("Input pmenu_list_vo is NULL!\n");
        return -1;
    }

    *pmenu_list_vo = g_vo_menu_list;

    return 0;
}


static int mpp_menu_vo_change_display(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  device   = 0;
    char str[256] = {0};
    char *model[] = {"LCD","HDMI","CVBS"};

    while (1) {
        printf("\n\n***************** Change VO Display **************************\n");
        printf("[0]-LCD (default)\n");
        printf("[1]-HDMI\n");
        printf("[2]-CVBS\n");
        printf(" Please Input VO display device [0]~[2] or (q-Quit): ");
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
        if (device < 0 || device > 2) {
            printf(" Input VO Display id:%d error!\n", device);
            continue;
        }

        switch (device) {
        case 0: {
            ret = mpp_menu_vo_switch_to_lcd(NULL,NULL);
            if (0 != ret) {
                goto error_change_display;
            }
        }
        break;
        case 1: {
            ret = mpp_menu_vo_switch_to_hdmi(NULL,NULL);
            if (0 != ret) {
                goto error_change_display;
            }
        }
        break;
        case 2: {
            ret = mpp_menu_vo_switch_to_cvbs(NULL,NULL);
            if (0 != ret) {
                goto error_change_display;
            }
        }
        break;
        default:
            break;
        }
        printf("VO Display changed to %s success \n",model[device]);
        return 0;
    }

error_change_display:
    printf("VO Display changed to %s fault\n",model[device]);
    return -1;
}

static int mpp_menu_vo_switch_to_lcd(void *pData, char *pTitle)
{
    int  ret = 0;

    printf("LCD default resolution: VO_OUTPUT_720P50");
    VO_INTF_TYPE_E  disp_type = VO_INTF_LCD;        //DISP_OUTPUT_TYPE_LCD;
    VO_INTF_SYNC_E  tv_mode   = VO_OUTPUT_720P50;   //VO_OUTPUT_720P50;

    /*first,get pub attr*/
    VO_PUB_ATTR_S stPubAttr;
    ret = AW_MPI_VO_GetPubAttr(0, &stPubAttr);
    if (0 == ret) {
        DB_PRT("Do AW_MPI_VO_GetPubAttr succeed VO_DEV:0  ret:%d \n",ret);

        /*second,set pub attr*/
        stPubAttr.enIntfType = disp_type;
        stPubAttr.enIntfSync = tv_mode;
        ret = AW_MPI_VO_SetPubAttr(0, &stPubAttr);
        if (0 ==ret) {
            DB_PRT("Do AW_MPI_VO_SetPubAttr succeed VO_DEV:0  ret:%d \n",ret);
            return 0;
        } else {
            ERR_PRT("Do AW_MPI_VO_GetPubAttr fail VO_DEV:0  ret:%d \n", ret);
            return -1;
        }
    } else {
        ERR_PRT("Do AW_MPI_VO_GetPubAttr fail VO_DEV:0  ret:%d \n", ret);
        return -1;
    }
    return 0;
}

static int mpp_menu_vo_switch_to_hdmi(void *pData, char *pTitle)
{
    int  ret        = 0;
    int  resolution = 0;
    char str[256]   = {0};

    while (1) {
        printf("\n\n***************** HDMI Resolution setting ************************\n");
        printf("[0]-1080P60 (default)\n");
        printf("[1]-1080P50\n");
        printf("[2]-1080I60\n");
        printf("[3]-1080I50\n");
        printf("[4]-720P60\n");
        printf("[5]-720P50\n");
        printf(" Please Input VO HDMI Resolution [0]~[2] or (q-Quit): ");
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

        resolution = atoi(str);
        if (resolution < 0 || resolution > 6) {
            printf(" Input HDMI Resolution:%d error!\n", resolution );
            continue;
        }

        VO_INTF_TYPE_E  disp_type = VO_INTF_HDMI;
        VO_INTF_SYNC_E  tv_mode;

        switch (resolution) {
        case 0:
            tv_mode   = VO_OUTPUT_1080P60;
            break;
        case 1:
            tv_mode   = VO_OUTPUT_1080P50;
            break;
        case 2:
            tv_mode   = VO_OUTPUT_1080I60;
            break;
        case 3:
            tv_mode   = VO_OUTPUT_1080I50;
            break;
        case 4:
            tv_mode   = VO_OUTPUT_720P60;
            break;
        case 5:
            tv_mode   = VO_OUTPUT_720P50;
            break;
        default:
            break;
        }
        /*first,get pub attr*/
        VO_PUB_ATTR_S stPubAttr;
        ret = AW_MPI_VO_GetPubAttr(0, &stPubAttr);
        if (0 == ret) {
            DB_PRT("Do AW_MPI_VO_GetPubAttr succeed VO_DEV:0  ret:%d \n",ret);

            /*second,set pub attr*/
            stPubAttr.enIntfType = disp_type;
            stPubAttr.enIntfSync = tv_mode;
            ret = AW_MPI_VO_SetPubAttr(0, &stPubAttr);
            if (0 == ret) {
                DB_PRT("Do AW_MPI_VO_SetPubAttr succeed VO_DEV:0  ret:%d \n",ret);
                return 0;
            } else {
                ERR_PRT("Do AW_MPI_VO_GetPubAttr fail VO_DEV:0  ret:%d \n", ret);
                return -1;
            }
        } else {
            ERR_PRT("Do AW_MPI_VO_GetPubAttr fail VO_DEV:0  ret:%d \n", ret);
            return -1;
        }
    }
    return 0;
}

static int mpp_menu_vo_switch_to_cvbs(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  type     = 0;
    char str[256] = {0};
    while (1) {
        printf("\n\n***************** CVBS System setting ************************\n");
        printf("[0]-PAL (default)\n");
        printf("[1]-NTSC\n");
        printf(" Please Input VO HDMI Resolution [0]~[2] or (q-Quit): ");
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

        type = atoi(str);
        if (type < 0 || type > 2) {
            printf(" Input CVBS System id:%d error!\n", type);
            continue;
        }

        VO_INTF_TYPE_E  disp_type = VO_INTF_CVBS;
        VO_INTF_SYNC_E  tv_mode;

        switch (type) {
        case 0:
            tv_mode   = VO_OUTPUT_PAL;
            break;
        case 1:
            tv_mode   = VO_OUTPUT_NTSC;
            break;
        default:
            break;
        }
        /*first,get pub attr*/
        VO_PUB_ATTR_S stPubAttr;
        ret = AW_MPI_VO_GetPubAttr(0, &stPubAttr);
        if (0 == ret) {
            DB_PRT("Do AW_MPI_VO_GetPubAttr succeed VO_DEV:0  ret:%d \n",ret);

            /*second,set pub attr*/
            stPubAttr.enIntfType = disp_type;
            stPubAttr.enIntfSync = tv_mode;
            ret = AW_MPI_VO_SetPubAttr(0, &stPubAttr);
            if (0 ==ret) {
                DB_PRT("Do AW_MPI_VO_SetPubAttr succeed VO_DEV:0  ret:%d \n",ret);
                return 0;
            } else {
                ERR_PRT("Do AW_MPI_VO_GetPubAttr fail VO_DEV:0  ret:%d \n", ret);
                return -1;
            }
        } else {
            ERR_PRT("Do AW_MPI_VO_GetPubAttr fail VO_DEV:0  ret:%d \n", ret);
            return -1;
        }
    }
    return 0;
}

static int mpp_menu_vo_show_layer(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  layer    = 0;
    char str[256] = {0};

    while (1) {
        printf("\n\n***************** Show Layer **************************\n");
        printf("Show Layer 0~15 \n");
        printf(" Please input val [0]~[15] or (q-Quit): ");
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

        layer = atoi(str);
        if (layer < 0 || layer > 15) {
            printf(" Input VO Display id:%d error!\n", layer);
            continue;
        }

        ret = AW_MPI_VO_ShowChn(layer,0);
        if(0 == ret) {
            DB_PRT("Do AW_MPI_VO_ShowChn succeed VO_LAYER:%d ret:%d \n",layer,ret);
            printf("Layer %d has showed",layer);
            return 0;
        } else {
            ERR_PRT("Do AW_MPI_VO_ShowChn fail VO_DEV:0  ret:%d \n", ret);
            return -1;
        }
        return 0;
    }
}

static int mpp_menu_vo_hide_layer(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  layer    = 0;
    char str[256] = {0};

    while (1) {
        printf("\n\n***************** Hide Layer **************************\n");
        printf("Hide Layer 0~15 \n");
        printf(" Please input val [0]~[15] or (q-Quit): ");
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

        layer = atoi(str);
        if (layer < 0 || layer > 15) {
            printf(" Input VO Layer id:%d error!\n", layer);
            continue;
        }

        ret = AW_MPI_VO_HideChn(layer,0);
        if(0 == ret) {
            DB_PRT("Do AW_MPI_VO_HideChn succeed VO_LAYER:%d ret:%d \n",layer,ret);
            printf("Layer %d has Hided",layer);
            return 0;
        } else {
            ERR_PRT("Do AW_MPI_VO_HideChn fail VO_DEV:0  ret:%d \n", ret);
            return -1;
        }
        return 0;
    }
}

static int mpp_menu_vo_pause_layer(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  layer      = 0;
    char str[256] = {0};

    while (1) {
        printf("\n\n***************** Pause Layer **************************\n");
        printf("Hide Layer 0~15 \n");
        printf(" Please input val [0]~[15] or (q-Quit): ");
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

        layer = atoi(str);
        if (layer < 0 || layer > 15) {
            printf(" Input VO Layer id:%d error!\n", layer);
            continue;
        }

        ret = AW_MPI_VO_PauseChn(layer,0);
        if (0 == ret) {
            DB_PRT("Do AW_MPI_VO_PauseChn succeed VO_LAYER:%d ret:%d \n",layer,ret);
            printf("Layer %d has Pause",layer);
            return 0;
        } else {
            ERR_PRT("Do AW_MPI_VO_PauseChn fail VO_DEV:0  ret:%d \n", ret);
            return -1;
        }
        return 0;
    }
}

static int mpp_menu_vo_resume_layer(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  layer      = 0;
    char str[256] = {0};

    while (1) {
        printf("\n\n***************** Pause Layer **************************\n");
        printf("Hide Layer 0~15 \n");
        printf(" Please input val [0]~[15] or (q-Quit): ");
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

        layer = atoi(str);
        if (layer < 0 || layer > 15) {
            printf(" Input VO Layer id:%d error!\n", layer);
            continue;
        }

        ret = AW_MPI_VO_ResumeChn(layer,0);
        if (0 == ret) {
            DB_PRT("Do AW_MPI_VO_ResumeChn succeed VO_LAYER:%d ret:%d \n",layer,ret);
            printf("Layer %d has Resume",layer);
            return 0;
        } else {
            ERR_PRT("Do AW_MPI_VO_ResumeChn fail VO_DEV:0  ret:%d \n", ret);
            return -1;
        }
        return 0;
    }

}

static int mpp_menu_vo_dispBuff_num_set(void *pData, char *pTitle)
{
    int  ret      = 0;
    int  val      = 0;
    int  layer    = 0;
    char str[256] = {0};

    while (1) {
        printf("\n\n***************** set ChnDispBuff_Num **************************\n");
        printf(" Description: vo channel cache display frame buf number.\n");
        printf(" Step 1 : set display Layer\n");
        printf(" Please input val [0]~[15] or (q-Quit): ");
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

        layer = atoi(str);
        if (layer < 0 || layer > 15) {
            printf(" Input VO Layer id:%d error!\n", layer);
            continue;
        }
        printf("You has chose layer : %d \n\n",layer);

        while(1) {
            printf("Step 2:set ChnDispBuff_Num\n");
            printf("Buffer Number : 0~  (default:0)\n");
            printf("Please input val  or (q-Quit): ");
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
            if (val < 0) {
                printf(" Input VO Display buffer Number id:%d error!\n", val);
                continue;
            }

            ret = AW_MPI_VO_SetChnDispBufNum(layer,0,val);
            if(0 == ret) {
                DB_PRT("Do AW_MPI_VO_SetChnDispBufNum succeed VO_LAYER:%d ret:%d \n",val,ret);
                return 0;
            } else {
                ERR_PRT("Do AW_MPI_VO_SetChnDispBufNum fail VO_DEV:0  ret:%d \n", ret);
                return -1;
            }
        }
    }
}


static int mpp_menu_vo_dispBuff_num_get(void *pData, char *pTitle)
{
    int          ret    = 0;
    int          layer    = 0;
    unsigned int Number = 0;
    char str[256] = {0};

    while(1) {
        printf("\n\n***************** get ChnDispBuff_Num **************************\n");
        printf("Description: vo channel cache display frame buf number.\n");
        printf("setting layer to get BuffNumber\n");
        printf("Please input layer 0~15 or (q-Quit): ");
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

        layer = atoi(str);
        if (layer < 0 || layer > 15) {
            printf(" Input VO Layer id:%d error!\n", layer);
            continue;
        }

        ret = AW_MPI_VO_GetChnDispBufNum(layer, 0, &Number);
        if(0 == ret) {
            DB_PRT("Do AW_MPI_VO_GetChnDispBufNum succeed VO_LAYER:%d ret:%d \n",layer,ret);
            printf("Layer: %d, Display-Buffer-Number: %d \n",layer,Number);
            return 0;
        } else {
            ERR_PRT("Do AW_MPI_VO_GetChnDispBufNum fail VO_DEV:0  ret:%d \n", ret);
            return -1;
        }
    }

}

