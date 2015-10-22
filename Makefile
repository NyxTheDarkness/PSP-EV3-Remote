TARGET = EV3_Remote
OBJS = main.o

# For OE firmware:
PSP_FW_VERSION = 200
BUILD_PRX = 1


CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)


LIBDIR =
LDFLAGS =
LIBS = -lpsputility -lpspgum -lpspgu -lm -lstdc++ -lpspgu

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = EV3 Remote
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak
