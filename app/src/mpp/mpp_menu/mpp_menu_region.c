/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file mpp_menu_region.c
 * @brief 该目录是对mpp中的 REGION(cover and overlay) 模块进行菜单设置项控制.
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-07-06
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
#include <time.h>
#include <pthread.h>

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
static int mpp_menu_region_vi_cover_show(void *pData, char *pTitle);
static int mpp_menu_region_vi_cover_hide(void *pData, char *pTitle);
static int mpp_menu_region_vi_cover_set(void *pData, char *pTitle);
static int mpp_menu_region_vi_overlay_time_show(void * pData,char * pTitle);
static int mpp_menu_region_vi_overlay_time_hide(void *pData, char *pTitle);


/************************************************************************************************/
/*                                      Global Variables                                        */
/************************************************************************************************/
static MENU_INODE g_region_menu_list[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"Region Show VI Cover",  mpp_menu_region_vi_cover_show,   NULL, NULL},
    {(char *)"Region Hide VI Cover",  mpp_menu_region_vi_cover_hide,   NULL, NULL},
    {(char *)"Region Set  VI Cover",  mpp_menu_region_vi_cover_set,    NULL, NULL},
    {(char *)"Region Show VI Overlay Time OSD", mpp_menu_region_vi_overlay_time_show, NULL, NULL},
    {(char *)"Region Hide VI Overlay Time OSD", mpp_menu_region_vi_overlay_time_hide, NULL, NULL},

    {(char *)"Previous Step (Quit VENC setting)", ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};

static RGN_HANDLE g_handle_array[RGN_HANDLE_MAX];

static RGN_HANDLE g_time_handle = 0xffff;
static int        g_show_flag   = 0;
static MPP_CHN_S  g_mpp_chn     = {0};
static pthread_t  g_thd_id;


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
int mpp_menu_region_get(PMENU_INODE *pmenu_list_region)
{
    if (NULL == pmenu_list_region) {
        DB_PRT("Input pmenu_list_region is NULL!\n");
        return -1;
    }

    *pmenu_list_region = g_region_menu_list;

    memset(g_handle_array, 0x7f, sizeof(g_handle_array));

    return 0;
}


static int mpp_menu_region_vi_cover_show(void *pData, char *pTitle)
{
    int  ret = 0, i = 0, num = 0, end = 0, val = 0;
    int  error_flag = 0, cnt = 0, index = 0;
    int  vi_dev = 0, vi_chn = 0;
    char str[256] = {0};
    char tmp[128] = {0};
    MPP_CHN_S       mpp_chn      = {0};
    RGN_ATTR_S      rgn_region   = {0};
    RGN_CHN_ATTR_S  rgn_chn_attr = {0};

    while (1) {
        printf("\n\n************* Show VI Cover **********************\n");
        printf(" Please Input VI Cover Handle id: 0~32 or (q-Quit): ");
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

        index = atoi(str);
        if (index < 0 || index > RGN_HANDLE_MAX) {
            printf(" Input Cover Handle id:%d error!\n", index);
            continue;
        }
        printf("you chose Cover Handle id : %d \n",index);

        printf("\n Please Input RGN Chn ATTR [vi_dev vi_chn x y w h] val. \ne.g 1 0 32 32 640 640 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        printf("---%s\n", str);
        memset(tmp, 0, sizeof(tmp));
        for (i = 0, num = 0, end = 0; 0 != str[i] && 0 == error_flag; i++) {
            if (' ' != str[i]) {
                tmp[cnt] = str[i];
                end = 1;
                cnt++;
            }

            if ((0 != end && ' ' == str[i]) || 0 == str[i+1]) {
                ret = is_digit_str(tmp);
                if (ret) {
                    printf(" Input %s error.\n\n", tmp);
                    error_flag = 1;
                    break;
                }

                val = atoi(tmp);
                switch(num) {
                case 0:
                    if (val > 16) {
                        printf(" Input vi_dev:%d error! must be vi_dev >= 0  && vi_dev < 16 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    vi_dev = val;
                    break;
                case 1:
                    if (val > 16) {
                        printf(" Input vi_chn:%d error! must be vi_chn >= 0  && vi_chn < 16 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    vi_chn = val;
                    break;
                case 2:
                    if (val > 10000) {
                        printf(" Input x:%d error! must be x < 10000 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.X = val;
                    break;
                case 3:
                    if (val > 10000) {
                        printf(" Input y:%d error! must be y < 10000 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.Y = val;
                    break;
                case 4:
                    if (val > 10000) {
                        printf(" Input w:%d error! must be w < 10000 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.Width = val;
                    break;
                case 5:
                    if (val > 10000) {
                        printf(" Input h:%d error! must be h < 10000 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.Height = val;
                    break;
                default:
                    break;
                }

                num++;
                end = 0;
                cnt = 0;
                memset(tmp, 0, sizeof(tmp));
            }
        }

        if (0 != error_flag || 6 != num) {
            ERR_PRT(" Input param error error_flag:%d param_num:%d! OK for e.g [vi_dev vi_chn x y w h] --> 1 0 32 32 640 640 or (q-Quit) \n\n",
                    error_flag, num);
            continue;
        }

        if (g_handle_array[index] > RGN_HANDLE_MAX) {
            rgn_region.enType = COVER_RGN;
            ret = AW_MPI_RGN_Create(index, &rgn_region);
            if (ret) {
                ERR_PRT("Do AW_MPI_RGN_Create fail! index:%d ret:0x%x\n", index, ret);
                return -1;
            }
            g_handle_array[index] = index;
        } else {
            index = g_handle_array[index];
        }

        mpp_chn.mModId = MOD_ID_VIU;
        mpp_chn.mDevId = vi_dev;
        mpp_chn.mChnId = vi_chn;

        rgn_chn_attr.bShow  = TRUE;
        rgn_chn_attr.enType = COVER_RGN;
        rgn_chn_attr.unChnAttr.stCoverChn.enCoverType = AREA_RECT;
        rgn_chn_attr.unChnAttr.stCoverChn.mColor      = 0x00fbfbfb;
        rgn_chn_attr.unChnAttr.stCoverChn.mLayer      = 0;

        ret = AW_MPI_RGN_AttachToChn(index, &mpp_chn, &rgn_chn_attr);
        if (ret) {
            ERR_PRT("Do AW_MPI_RGN_AttachToChn fail! index:%d  mModId:%d  mDevId:%d  mChnId:%d  bShow:%d  enType:%d enCoverType:%d  mColor:0x%x  mLayer:%d  x-y:(%d-%d) w-h:(%d-%d) ret:0x%x\n",
                    index, mpp_chn.mModId, mpp_chn.mDevId, mpp_chn.mChnId,
                    rgn_chn_attr.bShow, rgn_chn_attr.enType, rgn_chn_attr.unChnAttr.stCoverChn.enCoverType,
                    rgn_chn_attr.unChnAttr.stCoverChn.mColor, rgn_chn_attr.unChnAttr.stCoverChn.mLayer,
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.X, rgn_chn_attr.unChnAttr.stCoverChn.stRect.Y,
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.Width, rgn_chn_attr.unChnAttr.stCoverChn.stRect.Height, ret);
        } else {
            DB_PRT("Do AW_MPI_RGN_AttachToChn Success! index:%d  mModId:%d  mDevId:%d  mChnId:%d  bShow:%d  enType:%d enCoverType:%d  mColor:0x%x  mLayer:%d  x-y:(%d-%d) w-h:(%d-%d) ret:0x%x\n",
                   index, mpp_chn.mModId, mpp_chn.mDevId, mpp_chn.mChnId,
                   rgn_chn_attr.bShow, rgn_chn_attr.enType, rgn_chn_attr.unChnAttr.stCoverChn.enCoverType,
                   rgn_chn_attr.unChnAttr.stCoverChn.mColor, rgn_chn_attr.unChnAttr.stCoverChn.mLayer,
                   rgn_chn_attr.unChnAttr.stCoverChn.stRect.X, rgn_chn_attr.unChnAttr.stCoverChn.stRect.Y,
                   rgn_chn_attr.unChnAttr.stCoverChn.stRect.Width, rgn_chn_attr.unChnAttr.stCoverChn.stRect.Height, ret);
        }

        return ret;
    }

    return 0;
}


static int mpp_menu_region_vi_cover_hide(void *pData, char *pTitle)
{
    int  ret = 0, i = 0, num = 0, end = 0, val = 0;
    int  error_flag = 0, cnt = 0, index = 0;
    int  vi_dev = 0, vi_chn = 0;
    char str[256] = {0};
    char tmp[128] = {0};
    MPP_CHN_S mpp_chn = {0};

    while (1) {
        printf("\n\n************* Hide VI Cover **********************\n");
        printf("\n Please Input RGN Chn ATTR [index vi_dev vi_chn] val. \ne.g 1 1 0 or (q-Quit): ");
        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        printf("---%s\n", str);
        memset(tmp, 0, sizeof(tmp));
        for (i = 0, num = 0, end = 0; 0 != str[i] && 0 == error_flag; i++) {
            if (' ' != str[i]) {
                tmp[cnt] = str[i];
                end = 1;
                cnt++;
            }

            if ((0 != end && ' ' == str[i]) || 0 == str[i+1]) {
                ret = is_digit_str(tmp);
                if (ret) {
                    printf(" Input %s error.\n\n", tmp);
                    error_flag = 1;
                    break;
                }

                val = atoi(tmp);
                switch(num) {
                case 0:
                    if (val > RGN_HANDLE_MAX) {
                        printf(" Input vi_dev:%d error! must be index >= 0  && index < RGN_HANDLE_MAX \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    index = val;
                    break;
                case 1:
                    if (val > 16) {
                        printf(" Input vi_dev:%d error! must be vi_dev >= 0  && vi_dev < 16 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    vi_dev = val;
                    break;
                case 2:
                    if (val > 16) {
                        printf(" Input vi_chn:%d error! must be vi_chn >= 0  && vi_chn < 16 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    vi_chn = val;
                    break;
                default:
                    break;
                }

                num++;
                end = 0;
                cnt = 0;
                memset(tmp, 0, sizeof(tmp));
            }
        }

        if (0 != error_flag || 3 != num) {
            ERR_PRT(" Input param error error_flag:%d param_num:%d! OK for e.g [index vi_dev vi_chn] --> 1 1 0 or (q-Quit) \n\n",
                    error_flag, num);
            continue;
        }

        mpp_chn.mModId = MOD_ID_VIU;
        mpp_chn.mDevId = vi_dev;
        mpp_chn.mChnId = vi_chn;

        if (g_handle_array[index] < RGN_HANDLE_MAX) {
            ret = AW_MPI_RGN_DetachFromChn(index, &mpp_chn);
            if (ret) {
                ERR_PRT("Do AW_MPI_RGN_DetachFromChn fail! index:%d  mModId:%d  mDevId:%d  mChnId:%d  ret:0x%x\n",
                        index, mpp_chn.mModId, mpp_chn.mDevId, mpp_chn.mChnId, ret);
                return ret;
            } else {
                DB_PRT("Do AW_MPI_RGN_DetachFromChn success! index:%d  mModId:%d  mDevId:%d  mChnId:%d  ret:0x%x\n",
                       index, mpp_chn.mModId, mpp_chn.mDevId, mpp_chn.mChnId, ret);
            }

            ret = AW_MPI_RGN_Destroy(index);
            if (ret) {
                ERR_PRT("Do AW_MPI_RGN_Destroy fail! index:%d ret:0x%x\n", index, ret);
                return ret;
            } else {
                DB_PRT("Do AW_MPI_RGN_Destroy success! index:%d ret:0x%x\n", index, ret);
            }

            g_handle_array[index] = 0xffff;
        } else {
            ERR_PRT("The RGN_HANDLE:%d not create!\n", index);
        }

        return ret;
    }

    return 0;
}


static int mpp_menu_region_vi_cover_set(void *pData, char *pTitle)
{
    int  ret = 0, i = 0, num = 0, end = 0, val = 0;
    int  error_flag = 0, cnt = 0, index = 0;
    int  vi_dev = 0, vi_chn = 0;
    char str[256] = {0};
    char tmp[128] = {0};
    MPP_CHN_S       mpp_chn      = {0};
    RGN_CHN_ATTR_S  rgn_chn_attr = {0};

    while (1) {
        printf("\n\n************* Set VI Cover **********************\n");
        printf("\n Please Input RGN Chn ATTR [rgn_andle vi_dev vi_chn bShow x y w h] val. \ne.g 0 1 0 1 32 32 640 640 or (q-Quit): ");

        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0])
            continue;
        if ('q' == str[0])
            return 0;

        printf("---%s\n", str);
        memset(tmp, 0, sizeof(tmp));
        for (i = 0, num = 0, end = 0; 0 != str[i] && 0 == error_flag; i++) {
            if (' ' != str[i]) {
                tmp[cnt] = str[i];
                end = 1;
                cnt++;
            }

            if ((0 != end && ' ' == str[i]) || 0 == str[i+1]) {
                ret = is_digit_str(tmp);
                if (ret) {
                    printf(" Input %s error.\n\n", tmp);
                    error_flag = 1;
                    break;
                }

                val = atoi(tmp);
                switch(num) {
                case 0:
                    if (val > RGN_HANDLE_MAX) {
                        printf(" Input RGN_HANDLE:%d error! must be RGN_HANDLE >= 0  && RGN_HANDLE < 1024 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    index = val;
                    break;
                case 1:
                    if (val > 16) {
                        printf(" Input vi_dev:%d error! must be vi_dev >= 0  && vi_dev < 16 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    vi_dev = val;
                    break;
                case 2:
                    if (val > 16) {
                        printf(" Input vi_chn:%d error! must be vi_chn >= 0  && vi_chn < 16 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    vi_chn = val;
                    break;
                case 3:
                    if (val) {
                        rgn_chn_attr.bShow  = TRUE;
                    } else {
                        rgn_chn_attr.bShow  = FALSE;
                    }
                    break;
                case 4:
                    if (val > 10000) {
                        printf(" Input x:%d error! must be x < 10000 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.X = val;
                    break;
                case 5:
                    if (val > 10000) {
                        printf(" Input y:%d error! must be y < 10000 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.Y = val;
                    break;
                case 6:
                    if (val > 10000) {
                        printf(" Input w:%d error! must be w < 10000 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.Width = val;
                    break;
                case 7:
                    if (val > 10000) {
                        printf(" Input h:%d error! must be h < 10000 \n\n", val);
                        error_flag = 1;
                        break;
                    }
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.Height = val;
                    break;
                default:
                    break;
                }

                num++;
                end = 0;
                cnt = 0;
                memset(tmp, 0, sizeof(tmp));
            }
        }

        if (0 != error_flag || 8 != num) {
            ERR_PRT(" Input param error error_flag:%d param_num:%d! \n OK for e.g [rgn_andle vi_dev vi_chn bShow x y w h] --> 0 1 0 1 32 32 640 640 or (q-Quit) \n\n",
                    error_flag, num);
            continue;
        }

        if (g_handle_array[index] > RGN_HANDLE_MAX) {
            ERR_PRT("The RGN_HANDLE:%d not create!\n", index);
            return -1;
        } else {
            index = g_handle_array[index];
        }

        mpp_chn.mModId = MOD_ID_VIU;
        mpp_chn.mDevId = vi_dev;
        mpp_chn.mChnId = vi_chn;

        rgn_chn_attr.enType = COVER_RGN;
        rgn_chn_attr.unChnAttr.stCoverChn.enCoverType = AREA_RECT;
        rgn_chn_attr.unChnAttr.stCoverChn.mColor      = 0x55555555;
        rgn_chn_attr.unChnAttr.stCoverChn.mLayer      = 0;

        ret = AW_MPI_RGN_SetDisplayAttr(index, &mpp_chn, &rgn_chn_attr);
        if (ret) {
            ERR_PRT("Do AW_MPI_RGN_SetDisplayAttr fail! index:%d  mModId:%d  mDevId:%d  mChnId:%d  bShow:%d  enType:%d enCoverType:%d  mColor:0x%x  mLayer:%d  x-y:(%d-%d) w-h:(%d-%d) ret:0x%x\n",
                    index, mpp_chn.mModId, mpp_chn.mDevId, mpp_chn.mChnId,
                    rgn_chn_attr.bShow, rgn_chn_attr.enType, rgn_chn_attr.unChnAttr.stCoverChn.enCoverType,
                    rgn_chn_attr.unChnAttr.stCoverChn.mColor, rgn_chn_attr.unChnAttr.stCoverChn.mLayer,
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.X, rgn_chn_attr.unChnAttr.stCoverChn.stRect.Y,
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.Width, rgn_chn_attr.unChnAttr.stCoverChn.stRect.Height, ret);
        } else {
            DB_PRT("Do AW_MPI_RGN_SetDisplayAttr Success! index:%d  mModId:%d  mDevId:%d  mChnId:%d  bShow:%d  enType:%d enCoverType:%d  mColor:0x%x  mLayer:%d  x-y:(%d-%d) w-h:(%d-%d) ret:0x%x\n",
                   index, mpp_chn.mModId, mpp_chn.mDevId, mpp_chn.mChnId,
                   rgn_chn_attr.bShow, rgn_chn_attr.enType, rgn_chn_attr.unChnAttr.stCoverChn.enCoverType,
                   rgn_chn_attr.unChnAttr.stCoverChn.mColor, rgn_chn_attr.unChnAttr.stCoverChn.mLayer,
                   rgn_chn_attr.unChnAttr.stCoverChn.stRect.X, rgn_chn_attr.unChnAttr.stCoverChn.stRect.Y,
                   rgn_chn_attr.unChnAttr.stCoverChn.stRect.Width, rgn_chn_attr.unChnAttr.stCoverChn.stRect.Height, ret);
        }

        return ret;
    }

    return 0;
}

static void *mpp_menu_vi_overlay_show_time_proc(void *param)
{
    int  ret = 0, i = 0;
    char string_code[256] = {0};
    char *ptime_string = NULL;
    time_t sys_time, last_time;
    RGN_ATTR_S     rgn_region;
    RGB_PIC_S      rgb_pic  = {0};
    FONT_RGBPIC_S  font_pic = {0};
    BITMAP_S       bit_map  = {0};
    RGN_CHN_ATTR_S rgn_chn_attr = {0};

    for (i = 0; i < RGN_HANDLE_MAX; i++) {
        if (g_handle_array[i] > RGN_HANDLE_MAX) {
            g_time_handle     = i;
            g_handle_array[i] = i;
            DB_PRT("The vi overlay show time handle:%d \n", g_time_handle);
            break;
        }
    }

    ret = load_gb2312_file(FONT_SIZE_32);
    if (ret  < 0) {
        ERR_PRT("Do load_gb2312_file FONT_SIZE_32 fail! ret:%d\n", ret);
        return NULL;
    }

    rgb_pic.pic_addr        = NULL;
    rgb_pic.rgb_type        = OSD_RGB_32;
    rgb_pic.background[0]   = 0xC2;
    rgb_pic.background[1]   = 0xC2;
    rgb_pic.background[2]   = 0xC2;
    rgb_pic.background[2]   = 0xC2;
    rgb_pic.enable_mosaic   = 1;
    rgb_pic.mosaic_size     = 2;
    rgb_pic.mosaic_color[0] = 0x89;
    rgb_pic.mosaic_color[1] = 0x89;
    rgb_pic.mosaic_color[2] = 0x89;
    rgb_pic.mosaic_color[3] = 0x89;

    font_pic.font_type     = FONT_SIZE_32;
    font_pic.rgb_type      = rgb_pic.rgb_type;
    font_pic.background[0] = 0x8A;
    font_pic.background[1] = 0x8A;
    font_pic.background[2] = 0xE2;
    font_pic.background[3] = 0x2B;
    font_pic.foreground[0] = 0xFF;
    font_pic.foreground[1] = 0xFF;
    font_pic.foreground[2] = 0xFF;
    font_pic.foreground[3] = 0xFF;
    font_pic.enable_bg = 0;

    while(g_show_flag) {
        sys_time = time(NULL);
        if (sys_time == last_time) {
            usleep(50 * 1000);
            continue;
        }

        if (NULL != rgb_pic.pic_addr && g_time_handle < RGN_HANDLE_MAX) {
            ret = AW_MPI_RGN_DetachFromChn(g_time_handle, &g_mpp_chn);
            if (ret) {
                ERR_PRT("Do AW_MPI_RGN_DetachFromChn fail! index:%d  mModId:%d  mDevId:%d  mChnId:%d  ret:0x%x\n",
                        g_time_handle, g_mpp_chn.mModId, g_mpp_chn.mDevId, g_mpp_chn.mChnId, ret);
                break;
            }

            ret = AW_MPI_RGN_Destroy(g_time_handle);
            if (ret) {
                ERR_PRT("Do AW_MPI_RGN_Destroy fail! index:%d ret:0x%x\n", g_time_handle, ret);
                break;
            }

            ret = release_rgb_picture(&rgb_pic);
            if (ret  < 0) {
                ERR_PRT("Do release_rgb_picture fail! ret:%d\n", ret);
                break;
            }

            rgb_pic.pic_addr = NULL;
        }

        last_time = sys_time;
        ptime_string = ctime(&sys_time);
        if (NULL == ptime_string) {
            ERR_PRT("Do ctime fail! ret:%d\n", ret);
            break;
        }
        DB_PRT(" ptime_string:%s \n", ptime_string);
        string_code[0] = 0xB1;
        string_code[1] = 0xCA;
        string_code[2] = 0xE4;
        string_code[3] = 0xBC;
        string_code[4] = ':';
        strcpy(&string_code[5], ptime_string);

        ret = create_font_rectangle(string_code, &font_pic, &rgb_pic);
        if (ret) {
            ERR_PRT("Do create_font_rectangle fail! ret:%d\n", ret);
            break;
        }

        rgn_region.enType = OVERLAY_RGN;
        rgn_region.unAttr.stOverlay.mPixelFmt    = MM_PIXEL_FORMAT_RGB_8888;
        rgn_region.unAttr.stOverlay.mBgColor     = 0;
        rgn_region.unAttr.stOverlay.mBgColor    |= font_pic.background[0];
        rgn_region.unAttr.stOverlay.mBgColor    |= (font_pic.background[1] << 8);
        rgn_region.unAttr.stOverlay.mBgColor    |= (font_pic.background[2] << 16);
        rgn_region.unAttr.stOverlay.mBgColor    |= (font_pic.background[3] << 24);
        rgn_region.unAttr.stOverlay.mSize.Width  = rgb_pic.wide;
        rgn_region.unAttr.stOverlay.mSize.Height = rgb_pic.high;
        ret = AW_MPI_RGN_Create(g_time_handle, &rgn_region);
        if (ret) {
            ERR_PRT("Do AW_MPI_RGN_Create fail! index:%d ret:0x%x\n", g_time_handle, ret);
            break;
        }

        bit_map.mPixelFormat  = MM_PIXEL_FORMAT_RGB_8888;
        bit_map.mWidth  = rgb_pic.wide;
        bit_map.mHeight = rgb_pic.high;
        bit_map.mpData  = (void *)rgb_pic.pic_addr;
        ret = AW_MPI_RGN_SetBitMap(g_time_handle, &bit_map);
        if (ret) {
            ERR_PRT("Do AW_MPI_RGN_SetBitMap fail! index:%d ret:0x%x\n", g_time_handle, ret);
            break;
        }

        rgn_chn_attr.bShow  = TRUE;
        rgn_chn_attr.enType = OVERLAY_RGN;
        rgn_chn_attr.unChnAttr.stOverlayChn.stPoint.X = 64;
        rgn_chn_attr.unChnAttr.stOverlayChn.stPoint.Y = 64;
        rgn_chn_attr.unChnAttr.stOverlayChn.mFgAlpha  = 0;
        rgn_chn_attr.unChnAttr.stOverlayChn.mBgAlpha  = rgn_region.unAttr.stOverlay.mBgColor;
        rgn_chn_attr.unChnAttr.stOverlayChn.mLayer    = 0;
        ret = AW_MPI_RGN_AttachToChn(g_time_handle, &g_mpp_chn, &rgn_chn_attr);
        if (ret) {
            ERR_PRT("Do AW_MPI_RGN_AttachToChn fail! index:%d  mModId:%d  mDevId:%d  mChnId:%d  bShow:%d  enType:%d enCoverType:%d  mColor:0x%x  mLayer:%d  x-y:(%d-%d) w-h:(%d-%d) ret:0x%x\n",
                    g_time_handle, g_mpp_chn.mModId, g_mpp_chn.mDevId, g_mpp_chn.mChnId,
                    rgn_chn_attr.bShow, rgn_chn_attr.enType, rgn_chn_attr.unChnAttr.stCoverChn.enCoverType,
                    rgn_chn_attr.unChnAttr.stCoverChn.mColor, rgn_chn_attr.unChnAttr.stCoverChn.mLayer,
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.X, rgn_chn_attr.unChnAttr.stCoverChn.stRect.Y,
                    rgn_chn_attr.unChnAttr.stCoverChn.stRect.Width, rgn_chn_attr.unChnAttr.stCoverChn.stRect.Height, ret);
            break;
        }
    }

    if (NULL != rgb_pic.pic_addr && g_time_handle < RGN_HANDLE_MAX) {
        ret = AW_MPI_RGN_DetachFromChn(g_time_handle, &g_mpp_chn);
        if (ret) {
            ERR_PRT("Do AW_MPI_RGN_DetachFromChn fail! index:%d  mModId:%d  mDevId:%d  mChnId:%d  ret:0x%x\n",
                    g_time_handle, g_mpp_chn.mModId, g_mpp_chn.mDevId, g_mpp_chn.mChnId, ret);
        }

        ret = AW_MPI_RGN_Destroy(g_time_handle);
        if (ret) {
            ERR_PRT("Do AW_MPI_RGN_Destroy fail! index:%d ret:0x%x\n", g_time_handle, ret);
        }

        ret = release_rgb_picture(&rgb_pic);
        if (ret  < 0) {
            ERR_PRT("Do release_rgb_picture fail! ret:%d\n", ret);
        }
    }

    ret = unload_gb2312_font();
    if (ret  < 0) {
        ERR_PRT("Do unload_gb2312_font fail! ret:%d\n", ret);
    }

    DB_PRT("Out this function ... ... \n");
    return NULL;
}


static int mpp_menu_region_vi_overlay_time_show(void *pData, char *pTitle)
{
    int ret;

    if (g_show_flag) {
        ERR_PRT("The mpp_menu_vi_overlay_show_time_proc haved running!\n");
        return 0;
    }

    g_show_flag = 1;
    g_mpp_chn.mModId = MOD_ID_VIU;
    g_mpp_chn.mDevId = 1;
    g_mpp_chn.mChnId = 0;

    ret = pthread_create(&g_thd_id, NULL, mpp_menu_vi_overlay_show_time_proc, NULL);
    if (ret) {
        ERR_PRT("Do pthread_create fail! ret:%d\n", ret);
    }

    return 0;
}

static int mpp_menu_region_vi_overlay_time_hide(void *pData, char *pTitle)
{
    int ret;

    if (0 == g_show_flag) {
        ERR_PRT("The mpp_menu_vi_overlay_show_time_proc haved ending!\n");
        return 0;
    }

    g_show_flag = 0;
    usleep(10 *1000);
    pthread_join(g_thd_id, 0);

    return 0;
}

