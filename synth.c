#include "synth.h"
#include <stdlib.h>
#include <stdio.h>
#include"core.h"
#include <math.h>

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


                if (voices[v].mode == MODE_FDTD_GONG || voices[v].mode == MODE_FDTD_METALIC || voices[v].mode== MODE_FDTD_DRUM) {
                    int strike_x = (int)(GRID_SIZE * 0.38f);
                    int strike_y = (int)(GRID_SIZE * 0.58f);
                    strike_drum2d(voices[v].drum_mesh, strike_x, strike_y, 0.4f);
                    //int strike_pos = (int)(GRID_SIZE * 0.6f);
                    //strike_drum2d(voices[v].drum_mesh, strike_pos, strike_pos, 0.3f); // Jedno mocniejsze uderzenie
                }
                
                if (voices[v].mode == MODE_PIANO) {
                    voices[v].brightness   = 1.0f;
                    voices[v].sample_count = 0;
                    voices[v].amp_envelope = 1.0f;

                    // fc proporcjonalne do częstotliwości struny
                    // Chcemy fc ≈ 2 * fund_freq żeby przepuścić fundament + pierwszą harmoniczną
                    float fund_freq = 44100.0f / (float)voices[v].delay_line->size;
                    // alpha = 2π * fc / sr,  ale LP: y = alpha*x + (1-alpha)*y_prev
                    // fc = 3 * fund_freq żeby mieć margines
                    float lp_alpha = (2.0f * 3.14159f * fund_freq * 3.0f) / 44100.0f;
                    if (lp_alpha > 0.95f) lp_alpha = 0.95f;  // zabezpieczenie dla wysokich nut
                    
                    // Trzy przejścia LP przez bufor — kaskada wzmacnia tłumienie
                    float buf[2048] = {0};  // bufor tymczasowy, zakładamy max 2048
                    size_t bsize = voices[v].delay_line->size;
                    
                    // Generuj szum
                    for (size_t j = 0; j < bsize; j++)
                        buf[j] = generate_white_noise();
                    
                    // Przejście 1
                    float pn = 0.0f;
                    for (size_t j = 0; j < bsize; j++) {
                        pn = lp_alpha * buf[j] + (1.0f - lp_alpha) * pn;
                        buf[j] = pn;
                    }
                    // Przejście 2
                    pn = 0.0f;
                    for (size_t j = 0; j < bsize; j++) {
                        pn = lp_alpha * buf[j] + (1.0f - lp_alpha) * pn;
                        buf[j] = pn;
                    }
                    // Przejście 3
                    pn = 0.0f;
                    for (size_t j = 0; j < bsize; j++) {
                        pn = lp_alpha * buf[j] + (1.0f - lp_alpha) * pn;
                        buf[j] = pn;
                    }
                    
                    // Wstaw przefiltrowany szum do bufora KS
                    for (size_t j = 0; j < bsize; j++)
                        push_sample(voices[v].delay_line, buf[j]);
                } else{
                //szarpnięcie struny, szum do bufora
                    for (size_t j = 0; j < voices[v].delay_line->size; j++) {
                        float noise = generate_white_noise();
                        
                        //uciszamy szybko jesli to ktorys z tych mode
                        if (voices[v].mode == MODE_DRUM || voices[v].mode == MODE_SNARE) {
                            noise *= envelope; 
                            envelope *= 0.95f;  //sciszamh szum zgodnie z obwiednią- obwiednia 5% w dól
                        }
                        /*
                        else if (voices[v].mode == MODE_PIANO) {
                            //filtr dolnoprzepustowy
                            //noise = 0.2f * noise + 0.8f * previous_noise;
                            //previous_noise = noise;
                            float new_noise = generate_white_noise();
                            noise = 0.05f * new_noise + 0.95f * previous_noise;
                            previous_noise = noise;
                            
                            // uderzenie w czesc struny
                            noise *= envelope;
                            envelope *= 0.99f;  //szerokie uderzenie- powolny spadek
                        }
                            */
                        /*
                        else if (voices[v].mode == MODE_FDTD_GONG || voices[v].mode == MODE_FDTD_CIRCULAR) {
                            // Uderzamy w środek siatki z siłą 1.0
                            strike_drum2d(voices[v].drum_mesh, GRID_SIZE / 2, GRID_SIZE / 2, 1.0f);
                        }
                        */
                        
                        push_sample(voices[v].delay_line, noise);
                    }
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
                    voices[v].sample_count++;

                    // Obwiednia: -6dB po 2s (zamiast po 1s)
                    // 0.5^(1/(2*44100)) = 0.999992
                    voices[v].amp_envelope *= 0.999992f;

                    // Brightness: zaczyna od 1.0 (jasno), schodzi do 0.0 w ~1s
                    // Ale dynamic_alpha NIE może schodzić poniżej 0.85 — inaczej KS traci energię
                    voices[v].brightness *= 0.999977f;  // tau ≈ 0.65s
                    // alpha: 0.98 (jasny, mało tłumi) → 0.85 (ciemniejszy)
                    float dynamic_alpha = 0.85f + 0.13f * voices[v].brightness;

                    // LP w pętli sprzężenia
                    float smoothed = dynamic_alpha * current_sample
                                + (1.0f - dynamic_alpha) * voices[v].previous_sample;

                    // Allpass 1
                    float c = 0.05f;
                    float ap_out = -c * smoothed
                                + voices[v].ap_prev_in
                                + c * voices[v].ap_prev_out;
                    voices[v].ap_prev_in  = smoothed;
                    voices[v].ap_prev_out = ap_out;

                    // Allpass 2
                    float b = 0.03f;
                    float ap2_out = -b * ap_out
                                + voices[v].ap2_prev_in
                                + b * voices[v].ap2_prev_out;
                    voices[v].ap2_prev_in  = ap_out;
                    voices[v].ap2_prev_out = ap2_out;

                    processed_sample = ap2_out * voices[v].amp_envelope;
                } else if (voices[v].mode == MODE_FDTD_GONG || voices[v].mode == MODE_FDTD_METALIC || voices[v].mode== MODE_FDTD_DRUM) {
                    
                    int is_drum = (voices[v].mode == MODE_FDTD_DRUM) ? 1 : 0;

                    //ponieważ środek siatki może mieć małą amplitudę dajemy mu lekkie wzmocnienie
                    float raw_mesh_sample = process_drum2d(voices[v].drum_mesh, is_drum) * 2.0f; 
                    
                    if (voices[v].mode == MODE_FDTD_METALIC || voices[v].mode== MODE_FDTD_DRUM) {
                        //łagodny filtr dolnoprzepustowy na metal
                        //processed_sample = (0.95f * raw_mesh_sample) + (0.05f * voices[v].previous_sample);
                        
                        //voices[v].previous_sample = processed_sample; 
                        //processed_sample = raw_mesh_sample;
                        processed_sample = (voices[v].alpha * raw_mesh_sample) + ((1.0f - voices[v].alpha) * voices[v].previous_sample);
                        voices[v].previous_sample = processed_sample;
                    } else {
                        processed_sample = raw_mesh_sample;
                    }
                }

                if (voices[v].mode != MODE_FDTD_GONG && voices[v].mode != MODE_FDTD_METALIC && voices[v].mode != MODE_FDTD_DRUM) {
                    
                    voices[v].previous_sample = processed_sample;
    
                    processed_sample *= voices[v].damping;
                    push_sample(voices[v].delay_line, processed_sample);
                }

                //miksowanie
                mixed_sample += processed_sample;
            }
        }

        //zabezpieczenie na clipping
        if (num_voices > 0) {
            float final_out = mixed_sample / (float)num_voices;
            
            if (final_out > 1.0f){
                final_out = 1.0f;
                printf("Wszedł clipping");
            } 
            if (final_out < -1.0f) final_out = -1.0f;
            
            output[i] = final_out;
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