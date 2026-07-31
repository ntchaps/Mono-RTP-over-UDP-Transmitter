/*
 * app.c
 *
 *  Created on: Jul 8, 2026
 *      Author: nickt
 */

#include "main.h"
#include "app.h"
#include "audio.h"
#include "audio_sine.h"
#include "rtp.h"
#include "network.h"
#include "udp_stream.h" 
#include "debug_uart.h"

/*
 * Audio configuration.
 *
 * At an 8 kHz sample rate, 160 samples represent:
 *
 *     160 / 8000 = 0.020 seconds
 *
 * Therefore, one packet contains 20 ms of audio.
 */
#define AUDIO_SAMPLES_PER_PACKET    160U
#define AUDIO_PACKET_PERIOD_MS       20U

/*
 * Signed 16-bit PCM buffer used to hold one RTP packet's
 * worth of sine-wave samples.
 */
static int16_t audio_packet[AUDIO_SAMPLES_PER_PACKET];

/*
 * Time at which the next audio packet should be sent.
 */
static uint32_t next_send_time;

void App_Init(void)
{
    /*
     * Initialize the audio system.
     *
     * Check audio.c to determine whether Audio_Init() already calls
     * Audio_Sine_Init(). Do not call Audio_Sine_Init() twice.
     */
    Audio_Init();

    /*
     * Configure the W5500 network settings.
     */
    Network_Init();

    /*
     * Open and configure the UDP socket.
     */
    UDP_Stream_Init();

    /*
     * Initialize the RTP sequence number, timestamp, and SSRC.
     */
    RTP_Init();

    /*
     * Schedule the first packet immediately.
     */
    next_send_time = HAL_GetTick();
}

void App_Run(void)
{
    uint32_t current_time;

    current_time = HAL_GetTick();

    /*
     * Check whether it is time to send the next 20 ms audio packet.
     *
     * The signed subtraction keeps the comparison valid even when
     * HAL_GetTick() eventually wraps around.
     */
    if ((int32_t)(current_time - next_send_time) >= 0)
    {
        /*
         * Schedule the next packet relative to the expected send time.
         *
         * Using += instead of assigning current_time prevents gradual
         * timing drift.
         */
        next_send_time += AUDIO_PACKET_PERIOD_MS;

        /*
         * Generate 160 signed 16-bit sine-wave PCM samples.
         */
        Audio_Sine_Generate(
            audio_packet,
            AUDIO_SAMPLES_PER_PACKET
        );

        /*
         * Build the RTP header, append the PCM samples,
         * and send the completed packet through UDP.
         */
        int32_t result = RTP_SendAudio(
            audio_packet,
            AUDIO_SAMPLES_PER_PACKET
        );

        /*
         * Add a breakpoint, UART message, or LED indication here
         * while debugging transmission failures.
         */
        if (result < 0)
        {
            /* RTP or UDP send failed. */
            Debug_Printf("RTP or UDP send failed: %ld\r\n", (long)result);
        }
    }
}
