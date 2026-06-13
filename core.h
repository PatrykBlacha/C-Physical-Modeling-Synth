#ifndef CORE_H
#define CORE_H

#include <stddef.h> //do size_t

//bufor cykliczny
typedef struct {
    float* data;
    size_t size;  
    size_t write_idx; //aktualny indeks
} CircularBuffer;

CircularBuffer* create_buffer(size_t size);
void free_buffer(CircularBuffer* cb);
void push_sample(CircularBuffer* cb, float sample);
float read_sample(CircularBuffer* cb, size_t delay);
float read_sample_interpolated(CircularBuffer* cb, float delay);

//do white noise
void init_noise_generator();
float generate_white_noise();

#endif