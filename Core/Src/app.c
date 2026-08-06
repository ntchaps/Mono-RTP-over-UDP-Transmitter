/*
 * app.c
 *
 *  Created on: Jul 8, 2026
 *      Author: nickt
 */

#include "main.h"
#include "app.h"
#include "audio.h"
#include "rtp.h"
#include "network.h"
#include "udp_stream.h" 
#include "debug_uart.h"
#include "audio_input.h"
#include <math.h>

volatile int32_t app_last_send_result = -999;
volatile uint32_t app_send_attempt_count = 0;
volatile uint32_t app_send_success_count = 0;
volatile uint32_t app_send_failure_count = 0;




/*
 * Audio configuration.
 *
 * At an 48 kHz sample rate, 480 samples represent:
 *
 *     480 / 48000 = 0.010 seconds
 *
 * Therefore, one packet contains 10 ms of audio.
 */
#define AUDIO_SAMPLES_PER_PACKET    480U

/*
 * Signed 16-bit PCM buffer used to hold one RTP packet's
 * worth of sine-wave samples.
 */
static int16_t audio_packet[AUDIO_SAMPLES_PER_PACKET];

static void App_ConvertI2SToMono16(const uint16_t *input, int16_t *output)
{
    for (uint32_t frame = 0; frame < AUDIO_SAMPLES_PER_PACKET; frame++)
    {
        uint32_t input_index = frame * AUDIO_INPUT_WORDS_PER_FRAME;

        /*
         * Start by selecting the upper 16 bits of the left channel.
         * If the waveform is incorrect, test input_index + 1 instead.
         */
        output[frame] = (int16_t)input[input_index + 2U];
    }
}

void App_Init(void)
{
    /*
     * Configure the W5500 network settings.
     */
    Debug_Printf("Network init start\r\n");
    Network_Init();
    Debug_Printf("Network init done\r\n"); 
    /*
     * Open and configure the UDP socket.
     */
    Debug_Printf("UDP init\r\n");
    UDP_Stream_Init();
    Debug_Printf("UDP init done\r\n"); 

    /*
     * Initialize the RTP sequence number, timestamp, and SSRC.
     */
    Debug_Printf("RTP init\r\n");
    RTP_Init();
    Debug_Printf("RTP init done\r\n"); 
   
    /*
     * Initialize the audio system.
     *
     * Check audio.c to determine whether Audio_Init() already calls
     * Audio_Input_Init(). Do not call Audio_Input_Init() twice.
     */
    Debug_Printf("Audio init\r\n");
    Audio_Init();
    Debug_Printf("Audio init done\r\n"); 
}

void App_Run(void)
{
    if (audio_input_first_half_ready != 0U)
    {
        audio_input_first_half_ready = 0U;

        App_ConvertI2SToMono16(
            Audio_Input_GetFirstHalf(),
            audio_packet
        );

        app_send_attempt_count++;

        app_last_send_result = RTP_SendAudio(
            audio_packet,
            AUDIO_SAMPLES_PER_PACKET
        );

        if (app_last_send_result >= 0)
        {
            app_send_success_count++;
        }
        else
        {
            app_send_failure_count++;

            Debug_Printf(
                "RTP or UDP send failed: %ld\r\n",
                (long)app_last_send_result
            );
        }
    }

    if (audio_input_second_half_ready != 0U)
    {
        audio_input_second_half_ready = 0U;

        App_ConvertI2SToMono16(
            Audio_Input_GetSecondHalf(),
            audio_packet
        );

        app_send_attempt_count++;

        app_last_send_result = RTP_SendAudio(
            audio_packet,
            AUDIO_SAMPLES_PER_PACKET
        );

        if (app_last_send_result >= 0)
        {
            app_send_success_count++;
        }
        else
        {
            app_send_failure_count++;

            Debug_Printf(
                "RTP or UDP send failed: %ld\r\n",
                (long)app_last_send_result
            );
        }
    }
}
