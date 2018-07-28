/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file main.c
 * @brief : this main function file. For do app mpp sample!
 * @author id: wangguixing
 * @version v0.1 create
 * @date 2017-04-14
 */

/************************************************************************************************/
/*                                      Include Files                                           */
/************************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "main.h"
#include "common.h"
#include "menu.h"


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
#ifndef SAMPLE_MODE
static MENU_INODE gMenu_top[] = {
    /*  (Title),     (Function),    (Data),    (SubMenu)   */
    {(char *)"vi+(venc+rtsp)+vo", SampleViVoVenc,    NULL, NULL},
    {(char *)"vi+(venc+rtsp)",    SampleViVenc,      NULL, NULL},
    {(char *)"vi+vo",             SampleViVo,        NULL, NULL},
    {(char *)"vi+ise+(venc+rtsp)+vo", SampleViIseVencVo, NULL, NULL},
    {(char *)"[vi+venc+vo]+[ai+aenc+ao]+rtsp", SampleViVencVoAiAencAo, NULL, NULL},
    {(char *)"vi->yuv+venc+rtsp", SampleViYuvVenc,   NULL, NULL},
    {(char *)"vi+venc+mux",       SampleViVencMux,   NULL, NULL},
    {(char *)"vi+(venc+mux)+vo",  SampleViVencMuxVO, NULL, NULL},
    {(char *)"demux+vdec+vo",     SampleDemuxVdecVo, NULL, NULL},
    {(char *)"ai+aenc+mux+ao",    SampleAiAencMuxAo, NULL, NULL},
    {(char *)"demux+adec+ao",     SampleDemuxAdecAo, NULL, NULL},
    {(char *)"Quit",       ExitCurrentMenuLevel, NULL, NULL},
    {NULL, NULL, NULL, NULL},
};
#endif


/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
/* None */


/************************************************************************************************/
/*                                     Function Definitions                                     */
/************************************************************************************************/
int main(int argc, char *agrv[])
{
    int ret = 0;

#ifndef SAMPLE_MODE
    printf("\033[2J");
    ret = RunMenuCtrl(gMenu_top);
    if (ret) {
        DB_PRT("Do RunMenuCtrl fail! ret:%d\n", ret);
        return -1;
    }
#endif

    return 0;
}

