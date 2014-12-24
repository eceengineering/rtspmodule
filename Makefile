CC := arm-linux-gnueabihf-gcc

CFLAGS := -O3 -Wall -Wextra -Wno-unused-parameter -std=c99
LDFLAGS := -Wl,--as-needed
GFLAGS := $(shell pkg-config --cflags --libs gstreamer-0.10 gst-rtsp-server-0.10)
override CFLAGS += -D_GNU_SOURCE


all:

bbwatch: main.o cammodule.o v4l2cam.o rtspmodule.o rtspmedia.o
bins += bbwatch

all: $(bins)

ifndef V
QUIET_CC    = @echo '   CC         '$@ $<;
QUIET_LINK  = @echo '   LINK       '$@ 'from' $^ $(LIBS);
QUIET_CLEAN = @echo '   CLEAN      '$@;
QUIET_DLL   = @echo '   DLLCREATE  '$@;
endif

%.o:: %.c
	$(QUIET_CC)$(CC) $(CFLAGS) $(GFLAGS) -MMD -o $@ -c $<

bbwatch:
	$(QUIET_LINK)$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(GFLAGS) -lpthread

clean:
	$(QUIET_CLEAN)$(RM) $(bins) *.o *.d

-include *.d
