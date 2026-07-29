/*
 * rtp.c
 *
 *  Created on: Jul 28, 2026
 *      Author: nickt
 */

#include "rtp.h"
#include "udp_stream.h"

#define RTP_MAX_SAMPLES_PER_PACKET  160U
#define RTP_MAX_PAYLOAD_SIZE        \
    (RTP_MAX_SAMPLES_PER_PACKET * sizeof(int16_t))

#define RTP_MAX_PACKET_SIZE         \
    (RTP_HEADER_SIZE + RTP_MAX_PAYLOAD_SIZE)

static uint16_t rtp_sequence_number;
static uint32_t rtp_timestamp;
static uint32_t rtp_ssrc;

static void RTP_WriteUint16BE(uint8_t *buffer, uint16_t value)
{
    buffer[0] = (uint8_t)(value >> 8);
    buffer[1] = (uint8_t)value;
}

static void RTP_WriteUint32BE(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)(value >> 24);
    buffer[1] = (uint8_t)(value >> 16);
    buffer[2] = (uint8_t)(value >> 8);
    buffer[3] = (uint8_t)value;
}

void RTP_Init(void)
{
    /*
     * Fixed starting values are acceptable during early testing.
     * Later, initialize these with less predictable values.
     */
    rtp_sequence_number = 0U;
    rtp_timestamp = 0U;
    rtp_ssrc = RTP_SSRC;
}

int32_t RTP_SendAudio(
    const int16_t *samples,
    size_t sample_count)
{
    static uint8_t packet[RTP_MAX_PACKET_SIZE];
    size_t payload_index;

    if ((samples == NULL) ||
        (sample_count == 0U) ||
        (sample_count > RTP_MAX_SAMPLES_PER_PACKET))
    {
        return -1;
    }

    /*
     * Byte 0:
     * Version = 2
     * Padding = 0
     * Extension = 0
     * CSRC count = 0
     */
    packet[0] = 0x80U;

    /*
     * Byte 1:
     * Marker = 0
     * Payload type = 96
     */
    packet[1] = RTP_PAYLOAD_TYPE;

    RTP_WriteUint16BE(&packet[2], rtp_sequence_number);
    RTP_WriteUint32BE(&packet[4], rtp_timestamp);
    RTP_WriteUint32BE(&packet[8], rtp_ssrc);

    /*
     * L16-style PCM samples are placed in network byte order:
     * most-significant byte first.
     */
    payload_index = RTP_HEADER_SIZE;

    for (size_t i = 0U; i < sample_count; i++)
    {
        uint16_t sample = (uint16_t)samples[i];

        packet[payload_index++] = (uint8_t)(sample >> 8);
        packet[payload_index++] = (uint8_t)sample;
    }

    int32_t result = UDP_Stream_Send(
        packet,
        RTP_HEADER_SIZE + (sample_count * sizeof(int16_t))
    );

    if (result >= 0)
    {
        rtp_sequence_number++;
        rtp_timestamp += (uint32_t)sample_count;
    }

    return result;
}