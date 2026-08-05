/*
 * audio_input.c
 *
 *  Created on: Aug 5, 2026
 *      Author: nickt
 */

#include "audio_input.h"
#include "main.h"

extern I2S_HandleTypeDef hi2s2;

static int16_t audio_input_buffer[AUDIO_INPUT_BUFFER_SIZE];

volatile uint32_t audio_input_half_count = 0;
volatile uint32_t audio_input_full_count = 0;
volatile uint32_t audio_input_error_count = 0;
volatile uint32_t audio_input_start_status = 0xFFFFFFFFU;

void Audio_Input_Init(void)
{
    audio_input_half_count = 0;
    audio_input_full_count = 0;
    audio_input_error_count = 0;

    audio_input_start_status = (uint32_t)HAL_I2S_Receive_DMA(&hi2s2, (uint16_t *)audio_input_buffer, AUDIO_INPUT_BUFFER_SIZE);
}

const int16_t *Audio_Input_GetFirstHalf(void)
{
    return &audio_input_buffer[0];
}

const int16_t *Audio_Input_GetSecondHalf(void)
{
    return &audio_input_buffer[AUDIO_INPUT_BUFFER_SIZE / 2U];
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2)
    {
        audio_input_half_count++;
    }
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2)
    {
        audio_input_full_count++;
    }
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    if (hi2s->Instance == SPI2)
    {
        audio_input_error_count++;
    }
}
