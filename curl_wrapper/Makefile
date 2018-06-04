include $(STB_SDK_PATH)/learnning/learning-curl/build.cfg
M_PREFIX=$(LIBPLAYER_STAGING_DIR)/usr
ROOTFS?=$(M_PREFIX)
LIB_DIR?=$(M_PREFIX)/lib
HEADERS_DIR?=$(M_PREFIX)/curl_include

TARGET_OBJS=curl_learning_wrapper.o

TARGET=$(SDK_LIB_PATH)/libplayer/libcurl_base.so
INSTALL_DIR=${LIB_DIR}/libplayer/

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)

CFLAGS+=-O2 -fPIC
CFLAGS+= -I$(SRC)/../curl_include   -I$(SRC)/../../../amffmpeg/libavutil/
LDFLAGS+= -L$(SDK_LIB_PATH)/libplayer -shared -fPIC -lcurl

#AR =$(STAGING_DIR)/../../bin/arm-linux-gnueabihf-ar
export CC CFLAGS
all: force $(TARGET)
	#$(STRIP) $(TARGET)
	#$(MAKE) install
	
$(TARGET_OBJS):%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $< 

$(TARGET): $(TARGET_OBJS)
	$(CC) -o  $@  $< $(LDFLAGS) -lcurl_aml_common
install:
	#install -m 555 ${TARGET} $(INSTALL_LIB_DIR)
	cp  -rf $(SRC)/../curl_include/*.h  $(HEADERS_DIR)/

force:

clean:
	rm -rf *.o
	rm -f libcurl_base.so
	rm -f $(TARGET)
	#rm -f $(INSTALL_LIB_DIR)/libcurl_common.a

