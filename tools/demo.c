#include <stdio.h>
#include <stdlib.h>
#include <libklvanc/vanc.h>

/* CALLBACKS for message notification */
static int cb_PAYLOAD_INFORMATION(void *callback_context, struct vanc_context_s *ctx, struct packet_payload_information_s *pkt)
{
	printf("%s:%s()\n", __FILE__, __func__);

	/* Have the library display some debug */
	printf("Asking libklvanc to dump a struct\n");
	dump_PAYLOAD_INFORMATION(ctx, pkt);

	return 0;
}

static int cb_EIA_708B(void *callback_context, struct vanc_context_s *ctx, struct packet_eia_708b_s *pkt)
{
	printf("%s:%s()\n", __FILE__, __func__);

	/* Have the library display some debug */
	printf("Asking libklvanc to dump a struct\n");
	dump_EIA_708B(ctx, pkt);

	return 0;
}

static int cb_EIA_608(void *callback_context, struct vanc_context_s *ctx, struct packet_eia_608_s *pkt)
{
	printf("%s:%s()\n", __FILE__, __func__);

	/* Have the library display some debug */
	printf("Asking libklvanc to dump a struct\n");
	dump_EIA_608(ctx, pkt);

	return 0;
}

static int cb_SCTE_104(void *callback_context, struct vanc_context_s *ctx, struct packet_scte_104_s *pkt)
{
	printf("%s:%s()\n", __FILE__, __func__);

	/* Have the library display some debug */
	printf("Asking libklvanc to dump a struct\n");
	dump_SCTE_104(ctx, pkt);

	return 0;
}

static struct vanc_callbacks_s callbacks = 
{
	.payload_information	= cb_PAYLOAD_INFORMATION,
	.eia_708b		= cb_EIA_708B,
	.eia_608		= cb_EIA_608,
	.scte_104		= cb_SCTE_104,
};
/* END - CALLBACKS for message notification */

static int test_PAYLOAD_INFORMATION(struct vanc_context_s *ctx)
{
	unsigned short arr[] = {
		0x000,
		0x3ff,
		0x3ff,
		0x241,
		0x105,
		0x108,
		0x07c, /* Payload Field */
		0x000, /* Payload Field */
		0x000, /* Payload Field */
		0x000, /* Payload Field */
		0x000, /* Payload Field */
		0x010, /* Payload Field */
		0x000, /* Payload Field */
		0x008, /* Payload Field */
		0xFFF, /* Checksum */
	};

	/* report that this was from line 13, informational only. */
	int ret = vanc_packet_parse(ctx, 13, arr, sizeof(arr) / (sizeof(unsigned short)));
	if (ret < 0)
		return ret;

	return 0;
}

static int test_EIA_708B(struct vanc_context_s *ctx)
{
	unsigned short arr[] = {
		0x0, 0x0, 0x0, /* Spurious junk prefix for testing */
		0x000, 0x3ff, 0x3ff, 0x161, 0x101, 0x152, 0x296, 0x269,
		0x152, 0x14f, 0x277, 0x2b8, 0x1ad, 0x272, 0x1f4, 0x2fc,
		0x180, 0x180, 0x1fd, 0x180, 0x180, 0x2fa, 0x200, 0x200,
		0x2fa, 0x200, 0x200, 0x2fa, 0x200, 0x200, 0x2fa, 0x200,
		0x200, 0x2fa, 0x200, 0x200, 0x2fa, 0x200, 0x200, 0x2fa,
		0x200, 0x200, 0x2fa, 0x200, 0x200, 0x2fa, 0x200, 0x200,
		0x2fa, 0x200, 0x200, 0x2fa, 0x200, 0x200, 0x2fa, 0x200,
		0x200, 0x2fa, 0x200, 0x200, 0x2fa, 0x200, 0x200, 0x2fa,
		0x200, 0x200, 0x2fa, 0x200, 0x200, 0x2fa, 0x200, 0x200,
		0x2fa, 0x200, 0x200, 0x173, 0x2d1, 0x1e0, 0x200, 0x200,
		0x200, 0x200, 0x200, 0x200, 0x274, 0x2b8, 0x1ad, 0x194,
		0x1b4 /* Checksum */
	};

	int ret = vanc_packet_parse(ctx, 1, arr, sizeof(arr) / (sizeof(unsigned short)));
	if (ret < 0)
		return ret;

	return 0;
}

int demo_main(int argc, char *argv[])
{
	struct vanc_context_s *ctx;
	int ret;

	if (vanc_context_create(&ctx) < 0) {
		fprintf(stderr, "Error initializing library context\n");
		exit(1);
	}
	ctx->verbose = 1;
	ctx->callbacks = &callbacks;
	printf("Library initialized.\n");

	ret = test_PAYLOAD_INFORMATION(ctx);
	if (ret < 0)
		fprintf(stderr, "PAYLOAD_INFORMATION failed to parse\n");

	ret = test_EIA_708B(ctx);
	if (ret < 0)
		fprintf(stderr, "EIA_708B failed to parse\n");

	vanc_context_destroy(ctx);
	printf("Library destroyed.\n");

	return 0;
}
