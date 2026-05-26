#ifndef SYNTH_H
#define SYNTH_H

#include "core.h"
#include <stddef.h>

typedef enum {
    MODE_STRING, //tryb strunowy
    MODE_DRUM,    //perkusyjny
    MODE_SNARE,    //werbel z filtrem pasmowym
    MODE_HIHAT       //talerze z modulacja
} InstrumentMode;

typedef struct {
    CircularBuffer* delay_line;
    float damping;           //od struny
    float alpha;             //współczynnik filtra dolnoprzepustowego
    float previous_sample;
    int is_active;           //flaga czy struna gra(1) czy nie(0)
    InstrumentMode mode;     //tryb

    float start_time_sec;    //start grania
    int has_been_plucked;    //czy teraz gra

    float hpf_state;    //pamiec filtra gornoprzepustowego dla mode_snare
} Voice;

//petla karplusa
void synthesize(CircularBuffer* cb, float* output, size_t num_samples, InstrumentMode mode, float damping);
void synthesize_poly(Voice* voices, size_t num_voices, float* output, size_t num_samples);

#endif