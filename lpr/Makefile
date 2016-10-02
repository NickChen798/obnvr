CC          = $(CROSS_COMPILE)gcc
STRIP       = $(CROSS_COMPILE)strip

SRC_DIR     = $(shell /bin/pwd)
PRJ_DIR     = $(SRC_DIR)/..
PRJ_LIBS	= $(SRC_DIR)/libCVWnLPR.so $(PRJ_DIR)/dbg_msg/liblog_msg.a 

TEST_APP	= lpr_test
TARGET_LIB	= liblpr.a

SOURCES     = lpr.c
OBJECTS     = $(SOURCES:.c=.o)

INCS		= -I. -I$(PRJ_DIR) -I$(PRJ_DIR)/include
LIBS        = -lm -lpthread 
CFLAGS      = $(INCS) -Wall -O2 -D_DSP
LDFLAGS     = $(LIBS)

all: $(TARGET_LIB) $(TEST_APP)

$(TARGET_LIB): $(OBJECTS)
	$(AR) rcsv $@ $(OBJECTS)

$(TEST_APP)	: main.o $(TARGET_LIB) 
	$(CC) $(LDFLAGS) -o $@ main.o $(TARGET_LIB) $(PRJ_LIBS)
	$(STRIP) $@
	-cp $@ /tftpboot

.PHONY: clean
clean:
	rm -f *.o $(TARGET_LIB) $(TEST_APP)

