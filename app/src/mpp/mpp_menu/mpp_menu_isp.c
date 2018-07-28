/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_menu_isp.c
 * @brief 该目录是对mpp中的ISP模块进行菜单设置项控制.
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
static int mpp_menu_isp_set_ispdev(void *pData, char *pTitle);
static int mpp_menu_isp_brightness_set(void *pData, char *pTitle);
static int mpp_menu_isp_brightness_get(void *pData, char *pTitle);
static int mpp_menu_isp_contrast_set(void *pData, char *pTitle);
static int mpp_menu_isp_contrast_get(void *pData, char *pTitle);
static int mpp_menu_isp_saturation_set(void *pData, char *pTitle);
static int mpp_menu_isp_saturation_get(void *pData, char *pTitle);
static int mpp_menu_isp_sharpness_set(void *pData, char *pTitle);
static int mpp_menu_isp_sharpness_get(void *pData, char *pTitle);
static int mpp_menu_isp_flicker_set(void *pData, char *pTitle);
static int mpp_menu_isp_flicker_get(void *pData, char *pTitle);
static int mpp_menu_isp_colorTemp_set(void *pData, char *pTitle);
static int mpp_menu_isp_colorTemp_get(void *pData, char *pTitle);
static int mpp_menu_isp_exposure_set(void *pData, char *pTitle);
static int mpp_menu_isp_exposure_get(void *pData, char *pTitle);
static int mpp_menu_isp_exposure_time_set(void *pData, char *pTitle);
static int mpp_menu_isp_exposure_gain_set(void *pData, char *pTitle);
static int mpp_menu_isp_PltmWDR_set(void *pData, char *pTitle);
static int mpp_menu_isp_PltmWDR_get(void *pData, char *pTitle);
static int mpp_menu_isp_3NRAttr_set(void *pData, char *pTitle);
static int mpp_menu_isp_3NRAttr_get(void *pData, char *pTitle);
static int ircut_control(void *pData, char *pTitle);


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static MENU_INODE g_isp_menu_list[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"isp set device num",    mpp_menu_isp_set_ispdev,       NULL, NULL},
    {(char *)"isp get brightness",    mpp_menu_isp_brightness_get,   NULL, NULL},
    {(char *)"isp set brightness",    mpp_menu_isp_brightness_set,   NULL, NULL},
    {(char *)"isp get contrast",      mpp_menu_isp_contrast_get,     NULL, NULL},
    {(char *)"isp set contrast",      mpp_menu_isp_contrast_set,     NULL, NULL},
    {(char *)"isp get saturation",    mpp_menu_isp_saturation_get,   NULL, NULL},
    {(char *)"isp set saturation",    mpp_menu_isp_saturation_set,   NULL, NULL},
    {(char *)"isp get sharpness",     mpp_menu_isp_sharpness_get,    NULL, NULL},
    {(char *)"isp set sharpness",     mpp_menu_isp_sharpness_set,    NULL, NULL},
    {(char *)"isp get flicker",       mpp_menu_isp_flicker_get,      NULL, NULL},
    {(char *)"isp set flicker",       mpp_menu_isp_flicker_set,      NULL, NULL},
    {(char *)"isp get colorTemp",     mpp_menu_isp_colorTemp_get,    NULL, NULL},
    {(char *)"isp set colorTemp",     mpp_menu_isp_colorTemp_set,    NULL, NULL},
    {(char *)"isp get exposure",      mpp_menu_isp_exposure_get,     NULL, NULL},
    {(char *)"isp set exposure",      mpp_menu_isp_exposure_set,     NULL, NULL},
    {(char *)"isp get PltmWDR",       mpp_menu_isp_PltmWDR_get,      NULL, NULL},
    {(char *)"isp set PltmWDR",       mpp_menu_isp_PltmWDR_set,      NULL, NULL},
    {(char *)"isp get 3NRAttr",       mpp_menu_isp_3NRAttr_get,      NULL, NULL},
    {(char *)"isp set 3NRAttr",       mpp_menu_isp_3NRAttr_set,      NULL, NULL},
    {(char *)"IRcut control for bc_ipc", ircut_control,              NULL, NULL},
    {(char *)"Previous Step (Quit ISP setting)", ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};

static ISP_DEV g_isp_dev = 0;


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
int mpp_menu_isp_get(PMENU_INODE *pmenu_list_isp)
{
    if (NULL == pmenu_list_isp) {
        DB_PRT("Input menu_list_isp is NULL!\n");
        return -1;
    }

    *pmenu_list_isp = g_isp_menu_list;

    return 0;
}

static int mpp_menu_isp_set_ispdev(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  isp_dev   = 0;
    char str[256]  = {0};

    while (1) {
        printf("\n**************** Setting ISP device Number *************************\n");
        printf(" Please Input ISP_DEV 0 or 1(defualte:0) or (q-Quit): ");

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

        isp_dev = atoi(str);
        if (0 == isp_dev || isp_dev == 1) {
            g_isp_dev = isp_dev;
            printf(" Setting ISP_DEV:%d ok!\n", isp_dev);
            return 0;
        } else {
            printf(" Input ISP_DEV:%d error!\n", isp_dev);
            continue;
        }
    }

    return 0;
}


static int mpp_menu_isp_brightness_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    char str[256]  = {0};
    while (1) {
        printf("\n**************** Setting ISP brightness *************************\n");
        printf(" Please Input brightness val 0~252 or (q-Quit): ");

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
        if (0 <= val && val <= 252) {
            /*input 0~252,but the SetBrightness() need -126~126, so switch value */
            val = val - 126;
            ret = AW_MPI_ISP_SetBrightness(g_isp_dev, val);
            if (0 == ret) {
                DB_PRT("Do AW_MPI_ISP_SetBrightness succeed isp_dev:%d  value:%d  ret:%d \n",
                       g_isp_dev, val, ret);
                return 0;
            } else {
                ERR_PRT("Do AW_MPI_ISP_SetBrightness fail isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }
        } else {
            printf(" Input Brightness value:%d error!\n", val);
            continue;
        }
    }
}

static int mpp_menu_isp_brightness_get(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;

    printf("\n**************** Get ISP device Number *************************\n");
    ret = AW_MPI_ISP_GetBrightness(g_isp_dev, &val);
    if (0 == ret) {
        DB_PRT("Do AW_MPI_ISP_GetBrightness succeed isp_dev:%d  (value:%d)  ret:%d \n",
               g_isp_dev, (val+126), ret);
        return 0;
    } else {
        ERR_PRT("Do AW_MPI_ISP_GetBrightness fail isp_dev:%d  (value:%d)  ret:%d \n",
                g_isp_dev, (val+126), ret);
        return -1;
    }
}

static int mpp_menu_isp_contrast_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    char str[256]  = {0};
    while (1) {
        printf("\n**************** Setting ISP contrast *************************\n");
        printf(" Please Input contrast val 0~252 or (q-Quit): ");

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
        if (0 <= val && val <= 252) {
            /*input 0~252,but the SetContrast() need -126~126, so switch value */
            val = val - 126;
            ret = AW_MPI_ISP_SetContrast(g_isp_dev, val);
            if (0 == ret) {
                DB_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetContrast succeed isp_dev:%d  value:%d  ret:%d \n",
                       g_isp_dev, val, ret);
                return 0;
            } else {
                ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetContrast fail isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }
        } else {
            printf(" Input Contrast value:%d error!\n", val);
            continue;
        }
    }
}

static int mpp_menu_isp_contrast_get(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;

    printf("\n**************** Get ISP contrast *************************\n");
    ret = AW_MPI_ISP_GetContrast(g_isp_dev, &val);
    if (0 == ret) {
        DB_PRT("Do AW_MPI_ISP_GetContrast succeed isp_dev:%d  (value:%d)  ret:%d \n",
               g_isp_dev, (val+126), ret);
        return 0;
    } else {
        ERR_PRT("Do AW_MPI_ISP_GetBrightness fail isp_dev:%d  (value:%d)  ret:%d \n",
                g_isp_dev, (val+126), ret);
        return -1;
    }
}

static int mpp_menu_isp_saturation_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    char str[256]  = {0};
    while (1) {
        printf("\n**************** Setting ISP saturation *************************\n");
        printf(" Please Input saturation val 0~200 or (q-Quit): ");

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
        if (0 <= val && val <= 200) {
            /*input 0~200,but the SetContrast() need -100~100, so switch value */
            val = val - 100;
            ret = AW_MPI_ISP_SetSaturation(g_isp_dev, val);
            if (0 == ret) {
                DB_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetSaturation succeed isp_dev:%d  value:%d  ret:%d \n",
                       g_isp_dev, (val+100), ret);
                return 0;
            } else {
                ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetSaturation fail isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, (val+100), ret);
                return -1;
            }
        } else {
            printf(" Input Saturation value:%d error!\n", val);
            continue;
        }
    }
}

static int mpp_menu_isp_saturation_get(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;

    printf("\n**************** Get ISP saturation *************************\n");
    ret = AW_MPI_ISP_GetSaturation(g_isp_dev, &val);
    if (0 == ret) {
        DB_PRT("Do AW_MPI_ISP_GetSaturation succeed isp_dev:%d  (value:%d)  ret:%d \n",
               g_isp_dev, (val+100), ret);
        return 0;
    } else {
        ERR_PRT("Do AW_MPI_ISP_GetSaturation fail isp_dev:%d  (value:%d)  ret:%d \n",
                g_isp_dev, (val+100), ret);
        return -1;
    }
}
static int mpp_menu_isp_sharpness_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    char str[256]  = {0};
    while (1) {
        printf("\n**************** Setting ISP sharpness *************************\n");
        printf(" Please Input sharpness val 0~10 or (q-Quit): ");

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
        if (0 <= val && val <= 10) {
            ret = AW_MPI_ISP_SetSharpness(g_isp_dev, val);
            if (0 == ret) {
                DB_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetSharpness succeed isp_dev:%d  value:%d  ret:%d \n",
                       g_isp_dev, val, ret);
                return 0;
            } else {
                ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetSharpness fail isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }
        } else {
            printf(" Input sharpness value:%d error!\n", val);
            continue;
        }
    }
}

static int mpp_menu_isp_sharpness_get(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;

    printf("\n**************** Get ISP sharpness *************************\n");
    ret = AW_MPI_ISP_GetSharpness(g_isp_dev, &val);
    if (0 == ret) {
        DB_PRT("Do AW_MPI_ISP_GetSharpness succeed isp_dev:%d  (value:%d)  ret:%d \n",
               g_isp_dev, val, ret);
        return 0;
    } else {
        ERR_PRT("Do AW_MPI_ISP_GetSharpness fail isp_dev:%d  (value:%d)  ret:%d \n",
                g_isp_dev, val, ret);
        return -1;
    }
}

static int mpp_menu_isp_flicker_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    char str[256]  = {0};
    while (1) {
        printf("\n**************** Setting ISP flicker *************************\n");
        printf("[0]-Disable\n");
        printf("[1]-50Hz\n");
        printf("[2]-60Hz\n");
        printf("[3]-Auto\n");
        printf(" Please Input  flip value [0]~[3] or (q-Quit): ");

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
        if (0 <= val && val <= 3) {
            ret = AW_MPI_ISP_SetFlicker(g_isp_dev, val);
            if (0 == ret) {
                DB_PRT("Do AW_MPI_ISP_AW_MPI_ISPSetFlicker succeed isp_dev:%d  value:%d  ret:%d \n",
                       g_isp_dev, val, ret);
                return 0;
            } else {
                ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetFlicker fail isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }
        } else {
            printf(" Input flicker value:%d error!\n", val);
            continue;
        }
    }
}

static int mpp_menu_isp_flicker_get(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;

    printf("\n**************** Get ISP mirror *************************\n");
    ret = AW_MPI_ISP_GetFlicker(g_isp_dev, &val);
    if (0 == ret) {
        DB_PRT("Do AW_MPI_ISP_GetFlicker succeed isp_dev:%d  (value:%d)  ret:%d \n",
               g_isp_dev, val, ret);
        switch(val) {
        case 0: {
            printf("flicker status : Disable\n");
            break;
        }
        case 1: {
            printf("flicker status : 50Hz\n");
            break;
        }
        case 2: {
            printf("flicker status : 60Hz\n");
            break;
        }
        case 3: {
            printf("flicker status : Auto\n");
            break;
        }
        default:
            break;
        }
        return 0;
    } else {
        ERR_PRT("Do AW_MPI_ISP_GetFlicker fail isp_dev:%d  (value:%d)  ret:%d \n",
                g_isp_dev, val, ret);
        return -1;
    }
}

static int mpp_menu_isp_colorTemp_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    char str[256]  = {0};
    ISP_MODULE_ONOFF ModuleOnOff;

    while(1) {
        printf("\n**************** Setting ISP colorTemp on/off *************************\n");
        printf("[0]-OFF\n");
        printf("[1]-ON\n");
        printf(" Please Input colorTemp on/off 0 or 1 or (q-Quit): ");

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
            /*first get all-On/Off status*/
            ret = AW_MPI_ISP_GetModuleOnOff(g_isp_dev, &ModuleOnOff);
            if (0 != ret) {
                ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_GetModuleOnOff fail isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }

            /*then,set whilte balance On/Off status*/
            /*judge input value is deiffent from current value.if same,pass setting*/
            if(ModuleOnOff.wb != val) {
                ModuleOnOff.manual = 1;
                ModuleOnOff.wb     = val;
                ret = AW_MPI_ISP_SetModuleOnOff(g_isp_dev, &ModuleOnOff);
                if (0 == ret) {
                    DB_PRT("Do AW_MPI_ISP_AW_MPI_SetModuleOnOff succeed isp_dev:%d  value:%d  ret:%d \n",
                           g_isp_dev, val, ret);
                } else {
                    ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetColorTemp on/off fail  isp_dev:%d  value:%d  ret:%d \n",
                            g_isp_dev, val, ret);
                    return -1;
                }
            }

            if (1 == val) { //manual model
                /*Into model of setting, when enabled whilte balance.*/
                goto set_colorTem_model;
            } else {
                return 0;
            }

        } else {
            printf(" Input colorTemp value:%d error!\n", val);
            continue;
        }
    }

set_colorTem_model:
    while (1) {
        printf("\n**************** Setting ISP colorTemp model *************************\n");
        printf("[0]-Manual\n");
        printf("[1]-Auto\n");
        printf("[2]-Incandescent\n");
        printf("[3]-fluorescent\n");
        printf("[4]-fluorescent_h\n");
        printf("[5]-Horizon\n");
        printf("[6]-Daylight\n");
        printf("[7]-Flash\n");
        printf("[8]-Cloudy\n");
        printf("[9]-Shade\n");
        printf(" Please Input colorTemp value [0]~[9] or (q-Quit): ");

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
        if (0 <= val && val <= 9) {
            ret = AW_MPI_ISP_AWB_SetColorTemp(g_isp_dev, val);
            if (0 == ret) {
                DB_PRT("Do AW_MPI_ISP_AW_MPI_SetColorTemp model succeed isp_dev:%d  value:%d  ret:%d \n",
                       g_isp_dev, val, ret);
                return 0;
            } else {
                ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetColorTemp model fail isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }
        } else {
            printf(" Input colorTemp value:%d error!\n", val);
            continue;
        }
    }
}

static int mpp_menu_isp_colorTemp_get(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    ISP_MODULE_ONOFF ModuleOnOff;
    char *module[] = {"Manual","Auto","Incandescent","fluorescent",
                      "fluorescent_h","Horizon","Daylight","Flash","Cloudy","Shade"
                     };


    printf("\n**************** Get ISP colorTemp *************************\n");

    /*first get all-On/Off status*/
    ret = AW_MPI_ISP_GetModuleOnOff(g_isp_dev, &ModuleOnOff);
    if (0 == ret) {
        DB_PRT("Do AW_MPI_ISP_GetColorTemp succeed isp_dev:%d  (value:%d)  ret:%d \n",
               g_isp_dev, val, ret);

        if(1 == ModuleOnOff.wb) {
            /*then,get whilte balance model,when enabled whilte balance.*/
            ret = AW_MPI_ISP_AWB_GetColorTemp(g_isp_dev, &val);
            if(0 == ret) {
                printf("colorTemp on/off status : ON \n");
                printf("colorTemp model : %s\n",module[val]);
                return 0;
            } else {
                ERR_PRT("Do AW_MPI_ISP_GetColorTemp module on/off fail isp_dev:%d  (value:%d)  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }

        } else {
            printf("colorTemp on/off status : OFF\n");
        }

    } else {
        ERR_PRT("Do AW_MPI_ISP_GetColorTemp fail isp_dev:%d  (value:%d)  ret:%d \n",
                g_isp_dev, val, ret);
        return -1;
    }

    return 0;
}

static int mpp_menu_isp_exposure_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    int  curval    = 0;
    char str[256]  = {0};
    ISP_MODULE_ONOFF ModuleOnOff;

    while(1) {
        printf("\n**************** Setting ISP exposure on/off *************************\n");
        printf("[0]-OFF\n");
        printf("[1]-ON\n");
        printf(" Please Input exposure on/off val [0]~[1] or (q-Quit): ");

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
        if (0 <= val && val <= 1) {
            /*first get all-On/Off status*/
            ret = AW_MPI_ISP_GetModuleOnOff(g_isp_dev, &ModuleOnOff);
            if (0 != ret) {
                ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetExposure on/off fail isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }

            /*judge input value is deiffent from current value.if same,pass setting*/
            if(ModuleOnOff.ae != val) {
                ModuleOnOff.manual = 1;
                ModuleOnOff.ae = val;
                ret = AW_MPI_ISP_SetModuleOnOff(g_isp_dev, &ModuleOnOff);
                if (0 == ret) {
                    DB_PRT("Do AW_MPI_ISP_AW_MPI_SetExposure on/off succeed isp_dev:%d  value:%d  ret:%d \n",
                           g_isp_dev, val, ret);
                } else {
                    ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetExposure on/off fail  isp_dev:%d  value:%d  ret:%d \n",
                            g_isp_dev, val, ret);
                    return -1;
                }
            }

            if (1 == val) { // if Module is on
                /*Into model of setting, when enabled Exposure.*/
                goto set_exposure_model;
            } else {
                return 0;
            }

        } else {
            printf(" Input exposure value:%d error!\n", val);
            continue;
        }
    }

set_exposure_model:
    while(1) {
        printf("\n**************** Setting ISP exposure model *************************\n");
        printf("[0]Auto exposure \n");
        printf("[1]Manual exposure \n");
        printf(" Please Input exposure model val [0]~[1] or (q-Quit): ");

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
        if (0 <= val && val <= 1) {
            ret = AW_MPI_ISP_AE_GetMode(g_isp_dev, &curval);
            if(0 != ret) {
                ERR_PRT("Do AW_MPI_ISP_AE_GetMode fail isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }
            /*judge input value is deiffent from current value.if same,pass setting*/
            if(curval != val) {
                ret = AW_MPI_ISP_AE_SetMode(g_isp_dev, val);
                if (0 == ret) {
                    DB_PRT("Do AW_MPI_ISP_SetMode succeed isp_dev:%d  value:%d  ret:%d \n",
                           g_isp_dev, val, ret);
                } else {
                    ERR_PRT("Do AW_MPI_ISP_SetMode fail isp_dev:%d  value:%d  ret:%d \n",
                            g_isp_dev, val, ret);
                    return -1;
                }
            }

            if (1 == val) { //if mode is menual
                /*Into option of setting, when set Manual exposure.*/
                goto set_exposure_option;
            } else {
                return 0;
            }
        } else {
            printf(" Input exposure model value:%d error!\n", val);
            continue;
        }
    }

set_exposure_option:
    while(1) {
        printf("\n**************** Setting ISP exposure exposure option *************************\n");
        printf("[0]set exposure time \n");
        printf("[1]set exposure gain \n");
        printf(" Please Input exposure option val [0]~[1] or (q-Quit): ");

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
        if (0 == val) {
            ret = mpp_menu_isp_exposure_time_set(NULL,NULL);
            return ret;
        } else if (1 == val) {
            ret = mpp_menu_isp_exposure_gain_set(NULL,NULL);
            return ret;
        } else {
            printf(" Input exposure value:%d error!\n", val);
            continue;
        }
    }
}

static int mpp_menu_isp_exposure_time_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    char str[256]  = {0};
    while (1) {
        printf("\n**************** Setting ISP exposure time *************************\n");
        printf(" Please Input exposure time val  or (q-Quit): ");

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
        if  (0 <= val ) {
            ret = AW_MPI_ISP_AE_SetExposure(g_isp_dev, val);
            if (0 == ret) {
                DB_PRT("Do AW_MPI_ISP_SetExposure succeed isp_dev:%d  value:%d  ret:%d \n",
                       g_isp_dev, val, ret);
                return 0;
            } else {
                ERR_PRT("Do AW_MPI_ISP_SetExposure fail isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }
        } else {
            printf(" Input exposure time value:%d error!\n", val);
            continue;
        }
    }
}

static int mpp_menu_isp_exposure_gain_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    char str[256]  = {0};
    while (1) {
        printf("\n**************** Setting ISP exposure gain *************************\n");
        printf(" Please Input exposure time val 1600~36000 or (q-Quit): ");

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
        if (1600 <= val && val <= 36000) {
            ret = AW_MPI_ISP_AE_SetGain(g_isp_dev, val);
            if (0 == ret) {
                DB_PRT("Do AW_MPI_ISP_SetGain succeed isp_dev:%d  value:%d  ret:%d \n",
                       g_isp_dev, val, ret);
                return 0;
            } else {
                ERR_PRT("Do AW_MPI_ISP_SetGain fail isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }
        } else {
            printf(" Input exposure gain value:%d error!\n", val);
            continue;
        }
    }

}


static int mpp_menu_isp_exposure_get(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    ISP_MODULE_ONOFF ModuleOnOff;

    printf("\n**************** Get ISP exposure *************************\n");

    /*first get all-On/Off status*/
    ret = AW_MPI_ISP_GetModuleOnOff(g_isp_dev, &ModuleOnOff);
    if (0 == ret) {
        DB_PRT("Do AW_MPI_ISP_GetModuleOnOff succeed isp_dev:%d  (value:%d)  ret:%d \n",
               g_isp_dev, val, ret);

        if (1 == ModuleOnOff.ae) {
            printf("exposure on/off status : ON \n");

            /*second,get exposure model,when enabled exposure.*/
            ret = AW_MPI_ISP_AE_GetMode(g_isp_dev, &val);
            if (0 == ret) {
                /*third,get exposure time/gain value,when model was manual*/
                if (1 == val) {
                    printf("exposure model : Manual \n");
                    ret = AW_MPI_ISP_AE_GetExposure(g_isp_dev, &val);
                    if (0 == ret) {
                        DB_PRT("Do AW_MPI_ISP_GetExposure succeed isp_dev:%d  (value:%d)  ret:%d \n",
                               g_isp_dev, val, ret);
                        printf("exposure time : %d \n", val);
                    } else {
                        ERR_PRT("Do AW_MPI_ISP_GetExposure fail isp_dev:%d  (value:%d)  ret:%d \n",
                                g_isp_dev, val, ret);
                        return -1;
                    }

                    ret = AW_MPI_ISP_AE_GetGain(g_isp_dev, &val);
                    if (0 == ret) {
                        DB_PRT("Do AW_MPI_ISP_GetExposure succeed isp_dev:%d  (value:%d)  ret:%d \n",
                               g_isp_dev, val, ret);
                        printf("exposure gain : %d \n", val);
                    } else {
                        ERR_PRT("Do AW_MPI_ISP_GetExposure fail isp_dev:%d  (value:%d)  ret:%d \n",
                                g_isp_dev, val, ret);
                        return -1;
                    }

                    return 0;
                } else {
                    printf("exposure model : Auto \n");
                    return 0;
                }

            } else {
                ERR_PRT("Do AW_MPI_ISP_GetColorTemp module on/off fail isp_dev:%d  (value:%d)  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }
        } else {
            printf("exposure on/off status : OFF\n");
        }
    } else {
        ERR_PRT("Do AW_MPI_ISP_GetColorTemp fail isp_dev:%d  (value:%d)  ret:%d \n",
                g_isp_dev, val, ret);
        return -1;
    }

    return 0;
}

static int mpp_menu_isp_PltmWDR_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    char str[256]  = {0};
    ISP_MODULE_ONOFF ModuleOnOff;

    while(1) {
        printf("\n**************** Setting ISP PltmWDR *************************\n");
        printf(" Please Input PltmWDR val[0~255] or (q-Quit): ");

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
        if (0 <= val && val <= 255) {
            /*first,get ModuleOnOff status*/
            ret = AW_MPI_ISP_GetModuleOnOff(g_isp_dev, &ModuleOnOff);
            if (0 != ret) {
                ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetModuleOnOff on/off fail  isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }
            /*second,set paramters depend your inpunt value*/
            /*set PltmWDR on/off */
            ret = AW_MPI_ISP_SetModuleOnOff(g_isp_dev, &ModuleOnOff);
            if (0 == ret) {
                DB_PRT("Do AW_MPI_ISP_AW_MPI_SetModuleOnOff on/off succeed isp_dev:%d  value:%d  ret:%d \n",
                       g_isp_dev, val, ret);
                /*set PltmWDR level */
                ret = AW_MPI_ISP_SetPltmWDR(g_isp_dev, val);
                if (0 == ret) {
                    DB_PRT("Do AW_MPI_ISP_AW_MPI_SetPltmWDR succeed isp_dev:%d  value:%d  ret:%d \n",
                           g_isp_dev, val, ret);
                    return 0;

                } else {
                    ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetPltmWDR fail  isp_dev:%d  value:%d  ret:%d \n",
                            g_isp_dev, val, ret);
                    return -1;
                }
            } else {
                ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetModuleOnOff on/off fail  isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }

        } else {
            printf(" Input PltmWDR value:%d error!\n", val);
            continue;
        }
    }
}

static int mpp_menu_isp_PltmWDR_get(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    int  WDR_level = 0;
    ISP_MODULE_ONOFF   ModuleOnOff;

    printf("\n**************** Get ISP PltmWDR *************************\n");
    ret = AW_MPI_ISP_GetModuleOnOff(g_isp_dev, &ModuleOnOff);
    if (0 == ret) {
        DB_PRT("Do AW_MPI_ISP_GetModuleOnOff succeed isp_dev:%d  (value:%d)  ret:%d \n",
               g_isp_dev, val, ret);

        ret = AW_MPI_ISP_GetPltmWDR(g_isp_dev, &WDR_level);
        if (0 == ret) {
            DB_PRT("Do AW_MPI_ISP_SetPltmWDR succeed isp_dev:%d  (value:%d)  ret:%d \n",
                   g_isp_dev, val, ret);
            if (1 == ModuleOnOff.pltm && 1 == ModuleOnOff.manual ) {
                printf("PltmWDR On/Off status : ON \n");
                printf("PltmWDR level : %d \n", WDR_level);
            } else {
                printf("PltmWDR On/Off status : OFF \n");
            }
            return 0;
        } else {
            ERR_PRT("Do AW_MPI_ISP_SetPltmWDR fail isp_dev:%d  (value:%d)  ret:%d \n",
                    g_isp_dev, val, ret);
            return -1;
        }
    } else {
        ERR_PRT("Do AW_MPI_ISP_GetModuleOnOff fail isp_dev:%d  (value:%d)  ret:%d \n",
                g_isp_dev, val, ret);
        return -1;
    }

}

static int mpp_menu_isp_3NRAttr_set(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    char str[256]  = {0};
    ISP_MODULE_ONOFF ModuleOnOff;

    while(1) {
        printf("\n**************** Setting ISP 3NRAttr *************************\n");
        printf(" Please Input saturation val[0~100]or (q-Quit): ");


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
        if (0 <= val && val <= 100) {
            /*first ,get ModuleOnOff status*/
            ret = AW_MPI_ISP_GetModuleOnOff(g_isp_dev, &ModuleOnOff);
            if (0 != ret) {
                ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetModuleOnOff on/off fail  isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }
            /*second, set 3NR on/off paramters depend your input value*/
            /*set 3NRAttr on/off */
            ret = AW_MPI_ISP_SetModuleOnOff(g_isp_dev, &ModuleOnOff);
            if (0 == ret) {
                DB_PRT("Do AW_MPI_ISP_AW_MPI_SetModuleOnOff on/off succeed isp_dev:%d  value:%d  ret:%d \n",
                       g_isp_dev, val, ret);
                /*set PltmWDR level depend your input value*/
                ret = AW_MPI_ISP_Set3NRAttr(g_isp_dev, val);
                if (0 == ret) {
                    DB_PRT("Do AW_MPI_ISP_AW_MPI_Set3NRAttr succeed isp_dev:%d  value:%d  ret:%d \n",
                           g_isp_dev, val, ret);
                    return 0;

                } else {
                    ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_Set3NRAttr fail  isp_dev:%d  value:%d  ret:%d \n",
                            g_isp_dev, val, ret);
                    return -1;
                }
            } else {
                ERR_PRT("Do AW_MPI_ISP_AW_MPI_ISP_SetModuleOnOff on/off fail  isp_dev:%d  value:%d  ret:%d \n",
                        g_isp_dev, val, ret);
                return -1;
            }

        } else {
            printf(" Input 3NRAttr value:%d error!\n", val);
            continue;
        }
    }

}

static int mpp_menu_isp_3NRAttr_get(void *pData, char *pTitle)
{
    int  ret       = 0;
    int  val       = 0;
    ISP_MODULE_ONOFF   ModuleOnOff;

    printf("\n**************** Get ISP PltmWDR *************************\n");
    ret = AW_MPI_ISP_GetModuleOnOff(g_isp_dev, &ModuleOnOff);
    if (0 == ret) {
        DB_PRT("Do AW_MPI_ISP_GetModuleOnOff succeed isp_dev:%d  (value:%d)  ret:%d \n",
               g_isp_dev, val, ret);
        ret = AW_MPI_ISP_Get3NRAttr(g_isp_dev, &val);
        if (0 == ret) {
            DB_PRT("Do AW_MPI_ISP_Get3NRAttr succeed isp_dev:%d  (value:%d)  ret:%d \n",
                   g_isp_dev, val, ret);
            return 0;
        } else {
            ERR_PRT("Do AW_MPI_ISP_Get3NRAttr fail isp_dev:%d  (value:%d)  ret:%d \n",
                    g_isp_dev, val, ret);
            return -1;
        }
    } else {
        ERR_PRT("Do AW_MPI_ISP_GetModuleOnOff fail isp_dev:%d  (value:%d)  ret:%d \n",
                g_isp_dev, val, ret);
        return -1;
    }

}


#define AW_BC_IR_IO_1_NAME "231"
#define AW_BC_IR_IO_2_NAME "232"

#define AW_BC_IR_IO_PATH_1 "/sys/class/gpio/gpio"AW_BC_IR_IO_1_NAME"/value"
#define AW_BC_IR_IO_PATH_2 "/sys/class/gpio/gpio"AW_BC_IR_IO_2_NAME"/value"

#define AW_BC_IR_IO_EXPORT_PATH "/sys/class/gpio/export"

#define AW_BC_IR_IO_DIRECTION_PATH_1 "/sys/class/gpio/gpio"AW_BC_IR_IO_1_NAME"/direction"
#define AW_BC_IR_IO_DIRECTION_PATH_2 "/sys/class/gpio/gpio"AW_BC_IR_IO_2_NAME"/direction"

#define AW_BC_IO_PULL_STR "1"
#define AW_BC_IO_DOWN_STR "0"

#define AW_BC_IR_IO_DIFF_TIME  120 * 1000

static int g_ircut_init_flag = 0;

int write_str_to_path(const char* path, const char* str)
{
    FILE *fd = fopen(path, "w");
    if (fd == NULL) {
        printf("%s open failed!\n", path);
        return -1;
    }

    int ret = fwrite(str, strlen(str), 1, fd);
    if (ret == -1) {
        printf("write to path %s, value %s failed!\n", path, str);
    }

    fclose(fd);
    return 0;
}

int aw_bc_ir_init()
{
    int ret = write_str_to_path(AW_BC_IR_IO_EXPORT_PATH, AW_BC_IR_IO_1_NAME);
    if (ret == -1) {
        printf("export IO[%s] failed!", AW_BC_IR_IO_1_NAME);
        return -1;
    }

    ret = write_str_to_path(AW_BC_IR_IO_EXPORT_PATH, AW_BC_IR_IO_2_NAME);
    if (ret == -1) {
        printf("export IO[%s] failed!", AW_BC_IR_IO_2_NAME);
        return -1;
    }

    ret = write_str_to_path(AW_BC_IR_IO_DIRECTION_PATH_1, "out");
    if (ret == -1) {
        return -1;
    }

    return write_str_to_path(AW_BC_IR_IO_DIRECTION_PATH_2, "out");
}

int aw_bc_ir_on_off()
{
    static char last_value;

    const char *first  = last_value ? AW_BC_IO_PULL_STR : AW_BC_IO_DOWN_STR;
    const char *second = last_value ? AW_BC_IO_DOWN_STR : AW_BC_IO_PULL_STR;

    printf("start make ir gpio different\n");
    int ret = write_str_to_path(AW_BC_IR_IO_PATH_1, first);
    if (ret == -1) {
        return -1;
    }

    ret = write_str_to_path(AW_BC_IR_IO_PATH_2, second);
    if (ret == -1) {
        return -1;
    }

    usleep(AW_BC_IR_IO_DIFF_TIME);

    printf("stop gpio different\n");
    ret = write_str_to_path(AW_BC_IR_IO_PATH_1, AW_BC_IO_DOWN_STR);
    if (ret == -1) {
        return -1;
    }

    ret = write_str_to_path(AW_BC_IR_IO_PATH_2, AW_BC_IO_DOWN_STR);
    if (ret == -1) {
        return -1;
    }

    last_value = last_value ? 0 : 1;
    printf("last_value %d\n", last_value);

    return 0;
}

static int ircut_control(void *pData, char *pTitle)
{
    if (0 == g_ircut_init_flag) {
        aw_bc_ir_init();
        g_ircut_init_flag = 1;
    }

    aw_bc_ir_on_off();

    return 0;
}

