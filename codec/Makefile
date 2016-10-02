CC          = $(CROSS_COMPILE)gcc
STRIP       = $(CROSS_COMPILE)strip

SRC_DIR     = $(shell /bin/pwd)
PRJ_DIR     = $(SRC_DIR)/..
PRJ_LIBS	= $(PRJ_DIR)/dbg_msg/liblog_msg.a $(PRJ_DIR)/vsc/libvsc.a $(PRJ_DIR)/lpr/libCVWnLPR.so

TEST_APP	= codec_test
TARGET_LIB	= libcodec.a

SOURCES     = codec.c
OBJECTS     = $(SOURCES:.c=.o)

CODEC_LIBS   = $(SRC_DIR)/lib/libmpi.a
CODEC_LIBS   += $(SRC_DIR)/lib/libtde.a
CODEC_LIBS   += $(SRC_DIR)/lib/libaec.a
CODEC_LIBS   += $(SRC_DIR)/lib/libanr.a
CODEC_LIBS   += $(SRC_DIR)/lib/libjpeg.a
CODEC_LIBS   += $(SRC_DIR)/lib/libpciv.a
CODEC_LIBS   += $(SRC_DIR)/lib/libresampler.a
CODEC_LIBS   += $(SRC_DIR)/lib/libVoiceEngine.a
CODEC_LIBS   += $(SRC_DIR)/lib/libhdmi.a

INCS		= -I. -I$(PRJ_DIR) -I$(PRJ_DIR)/include -I/$(SRC_DIR)/include
LIBS        = -lm -lpthread 
CFLAGS      = $(INCS) -Wall -O2 -D_DSP
LDFLAGS     = $(LIBS)

all: $(TARGET_LIB) $(TEST_APP)

$(TARGET_LIB): $(OBJECTS)
	$(AR) rcsv $@ $(OBJECTS) 

$(TEST_APP)	: main.o $(TARGET_LIB) 
	$(CC) $(LDFLAGS) -o $@ main.o $(TARGET_LIB) $(PRJ_LIBS) $(CODEC_LIBS)
	$(STRIP) $@
	-cp $@ /tftpboot

.PHONY: clean
clean:
	rm -f *.o $(TARGET_LIB) $(TEST_APP)

