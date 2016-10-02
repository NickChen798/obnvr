CC          = $(CROSS_COMPILE)gcc
STRIP       = $(CROSS_COMPILE)strip

SRC_DIR     = $(shell /bin/pwd)
PRJ_DIR     = $(SRC_DIR)/..
PRJ_LIBS	= $(PRJ_DIR)/dbg_msg/liblog_msg.a $(PRJ_DIR)/vsc/libvsc.a $(PRJ_DIR)/codec/libcodec.a $(PRJ_DIR)/lpr/libCVWnLPR.so
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libmpi.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libtde.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libaec.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libanr.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libjpeg.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libpciv.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libresampler.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libVoiceEngine.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libhdmi.a

TEST_APP	= storage_test 
TEST_APP2	= event_test 
TARGET_LIB	= libstorage.a

SOURCES     = recorder.c reader.c ob_file.c ob_event.c
OBJECTS     = $(SOURCES:.c=.o)

INCS		= -I. -I$(PRJ_DIR) -I$(PRJ_DIR)/include
LIBS        = -lm -lpthread 
CFLAGS      = $(INCS) -Wall -O2
LDFLAGS     = $(LIBS)

all: $(TARGET_LIB) $(TEST_APP) $(TEST_APP2)

$(TARGET_LIB): $(OBJECTS)
	$(AR) rcsv $@ $(OBJECTS)

$(TEST_APP)	: main.o $(TARGET_LIB) 
	$(CC) $(LDFLAGS) -o $@ main.o $(TARGET_LIB) $(PRJ_LIBS)
	$(STRIP) $@
	-cp $@ /tftpboot

$(TEST_APP2): event_test.o $(TARGET_LIB) 
	$(CC) $(LDFLAGS) -o $@ event_test.o $(TARGET_LIB) $(PRJ_LIBS)
	$(STRIP) $@
	-cp $@ /tftpboot

.PHONY: clean
clean:
	rm -f *.o $(TARGET_LIB) $(TEST_APP) $(TEST_APP2)

