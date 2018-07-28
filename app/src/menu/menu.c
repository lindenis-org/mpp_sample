/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file menu.c
 * @brief 为其它模块提供公共服务,如网络服务,时间服务,配置文件操作等
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-04-14
 */

/************************************************************************************************/
/*                                      Include Files                                           */
/************************************************************************************************/

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


#include "common.h"
#include "menu.h"


/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
#define MAX_MENU_LEVEL 16


/************************************************************************************************/
/*                                    Structure Declarations                                    */
/************************************************************************************************/
typedef struct tag_MENU_STACK {
    int          stackSize;
    int          stackTop;
    PMENU_INODE  menuHead[MAX_MENU_LEVEL];
} MENU_STACK, *PMENU_STACK;


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
static int InitMenuStack(PMENU_STACK pMenuStack)
{
    if (NULL == pMenuStack) {
        DB_PRT("Input pMenuStack is NULL!\n");
    }
    memset(pMenuStack, 0, sizeof(MENU_STACK));

    pMenuStack->stackSize = sizeof(pMenuStack->menuHead)/sizeof(PMENU_INODE);
    //DB_PRT("This pMenuStack->stackSize:%d  \n", pMenuStack->stackSize);

    return 0;
}


static int PushMenuStack(PMENU_STACK pMenuStack, PMENU_INODE pMenuList)
{
    if (NULL == pMenuStack) {
        DB_PRT("Input pMenuStack is NULL!\n");
    }

    /* Check stack is full? */
    if (pMenuStack->stackTop >= pMenuStack->stackSize-1) {
        ERR_PRT("This stack is full! Can't push data!\n");
        return -1;
    }

    /* Push data to stack */
    pMenuStack->menuHead[pMenuStack->stackTop] = pMenuList;
    pMenuStack->stackTop++;

    return 0;
}


static int PopMenuStack(PMENU_STACK pMenuStack, PMENU_INODE *pMenuList)
{
    /* Check stack is empty? */
    if (0 == pMenuStack->stackTop) {
        DB_PRT("This stack is empty! Can't pop data!\n");
        return -1;
    }

    /* Push data to stack */
    pMenuStack->stackTop--;
    *pMenuList = pMenuStack->menuHead[pMenuStack->stackTop];
    pMenuStack->menuHead[pMenuStack->stackTop] = NULL;

    return 0;
}


static int DisplayMenuList(const PMENU_INODE pMenuList, int *max_menu)
{
    int cnt = 0;

    if (NULL == pMenuList || NULL == max_menu) {
        ERR_PRT("Input pMenuList or max_menu is NULL!\n");
        return -1;
    }

    printf("\n***************************************************************\n");
    for (cnt = 0; NULL != (pMenuList + cnt)->title; cnt++) {
        printf(" %2d : %s\n", cnt + 1, (pMenuList + cnt)->title);
    }
    printf("***************************************************************\n");
    printf(" Please choice 1~%d num: ", cnt);

    *max_menu = cnt;
    return 0;
}


int RunMenuCtrl(PMENU_INODE pMenuList)
{
    int  ret       = 0;
    int  max_id    = 0;
    int  menu_id   = 0;
    char str[256]  = {0};
    PMENU_INODE curMenuList = NULL;
    MENU_STACK  stMenuStack;

    if (NULL == pMenuList) {
        ERR_PRT("Input pMenuList is NULL!\n");
        return -1;
    }

    InitMenuStack(&stMenuStack);
    curMenuList = pMenuList;

    while (1) {
        ret = DisplayMenuList(curMenuList, &max_id);
        if (ret < 0) {
            ERR_PRT("Do DisplayMenuList fail! ret:%d\n", ret);
        }

        memset(str, 0, sizeof(str));
        gets(str);
        printf("\n");
        if (0 == str[0]) {
            printf("\033[2J");
            continue;
        }
        ret = is_digit_str(str);
        if (ret) {
            printf(" Input %s error.\n\n", str);
            continue;
        }

        menu_id = atoi(str);
        if (0 == menu_id || menu_id > max_id) {
            printf(" Input %s error. Range 1~%d\n\n", str, max_id);
            continue;
        }

        if (NULL == (curMenuList+menu_id-1)->func && NULL == (curMenuList+menu_id-1)->subMenu) {
            printf("\n %d:%s function is not complete!\n\n", menu_id, (curMenuList+menu_id-1)->title);
            continue;
        }

        /* If do  ExitCurrentMenuLevel function, will be return last menu level. */
        if (ExitCurrentMenuLevel == (curMenuList+menu_id-1)->func) {
            ret = PopMenuStack(&stMenuStack, &curMenuList);
            if (ret < 0) {
                DB_PRT("The end! bye-bye!\n");
                return 0;
            }
            continue;
        } else {
            if (NULL != (curMenuList+menu_id-1)->func) {
                printf(" Start run %s function......\n\n", (curMenuList+menu_id-1)->title);
                ret = ((curMenuList+menu_id-1)->func)((curMenuList+menu_id-1)->data, (curMenuList+menu_id-1)->title);
                if (ret) {
                    printf("\n Run %s function fail!!! ret:%d\n", (curMenuList+menu_id-1)->title, ret);
                    continue;
                }
                printf("\n Run %s function success!\n\n", (curMenuList+menu_id-1)->title);
            }

            if (NULL != (curMenuList+menu_id-1)->subMenu) {
                ret = PushMenuStack(&stMenuStack, curMenuList);
                if (ret) {
                    ERR_PRT("Do PushMenuStack fail! ret:%d\n", ret);
                    return -1;
                }

                curMenuList = (curMenuList+menu_id-1)->subMenu;
                continue;
            }
        }

    }

    return 0;
}


int ExitCurrentMenuLevel(void *pData, char *pTitle)
{
    return 0;
}

