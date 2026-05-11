#ifndef WAV_IO_H
#define WAV_IO_H

#include <stddef.h>
#include "core.h"

void save_to_wav(const char* filename, float* buffer, size_t num_samples, int sample_rate);
int load_wav_to_buffer(const char* filename, CircularBuffer* cb);

#endif