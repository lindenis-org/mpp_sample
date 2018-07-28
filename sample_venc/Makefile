include ../Makefile.param

# target
TARGET := sample_venc

# target source
SRC := \
	sample_venc.c \
	sample_venc_mem.c

# header
INC := -I.\
    -I/usr/include/mpp/system/include \
    -I/usr/include/mpp/system/include/rgb_ctrl \
	-I/usr/include/mpp/system/include/libion \
    -I/usr/include/mpp/middleware/include \
    -I/usr/include/mpp/middleware/include/utils \
    -I/usr/include/mpp/middleware/include/media \
    -I/usr/include/mpp/middleware/media/include \
    -I/usr/include/mpp/middleware/media/include/utils \
    -I/usr/include/mpp/middleware/media/include/component \
    -I/usr/include/mpp/middleware/media/LIBRARY/libcedarc/include \
    -I/usr/include/mpp/middleware/media/LIBRARY/libcedarx/libcore/common/iniparser \
    -I/usr/include/mpp/middleware/sample/configfileparser

# share libraries
LIBS := \
	-llog \
	-lion \
    -lvencoder \
    -lcdx_common \
    -lsample_confparser \
    -lcedarxrender \
   	-lmpp_vi \
   	-lmpp_isp \
   	-lmpp_ise \
   	-lmpp_vo \
   	-lmpp_component \
    -lmedia_mpp \
    -lISP \
    -lMemAdapter \
    -lmedia_utils

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
