/*
 * audio_sine.h
 *
 *  Created on: Jul 28, 2026
 *      Author: nickt
 */

#ifndef SRC_AUDIO_SINE_H_
#define SRC_AUDIO_SINE_H_

void Audio_Sine_Init(void);

void Audio_Sine_Generate(
    int16_t *buffer,
    size_t sample_count
);

#endif /* SRC_AUDIO_SINE_H_ */