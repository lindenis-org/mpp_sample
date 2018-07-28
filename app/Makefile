###############################################################################
## Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.
## file     : Makefile  $(TOP_DIR)
## brief    : for all project main makefile control
## author   : wangguixing
## versoion : V0.1  create
## date     : 2017.04.14
###############################################################################

TOP_DIR := $(shell pwd)

export TOP_DIR

include ./Makefile.param

.PHONY: all clean

all:
	$(call make_subdir, ./src)
#	$(CP) $(TOP_REL_DIR)/sample_vi_ise_venc_vo ~/nfs/app_mpp_sample
#	$(CP) $(TOP_REL_DIR)/sample_demux_adec_ao ~/nfs/app_mpp_sample
#	$(CP) $(TOP_REL_DIR)/sample_ai_aenc_mux_ao ~/nfs/app_mpp_sample
#	$(CP) $(TOP_REL_DIR)/sample_demux_vdec_vo ~/nfs/app_mpp_sample
#	$(CP) $(TOP_REL_DIR)/sample_vi_venc_mux_vo ~/nfs/app_mpp_sample
#	$(CP) $(TOP_REL_DIR)/sample_vi_venc_mux ~/nfs/app_mpp_sample
#	$(CP) $(TOP_REL_DIR)/sample_vi_vo_venc ~/nfs/app_mpp_sample
#	$(CP) $(TOP_REL_DIR)/sample_vi_venc ~/nfs/app_mpp_sample
#	$(CP) $(TOP_REL_DIR)/sample_vi_vo ~/nfs/app_mpp_sample
#	$(CP) $(TOP_REL_DIR)/app_mpp_sample ~/nfs/app_mpp_sample


clean:
	$(call make_clean_subdir, ./src)
	$(call make_clean_subdir, ./src/common)
	$(call make_clean_subdir, ./src/mpp)
	$(call make_clean_subdir, ./src/menu)
