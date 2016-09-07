#include <stdint.h>

void klvanc_v210_planar_unpack_c(const uint32_t * src, uint16_t * y, uint16_t * u, uint16_t * v, int width);
void klvanc_v210_line_to_nv20_c(const uint32_t * src, uint16_t * dst, int width);
void klvanc_v210_downscale_line_c(uint16_t * src, uint8_t * dst, int lines);
void klvanc_v210_line_to_uyvy_c(uint32_t * src, uint16_t * dst, int width);
