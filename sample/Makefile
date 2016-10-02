CC          = $(CROSS_COMPILE)gcc
STRIP       = $(CROSS_COMPILE)strip

SRC_DIR     = $(shell /bin/pwd)
PRJ_DIR     = $(SRC_DIR)/..
PRJ_LIBS	= 

TEST_APP	= sample_test
TARGET_LIB	= libsample.a

SOURCES     = sample.c
OBJECTS     = $(SOURCES:.c=.o)

INCS		= -I. -I$(PRJ_DIR) -I$(PRJ_DIR)/include
LIBS        = -lm -lpthread 
CFLAGS      = $(INCS) -Wall -O2
LDFLAGS     = $(LIBS)

all: $(TARGET_LIB) $(TEST_APP)

$(TARGET_LIB): $(OBJECTS)
	$(AR) rcsv $@ $(OBJECTS)

$(TEST_APP)	: main.o $(TARGET_LIB) 
	$(CC) $(LDFLAGS) -o $@ main.o $(TARGET_LIB) $(PRJ_LIBS)
	$(STRIP) $@

.PHONY: clean
clean:
	rm -f *.o $(TARGET_LIB) $(TEST_APP)

