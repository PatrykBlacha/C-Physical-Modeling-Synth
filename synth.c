#include "synth.h"
#include <stdlib.h>
#include"core.h"

void synthesize_poly(Voice* voices, size_t num_voices, float* output, size_t num_samples) {
    
    //petla po nagraniu
    for (size_t i = 0; i < num_samples; i++) {
        float mixed_sample = 0.0f; //próbka na wszystkie struny

        float current_time = (float)i / 44100.0f;   //czas w sekundach
        
        //Pętla iterująca po wszystkich strunach
        for (size_t v = 0; v < num_voices; v++) {

            if (current_time >= voices[v].start_time_sec && voices[v].has_been_plucked == 0) {

                //szarpnięcie struny, szum do bufora
                for (size_t j = 0; j < voices[v].delay_line->size; j++) {
                    push_sample(voices[v].delay_line, generate_white_noise());
                }

                //zaznaczenie że już gra
                voices[v].is_active = 1;
                voices[v].has_been_plucked = 1;
            }

            if (voices[v].is_active) {
                
                //probka z bufora
                float current_sample = read_sample(voices[v].delay_line, voices[v].delay_line->size);
                float processed_sample = 0.0f;

                //Filtrdolnoprzepustowy z alfa
                if (voices[v].mode == MODE_STRING) {
                    processed_sample = (voices[v].alpha * current_sample) + 
                                       ((1.0f - voices[v].alpha) * voices[v].previous_sample);
                                       
                } else if (voices[v].mode == MODE_DRUM) {
                    float sign = (rand() % 2 == 0) ? 1.0f : -1.0f; 
                    processed_sample = sign * ((voices[v].alpha * current_sample) + 
                                               ((1.0f - voices[v].alpha) * voices[v].previous_sample));
                }

                //Damping
                processed_sample *= voices[v].damping;

                //sprzężenie zwrotne
                push_sample(voices[v].delay_line, processed_sample);
                voices[v].previous_sample = current_sample;

                //miksowanie
                mixed_sample += processed_sample;
            }
        }

        //zabezpieczenie na clipping
        if (num_voices > 0) {
            output[i] = mixed_sample / (float)num_voices;
        } else {
            output[i] = 0.0f;
        }
    }
}


/*
#include "synth.h"
#include <stdlib.h>
void synthesize(CircularBuffer* cb, float* output, size_t num_samples, InstrumentMode mode, float damping) {
    float previous_sample = 0.0f; //pamiec do filtru dolnoprzepustowego

    for (size_t i = 0; i < num_samples; i++) {
        //odczyto opóźnionej próbki z bufora
        float current_sample = read_sample(cb, cb->size);
        float processed_sample = 0.0f;

        //strunowy
        if (mode == MODE_STRING) {
            processed_sample = 0.5f * (current_sample + previous_sample);
            
        } else if (mode == MODE_DRUM) {
            //losowwa inwersja fazy
            float sign = (rand() % 2 == 0) ? 1.0f : -1.0f; 
            processed_sample = sign * 0.5f * (current_sample + previous_sample);
        }

        //współczynik tłumienia
        processed_sample *= damping;

        //sprężenie zwrotne
        push_sample(cb, processed_sample);

        //do pliku
        output[i] = processed_sample;

        previous_sample = current_sample;
    }
}
*/

/*

#include "synth.h"
#include <stdlib.h>
#include <math.h> // Wymagane dla funkcji sinf()

void synthesize(CircularBuffer* cb, float* output, size_t num_samples, InstrumentMode mode, float damping) {
    float previous_sample = 0.0f; 

    // Parametry Tremolo (możesz się nimi później pobawić)
    float lfo_rate = 5.0f;   // Szybkość falowania (5 razy na sekundę)
    float lfo_depth = 0.5f;  // Głębokość falowania (od 0.0 do 1.0)
    float pi_2 = 6.2831853f; // 2 * PI

    for (size_t i = 0; i < num_samples; i++) {
        float current_sample = read_sample(cb, cb->size);
        float processed_sample = 0.0f;

        if (mode == MODE_STRING) {
            processed_sample = 0.5f * (current_sample + previous_sample);
        } else if (mode == MODE_DRUM) {
            float sign = (rand() % 2 == 0) ? 1.0f : -1.0f; 
            processed_sample = sign * 0.5f * (current_sample + previous_sample);
        }

        processed_sample *= damping;
        push_sample(cb, processed_sample);

        // --- TREMOLO (Modulacja Amplitudy) ---
        // 1. Obliczamy aktualny czas w sekundach
        float time_sec = (float)i / 44100.0f;
        
        // 2. Generujemy falę LFO (od -1.0 do 1.0)
        float lfo_value = sinf(pi_2 * lfo_rate * time_sec);
        
        // 3. Przesuwamy i skalujemy LFO, żeby działało jak "gałka głośności" (od 1.0 w dół)
        float tremolo_multiplier = 1.0f - (lfo_depth * (0.5f * lfo_value + 0.5f));

        // 4. Mnożymy nasz dźwięk ze struny przez mnożnik Tremolo
        output[i] = processed_sample * tremolo_multiplier;

        previous_sample = current_sample;
    }
}

*/