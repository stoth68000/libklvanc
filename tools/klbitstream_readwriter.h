/* Copyright Kernel Labs Inc. 2016. All Rights Reserved. */

#include <stdint.h>
#include <string.h>

#ifndef KLBITSTREAM_READWRITER_H
#define KLBITSTREAM_READWRITER_H

struct bs_context_s
{
	uint8_t  *buf;
	uint32_t  buflen;	/* Bytes */
	uint32_t  buflen_used;	/* Bytes */
	uint8_t   reg_used;	/* bits 1..8 */

	/* A shift register */
	/* Write bits are clocked in from LSB. */
	/* Read bits are clocked out from the MSB. */
	uint8_t  reg;
};
#define bs_get_byte_count(ctx) ((ctx)->buflen_used)
#define bs_get_buffer(ctx) ((ctx)->buf)

static __inline__ struct bs_context_s * bs_alloc()
{
	return (struct bs_context_s *)calloc(1, sizeof(struct bs_context_s));
}

static __inline__ int bs_save(struct bs_context_s *ctx, const char *fn)
{
	FILE *fh = fopen(fn, "wb");
	if (!fh)
		return -1;

	fwrite(ctx->buf, 1, ctx->buflen_used, fh);
	fclose(fh);

	return 0;
}

static __inline__ void bs_free(struct bs_context_s *ctx)
{
	free(ctx);
}

static __inline__ void bs_init(struct bs_context_s *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
}

static __inline__ void bs_write_set_buffer(struct bs_context_s *ctx, uint8_t *buf, uint32_t lengthBytes)
{
	bs_init(ctx);
	ctx->buf = buf;
	ctx->buflen = lengthBytes;
}

static __inline__ void bs_read_buffer_set(struct bs_context_s *ctx, uint8_t *buf, uint32_t lengthBytes)
{
	bs_write_set_buffer(ctx, buf, lengthBytes);
	ctx->buflen_used = lengthBytes;
}

/* Clock into the LSB */
static __inline__ void bs_write_bit(struct bs_context_s *ctx, uint32_t bit)
{
	bit &= 1;
	if (ctx->reg_used < 8) {
		ctx->reg <<= 1;
		ctx->reg |= bit;
		ctx->reg_used++;
	}

	if (ctx->reg_used == 8) {
		*(ctx->buf + ctx->buflen_used++) = ctx->reg;
		ctx->reg_used = 0;
		printf("Wrote %02x, size = %d\n", *(ctx->buf + ctx->buflen_used - 1), ctx->buflen_used);
	}
}

static __inline__ void bs_write_bits(struct bs_context_s *ctx, uint32_t bits, uint32_t bitcount)
{
	for (int i = (bitcount - 1); i >= 0; i--)
		bs_write_bit(ctx, bits >> i);
}

static __inline__ void bs_write_buffer_complete(struct bs_context_s *ctx)
{
	if (ctx->reg_used > 0) {
printf("reg_used = %d\n", ctx->reg_used);
		for (int i = ctx->reg_used; i <= 8; i++)
			bs_write_bit(ctx, 0);
	}
	printf("complete. buf size = %d bytes\n", ctx->buflen_used);
}

/* Bits are clocked in MSB */
#define bs_write_bits8(ctx, bits) \
	bs_write_bit(ctx, bits & 0x80 ? 1 : 0); \
	bs_write_bit(ctx, bits & 0x40 ? 1 : 0); \
	bs_write_bit(ctx, bits & 0x20 ? 1 : 0); \
	bs_write_bit(ctx, bits & 0x10 ? 1 : 0); \
	bs_write_bit(ctx, bits & 0x08 ? 1 : 0); \
	bs_write_bit(ctx, bits & 0x04 ? 1 : 0); \
	bs_write_bit(ctx, bits & 0x02 ? 1 : 0); \
	bs_write_bit(ctx, bits & 0x01 ? 1 : 0); \

/* Bits are clocked in MSB */
#define bs_write_bits16(ctx, bits)		\
	bs_write_bits8(ctx, bits >> 8);		\
	bs_write_bits8(ctx, bits);		\

/* Bits are clocked in MSB */
#define bs_write_bits32(ctx, bits)		\
	bs_write_bits8(ctx, bits >> 24);	\
	bs_write_bits8(ctx, bits >> 16);	\
	bs_write_bits8(ctx, bits >>  8);	\
	bs_write_bits8(ctx, bits);		\

/* Clocked out from MSB */
static __inline__ uint32_t bs_read_bit(struct bs_context_s *ctx)
{
	uint32_t bit;

	if (ctx->reg_used == 0) {
		ctx->reg = *(ctx->buf + ctx->buflen_used++);
		ctx->reg_used = 8;
	}

	if (ctx->reg_used <= 8) {
		bit = ctx->reg & 0x80 ? 1 : 0;
		ctx->reg <<= 1;
		ctx->reg_used--;
	}

printf("%d ", bit);
	if (ctx->reg_used == 0)
		printf(" -- ");
	return bit;
}

/* Bits are clocked out MSB */
#define bs_read_bits8(ctx) \
	(	bs_read_bit(ctx) << 7 | \
		bs_read_bit(ctx) << 6 | \
		bs_read_bit(ctx) << 5 | \
		bs_read_bit(ctx) << 4 | \
		bs_read_bit(ctx) << 3 | \
		bs_read_bit(ctx) << 2 | \
		bs_read_bit(ctx) << 1 | \
		bs_read_bit(ctx) \
	)

#define bs_read_bits16(ctx) \
	( bs_read_bits8(ctx) << 8 | bs_read_bits8(ctx) )

#define bs_read_bits32(ctx) \
	( bs_read_bits8(ctx) << 24 | bs_read_bits8(ctx) << 16 | bs_read_bits8(ctx) << 8 | bs_read_bits8(ctx) )

#endif /* KLBITSTREAM_READWRITER_H */
