###############################################################################
## Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.
## file     : Makefile  $(TOP_DIR)/src/mpp/mpp_vi_venc
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


###############################################################
# You can change $(TOP_DIR)/Makefile.param of APP_BUILD_MODE
# val. SAMPLE_MODE or MENU_MODE
###############################################################
ifeq ($(APP_BUILD_MODE), SAMPLE_MODE)

TARGET_BIN = sample_vi_venc_mux

COPY_TO_DIR = $(TOP_DIR)/release

#include $(BUILD_BIN)
include $(BUILD_CPP_BIN)

else

TARGET_LIB = libsample_vi_venc_mux.a

COPY_TO_DIR = $(TOP_DIR)/lib

include $(BUILD_STATIC_LIB)

endif


