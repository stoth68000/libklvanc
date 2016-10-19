
AUTOMAKE_OPTIONS = foreign

BLACKMAGIC_SDK_PATH = $(top_srcdir)/include/decklink-sdk

CFLAGS = -Wall -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" -D_FILE_OFFSET_BITS=64 -O3 \
	-D_BSD_SOURCE \
	-I$(BLACKMAGIC_SDK_PATH) \
	-I$(top_srcdir)/src \
	-L$(top_srcdir)/src/.libs -lklvanc -lpthread \
	-DKL_USERSPACE -lz -ldl -lklvanc

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
libklvanc_noinst_include_HEADERS  = hexdump.h
libklvanc_noinst_include_HEADERS += klringbuffer.h
libklvanc_noinst_include_HEADERS += pes_extractor.h
libklvanc_noinst_include_HEADERS += ts_packetizer.h
libklvanc_noinst_include_HEADERS += udp.h
libklvanc_noinst_include_HEADERS += url.h
libklvanc_noinst_include_HEADERS += version.h
