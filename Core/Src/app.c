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
#include "audio_input.h"

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
* To use test packets from sine wave LUT.
*/
//#define AUDIO_PACKET_PERIOD_MS       10U

/*
 * Signed 16-bit PCM buffer used to hold one RTP packet's
 * worth of sine-wave samples.
 */
static int16_t audio_packet[AUDIO_SAMPLES_PER_PACKET];

/*
 * Sine wave LUT test time at which the next audio packet should be sent.
 */
// static uint32_t next_send_time;

static void App_ConvertI2SToMono16(const uint16_t *input, int16_t *output)
{
    for (uint32_t frame = 0; frame < AUDIO_SAMPLES_PER_PACKET; frame++)
    {
        uint32_t input_index = frame * AUDIO_INPUT_WORDS_PER_FRAME;

        /*
         * Start by selecting the upper 16 bits of the left channel.
         * If the waveform is incorrect, test input_index + 1 instead.
         */
        output[frame] = (int16_t)input[input_index];
    }
}

void App_Init(void)
{
    /*
     * Initialize the audio system.
     *
     * Check audio.c to determine whether Audio_Init() already calls
     * Audio_Input_Init(). Do not call Audio_Input_Init() twice.
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
   
}

void App_Run(void)
{
    // uint32_t current_time;

    // current_time = HAL_GetTick();

    /*
     * Check whether it is time to send the next 20 ms audio packet.
     *
     * The signed subtraction keeps the comparison valid even when
     * HAL_GetTick() eventually wraps around.
     */
    // if ((int32_t)(current_time - next_send_time) >= 0)
    // {
        // /*
        //  * Schedule the next packet relative to the expected send time.
        //  *
        //  * Using += instead of assigning current_time prevents gradual
        //  * timing drift.
        //  */
        // next_send_time += AUDIO_PACKET_PERIOD_MS;

        // /*
        //  * Generate 160 signed 16-bit sine-wave PCM samples.
        //  */
        // Audio_Sine_Generate(
        //     audio_packet,
        //     AUDIO_SAMPLES_PER_PACKET
        // );

        // /*
        //  * Build the RTP header, append the PCM samples,
        //  * and send the completed packet through UDP.
        //  */
        // int32_t result = RTP_SendAudio(
        //     audio_packet,
        //     AUDIO_SAMPLES_PER_PACKET
        // );
    const uint16_t *input_half = NULL;

        if (audio_input_first_half_ready != 0U)
        {
            audio_input_first_half_ready = 0U;
            input_half = Audio_Input_GetFirstHalf();
        }
        else if (audio_input_second_half_ready != 0U)
        {
            audio_input_second_half_ready = 0U;
            input_half = Audio_Input_GetSecondHalf();
        }

        if (input_half == NULL)
        {
            return;
        }

    App_ConvertI2SToMono16(input_half, audio_packet);

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
