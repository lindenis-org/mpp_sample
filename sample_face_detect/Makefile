include ../Makefile.param

# target
TARGET := sample_face_detect

# target source
SRC := \
    sample_face_detect.c \
    osd_helper.c \
    mpp_eve.c \
    mpp_vi.c \
    mpp_vo.c \
    mpp_sys.c \
    mpp_venc.c \
    mpp_osd.c

# header
INC := -I.\
	-I/usr/include/mpp/system/include \
	-I/usr/include/mpp/middleware/include \
	-I/usr/include/mpp/middleware/include/utils \
	-I/usr/include/mpp/middleware/include/media/ \
	-I/usr/include/mpp/middleware/media/include \
	-I/usr/include/mpp/middleware/media/include/utils \
	-I/usr/include/mpp/middleware/media/include/component \
	-I/usr/include/mpp/middleware/media/LIBRARY/libisp/include \
	-I/usr/include/mpp/middleware/media/LIBRARY/libisp/include/V4l2Camera \
	-I/usr/include/mpp/middleware/media/LIBRARY/libisp/isp_tuning \
	-I/usr/include/mpp/middleware/media/LIBRARY/libAIE_Vda/include \
	-I/usr/include/mpp/middleware/media/LIBRARY/include_stream \
	-I/usr/include/mpp/middleware/media/LIBRARY/include_ai_common \
	-I/usr/include/mpp/middleware/media/LIBRARY/include_eve_common \
	-I/usr/include/mpp/middleware/media/LIBRARY/libcedarc/include \
	-I/usr/include/mpp/middleware/media/LIBRARY/libeveface/include \
    -I/usr/include/mpp/middleware/sample/configfileparser

# share libraries
LIBS := \
	-llog \
    -lmedia_utils \
    -lmedia_mpp \
    -lmpp_vi \
    -lmpp_vo \
    -lmpp_isp \
    -lmpp_ise \
    -lmpp_component \
	-leve_event \
    -lsample_confparser

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
