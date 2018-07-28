#/bin/bash

###############################################################################
## Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.
## file     : update_lib_head.sh  $(TOP_DIR)/scripts
## brief    : update library and head file.
## author   : wangguixing
## versoion : V0.1 create
## date     : 2017.04.08
###############################################################################

#SRC_DIR=/home/wangguixing/workspace/v5/V5IPC/IPCLinuxPlatform/middleware/

SRC_DIR=$TARGET_TOP/middleware
SYS_DIR=$TARGET_TOP/system

LIB_DIR=../lib/
INC_DIR=../include/

echo $SRC_DIR
echo $DST_DIR


#find $LIB_DIR -name \* | awk '/^..\/lib\//{print }'
#find $LIB_DIR -name \* |  xargs -I '{}' find $SRC_DIR -name '{}' | xargs ls -l


find $SRC_DIR -name libcedarxrender.so     | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libmpp_vi.so           | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libmpp_isp.so          | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libmpp_ise.so          | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libmpp_vo.so           | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libmpp_component.so    | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libmedia_mpp.so        | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libISP.so              | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libMemAdapter.so       | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libvencoder.so         | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libibadecoder.so       | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libmedia_utils.so      | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name lib_ise_bi.so          | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name lib_ise_bi_soft.so     | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name lib_ise_mo.so          | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name lib_ise_sti.so         | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libcedarxstream.so     | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libcdx_common.so       | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libcdx_parser.so       | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libcdx_stream.so       | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libvdecoder.so         | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libnormal_audio.so     | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libcedarx_aencoder.so  | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libisp_ini.so          | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libVE.so               | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libcdc_base.so         | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libcdx_base.so         | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libvideoengine.so      | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SRC_DIR -name libadecoder.so         | xargs -I '{}' cp -rf '{}' $LIB_DIR

find $SYS_DIR -name libhwdisplay.so        | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SYS_DIR -name libion.so              | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SYS_DIR -name libcutils.so           | xargs -I '{}' cp -rf '{}' $LIB_DIR
find $SYS_DIR -name liblog.so               | xargs -I '{}' cp -rf '{}' $LIB_DIR

find $SRC_DIR -name libasound.so.2        | xargs -I '{}' cp -rf '{}' $LIB_DIR


find $SRC_DIR -name vdecoder.h            | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name veAdapter.h           | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name veInterface.h         | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name vencoder.h            | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name sdecoder.h            | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name typedef.h             | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name memoryAdapter.h       | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name sc_interface.h        | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name cdc_config.h          | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name sunxi_camera_v2.h     | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_rolloff.h         | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_tone_mapping.h    | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_tuning.h          | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_type.h            | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name platform.h            | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_manage.h          | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_module_cfg.h      | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_pltm.h            | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_reg.h             | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_debug.h           | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_ini_parse.h       | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_iso_config.h      | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_comm.h            | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_cmd_intf.h        | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_3a_af.h           | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_3a_afs.h          | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_3a_awb.h          | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_3a_md.h           | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_base.h            | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_3a_ae.h           | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name DemuxCompStream.h     | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name isp_tuning_priv.h     | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name mpi_venc_common.h     | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name mpi_vi_common.h       | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name mm_component.h        | xargs -I '{}' cp -rf '{}' $INC_DIR

find $SRC_DIR -name ISE_lib_mo.h          | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name ISE_lib_sti.h         | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name ISE_lib_bi.h          | xargs -I '{}' cp -rf '{}' $INC_DIR

find $SRC_DIR -name sun8iw12p1_cfg.h      | xargs -I '{}' cp -rf '{}' $INC_DIR/platform/
find $SRC_DIR -name sun8iw6p1_cfg.h       | xargs -I '{}' cp -rf '{}' $INC_DIR/platform/
find $SRC_DIR -name sun8iw8p1_cfg.h       | xargs -I '{}' cp -rf '{}' $INC_DIR/platform/

find $SRC_DIR -name ClockCompPortIndex.h  | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name FsWriter.h            | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name cedarx_stream.h       | xargs -I '{}' cp -rf '{}' $INC_DIR

find $SRC_DIR -name aw_ai_eve_type.h      | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name aw_ai_common_type.h   | xargs -I '{}' cp -rf '{}' $INC_DIR
find $SRC_DIR -name aw_ai_eve_event_interface.h   | xargs -I '{}' cp -rf '{}' $INC_DIR

#find $SRC_DIR -name MediaStream.h         | xargs -I '{}' cp -rf '{}' $INC_DIR/rtsp
#find $SRC_DIR -name TinyServer.h          | xargs -I '{}' cp -rf '{}' $INC_DIR/rtsp

cp -rf $SRC_DIR/include/media     $INC_DIR

cp -rf $SRC_DIR/include/utils/    $INC_DIR

cp -rf $SRC_DIR/media/LIBRARY/libISE/*.h  $INC_DIR
