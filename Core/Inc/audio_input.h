/*
 * audio_input.h
 *
 *  Created on: Aug 5, 2026
 *      Author: nickt
 */
 

#ifndef SRC_AUDIO_INPUT_H_
#define SRC_AUDIO_INPUT_H_

#include "main.h"
#include <stdint.h>

#define AUDIO_INPUT_FRAMES_PER_HALF 480U
#define AUDIO_INPUT_WORDS_PER_FRAME 4U
#define AUDIO_INPUT_HALF_SIZE (AUDIO_INPUT_FRAMES_PER_HALF * AUDIO_INPUT_WORDS_PER_FRAME)
#define AUDIO_INPUT_BUFFER_SIZE (AUDIO_INPUT_HALF_SIZE * 2U)

extern volatile uint32_t audio_input_half_count;
extern volatile uint32_t audio_input_full_count;
extern volatile uint32_t audio_input_error_count;
extern volatile uint32_t audio_input_start_status;

extern volatile uint8_t audio_input_first_half_ready;
extern volatile uint8_t audio_input_second_half_ready;

void Audio_Input_Init(void);
const uint16_t *Audio_Input_GetFirstHalf(void);
const uint16_t *Audio_Input_GetSecondHalf(void);

#endif /* SRC_AUDIO_INPUT_H_ */