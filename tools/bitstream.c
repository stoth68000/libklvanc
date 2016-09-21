/* The following bitstream implementation was found on stackoverflow:
 * http://stackoverflow.com/questions/22202879/efficient-c-bitstream-implementation
 * It has no copyight and thus considered public domain.
 */

#include <stdio.h>
#include <stdlib.h>

#include "bitstream.h"

void bitstream_attach(wBitStream* bs, uint8_t* buffer, uint32_t capacity)
{
    bs->position = 0;
    bs->buffer = buffer;
    bs->offset = 0;
    bs->accumulator = 0;
    bs->pointer = bs->buffer;
    bs->capacity = capacity;
    bs->length = bs->capacity * 8;
}

wBitStream* bitstream_new()
{
    wBitStream* bs = NULL;

    bs = (wBitStream*) calloc(1, sizeof(wBitStream));

    return bs;
}

void bitstream_free(wBitStream* bs)
{
    free(bs);
}

int bitstream_file_save(wBitStream* bs, const char *fn)
{
	int ret = 0;

	FILE *fh = fopen(fn, "wb");
	if (!fh)
		return -1;

	if (fwrite(bs->buffer, bitstream_getlength_bytes(bs), 1, fh) != bitstream_getlength_bytes(bs))
		ret = -1;

	fclose(fh);

	return ret;
}
