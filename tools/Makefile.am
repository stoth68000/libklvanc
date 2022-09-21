
AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" -D_FILE_OFFSET_BITS=64 \
	-D_BSD_SOURCE \
	-I$(top_srcdir)/src \
	-D__STDC_FORMAT_MACROS

AM_CFLAGS = -Wall -O3

LDADD = ../src/libklvanc.la -lpthread -ldl

if DEBUG
	CFLAGS += -g
endif

SRC  = klvanc_util.c
SRC += demo.c
SRC += parse.c
SRC += smpte2038.c
SRC += scte104.c
SRC += genscte104.c
SRC += gensmpte2038.c
SRC += eia708.c
SRC += smpte12_2.c
SRC += afd.c
SRC += udp.c
SRC += url.c
SRC += ts_packetizer.c
SRC += klringbuffer.c
SRC += pes_extractor.c

bin_PROGRAMS  = klvanc_util
bin_PROGRAMS += klvanc_parse
bin_PROGRAMS += klvanc_smpte2038
bin_PROGRAMS += klvanc_scte104
bin_PROGRAMS += klvanc_genscte104
bin_PROGRAMS += klvanc_gensmpte2038
bin_PROGRAMS += klvanc_eia708
bin_PROGRAMS += klvanc_smpte12_2
bin_PROGRAMS += klvanc_afd

klvanc_util_SOURCES = $(SRC)
klvanc_parse_SOURCES = $(SRC)
klvanc_smpte2038_SOURCES = $(SRC)
klvanc_scte104_SOURCES = $(SRC)
klvanc_genscte104_SOURCES = $(SRC)
klvanc_gensmpte2038_SOURCES = $(SRC)
klvanc_eia708_SOURCES = $(SRC)
klvanc_smpte12_2_SOURCES = $(SRC)
klvanc_afd_SOURCES = $(SRC)

libklvanc_noinst_includedir = $(includedir)

noinst_HEADERS  = hexdump.h
noinst_HEADERS += klringbuffer.h
noinst_HEADERS += pes_extractor.h
noinst_HEADERS += ts_packetizer.h
noinst_HEADERS += udp.h
noinst_HEADERS += url.h
noinst_HEADERS += version.h

test: klvanc_eia708 klvanc_genscte104 klvanc_scte104 klvanc_smpte12_2 klvanc_afd klvanc_smpte2038 klvanc_gensmpte2038
	./klvanc_eia708
	./klvanc_genscte104
	./klvanc_scte104
	./klvanc_smpte12_2
	./klvanc_gensmpte2038
	./klvanc_afd
	./klvanc_smpte2038 -i ../samples/smpte2038-sample-pid-01e9.ts -P 0x1e9
