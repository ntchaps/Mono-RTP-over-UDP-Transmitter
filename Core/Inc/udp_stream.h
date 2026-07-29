/*
 * udp_stream.h
 *
 *  Created on: Jul 28, 2026
 *      Author: nickt
 */

#include <stdint.h>

#ifndef SRC_UDP_STREAM_H_
#define SRC_UDP_STREAM_H_

void UDP_Stream_Init(void);

int32_t UDP_Stream_Send(
    const uint8_t *packet,
    uint16_t packet_length
);

#endif /* SRC_UDP_STREAM_H_ */