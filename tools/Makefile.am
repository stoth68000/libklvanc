
AUTOMAKE_OPTIONS = foreign

BLACKMAGIC_SDK_PATH = ../include/decklink-sdk

CFLAGS = -Wall -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" -D_FILE_OFFSET_BITS=64 -O3 \
	-D_BSD_SOURCE \
	--std=c99 \
	-I../src \
	-I$(BLACKMAGIC_SDK_PATH) \
	-L../src/.libs -lklvanc -lpthread \
	-DKL_USERSPACE -L/usr/local/lib -lz -ldl -lklvanc

if DEBUG
	CFLAGS += -g
endif

CXXFLAGS = $(CFLAGS)

SRC  = klvanc_util.c
SRC += demo.c
SRC += capture.cpp
SRC += $(BLACKMAGIC_SDK_PATH)//DeckLinkAPIDispatch.cpp

bin_PROGRAMS  = klvanc_util
bin_PROGRAMS += klvanc_capture

klvanc_util_SOURCES = $(SRC)
klvanc_capture_SOURCES = $(SRC)

