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
#include "core-private.h"

#include <stdio.h>
#include <stdint.h>

/* Wikipedia:
 * The last word in an ANC packet is the Checksum word. It is computed
 * by computing the sum (modulo 512) of bits 0-8 (not bit 9), of all the other
 * words in the ANC packet, excluding the packet start sequence.
 * Bit 9 of the checksum word is then defined as the inverse of bit 8.
 * Note that the checksum word does not contain a parity bit; instead, the
 * parity bits of other words are included in the checksum calculations.
 * Checksumming begins after the AFD and should conclude on the last word
 * of the message, sans the checksum byte itself.
 *
 * To be clear, on the words 111, 222 and 333 should be passed to this routine.
 * 000 03FF 03FF 111 222 333 CHKSUM
 * So pass the address of 111 with a wordCount of 3.
 */
uint16_t klvanc_checksum_calculate(const uint16_t *words, int wordCount)
{
	uint16_t s = 0;
	for (uint16_t i = 0; i < wordCount; i++) {
		s += *(words + i);
		s &= 0x01ff;
	}
	s |= ((~s & 0x0100) << 1);

	return s;
}

/* For a given list of words, excludint the ADF, ending in a checksum,
 * calculate the checksum and verify the last word in the array is a correct
 * calculation of all the prior words in the array.
 *
 * To be clear, on the words 111, 222, 333 and CHECKSUM should be passed to this routine.
 * 000 03FF 03FF 111 222 333 CHECKSUM
 * So pass the address of 111 with a wordCount of 3.
 *
 * Returns: Boolean true or false.
 */
int klvanc_checksum_is_valid(const uint16_t *words, int wordCount)
{
#define LOCAL_DEBUG 0
#if 0
	printf("%s(%p, %d): ", __func__, words, wordCount);
	for (int i = 0; i < wordCount; i++)
		printf("%04x ", *(words + i));
	printf("\n");
#endif
	uint16_t sum = klvanc_checksum_calculate(words, wordCount - 1);
	if (sum != *(words + (wordCount - 1))) {
#if LOCAL_DEBUG
		fprintf(stderr, "Checksum calculated as %04x, but passed as %04x\n", sum, *(words + (wordCount - 1)));
#endif
		return 0;
	}

	return 1;
}
