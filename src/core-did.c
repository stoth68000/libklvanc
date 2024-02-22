/*
 * Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved
 *
 * Address: Kernel Labs Inc., PO Box 745, St James, NY. 11780
 * Contact: sales@kernellabs.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <libklvanc/vanc.h>
#include <libklvanc/did.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* The content in this list comes from the SMPTE Registration Authority
   https://smpte-ra.org/smpte-ancillary-data-smpte-st-291 */
static struct did_s {
	uint16_t did;
	uint16_t sdid;
	const char *spec;
	const char *desc;
} dids[] = {
	{ 0x00, 0x00,   "S291", "Undefined Data"},
	{ 0x08, 0x08,   "S353", "MPEG recoding data, VANC space"},
	{ 0x08, 0x0C,   "S353", "MPEG recoding data, HANC space"},
	{ 0x40, 0x01,   "S305", "SDTI transport in active frame space"},
	{ 0x40, 0x02,   "S348", "HD-SDTI transport in active frame space"},
	{ 0x40, 0x04,   "S427", "Link Encryption Message 1"},
	{ 0x40, 0x05,   "S427", "Link Encryption Message 2"},
	{ 0x40, 0x06,   "S427", "Link Encryption Metadata"},
	{ 0x40, 0xfe,   "KLABS","UINT64_T little endian frame counter"},
	{ 0x41, 0x01,   "S352", "Payload Identification, HANC space"},
	{ 0x41, 0x05,"S2016-3", "Active Format Description and Bar Data"},
	{ 0x41, 0x06,"S2016-4", "Pan-Scan Data"},
	{ 0x41, 0x07,  "S2010", "ANSI/SCTE 104 messages"},
	{ 0x41, 0x08,  "S2031", "DVB/SCTE VBI data"},
	{ 0x41, 0x09,  "S2056", "MPEG TS packets in VANC"},
	{ 0x41, 0x0A,  "S2068", "Stereoscopic 3D Frame Compatible Packing and Signaling"},
	{ 0x41, 0x0B,"S2064-2", "Lip Sync data as specified by ST 2064-1"},
	{ 0x41, 0x0C,"S2108-1", "Extended HDR/WCG for SDI"},
	{ 0x41, 0x0D,"S2108-2", "Vertical Ancillary Data Mapping of KLV Formatted HDR/WCG Metadata"},
	{ 0x43, 0x01,"ITU-R BT.1685", "Structure of inter-station control data"},
	{ 0x43, 0x02,   "RDD8", "Subtitling Distribution packet (SDP)"},
	{ 0x43, 0x03,   "RDD8", "Transport of ANC packet in an ANC Multipacket"},
	{ 0x43, 0x04,"ARIB RT-B29", "Monitoring Metadata for ARIB Broadcasting chain"},
	{ 0x43, 0x05,  "RDD18", "Acquisition Metadata Sets for Video Camera Parameters"},
	{ 0x44, 0x04,  "RP214", "KLV Metadata transport in VANC space"},
	{ 0x44, 0x14,  "RP214", "KLV Metadata transport in HANC space"},
	{ 0x44, 0x44,  "RP223", "UMID and Program Identification Label Data"},
	{ 0x45, 0x01,"S2020-1", "Compressed Audio Metadata"},
	{ 0x45, 0x02,"S2020-1", "Compressed Audio Metadata"},
	{ 0x45, 0x03,"S2020-1", "Compressed Audio Metadata"},
	{ 0x45, 0x04,"S2020-1", "Compressed Audio Metadata"},
	{ 0x45, 0x05,"S2020-1", "Compressed Audio Metadata"},
	{ 0x45, 0x06,"S2020-1", "Compressed Audio Metadata"},
	{ 0x45, 0x07,"S2020-1", "Compressed Audio Metadata"},
	{ 0x45, 0x08,"S2020-1", "Compressed Audio Metadata"},
	{ 0x45, 0x09,"S2020-1", "Compressed Audio Metadata"},
	{ 0x46, 0x01,  "S2051", "Two Frame Marker in HANC"},
	{ 0x50, 0x01,   "RDD8", "WSS Data"},

	/* Note: Not registered with SMPTE-RA (taken from ARIB STD-B37 Version 2.4-E1) */
	{ 0x5F, 0xDC,"ARIB STD-B37", "ARIB Captioning - Mobile"},
	{ 0x5F, 0xDD,"ARIB STD-B37", "ARIB Captioning - Analog"},
	{ 0x5F, 0xDE,"ARIB STD-B37", "ARIB Captioning - SD"},
	{ 0x5F, 0xDF,"ARIB STD-B37", "ARIB Captioning - HD"},

	{ 0x51, 0x01,  "RP215", "Film Codes in VANC space"},
	{ 0x60, 0x60, "S12M-2", "Ancillary Time Code"},
	{ 0x60, 0x61, "S12M-3", "Time Code for High Frame Rate Signals"},
	{ 0x60, 0x62,  "S2103", "Generic Time Label"},
	{ 0x61, 0x01, "S334-1", "EIA 708B Data mapping into HDTV VBI, VANC space"},
	{ 0x61, 0x02, "S334-1", "EIA 608 Data mapping into HDTV VBI, VANC space"},

	/* Note: Not registered with SMPTE-RA (origin/validity unclear) */
	{ 0x61, 0x03,   "S334", "World System Teletext Description Packet"},
	{ 0x61, 0x04,   "S334", "Subtitling Data Essence (SDE)"},
	{ 0x61, 0x05,   "S334", "ARIB Captioning - HD"},
	{ 0x61, 0x06,   "S334", "ARIB Captioning - SD"},
	{ 0x61, 0x07,   "S334", "ARIB Captioning - Analog"},

	{ 0x62, 0x01,  "RP207", "Program Description in VANC space"},
	{ 0x62, 0x02, "S334-1", "Data broadcast (DTV) in VANC space"},
	{ 0x62, 0x03,  "RP208", "VBI Data in VANC space"},
	{ 0x64, 0x64,  "RP196", "Time Code in HANC space (Deprecated)"},
	{ 0x64, 0x7F,  "RP196", "VITC in HANC space (Deprecated)"},
};

static struct did_s dids_t1[] = {
	{ 0x00, 0x00,  "S291", "Undefined Data (Deprecated)"},
	{ 0x80, 0x00,  "S291", "Packet marked for deletion"},
	{ 0x84, 0x00,  "S291", "End packet deleted (Deprecated)"},
	{ 0x88, 0x00,  "S291", "Start packet deleted (Deprecated)"},
	{ 0xA0, 0x00,"S299-2", "Audio data in HANC space (3G) G8 Control"},
	{ 0xA1, 0x00,"S299-2", "Audio data in HANC space (3G) G7 Control"},
	{ 0xA2, 0x00,"S299-2", "Audio data in HANC space (3G) G6 Control"},
	{ 0xA3, 0x00,"S299-2", "Audio data in HANC space (3G) G5 Control"},
	{ 0xA4, 0x00,"S299-2", "Audio data in HANC space (3G) G8"},
	{ 0xA5, 0x00,"S299-2", "Audio data in HANC space (3G) G7"},
	{ 0xA6, 0x00,"S299-2", "Audio data in HANC space (3G) G6"},
	{ 0xA7, 0x00,"S299-2", "Audio data in HANC space (3G) G5"},
	{ 0xE0, 0x00,"S299-1", "Audio data in HANC space (HDTV)"},
	{ 0xE1, 0x00,"S299-1", "Audio data in HANC space (HDTV)"},
	{ 0xE2, 0x00,"S299-1", "Audio data in HANC space (HDTV)"},
	{ 0xE3, 0x00,"S299-1", "Audio data in HANC space (HDTV)"},
	{ 0xE4, 0x00,"S299-1", "Audio data in HANC space (HDTV)"},
	{ 0xE5, 0x00,"S299-1", "Audio data in HANC space (HDTV)"},
	{ 0xE6, 0x00,"S299-1", "Audio data in HANC space (HDTV)"},
	{ 0xE7, 0x00,"S299-1", "Audio data in HANC space (HDTV)"},
	{ 0xEC, 0x00,  "S272", "Audio data in HANC space (SDTV)"},
	{ 0xED, 0x00,  "S272", "Audio data in HANC space (SDTV)"},
	{ 0xEE, 0x00,  "S272", "Audio data in HANC space (SDTV)"},
	{ 0xEF, 0x00,  "S272", "Audio data in HANC space (SDTV)"},
	{ 0xF0, 0x00,  "S315", "Camera Position (HANC or VANC space)"},
	{ 0xF4, 0x00, "RP165", "Error Detection and Handling, HANC space"},
	{ 0xF8, 0x00,  "S272", "Audio Data in HANC space (SDTV)"},
	{ 0xF9, 0x00,  "S272", "Audio Data in HANC space (SDTV)"},
	{ 0xFA, 0x00,  "S272", "Audio Data in HANC space (SDTV)"},
	{ 0xFB, 0x00,  "S272", "Audio Data in HANC space (SDTV)"},
	{ 0xFC, 0x00,  "S272", "Audio Data in HANC space (SDTV)"},
	{ 0xFD, 0x00,  "S272", "Audio Data in HANC space (SDTV)"},
	{ 0xFE, 0x00,  "S272", "Audio Data in HANC space (SDTV)"},
	{ 0xFF, 0x00,  "S272", "Audio Data in HANC space (SDTV)"},
};

const char *klvanc_didLookupDescription(uint16_t did, uint16_t sdid)
{
	for (unsigned int i = 0; i < (sizeof(dids) / sizeof(struct did_s)); i++) {
		if ((did == dids[i].did) && (sdid == dids[i].sdid))
			return dids[i].desc;
	}

	/* No match for Type 2, check type 1 */
	for (unsigned int i = 0; i < (sizeof(dids_t1) / sizeof(struct did_s)); i++) {
		if (did == dids_t1[i].did)
			return dids_t1[i].desc;
	}

	return "Undefined";
}

const char *klvanc_didLookupSpecification(uint16_t did, uint16_t sdid)
{
	for (unsigned int i = 0; i < (sizeof(dids) / sizeof(struct did_s)); i++) {
		if ((did == dids[i].did) && (sdid == dids[i].sdid))
			return dids[i].spec;
	}

	/* No match for Type 2, check type 1 */
	for (unsigned int i = 0; i < (sizeof(dids_t1) / sizeof(struct did_s)); i++) {
		if (did == dids_t1[i].did)
			return dids_t1[i].spec;
	}

	return "Undefined";
}
