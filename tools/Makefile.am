
AUTOMAKE_OPTIONS = foreign

BLACKMAGIC_SDK_PATH = ../include/decklink-sdk

CFLAGS = -Wall -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" -D_FILE_OFFSET_BITS=64 -O3 \
	-D_BSD_SOURCE \
	--std=c99 \
	-I$(BLACKMAGIC_SDK_PATH) \
	-I../src \
	-I/usr/local/include \
	-L../src/.libs -L/usr/local/lib -lklvanc -lpthread \
	-DKL_USERSPACE -L/usr/local/lib -lz -ldl -lklvanc

if DEBUG
	CFLAGS += -g
endif

CXXFLAGS = $(CFLAGS)

SRC  = klvanc_util.c
SRC += $(BLACKMAGIC_SDK_PATH)//DeckLinkAPIDispatch.cpp
SRC += demo.c
SRC += capture.cpp
SRC += smpte2038.c
SRC += udp.c
SRC += url.c

bin_PROGRAMS  = klvanc_util
bin_PROGRAMS += klvanc_capture
bin_PROGRAMS += klvanc_smpte2038

klvanc_util_SOURCES = $(SRC)
klvanc_capture_SOURCES = $(SRC)
klvanc_smpte2038_SOURCES = $(SRC)

