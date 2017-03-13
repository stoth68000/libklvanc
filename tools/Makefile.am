
AUTOMAKE_OPTIONS = foreign

BLACKMAGIC_SDK_PATH = $(top_srcdir)/include/decklink-sdk

AM_CPPFLAGS = -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" -D_FILE_OFFSET_BITS=64 \
	-D_BSD_SOURCE \
	-I$(BLACKMAGIC_SDK_PATH) \
	-I$(top_srcdir)/src \
	-DKL_USERSPACE -D__STDC_FORMAT_MACROS

AM_CFLAGS = -Wall -O3

LDADD = ../src/libklvanc.la -lpthread -lz -ldl

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
SRC += ts_packetizer.c
SRC += klringbuffer.c
SRC += pes_extractor.c

bin_PROGRAMS  = klvanc_util
bin_PROGRAMS += klvanc_capture
bin_PROGRAMS += klvanc_smpte2038

klvanc_util_SOURCES = $(SRC)
klvanc_capture_SOURCES = $(SRC)
klvanc_smpte2038_SOURCES = $(SRC)

libklvanc_noinst_includedir = $(includedir)

noinst_HEADERS  = hexdump.h
noinst_HEADERS += klringbuffer.h
noinst_HEADERS += pes_extractor.h
noinst_HEADERS += ts_packetizer.h
noinst_HEADERS += udp.h
noinst_HEADERS += url.h
noinst_HEADERS += version.h
