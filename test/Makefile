include $(STB_SDK_PATH)/learnning/learning-curl/build.cfg
M_PREFIX=$(LIBPLAYER_STAGING_DIR)/usr
ROOTFS?=$(M_PREFIX)
LIB_DIR?=$(M_PREFIX)/lib
HEADERS_DIR?=$(M_PREFIX)/curl_include

TARGET_OBJS=test.o

TARGET=$(SDK_LIB_PATH)/libplayer/test
INSTALL_DIR=${LIB_DIR}/libplayer/

TOPDIR=$(shell pwd)
SRC?=$(TOPDIR)

CFLAGS+=-O2 -g -fPIC
CFLAGS+= -g -I$(SRC)/../curl_include -I$(SRC)/../H264dec_Include 
LDFLAGS+= -L$(SDK_LIB_PATH)/libplayer -Wl,-Bstatic -lcurl_aml_common -lvlc -Wl,-Bdynamic -lm -lz -ldl -lrt
#AR =$(STAGING_DIR)/../../bin/arm-linux-gnueabihf-ar

all: $(TARGET) 
#	$(STRIP) $(TARGET)
#	$(MAKE) install
$(TARGET): $(TARGET_OBJS)
	$(CC) $(CFLAGS)  $< -o $@ $(LDFLAGS)

install:
	#-install -m 555 ${TARGET} $(INSTALL_LIB_DIR)
	cp	-rf $(SRC)/../curl_include/*.h  $(HEADERS_DIR)/

force:

clean:
	rm -rf *.o 
	rm -f test
	rm -f $(TARGET)
	#rm -f $(INSTALL_LIB_DIR)/libcurl_common.a

