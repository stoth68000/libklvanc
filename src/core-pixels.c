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
#include <libklvanc/pixels.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define av_le2ne32(x) (x)

#define READ_PIXELS(a, b, c)         \
    do {                             \
        val  = av_le2ne32( *src++ ); \
        *a++ =  val & 0x3ff;         \
        *b++ = (val >> 10) & 0x3ff;  \
        *c++ = (val >> 20) & 0x3ff;  \
    } while (0)

void klvanc_v210_planar_unpack_c(const uint32_t * src, uint16_t * y, uint16_t * u, uint16_t * v, int width)
{
	uint32_t val;

	for (int i = 0; i < width - 5; i += 6) {
		READ_PIXELS(u, y, v);
		READ_PIXELS(y, u, y);
		READ_PIXELS(v, y, u);
		READ_PIXELS(y, v, y);
	}
}

/* Convert v210 to the native HD-SDI pixel format.
 * bmdFormat10BitYUV :‘v210’4:2:2Representation
 * Twelve 10-bit unsigned components are packed into four 32-bit little-endian words.
 * See BlackMagic SDK page 280 for a detailed description.
 */
int klvanc_v210_line_to_nv20_c(const uint32_t * src, uint16_t * dst, int dstSizeBytes, int width)
{
	if (!src || !dst || !width)
		return -1;

	if (dstSizeBytes < (width * 6))
		return -1;

	int w;
	uint32_t val = 0;
	uint16_t *uv = dst + width;
	for (w = 0; w < (width - 5); w += 6) {
		READ_PIXELS(uv, dst, uv);
		READ_PIXELS(dst, uv, dst);
		READ_PIXELS(uv, dst, uv);
		READ_PIXELS(dst, uv, dst);
	}

	if (w < width - 1) {
		READ_PIXELS(uv, dst, uv);

		val = av_le2ne32(*src++);
		*dst++ = val & 0x3ff;
	}

	if (w < width - 3) {
		*uv++ = (val >> 10) & 0x3ff;
		*dst++ = (val >> 20) & 0x3ff;

		val = av_le2ne32(*src++);
		*uv++ = val & 0x3ff;
		*dst++ = (val >> 10) & 0x3ff;
	}

	return 0;
}

/* Downscale 10-bit lines to 8-bit lines for processing by libzvbi.
 * Width is always 720*2 samples */
void klvanc_v210_downscale_line_c(uint16_t * src, uint8_t * dst, int lines)
{
	for (int i = 0; i < 720 * 2 * lines; i++)
		dst[i] = src[i] >> 2;
}

/* Convert v210 to the native SD-SDI pixel format.
 * Width is always 720 samples */

/*
https://msdn.microsoft.com/en-us/library/windows/desktop/bb970578(v=vs.85).aspx#_420formats
https://developer.apple.com/library/mac/technotes/tn2162/_index.html#//apple_ref/doc/uid/DTS40013070-CH1-TNTAG8-V210__4_2_2_COMPRESSION_TYPE
y0-5  = Y
cb0-2 = U
cr0-2 = V
XXnn nnnn  nnnn bbbb  bbbb bbaa  aaaa aaaa
*/
void klvanc_v210_line_to_uyvy_c(const uint32_t * src, uint16_t * dst, int width)
{
	uint32_t val;
	for (int i = 0; i < width; i += 6) {
		READ_PIXELS(dst, dst, dst);
		READ_PIXELS(dst, dst, dst);
		READ_PIXELS(dst, dst, dst);
		READ_PIXELS(dst, dst, dst);
	}
}

static inline void put_le32(uint8_t **p, uint32_t d)
{
	uint32_t **x = (uint32_t **) p;
	**x = av_le2ne32(d);
	(*p) += 4;
}

void klvanc_y10_to_v210(uint16_t *src, uint8_t *dst, int width)
{
	size_t len = width / 6;
	size_t w;

	for (w = 0; w < len; w++) {
		put_le32(&dst, 0x200          | (src[w * 6 + 0] << 10) | (0x200 << 20));
		put_le32(&dst, src[w * 6 + 1] | (0x200 << 10)          | (src[w * 6 + 2] << 20));
		put_le32(&dst, 0x200          | (src[w * 6 + 3] << 10) | (0x200 << 20));
		put_le32(&dst, src[w * 6 + 4] | (0x200 << 10)          | (src[w * 6 + 5] << 20));
	}

	/* Handle remaining 0-5 bytes if any */
	if (width % 6 > 0)
		put_le32(&dst, 0x200          | (src[w * 6 + 0] << 10) | (0x200 << 20));
	if (width % 6 > 2)
		put_le32(&dst, src[w * 6 + 1] | (0x200 << 10)          | (src[w * 6 + 2] << 20));
	else if (width % 6 > 1)
		put_le32(&dst, src[w * 6 + 1] | (0x200 << 10)          | (0x040 << 20));
	if (width % 6 > 3)
		put_le32(&dst, 0x200          | (src[w * 6 + 3] << 10) | (0x200 << 20));
	if (width % 6 > 4)
		put_le32(&dst, src[w * 6 + 4] | (0x200 << 10)          | (0x040 << 20));
}

void klvanc_uyvy_to_v210(uint16_t *src, uint8_t *dst, int width)
{
	size_t len = width / 12;
	size_t w;

	for (w = 0; w < len; w++) {
		put_le32(&dst, (src[w * 12 + 0]) |
			 (src[w * 12 + 1] << 10) |
			 (src[w * 12 + 2] << 20));
		put_le32(&dst, (src[w * 12 + 3]) |
			 (src[w * 12 + 4] << 10) |
			 (src[w * 12 + 5] << 20));
		put_le32(&dst, (src[w * 12 + 6]) |
			 (src[w * 12 + 7] << 10) |
			 (src[w * 12 + 8] << 20));
		put_le32(&dst, (src[w * 12 + 9]) |
			 (src[w * 12 + 10] << 10) |
			 (src[w * 12 + 11] << 20));
	}

	/* Handle remaining 0-11 bytes if any */
	if (width % 12 > 2)
		put_le32(&dst, (src[w * 12 + 0]) |
			 (src[w * 12 + 1] << 10) |
			 (src[w * 12 + 2] << 20));
	else if (width % 12 > 1)
		put_le32(&dst, (src[w * 12 + 0]) |
			 (src[w * 12 + 1] << 10) |
			 (0x200 << 20));
	else if (width % 12 > 0)
		put_le32(&dst, (src[w * 12 + 0]) |
			 (0x040 << 10) |
			 (0x200 << 20));

	if (width % 12 > 5)
		put_le32(&dst, (src[w * 12 + 3]) |
			 (src[w * 12 + 4] << 10) |
			 (src[w * 12 + 5] << 20));
	else if (width % 12 > 4)
		put_le32(&dst, (src[w * 12 + 3]) |
			 (src[w * 12 + 4] << 10) |
			 (0x040 << 20));
	else if (width % 12 > 3)
		put_le32(&dst, (src[w * 12 + 3]) |
			 (0x200 << 10) |
			 (0x040 << 20));


	if (width % 12 > 8)
		put_le32(&dst, (src[w * 12 + 6]) |
			 (src[w * 12 + 7] << 10) |
			 (src[w * 12 + 8] << 20));
	else if (width % 12 > 7)
		put_le32(&dst, (src[w * 12 + 6]) |
			 (src[w * 12 + 7] << 10) |
			 (0x200 << 20));
	else if (width % 12 > 6)
		put_le32(&dst, (src[w * 12 + 6]) |
			 (0x040 << 10) |
			 (0x200 << 20));

	if (width % 12 > 10)
		put_le32(&dst, (src[w * 12 + 9]) |
			 (src[w * 12 + 10] << 10) |
			 (0x040 << 20));
	else if (width % 12 > 9)
		put_le32(&dst, (src[w * 12 + 9]) |
			 (0x200 << 10) |
			 (0x040 << 20));
}
