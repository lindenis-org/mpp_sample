###############################################################################
## Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.
## file     : build_lib.mk  $(TOP_DIR)/scripts
## brief    : for make static lib rule.
## author   : wangguixing
## versoion : V0.1 create
## date     : 2017.04.14
###############################################################################

ifndef SRCS
SRCS     := $(wildcard *.c)
SRCS_CXX := $(wildcard *.cpp)
endif

OBJS     := $(SRCS:%.c=%.o)
OBJS_CXX := $(SRCS_CXX:%.cpp=%.o)


.PHONY:	clean all

TARGET := 

ifeq ($(COPY_TO_DIR), )
	TARGET := $(TARGET_LIB)
else
    TARGET := $(COPY_TO_DIR)/$(TARGET_LIB)
endif


ifndef PREPARATION
all:$(TARGET)
else
all:$(PREPARATION) $(TARGET)
endif


$(COPY_TO_DIR)/$(TARGET_LIB): $(TARGET_LIB)
	$(MKDIR) -p $(COPY_TO_DIR)
	$(CP) $< $(COPY_TO_DIR)

# Notic: This is not allowed *.c file and *.cpp exist simultaneously!
$(TARGET_LIB): $(PREPARATION) $(OBJS) $(OBJS_CXX)
	$(AR) $(AR_FLAGS) $@ $^

$(OBJS):%.o:%.c
	$(CC) -c $<  $(CFLAGS) -o $@

$(OBJS_CXX):%.o:%.cpp
	$(CXX) -c $<  $(CXXFlAGS) -o $@

clean:
	@$(RM) $(TARGET_LIB)
	@$(RM) $(OBJS) $(OBJS_CXX)
ifneq ($(COPY_TO_DIR), )
	$(RM) $(COPY_TO_DIR)/$(TARGET_LIB)
endif

