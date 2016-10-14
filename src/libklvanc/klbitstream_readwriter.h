/**
 * @file        klbitstream_readwriter.h
 * @author      Kernel Labs Inc
 * @brief       Simplistic bitstream reader/writer capable of supporting 1..32
 *              1..32 bit writes or reads.
 *              Buffers are used exclusively in either read or write mode, and cannot be combined.
 */

/* Copyright Kernel Labs Inc. 2016. All Rights Reserved. */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef KLBITSTREAM_READWRITER_H
#define KLBITSTREAM_READWRITER_H

struct klbs_context_s
{
	/* Private, so not inspect directly. Use macros where necessary. */
	uint8_t  *buf;		/* Pointer to the user allocated read/write buffer */
	uint32_t  buflen;	/* Total buffer size - Bytes */
	uint32_t  buflen_used;	/* Amount of data previously WRITTEN to the buffer. Invalid for read bitstreams. */
	uint8_t   reg_used;	/* bits 1..8 */

	/* An 8bit shift register */
	/* Write bits are clocked in from LSB. */
	/* Read bits are clocked out from the MSB. */
	uint8_t  reg;
};

/**
 * @brief       Helper Macro. Return the number of used bytes in the buffer (one valid for write buffers).
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 * @return      Return the number of used bytes in the buffer.
 */
#define klbs_get_byte_count(ctx) ((ctx)->buflen_used)

/**
 * @brief       Helper Macro. Return the buffer address.
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 * @return      Buffer address.
 */
#define klbs_get_buffer(ctx) ((ctx)->buf)

/**
 * @brief       Allocate a new bitstream context, for read or write use.
 * @return      struct klbs_context_s *  The context itself, or NULL on error.
 */
static __inline__ struct klbs_context_s * klbs_alloc()
{
	return (struct klbs_context_s *)calloc(1, sizeof(struct klbs_context_s));
}

/**
 * @brief       Save the entire buffer to file.
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 * @param[in]   const char *fn  Output filename.
 * @return      0 - Success
 * @return      < 0 - Error
 */
static __inline__ int klbs_save(struct klbs_context_s *ctx, const char *fn)
{
	FILE *fh = fopen(fn, "wb");
	if (!fh)
		return -1;

	fwrite(ctx->buf, 1, ctx->buflen_used, fh);
	fclose(fh);

	return 0;
}

/**
 * @brief       Destroy and deallocate a previously allocated context.
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 */
static __inline__ void klbs_free(struct klbs_context_s *ctx)
{
	free(ctx);
}

/**
 * @brief       Initialize / reset a previously allocated context.
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 */
static __inline__ void klbs_init(struct klbs_context_s *ctx)
{
	memset(ctx, 0, sizeof(*ctx));
}

/**
 * @brief       Associate a previously allocated user buffer 'buf' with the bitstream context.
 *              The context will write data directly into this user buffer.
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 * @param[in]   uint8_t *buf  Buffer the bistream write calls will modify
 * @param[in]   uint32_t *buf  Buffer size in bytes.
 */
static __inline__ void klbs_write_set_buffer(struct klbs_context_s *ctx, uint8_t *buf, uint32_t lengthBytes)
{
	klbs_init(ctx);
	ctx->buf = buf;
	ctx->buflen = lengthBytes;
}

/**
 * @brief       Associate a previously allocated user buffer 'buf' with the bitstream context.
 *              The context will read data directly from this user buffer.
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 * @param[in]   uint8_t *buf  Buffer the bistream will read from.
 * @param[in]   uint32_t *buf  Buffer size in bytes.
 */
static __inline__ void klbs_read_set_buffer(struct klbs_context_s *ctx, uint8_t *buf, uint32_t lengthBytes)
{
	klbs_write_set_buffer(ctx, buf, lengthBytes);
}

/**
 * @brief       Write a single bit into the previously associated user buffer.
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 * @param[in]   uint32_t bit  A single bit.
 */
static __inline__ void klbs_write_bit(struct klbs_context_s *ctx, uint32_t bit)
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
//		printf("Wrote %02x, size = %d\n", *(ctx->buf + ctx->buflen_used - 1), ctx->buflen_used);
	}
}

/**
 * @brief       Pad the bitstream buffer into byte alignment, stuff the 'bit' mutiple times to align.
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 * @param[in]   uint32_t bit  A single bit.
 */
static __inline__ void klbs_write_byte_stuff(struct klbs_context_s *ctx, uint32_t bit)
{
	while (ctx->reg_used > 0)
		klbs_write_bit(ctx, bit);
}

/**
 * @brief       Write multiple bits of data into the previously associated user buffer.
 *              Writes are LSB justified, so the bits value 0x101, is nine bits.
 *              Omitting this step could lead to a bistream thats one byte too short.
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 * @param[in]   uint32_t bits  data pattern.
 * @param[in]   uint32_t bitcount  number of bits to write
 */
static __inline__ void klbs_write_bits(struct klbs_context_s *ctx, uint32_t bits, uint32_t bitcount)
{
	for (int i = (bitcount - 1); i >= 0; i--)
		klbs_write_bit(ctx, bits >> i);
}

/**
 * @brief       Flush any intermediate bits out to the buffer, once no omre data needs to be written.
 *              This ensures that any dangling trailing bits are properly stuffer and written to the buffer.
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 */
static __inline__ void klbs_write_buffer_complete(struct klbs_context_s *ctx)
{
	if (ctx->reg_used > 0) {
//printf("reg_used = %d\n", ctx->reg_used);
		for (int i = ctx->reg_used; i <= 8; i++)
			klbs_write_bit(ctx, 0);
	}
//	printf("complete. buf size = %d bytes\n", ctx->buflen_used);
}

/**
 * @brief       Read a single bit from the bitstream.
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 * @return      uint32_t  a bit
 */
static __inline__ uint32_t klbs_read_bit(struct klbs_context_s *ctx)
{
	uint32_t bit = 0;

	if (ctx->reg_used == 0) {
		ctx->reg = *(ctx->buf + ctx->buflen_used++);
		ctx->reg_used = 8;
	}

	if (ctx->reg_used <= 8) {
		bit = ctx->reg & 0x80 ? 1 : 0;
		ctx->reg <<= 1;
		ctx->reg_used--;
	}
#if 0
printf("%d ", bit);
	if (ctx->reg_used == 0)
		printf(" -- ");
#endif
	return bit;
}

/**
 * @brief       Read between 1..32 bits from the bitstream.
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 * @return      uint32_t  bits
 */
static __inline__ uint32_t klbs_read_bits(struct klbs_context_s *ctx, uint32_t bitcount)
{
	uint32_t bits = 0;
	for (uint32_t i = 1; i <= bitcount; i++) {
		bits <<= 1;
		bits |= klbs_read_bit(ctx);
	}
	return bits;
}

/**
 * @brief       Read and discard all bits in the buffer until we're byte aligned again.\n
 *              The sister function to klbs_write_byte_stuff();
 * @param[in]   struct klbs_context_s *ctx  bitstream context
 */
static __inline__ void klbs_read_byte_stuff(struct klbs_context_s *ctx)
{
	while (ctx->reg_used > 0)
		klbs_read_bit(ctx);
}



#endif /* KLBITSTREAM_READWRITER_H */
