/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file common.c
 * @brief 为其它模块提供公共服务,如网络服务,时间服务,配置文件操作等
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-04-14
 */

/************************************************************************************************/
/*                                      Include Files                                           */
/************************************************************************************************/

#include <string.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <net/route.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"


/************************************************************************************************/
/*                                     Macros & Typedefs                                        */
/************************************************************************************************/
#ifndef MAX
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif

#ifndef MIN
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#endif


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

int is_digit_char(char num)
{
    if (num < '0' || num > '9')
        return -1;

    return 0;
}


int is_digit_str(char *str)
{
    while('\0' != *str) {
        if (*str < '0' || *str > '9')
            return -1;
        str++;
    }

    return 0;
}


int get_net_dev_ip(const char *netdev_name, char *ip)
{
    int            fd     = -1;
    unsigned int   u32ip  =  0;
    struct ifreq   ifr;
    struct in_addr sin_addr;

    if (NULL == netdev_name || NULL == ip) {
        printf("[FUN]:%s  [LINE]:%d  Input netdev_name or ip is NULL!\n", __func__, __LINE__);
        return -1;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd <= 0) {
        printf("[FUN]:%s  [LINE]:%d  Fail to create socket! errno[%d] errinfo[%s]\n", __func__, __LINE__,
               errno, strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, netdev_name, sizeof(ifr.ifr_name));
    ifr.ifr_addr.sa_family = AF_INET;
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        printf("[FUN]:%s  [LINE]:%d  Fail to ioctl SIOCGIFADDR. devname[%s] errno[%d] errinfo[%s]\n", __func__, __LINE__,
               netdev_name, errno, strerror(errno));
        close(fd);
        return -1;
    }

    close(fd);

    u32ip = ntohl(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr);

    sin_addr.s_addr = htonl(u32ip);
    sprintf(ip, "%s", inet_ntoa(sin_addr));
    return 0;
}


void draw_rectangle_nv21(unsigned char *pNV21_y, unsigned char *pNV21_vu, int width, int height, int linewidth, int sx, int sy, int ex, int ey)
{
    int i, j;
    unsigned char *pY  = pNV21_y;
    unsigned char *pVU = pNV21_vu;
    unsigned char *pSrc, *pSrc1;
    int halfline = (linewidth >> 1);
    int sty, endy, stx, endx;
    int colory = 255, coloru = 255, colorv = 0;

    //top line
    sty  = MAX(sy - halfline, 0);
    endy = MIN(sy + halfline, height - 1);
    for(i = sty; i <= endy; i++) {
        pSrc = (unsigned char *)(pVU + (i / 2)  * width);
        pSrc1 = (unsigned char *)(pY + i * width);
        for(j = sx; j <= ex; j ++) {
            pSrc1[j] = colory;
            pSrc[2*(j >> 1)] = coloru;
            pSrc[2*(j >> 1) + 1] = colorv;
        }
    }

    //bottom line
    sty  = MAX(ey - halfline, 0);
    endy = MIN(ey + halfline, height - 1);
    for(i = sty; i <= endy; i++) {
        pSrc = (unsigned char *)(pVU + (i / 2)  * width);
        pSrc1 = (unsigned char *)(pY + i * width);
        for(j = sx; j <= ex; j ++) {
            pSrc1[j] = colory;
            pSrc[2*(j >> 1)] = coloru;
            pSrc[2*(j >> 1) + 1] = colorv;
        }
    }

    //left line
    stx  = MAX(sx - halfline, 0);
    endx = MIN(sx + halfline, width - 1);
    sty  = MAX(sy - halfline, 0);
    endy = MIN(ey + halfline, height - 1);
    for(i = sty; i <= endy; i++) {
        pSrc = (unsigned char *)(pVU + (i / 2)  * width);
        pSrc1 = (unsigned char *)(pY + i * width);
        for(j = stx; j <= endx; j ++) {
            pSrc1[j] = colory;
            pSrc[2*(j >> 1)] = coloru;
            pSrc[2*(j >> 1) + 1] = colorv;
        }
    }

    //right line
    stx  = MAX(ex - halfline, 0);
    endx = MIN(ex + halfline, width - 1);

    for(i = sty; i <= endy; i++) {
        pSrc = (unsigned char *)(pVU + (i / 2)  * width);
        pSrc1 = (unsigned char *)(pY + i * width);
        for(j = stx; j <= endx; j ++) {
            pSrc1[j] = colory;
            pSrc[2*(j >> 1)] = coloru;
            pSrc[2*(j >> 1) + 1] = colorv;
        }
    }
}

