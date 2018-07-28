###############################################################################
## Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.
## file     : Makefile  $(TOP_DIR)/src
## brief    : for all project main makefile control
## author   : wangguixing
## versoion : V0.1  create
## date     : 2017.04.14
###############################################################################

###############################################################
# PREPARATION : Do PREPARATION before build target bin or LIB.
#               But! This will define after include $(BUILD_BIN).
#               e.g.
#                   include $(BUILD_BIN)
#
#                   $(PREPARATION):
#                   	$(call make_subdir, ./common)
#
# COPY_TO_DIR : copy target bin to destination dir
#
# TARGET_BIN  : target bin
# BUILD_BIN   : will build bin for target
# 
# TARGET_LIB        : target static lib
# BUILD_STATIC_LIB  : will build static lib for target lib
#
# ENV_CLEAR: If call TOP_MK_PARAM file, best to call ENV_CLEAR,
#            so that set CFLAG ... init.
#
# TOP_MK_PARAM : This is top dir Makefile.param file. if you
#                want to independent build local files, you
#                must be include Absolute top path Makefile.param.
#                e.g.  include ../../../Makefile.param
#
###############################################################

include $(ENV_CLEAR)

include $(TOP_MK_PARAM)

.PHONY: all clean

PREPARATION := prepar

TARGET_BIN = app_mpp_sample

COPY_TO_DIR = $(TOP_DIR)/release

#ifeq ($(APP_BUILD_MODE), MENU_MODE)
CXXFlAGS += -I$(TOP_DIR)/src
CXXFlAGS += -lsample_vi_venc
CXXFlAGS += -lsample_vi_vo_venc
CXXFlAGS += -lsample_vi_vo
CXXFlAGS += -lsample_vi_venc_mux_vo
CXXFlAGS += -lsample_vi_venc_mux
CXXFlAGS += -lsample_vi_ise_venc_vo
CXXFlAGS += -lsample_demux_vdec_vo
CXXFlAGS += -lsample_demux_adec_ao
CXXFlAGS += -lsample_ai_aenc_mux_ao
CXXFlAGS += -lsample_vi_ai_encode_vo_ao
CXXFlAGS += -lsample_vi_yuv_venc
CXXFlAGS += -lsample_vi_eve_vo
CXXFlAGS += -lmpp_menu
CXXFlAGS += -lmenu
CXXFlAGS += -lmpp_com
CXXFlAGS += -lcommon
CXXFlAGS += -lTinyServer
CXXFlAGS += -leve_event
#endif

include $(BUILD_CPP_BIN)

$(PREPARATION):
	@$(RM) $(TARGET_BIN)
	@$(RM) $(COPY_TO_DIR)/$(TARGET_BIN)
	$(call make_subdir, ./common)
	$(call make_subdir, ./menu)
	$(call make_subdir, ./mpp)

