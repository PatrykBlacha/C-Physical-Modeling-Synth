#include "core.h"
#include <stdlib.h>
#include <time.h>

CircularBuffer* create_buffer(size_t size) {
    CircularBuffer* cb = (CircularBuffer*)malloc(sizeof(CircularBuffer));
    if (!cb) return NULL;

    cb->data = (float*)calloc(size, sizeof(float)); 
    if (!cb->data) {
        free(cb);
        return NULL;
    }
    
    cb->size = size;
    cb->write_idx = 0;
    return cb;
}

void free_buffer(CircularBuffer* cb) {
    if (cb) {
        if (cb->data) free(cb->data);
        free(cb);
    }
}

void push_sample(CircularBuffer* cb, float sample) {
    cb->data[cb->write_idx] = sample;
    cb->write_idx = (cb->write_idx + 1) % cb->size;
}

float read_sample(CircularBuffer* cb, size_t delay) {
    size_t read_idx = (cb->write_idx + cb->size - delay) % cb->size;
    return cb->data[read_idx];
}

float read_sample_interpolated(CircularBuffer* cb, float delay) {
    size_t delay_int = (size_t)delay;
    float frac = delay - (float)delay_int;
    
    float s1 = read_sample(cb, delay_int);
    float s2 = read_sample(cb, delay_int + 1);
    
    return s1 + frac * (s2 - s1);
}


void init_noise_generator() {
    srand((unsigned int)time(NULL));
}

float generate_white_noise() {
    return ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f; //od -1 do 1
}