/*
 * audio.c
 *
 *  Created on: Jul 17, 2026
 *      Author: nickt
 */
#include "main.h"
#include "audio.h"
#include "audio_sine.h"

extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim2;

void Audio_Init(void) 
{
    Audio_Sine_Init();
}