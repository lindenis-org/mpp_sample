#ifndef __MPP_VO__
#define __MPP_VO__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>

#include <utils/plat_log.h>
#include "mm_comm_vo.h"
#include "vo/hwdisplay.h"
#include "mpi_vo.h"
#include "mm_comm_sys.h"
#include "mm_comm_video.h"
#include "mm_common.h"

typedef struct _VO_Params {
	VO_DEV 		iVoDev;
	VO_CHN 		iVoChn;
	VO_LAYER 	iVoLayer;
	int 		iMiniGUILayer;

	int 		iDispType;
	int 		iDispSync;

	int 		iWidth;
	int 		iHeight;
	int 		iFrameNum;
} VO_Params;

int create_vo(VO_Params*     pVOParams);
int destroy_vo(VO_Params*     pVOParams);


#endif
