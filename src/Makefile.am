
AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libklvanc.la

libklvanc_la_SOURCES  = core.c
libklvanc_la_SOURCES += core-packet-eia_708b.c
libklvanc_la_SOURCES += core-packet-eia_608.c
libklvanc_la_SOURCES += core-packet-scte_104.c
libklvanc_la_SOURCES += core-packet-afd.c
libklvanc_la_SOURCES += core-packet-sdp.c
libklvanc_la_SOURCES += core-packet-smpte_12_2.c
libklvanc_la_SOURCES += core-packets.c
libklvanc_la_SOURCES += core-lines.c
libklvanc_la_SOURCES += core-did.c
libklvanc_la_SOURCES += core-pixels.c
libklvanc_la_SOURCES += core-checksum.c
libklvanc_la_SOURCES += smpte2038.c
libklvanc_la_SOURCES += core-cache.c
libklvanc_la_SOURCES += core-packet-kl_u64le_counter.c
libklvanc_la_SOURCES += core-private.h xorg-list.h
libklvanc_la_SOURCES += klbitstream_readwriter.h

libklvanc_la_CFLAGS = -Wall -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" \
	-D_FILE_OFFSET_BITS=64 -O3 -D_BSD_SOURCE -I$(top_srcdir)/include

if DEBUG
  libklvanc_la_CFLAGS += -g
endif

libklvanc_includedir = $(includedir)/libklvanc

libklvanc_include_HEADERS  = libklvanc/vanc.h
libklvanc_include_HEADERS += libklvanc/did.h
libklvanc_include_HEADERS += libklvanc/pixels.h
libklvanc_include_HEADERS += libklvanc/smpte2038.h
libklvanc_include_HEADERS += libklvanc/vanc-eia_708b.h
libklvanc_include_HEADERS += libklvanc/vanc-eia_608.h
libklvanc_include_HEADERS += libklvanc/vanc-scte_104.h
libklvanc_include_HEADERS += libklvanc/vanc-smpte_12_2.h
libklvanc_include_HEADERS += libklvanc/vanc-packets.h
libklvanc_include_HEADERS += libklvanc/vanc-lines.h
libklvanc_include_HEADERS += libklvanc/vanc-afd.h
libklvanc_include_HEADERS += libklvanc/vanc-sdp.h
libklvanc_include_HEADERS += libklvanc/vanc-checksum.h
libklvanc_include_HEADERS += libklvanc/klrestricted_code_path.h
libklvanc_include_HEADERS += libklvanc/cache.h
libklvanc_include_HEADERS += libklvanc/vanc-kl_u64le_counter.h

