CC          = $(CROSS_COMPILE)gcc
STRIP       = $(CROSS_COMPILE)strip

PRJ_DIR     = $(shell /bin/pwd)

TARGET    	= obnvr

SOURCES     = main.c
OBJECTS     = $(SOURCES:.c=.o)

INCS		= -I. -I$(PRJ_DIR)/include 
LIBS        = -lm -lpthread 
CFLAGS      = $(INCS) -Wall -O2 -D_DSP
LDFLAGS     = $(LIBS)
PRJ_LIBS	= $(PRJ_DIR)/dbg_msg/liblog_msg.a $(PRJ_DIR)/lpr/liblpr.a $(PRJ_DIR)/lpr/libCVWnLPR.so $(PRJ_DIR)/vsc/libvsc.a $(PRJ_DIR)/ipc/libipc.a $(PRJ_DIR)/storage/libstorage.a

PRJ_LIBS   += $(PRJ_DIR)/codec/libcodec.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libmpi.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libtde.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libaec.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libanr.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libjpeg.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libpciv.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libresampler.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libVoiceEngine.a
PRJ_LIBS   += $(PRJ_DIR)/codec/lib/libhdmi.a
PRJ_LIBS   += $(PRJ_DIR)/mcu/libmcu.a


all: $(TARGET)

$(TARGET)	: main.o 
	$(CC) $(LDFLAGS) -o $@ main.o $(PRJ_LIBS)
	$(STRIP) $@

# Libraries
SUBDIRS     = lpr vsc storage codec

$(SUBDIRS):
	make -C $@

.PHONY: lib
lib: 
	@for dir in $(SUBDIRS); do \
	    make -C $$dir; \
	done

.PHONY: clean
clean:
	rm -f *.o $(TARGET)

.PHONY: libclean
libclean:
	@for dir in $(SUBDIRS); do \
	    make -C $$dir clean; \
	done
