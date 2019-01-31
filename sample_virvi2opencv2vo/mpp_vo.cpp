#include "mpp_vo.h"

// VO 回调函数
static ERRORTYPE VideoOutCallbackWrapper(void *cookie, MPP_CHN_S *pChn, MPP_EVENT_TYPE event, void *pEventData)
{
	ERRORTYPE ret = SUCCESS;
	if (MOD_ID_VOU == pChn->mModId) {
		switch(event) {
		case MPP_EVENT_RELEASE_VIDEO_BUFFER: {
			// 会在这里通知释放buffer，我们暂时跳过这个机制
			break;
		}
		case MPP_EVENT_SET_VIDEO_SIZE: {
			SIZE_S *pDisplaySize = (SIZE_S*)pEventData;
			alogd("vo report video display size[%dx%d]", pDisplaySize->Width, pDisplaySize->Height);
			break;
		}
		case MPP_EVENT_RENDERING_START: {
			alogd("vo report rendering start");
			break;
		}
		default: {
			//postEventFromNative(this, event, 0, 0, pEventData);
			aloge("fatal error! unknown event[0x%x] from channel[0x%x][0x%x][0x%x]!", event, pChn->mModId, pChn->mDevId, pChn->mChnId);
			ret = ERR_VO_ILLEGAL_PARAM;
			break;
		}
		}
	}
	return ret;
}


int create_vo(VO_Params*      pVOParams)
{
	// 创建VO
	AW_MPI_VO_Enable(pVOParams->iVoDev);
	//AW_MPI_VO_AddOutsideVideoLayer(pVOParams->iMiniGUILayer);
	//AW_MPI_VO_CloseVideoLayer(pVOParams->iMiniGUILayer); /* close ui layer. */
	AW_MPI_VO_EnableVideoLayer(pVOParams->iVoLayer);

	// 配置VO输出设备
	VO_PUB_ATTR_S spPubAttr;
	AW_MPI_VO_GetPubAttr(pVOParams->iVoDev, &spPubAttr);
	spPubAttr.enIntfType = pVOParams->iDispType;
	spPubAttr.enIntfSync = (VO_INTF_SYNC_E )pVOParams->iDispSync;
	AW_MPI_VO_SetPubAttr(pVOParams->iVoDev, &spPubAttr);

	// 配置VO图层参数
	VO_VIDEO_LAYER_ATTR_S stLayerAttr;
	AW_MPI_VO_GetVideoLayerAttr(pVOParams->iVoDev, &stLayerAttr);
	stLayerAttr.stDispRect.X      = 0;
	stLayerAttr.stDispRect.Y      = 0;
	stLayerAttr.stDispRect.Width  = pVOParams->iWidth;
	stLayerAttr.stDispRect.Height = pVOParams->iHeight;
	AW_MPI_VO_SetVideoLayerAttr(pVOParams->iVoLayer, &stLayerAttr);
	AW_MPI_VO_EnableChn(pVOParams->iVoLayer, pVOParams->iVoChn);
	AW_MPI_VO_SetVideoLayerPriority(pVOParams->iVoLayer, 11);

	// 配置VO回调函数
	char *str = "hello world!";
	MPPCallbackInfo cbInfo;
	cbInfo.cookie   = str; //(void*)&stUserConfig;
	cbInfo.callback = (MPPCallbackFuncType)&VideoOutCallbackWrapper;
	AW_MPI_VO_RegisterCallback(pVOParams->iVoLayer, pVOParams->iVoChn, &cbInfo);
	AW_MPI_VO_SetChnDispBufNum(pVOParams->iVoLayer, pVOParams->iVoChn, 2);
	AW_MPI_VO_StartChn(pVOParams->iVoLayer, pVOParams->iVoChn);

	return 0;
}

int destroy_vo(VO_Params* pVOParams)
{
	// 关闭VO
	AW_MPI_VO_StopChn(pVOParams->iVoLayer, pVOParams->iVoChn);
	AW_MPI_VO_DisableChn(pVOParams->iVoLayer, pVOParams->iVoChn);
	AW_MPI_VO_DisableVideoLayer(pVOParams->iVoLayer);
	//AW_MPI_VO_RemoveOutsideVideoLayer(pVOParams->iMiniGUILayer);
	AW_MPI_VO_Disable(pVOParams->iVoDev);

	return 0;
}

