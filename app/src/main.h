/************************************************************************************************/
/* Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.                                          */
/************************************************************************************************/
/**
 * @file main.h
 * @brief 对每一个sample code的用例进行整合
 * @author id: wangguixing
 * @version v0.1
 * @date 2017-04-14
 */

#ifndef _MAIN_MPP_H_
#define _MAIN_MPP_H_

/************************************************************************************************/
/*                                      Include Files                                           */
/************************************************************************************************/
/* None */

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
/* None */

/************************************************************************************************/
/*                                    Function Declarations                                     */
/************************************************************************************************/
//#ifndef SAMPLE_MODE
int SampleViVo(void *pData, char *pTitle);
int SampleViVenc(void *pData, char *pTitle);
int SampleViVoVenc(void *pData, char *pTitle);
int SampleViVencMux(void *pData, char *pTitle);
int SampleViVencMuxVO(void *pData, char *pTitle);
int SampleViIseVencVo(void *pData, char *pTitle);
int SampleDemuxVdecVo(void *pData, char *pTitle);
int SampleViVencVoAiAencAo(void *pData, char *pTitle);
int SampleViYuvVenc(void *pData, char *pTitle);
//#endif


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

int SampleAiAencMuxAo(void *pData, char *pTitle);
int SampleDemuxAdecAo(void *pData, char *pTitle);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif /* _MAIN_MPP_H_ */
