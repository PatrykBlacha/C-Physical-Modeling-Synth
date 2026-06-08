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

            float envelope = 1.0f;  //startowa głośność
            float previous_noise = 0.0f; // pamięc dla mloteczka

            if (current_time >= voices[v].start_time_sec && voices[v].has_been_plucked == 0) {

                //szarpnięcie struny, szum do bufora
                for (size_t j = 0; j < voices[v].delay_line->size; j++) {
                    float noise = generate_white_noise();
                    
                    //uciszamy szybko jesli to ktorys z tych mode
                    if (voices[v].mode == MODE_DRUM || voices[v].mode == MODE_SNARE) {
                        noise *= envelope; 
                        envelope *= 0.95f;  //sciszamh szum zgodnie z obwiednią- obwiednia 5% w dól
                    }
                    else if (voices[v].mode == MODE_PIANO) {
                        //filtr dolnoprzepustowy
                        noise = 0.5f * noise + 0.5f * previous_noise;
                        previous_noise = noise;
                        
                        // uderzenie w czesc struny
                        noise *= envelope;
                        envelope *= 0.99f;  //szerokie uderzenie- powolny spadek
                    }
                    else if (voices[v].mode == MODE_FDTD_DRUM) {
                        // Uderzamy w środek siatki z siłą 1.0
                        strike_drum2d(voices[v].drum_mesh, GRID_SIZE / 2, GRID_SIZE / 2, 1.0f);
                    }
                    
                    push_sample(voices[v].delay_line, noise);
                }

                //zaznaczenie że już gra
                voices[v].is_active = 1;
                voices[v].has_been_plucked = 1;
                voices[v].hpf_state = 0.0f; 
            }

            if (voices[v].is_active) {
                
                //probka z bufora
                float current_sample;
                float processed_sample = 0.0f;

                //Filtrdolnoprzepustowy z alfa
                if (voices[v].mode == MODE_HIHAT) {
                    //próbka z losowego miejsca blisko końca bufora, powiino dać metalicznosc
                    int mod_offset = rand() % 10; 
                    current_sample = read_sample(voices[v].delay_line, voices[v].delay_line->size - mod_offset);
                } else {
                    //odczyt standardowey
                    current_sample = read_sample(voices[v].delay_line, voices[v].delay_line->size);
                }

                //filtracja zalezna od trybu
                if (voices[v].mode == MODE_STRING) {
                    processed_sample = (voices[v].alpha * current_sample) + ((1.0f - voices[v].alpha) * voices[v].previous_sample);
                
                } else if (voices[v].mode == MODE_DRUM) {
                    //Mocny filtr dolnoprzepustowy bez odwracania znaku
                    processed_sample = (voices[v].alpha * current_sample) + ((1.0f - voices[v].alpha) * voices[v].previous_sample);
                
                } else if (voices[v].mode == MODE_HIHAT) {
                    //agresywne odwracanie fazy + modulowany odczyt
                    float sign = (rand() % 100 < 50) ? -1.0f : 1.0f; 
                    processed_sample = sign * ((voices[v].alpha * current_sample) + ((1.0f - voices[v].alpha) * voices[v].previous_sample));
                
                } else if (voices[v].mode == MODE_SNARE) {
                    //uśrednainiane z odwracaniem fazy 
                    float sign = (rand() % 100 < 50) ? -1.0f : 1.0f; 
                    float smoothed = 0.5f * current_sample + 0.5f * voices[v].previous_sample;
                    processed_sample = sign * smoothed;
                    
                    //FILTR GÓRNOPRZEPUSTOWY
                    float hpf_out = processed_sample - voices[v].hpf_state;
                    voices[v].hpf_state = processed_sample;
                    processed_sample = hpf_out;
                } else if (voices[v].mode == MODE_PIANO) {
                    // 1. Uderzenie miękkim młotkiem (lekki filtr dolnoprzepustowy na wejściu)
                    float smoothed = (voices[v].alpha * current_sample) + ((1.0f - voices[v].alpha) * voices[v].previous_sample);
                    
                    // 2. Filtr Wszechprzepustowy (Sztywność struny)
                    // Parametr 'c' kontroluje sztywność. (0.0 to gumka, 0.6 to gruby drut)
                    float c = 0.5f; 
                    
                    float ap_out = (c * smoothed) + voices[v].ap_prev_in - (c * voices[v].ap_prev_out);
                    
                    // Zapisujemy stany do pamięci na następny obieg
                    voices[v].ap_prev_in = smoothed;
                    voices[v].ap_prev_out = ap_out;
                    
                    processed_sample = ap_out;
                } else if (voices[v].mode == MODE_FDTD_DRUM) {
                    // Cała matematyka zamknięta jest w zewnętrznym module!
                    processed_sample = process_drum2d(voices[v].drum_mesh); 
                }

                if (voices[v].mode != MODE_FDTD_DRUM) {
                    //Damping
                    processed_sample *= voices[v].damping;

                    //sprzężenie zwrotne
                    push_sample(voices[v].delay_line, processed_sample);
                    voices[v].previous_sample = current_sample;
                }

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