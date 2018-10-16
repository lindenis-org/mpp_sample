TARGET          = sample_virvi2venc2vdec2vo
INCLUDES        += -I. \
                   -I$(STAGING_DIR)/usr/include \
                   -I$(STAGING_DIR)/usr/include/utils \
                   -I$(STAGING_DIR)/usr/include/device \
                   -I$(STAGING_DIR)/usr/include/media \
                   -I$(STAGING_DIR)/usr/include/vo
LIBS			:= -llog \
					-lMemAdapter \
					-lvdecoder \
					-lmedia_utils \
					-lmedia_mpp \
					-lmpp_vi \
					-lmpp_vo \
					-lmpp_isp \
					-lmpp_ise \
					-lmpp_component \
					-lsample_confparser \
					-l_ise_mo \
					-lpthread
				   
SRCS := \
	sample_virvi2venc2vdec2vo.c 

OBJS    = $(SRCS:.c=.o)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

all:$(TARGET) install

install:
	mkdir -p $(CONFIG_PREFIX)/usr/bin
	mkdir -p $(CONFIG_PREFIX)/etc
	cp $(TARGET) $(CONFIG_PREFIX)/usr/bin
	cp sample_virvi2venc2vdec2vo.conf $(CONFIG_PREFIX)/etc/

clean:
	rm -rf $(TARGET) *.o *~
