#include <libklvanc/vanc.h>
#include <libklvanc/did.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static struct did_s {
	uint16_t did;
	uint16_t sdid;
	const char *spec;
	const char *desc;
} dids[] = {
	{ 0x00, 0x00,   "S291", "Undefined Data"},
	{ 0x41, 0x01,   "S352", "Payload Identification, VANC space"},
	{ 0x41, 0x05,   "S352 / S2016-3", "AFD and Bar Data / Payload Identification, VANC space (Missconfigured upstream?)"},
	{ 0x41, 0x06,"S2016-4", "Pan-Scan Data"},
	{ 0x41, 0x07,  "S2010", "ANSI/SCTE 104"},
	{ 0x41, 0x08,  "S2031", "DVB/SCTE VBI data"},
	{ 0x60, 0x60, "S12M-2", "Ancillary Time Code"},
	{ 0x61, 0x01,   "S334", "EIA 708B Data mapping into HDTV VBI, VANC space"},
	{ 0x61, 0x02,   "S334", "EIA 608 Data mapping into HDTV VBI, VANC space"},
	{ 0x61, 0x03,   "S334", "World System Teletext Description Packet"},
	{ 0x61, 0x04,   "S334", "Subtitling Data Essence (SDE)"},
	{ 0x61, 0x05,   "S334", "ARIB Captioning - HD"},
	{ 0x61, 0x06,   "S334", "ARIB Captioning - SD"},
	{ 0x61, 0x07,   "S334", "ARIB Captioning - Analog"},
};

const char *klvanc_didLookupDescription(uint16_t did, uint16_t sdid)
{
	for (unsigned int i = 0; i < (sizeof(dids) / sizeof(struct did_s)); i++) {
		if ((did == dids[i].did) && (sdid == dids[i].sdid))
			return dids[i].desc;
	}

	return "Undefined";
}

const char *klvanc_didLookupSpecification(uint16_t did, uint16_t sdid)
{
	for (unsigned int i = 0; i < (sizeof(dids) / sizeof(struct did_s)); i++) {
		if ((did == dids[i].did) && (sdid == dids[i].sdid))
			return dids[i].spec;
	}

	return "Undefined";
}
