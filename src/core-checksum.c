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
uint16_t vanc_checksum_calculate(uint16_t *words, int wordCount)
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
int vanc_checksum_is_valid(uint16_t *words, int wordCount)
{
	uint16_t sum = vanc_checksum_calculate(words, wordCount - 1);
	if (sum != *(words + (wordCount - 1)))
		return 0;

	return 1;
}
