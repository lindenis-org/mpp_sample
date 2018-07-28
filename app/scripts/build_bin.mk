###############################################################################
## Copyright (C), 2016-2017, Allwinner Tech. Co., Ltd.
## file     : build_bin.mk  $(TOP_DIR)/scripts
## brief    : for all project do make build control
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


.PHONY:	clean strip all

TARGET := 

ifeq ($(COPY_TO_DIR), )
	TARGET := $(TARGET_BIN)
else
    TARGET := $(COPY_TO_DIR)/$(TARGET_BIN)
endif


ifndef PREPARATION
all:$(TARGET)
else
all:$(PREPARATION) $(TARGET)
endif


$(COPY_TO_DIR)/$(TARGET_BIN): $(TARGET_BIN)
	$(MKDIR) -p $(COPY_TO_DIR)
	$(CP) $< $(COPY_TO_DIR)

# Notic: This is not allowed *.c file and *.cpp exist simultaneously!
$(TARGET_BIN):$(OBJS) $(OBJS_CXX)
	$(CC) -o $@ $(OBJS) $(OBJS_CXX) $(CFLAGS) $(LD_FLAGS)
ifeq ($(STRIP_FLAG), Y)
	$(STRIP) -S $(TARGET_BIN)
endif

$(OBJS):%.o:%.c
	$(CC) -c $<  $(CFLAGS) -o $@


$(OBJS_CXX):%.o:%.cpp
	$(CXX) -c $<  $(CXXFlAGS) -o $@

include $(SRCS:.c=.d)
%d: %c
	echo "create depending=============="
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$


clean:
	@$(RM) $(TARGET_BIN)
	@$(RM) $(OBJS) $(OBJS_CXX)
	@$(RM) *.d
ifneq ($(COPY_TO_DIR), )
	$(RM) $(COPY_TO_DIR)/$(TARGET_BIN)
endif


strip:
	$(STRIP) -S $(TARGET_BIN)

