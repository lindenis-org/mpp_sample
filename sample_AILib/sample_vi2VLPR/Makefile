include ../../Makefile.param

# target
TARGET := sample_vi2VLPR

# target source
SRC := sample_vi2VLPR.c

# header
INC := -I.\
    -I/usr/include/mpp/system/include \
    -I/usr/include/mpp/middleware/include/utils \
    -I/usr/include/mpp/middleware/include/media/ \
    -I/usr/include/mpp/middleware/include \
    -I/usr/include/mpp/middleware/media/include \
    -I/usr/include/mpp/middleware/media/include/component \
    -I/usr/include/mpp/middleware/media/LIBRARY/libisp/include \
    -I/usr/include/mpp/middleware/media/LIBRARY/libisp/include/V4l2Camera \
    -I/usr/include/mpp/middleware/media/LIBRARY/libisp/isp_tuning \
    -I/usr/include/mpp/middleware/media/LIBRARY/libisp/include/device \
    -I/usr/include/mpp/middleware/media/LIBRARY/libisp/isp_dev \
    -I/usr/include/mpp/middleware/media/include/utils \
    -I/usr/include/mpp/middleware/media/LIBRARY/libcedarc/include \
    -I/usr/include/mpp/middleware/media/LIBRARY/libcedarx/libcore/common/iniparser/ \
    -I/usr/include/mpp/middleware/media/LIBRARY/include_ai_common/ \
    -I/usr/include/mpp/middleware/media/LIBRARY/libVLPR/include/ \
    -I/usr/include/mpp/middleware/sample/configfileparser

# share libraries
LIBS := \
    -llog \
	-lion \
    -lmedia_utils \
    -lmedia_mpp \
    -lmpp_vi \
    -lmpp_vo \
    -lmpp_isp \
    -lmpp_ise \
    -lsample_confparser \
    -lISP \
    -lCVEMiddleInterface \
	-lai_VLPR

CFLAGS += $(INC)

OBJS := $(SRC:%.c=%.o)

.PHONY: all
all: $(OBJS) $(TARGET)

%.o: %.c
	@$(if $(strip $(Q)), echo '  CC    $@')
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	@$(if $(strip $(Q)), echo '  LD    $@')
	$(Q)$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@
	@$(if $(strip $(Q)), echo '  STRIP $@')
	$(Q)$(STRIP) $@

.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJS)
