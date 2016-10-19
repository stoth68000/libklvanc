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

#ifndef TS_PACKETIZER_H
#define TS_PACKETIZER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int ts_packetizer(uint8_t *buf, unsigned int byteCount, uint8_t **pkts, uint32_t *packetCount, int packetSize, uint8_t *cc, uint16_t pid);

#ifdef __cplusplus
};
#endif


#endif /* TS_PACKETIZER_H */
