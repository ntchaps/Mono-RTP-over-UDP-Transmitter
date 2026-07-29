/*
 * rtp.h
 *
 *  Created on: Jul 28, 2026
 *      Author: nickt
 */

#ifndef SRC_RTP_H_
#define SRC_RTP_H_

#include <stdint.h>
#include <stddef.h>

#define RTP_HEADER_SIZE       12U
#define RTP_PAYLOAD_TYPE      96U
#define RTP_SSRC              0x12345678UL

void RTP_Init(void);

int32_t RTP_SendAudio(
    const int16_t *samples, size_t sample_count
);

#endif /* SRC_RTP_H_ */