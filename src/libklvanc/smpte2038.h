/* Copyright (c) 2016 Kernel Labs Inc. */

#ifndef SMPTE2038_H
#define SMPTE2038_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct smpte2038_anc_data_line_s
{
	uint8_t		reserved_000000;
	uint8_t		c_not_y_channel_flag;
	uint16_t	line_number;
	uint16_t	horizontal_offset;
	uint16_t	DID;
	uint16_t	SDID;
	uint16_t	data_count;
	uint16_t	*user_data_words;
	uint16_t	checksum_word;
};

struct smpte2038_anc_data_packet_s
{
	uint32_t	packet_start_code_prefix;
	uint8_t		stream_id;
	uint16_t	PES_packet_length;
	uint8_t		reserved_10;
	uint8_t		PES_scrambling_control;
	uint8_t		PES_priority;
	uint8_t		data_alignment_indicator;
	uint8_t		copyright;
	uint8_t		original_or_copy;
	uint8_t		PTS_DTS_flags;
	uint8_t		ESCR_flag;
	uint8_t		ES_rate_flag;
	uint8_t		DSM_trick_mode_flag;
	uint8_t		additional_copy_info_flag;
	uint8_t		PES_CRC_flag;
	uint8_t		PES_extension_flag;
	uint8_t		PES_header_data_length;
	uint8_t		reserved_0010;
	uint64_t	PTS;

	int lineCount;
	struct smpte2038_anc_data_line_s *lines;
};

int  smpte2038_parse_section(uint8_t *section, unsigned int byteCount, struct smpte2038_anc_data_packet_s **result);
void smpte2038_smpte2038_anc_data_packet_dump(struct smpte2038_anc_data_packet_s *h);
void smpte2038_anc_data_packet_free(struct smpte2038_anc_data_packet_s *pkt);

#ifdef __cplusplus
};
#endif

#endif /* SMPTE2038_H */
