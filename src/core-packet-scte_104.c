#include <libklvanc/vanc.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRINT_DEBUG_MEMBER_INT(m) printf(" %s = 0x%x\n", #m, m);

int dump_SCTE_104(struct vanc_context_s *ctx, void *p)
{
	struct packet_scte_104_s *pkt = p;

	if (ctx->verbose)
		printf("%s() %p\n", __func__, (void *)pkt);

        struct splice_request_data *d = &pkt->sr_data;
	struct single_operation_message *m = &pkt->so_msg;

	printf("SCTE104 struct\n");
	PRINT_DEBUG_MEMBER_INT(pkt->payloadDescriptorByte);

	PRINT_DEBUG_MEMBER_INT(m->opID);
	printf("   opID = %s\n", m->opID == INIT_REQUEST_DATA ? "init_request_data" : "UNSUPPORTED");
	PRINT_DEBUG_MEMBER_INT(m->messageSize);
	printf("   message_size = %d bytes\n", m->messageSize);
	PRINT_DEBUG_MEMBER_INT(m->result);
	PRINT_DEBUG_MEMBER_INT(m->result_extension);
	PRINT_DEBUG_MEMBER_INT(m->protocol_version);
	PRINT_DEBUG_MEMBER_INT(m->AS_index);
	PRINT_DEBUG_MEMBER_INT(m->message_number);
	PRINT_DEBUG_MEMBER_INT(m->DPI_PID_index);

	if (m->opID == INIT_REQUEST_DATA) {
		PRINT_DEBUG_MEMBER_INT(d->splice_insert_type);
		printf("   splice_insert_type = %s\n",
			d->splice_insert_type == SPLICESTART_IMMEDIATE ? "spliceStart_immediate" :
			d->splice_insert_type == SPLICEEND_IMMEDIATE ? "spliceEnd_immediate" : "UNSUPPORTED");
		PRINT_DEBUG_MEMBER_INT(d->splice_event_id);
		PRINT_DEBUG_MEMBER_INT(d->unique_program_id);
		PRINT_DEBUG_MEMBER_INT(d->pre_roll_time);
		PRINT_DEBUG_MEMBER_INT(d->brk_duration);
		printf("   break_duration = %d (1/10th seconds)\n", d->brk_duration);
		PRINT_DEBUG_MEMBER_INT(d->avail_num);
		PRINT_DEBUG_MEMBER_INT(d->avails_expected);
		PRINT_DEBUG_MEMBER_INT(d->auto_return_flag);
	} else
		printf("   unsupported m->opID = 0x%x\n", m->opID);

	for (int i = 0; i < pkt->payloadLengthBytes; i++)
		printf("%02x ", pkt->payload[i]);
	printf("\n");

	return KLAPI_OK;
}

int parse_SCTE_104(struct vanc_context_s *ctx, struct packet_header_s *hdr, void **pp)
{
	if (ctx->verbose)
		printf("%s()\n", __func__);

	struct packet_scte_104_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));

        pkt->payloadDescriptorByte = hdr->payload[0];
        pkt->version               = (pkt->payloadDescriptorByte >> 3) & 0x03;
        pkt->continued_pkt         = (pkt->payloadDescriptorByte >> 2) & 0x01;
        pkt->following_pkt         = (pkt->payloadDescriptorByte >> 1) & 0x01;
        pkt->duplicate_msg         = pkt->payloadDescriptorByte & 0x01;

	/* We only support SCTE104 messages of type
	 * single_operation_message() that are completely
	 * self containined with a single VANC line, and
	 * are not continuation messages.
	 * Eg. payloadDescriptor value 0x08.
	 */
	if (pkt->payloadDescriptorByte != 0x08) {
		free(pkt);
		return -1;
	}

	/* First byte is the padloadDescriptor, the rest is the SCTE104 message...
	 * up to 200 bytes in length item 5.3.3 page 7 */
	for (int i = 0; i < 200; i++)
		pkt->payload[i] = hdr->payload[1 + i];

	struct single_operation_message *m = &pkt->so_msg;
	m->opID             = pkt->payload[0] << 8 | pkt->payload[1];
	m->messageSize      = pkt->payload[2] << 8 | pkt->payload[3];
	m->result           = pkt->payload[4] << 8 | pkt->payload[5];
	m->result_extension = pkt->payload[6] << 8 | pkt->payload[7];
	m->protocol_version = pkt->payload[8];
	m->AS_index         = pkt->payload[9];
	m->message_number   = pkt->payload[10];
	m->DPI_PID_index    = pkt->payload[11] << 8 | pkt->payload[12];

	struct splice_request_data *d = &pkt->sr_data;
	if (m->opID == INIT_REQUEST_DATA) {
		d->splice_insert_type  = pkt->payload[13];
		d->splice_event_id     = pkt->payload[14] << 24 |
			pkt->payload[15] << 16 | pkt->payload[16] <<  8 | pkt->payload[17];
		d->unique_program_id   = pkt->payload[18] << 8 | pkt->payload[19];
		d->pre_roll_time       = pkt->payload[20] << 8 | pkt->payload[21];
		d->brk_duration        = pkt->payload[22] << 8 | pkt->payload[23];
		d->avail_num           = pkt->payload[24];
		d->avails_expected     = pkt->payload[25];
		d->auto_return_flag    = pkt->payload[26];
	} else {
		fprintf(stderr, "%s() Unsupported opID = %x, error.\n", __func__, m->opID);
		free(pkt);
		return -1;
	}

	/* We only support spliceStart_immediate and spliceEnd_immediate */
	switch (d->splice_insert_type) {
	case SPLICESTART_IMMEDIATE:
	case SPLICEEND_IMMEDIATE:
		break;
	default:
		/* We don't support this splice command */
		fprintf(stderr, "%s() splice_insert_type 0x%x, error.\n", __func__, d->splice_insert_type);
		free(pkt);
		return -1;
	}

	if (ctx->callbacks && ctx->callbacks->scte_104)
		ctx->callbacks->scte_104(ctx->callback_context, ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

