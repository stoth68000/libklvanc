
AUTOMAKE_OPTIONS = foreign

AM_CPPFLAGS = -DVERSION=\"$(VERSION)\" -DPROG="\"$(PACKAGE)\"" -D_FILE_OFFSET_BITS=64 \
	-D_BSD_SOURCE \
	-I$(top_srcdir)/src \
	-D__STDC_FORMAT_MACROS

AM_CFLAGS = -Wall -O3

LDADD = ../src/libklvanc.la -lpthread

if WINDOWS
AM_CPPFLAGS += -DKLVANC_DISABLE_SMPTE2038_TOOL \
	-DKLVANC_DISABLE_TEST_API_TOOL
endif

if DEBUG
	CFLAGS += -g
endif

SRC  = klvanc_util.c
SRC += demo.c
SRC += parse.c
SRC += scte104.c
SRC += genscte104.c
SRC += gensmpte2038.c
SRC += eia708.c
SRC += smpte12_2.c
SRC += smpte12_3.c
SRC += smpte299.c
SRC += afd.c
SRC += sdp.c
SRC += ts_packetizer.c

if !WINDOWS
SRC += smpte2038.c
SRC += udp.c
SRC += url.c
SRC += klringbuffer.c
SRC += pes_extractor.c
SRC += test_api.c
endif

bin_PROGRAMS  = klvanc_util
bin_PROGRAMS += klvanc_parse
bin_PROGRAMS += klvanc_scte104
bin_PROGRAMS += klvanc_genscte104
bin_PROGRAMS += klvanc_gensmpte2038
bin_PROGRAMS += klvanc_eia708
bin_PROGRAMS += klvanc_smpte12_2
bin_PROGRAMS += klvanc_smpte12_3
bin_PROGRAMS += klvanc_smpte299
bin_PROGRAMS += klvanc_afd
bin_PROGRAMS += klvanc_sdp

if !WINDOWS
bin_PROGRAMS += klvanc_smpte2038
bin_PROGRAMS += klvanc_test_api
endif

klvanc_util_SOURCES = $(SRC)
klvanc_parse_SOURCES = $(SRC)
klvanc_scte104_SOURCES = $(SRC)
klvanc_genscte104_SOURCES = $(SRC)
klvanc_gensmpte2038_SOURCES = $(SRC)
klvanc_eia708_SOURCES = $(SRC)
klvanc_smpte12_2_SOURCES = $(SRC)
klvanc_smpte12_3_SOURCES = $(SRC)
klvanc_smpte299_SOURCES = $(SRC)
klvanc_afd_SOURCES = $(SRC)
klvanc_sdp_SOURCES = $(SRC)

if !WINDOWS
klvanc_smpte2038_SOURCES = $(SRC)
klvanc_test_api_SOURCES = $(SRC)
endif

libklvanc_noinst_includedir = $(includedir)

noinst_HEADERS  = hexdump.h
noinst_HEADERS += klringbuffer.h
noinst_HEADERS += pes_extractor.h
noinst_HEADERS += ts_packetizer.h
noinst_HEADERS += udp.h
noinst_HEADERS += url.h
noinst_HEADERS += version.h

TEST_BINARIES = klvanc_eia708$(EXEEXT) klvanc_genscte104$(EXEEXT) \
	klvanc_scte104$(EXEEXT) klvanc_smpte12_2$(EXEEXT) \
	klvanc_smpte12_3$(EXEEXT) klvanc_smpte299$(EXEEXT) \
	klvanc_afd$(EXEEXT) klvanc_sdp$(EXEEXT) \
	klvanc_gensmpte2038$(EXEEXT)

if !WINDOWS
TEST_BINARIES += klvanc_smpte2038$(EXEEXT) klvanc_test_api$(EXEEXT)
endif

test: $(TEST_BINARIES)
	./klvanc_eia708$(EXEEXT)
	./klvanc_genscte104$(EXEEXT)
	./klvanc_scte104$(EXEEXT)
	./klvanc_smpte12_2$(EXEEXT)
	./klvanc_smpte12_3$(EXEEXT)
	./klvanc_smpte299$(EXEEXT)
	./klvanc_gensmpte2038$(EXEEXT)
	./klvanc_afd$(EXEEXT)
	./klvanc_sdp$(EXEEXT)
if !WINDOWS
	./klvanc_smpte2038$(EXEEXT) -i ../samples/smpte2038-sample-pid-01e9.ts -P 0x1e9
	./klvanc_test_api$(EXEEXT)
endif
