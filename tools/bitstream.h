/* The following bitstream implementation was found on stackoverflow:
 * http://stackoverflow.com/questions/22202879/efficient-c-bitstream-implementation
 * It has no copyight and thus considered public domain.
 */

#ifndef BITSTREAM_H
#define BITSTREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct _wBitStream
{
    uint8_t *buffer;
    uint8_t *pointer;
    uint32_t position;
    uint32_t length;
    uint32_t capacity;
    uint32_t mask;
    uint32_t offset;
    uint32_t prefetch;
    uint32_t accumulator;
};
typedef struct _wBitStream wBitStream;

#define bitstream_prefetch(_bs) do { \
    (_bs->prefetch) = 0; \
    if (((uint32_t) (_bs->pointer - _bs->buffer)) < (_bs->capacity + 4)) \
        (_bs->prefetch) |= (*(_bs->pointer + 4) << 24); \
    if (((uint32_t) (_bs->pointer - _bs->buffer)) < (_bs->capacity + 5)) \
        (_bs->prefetch) |= (*(_bs->pointer + 5) << 16); \
    if (((uint32_t) (_bs->pointer - _bs->buffer)) < (_bs->capacity + 6)) \
        (_bs->prefetch) |= (*(_bs->pointer + 6) << 8); \
    if (((uint32_t) (_bs->pointer - _bs->buffer)) < (_bs->capacity + 7)) \
        (_bs->prefetch) |= (*(_bs->pointer + 7) << 0); \
} while(0)

#define bitstream_fetch(_bs) do { \
    (_bs->accumulator) = 0; \
    if (((uint32_t) (_bs->pointer - _bs->buffer)) < (_bs->capacity + 0)) \
        (_bs->accumulator) |= (*(_bs->pointer + 0) << 24); \
    if (((uint32_t) (_bs->pointer - _bs->buffer)) < (_bs->capacity + 1)) \
        (_bs->accumulator) |= (*(_bs->pointer + 1) << 16); \
    if (((uint32_t) (_bs->pointer - _bs->buffer)) < (_bs->capacity + 2)) \
        (_bs->accumulator) |= (*(_bs->pointer + 2) << 8); \
    if (((uint32_t) (_bs->pointer - _bs->buffer)) <(_bs->capacity + 3)) \
        (_bs->accumulator) |= (*(_bs->pointer + 3) << 0); \
    bitstream_prefetch(_bs); \
} while(0)

#define bitstream_flush(_bs) do { \
    if (((uint32_t) (_bs->pointer - _bs->buffer)) < (_bs->capacity + 0)) \
        *(_bs->pointer + 0) = (_bs->accumulator >> 24); \
    if (((uint32_t) (_bs->pointer - _bs->buffer)) < (_bs->capacity + 1)) \
        *(_bs->pointer + 1) = (_bs->accumulator >> 16); \
    if (((uint32_t) (_bs->pointer - _bs->buffer)) < (_bs->capacity + 2)) \
        *(_bs->pointer + 2) = (_bs->accumulator >> 8); \
    if (((uint32_t) (_bs->pointer - _bs->buffer)) < (_bs->capacity + 3)) \
        *(_bs->pointer + 3) = (_bs->accumulator >> 0); \
} while(0)

#define bitstream_shift(_bs, _nbits) do { \
    _bs->accumulator <<= _nbits; \
    _bs->position += _nbits; \
    _bs->offset += _nbits; \
    if (_bs->offset < 32) { \
        _bs->mask = ((1 << _nbits) - 1); \
        _bs->accumulator |= ((_bs->prefetch >> (32 - _nbits)) & _bs->mask); \
        _bs->prefetch <<= _nbits; \
    } else { \
        _bs->mask = ((1 << _nbits) - 1); \
        _bs->accumulator |= ((_bs->prefetch >> (32 - _nbits)) & _bs->mask); \
        _bs->prefetch <<= _nbits; \
        _bs->offset -= 32; \
        _bs->pointer += 4; \
        bitstream_prefetch(_bs); \
        if (_bs->offset) { \
            _bs->mask = ((1 << _bs->offset) - 1); \
            _bs->accumulator |= ((_bs->prefetch >> (32 - _bs->offset)) & _bs->mask); \
            _bs->prefetch <<= _bs->offset; \
        } \
    } \
} while(0)

#define bitstream_write_bits(_bs, _bits, _nbits) do { \
    _bs->position += _nbits; \
    _bs->offset += _nbits; \
    if (_bs->offset < 32) { \
        _bs->accumulator |= (_bits << (32 - _bs->offset)); \
    } else { \
        _bs->offset -= 32; \
        _bs->mask = ((1 << (_nbits - _bs->offset)) - 1); \
        _bs->accumulator |= ((_bits >> _bs->offset) & _bs->mask); \
        bitstream_flush(bs); \
        _bs->accumulator = 0; \
        _bs->pointer += 4; \
        if (_bs->offset) { \
            _bs->mask = ((1 << _bs->offset) - 1); \
            _bs->accumulator |= ((_bits & _bs->mask) << (32 - _bs->offset)); \
        } \
    } \
} while(0)

#define bitstream_getremaininglength(_bs) \
        (_bs->length - _bs->position)

#define bitstream_getlength_bytes(_bs) \
        ((_bs->position % 8) ? (_bs->position / 8) + 1 : (_bs->position / 8))

void        bitstream_attach(wBitStream* bs, uint8_t* buffer, uint32_t capacity);
wBitStream *bitstream_new();
void        bitstream_free(wBitStream* bs);
int         bitstream_file_save(wBitStream* bs, const char *fn);

#ifdef __cplusplus
};
#endif

#endif /* BITSTREAM_H */
