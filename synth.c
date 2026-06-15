#include "synth.h"
#include <stdlib.h>
#include <stdio.h>
#include"core.h"
#include <math.h>

void synthesize_poly(Voice* voices, size_t num_voices, float* output, size_t num_samples) {

    CircularBuffer* comb1 = create_buffer(1557);
    CircularBuffer* comb2 = create_buffer(1617);
    CircularBuffer* comb3 = create_buffer(1491);
    CircularBuffer* comb4 = create_buffer(1422);
    float rev_feedback = 0.85f;

    //petla po nagraniu
    for (size_t i = 0; i < num_samples; i++) {
        float mixed_others = 0.0f; //próbka na wszystkie struny
        float mixed_piano = 0.0f;

        float current_time = (float)i / 44100.0f;   //czas w sekundach
        
        //Pętla iterująca po wszystkich strunach
        for (size_t v = 0; v < num_voices; v++) {

            float out_sample= 0.0f;
            float envelope = 1.0f;  //startowa głośność
            float previous_noise = 0.0f; // pamięc dla mloteczka do uderzeina

            if (current_time >= voices[v].start_time_sec && voices[v].has_been_plucked == 0) {


                if (voices[v].mode == MODE_FDTD_GONG || voices[v].mode == MODE_FDTD_METALIC || voices[v].mode== MODE_FDTD_DRUM) {
                    int strike_x = (int)(GRID_SIZE * 0.38f);
                    int strike_y = (int)(GRID_SIZE * 0.58f);
                    strike_drum2d(voices[v].drum_mesh, strike_x, strike_y, 0.4f);
                }
                
                if (voices[v].mode == MODE_PIANO) {
                    voices[v].brightness   = 1.0f;
                    voices[v].sample_count = 0;
                    voices[v].amp_envelope = 1.0f;
                    //fc proporcjonalne do częstotliwości struny
                    //chcemy fc ≈ 2 * fund_freq żeby przepuścić fundament + pierwszą harmoniczną
                    float fund_freq = 44100.0f / (float)voices[v].delay_line->size;

                    float lp_alpha = (2.0f * 3.14159f * fund_freq * 3.0f) / 44100.0f;

                    //Minimum 0.15 — inaczej bufor wypełnia się quasi-DC
                    if (lp_alpha < 0.15f) lp_alpha = 0.15f;
                    if (lp_alpha > 0.95f) lp_alpha = 0.95f;
                    
                    //trzy przejścia LP przez bufor - kaskada wzmacnia tłumienie
                    size_t bsize = voices[v].delay_line->size;
                    float* buf = (float*)calloc(bsize, sizeof(float));
                    
                    //zamiast szumu, generujemy impuls uderzenia (pół fali sinusa)
                    size_t strike_width = bsize / 8; 
                    if (strike_width < 1) strike_width = 1;

                    float dc_sum = 0.0f; 

                    for (size_t j = 0; j < bsize; j++) {
                        if (j < strike_width) {
                            //faza od 0.0 do 1.0 tylko wewnątrz szerokości młoteczka!
                            float phase = (float)j / (float)strike_width;
                            
                            float hammer_noise = generate_white_noise() * (1.0f - phase);
                            hammer_noise = hammer_noise * hammer_noise * hammer_noise; // Do sześcianu dla drastycznie krótszego "cyknięcia"

                            //głuchy thump zostaje, ale wyciszamy całość
                            float wood_thump = sinf(3.14159f * phase); 

                            //15% szumu i 85% głuchego drewna, dodatkowo całość mocno ściszona (* 0.4f)
                            buf[j] = ((hammer_noise * 0.15f) + (wood_thump * 0.85f)) * 0.4f;
                        } else {
                            buf[j] = 0.0f; //reszta struna cicho
                        }
                        dc_sum += buf[j];
                    }

                    //Usuwanie składowej stałej
                    float dc_offset = dc_sum / (float)bsize;
                    for (size_t j = 0; j < bsize; j++) {
                        buf[j] -= dc_offset; 
                    }

                    //Przejście 1
                    float pn = 0.0f;
                    for (size_t j = 0; j < bsize; j++) {
                        pn = lp_alpha * buf[j] + (1.0f - lp_alpha) * pn;
                        buf[j] = pn;
                    }
                    //Przejście 2
                    pn = 0.0f;
                    for (size_t j = 0; j < bsize; j++) {
                        pn = lp_alpha * buf[j] + (1.0f - lp_alpha) * pn;
                        buf[j] = pn;
                    }
                    //Przejście 3
                    pn = 0.0f;
                    for (size_t j = 0; j < bsize; j++) {
                        pn = lp_alpha * buf[j] + (1.0f - lp_alpha) * pn;
                        buf[j] = pn;
                    }
                    
                    //przefiltrowany szum do bufora KS
                    for (size_t j = 0; j < bsize; j++)
                        push_sample(voices[v].delay_line, buf[j]);  
                    free(buf);
                } else if (voices[v].delay_line!=NULL){
                //szarpnięcie struny, szum do bufora
                    for (size_t j = 0; j < voices[v].delay_line->size; j++) {
                        float noise = generate_white_noise();
                        
                        //uciszamy szybko jesli to ktorys z tych mode
                        if (voices[v].mode == MODE_DRUM || voices[v].mode == MODE_SNARE) {
                            noise *= envelope; 
                            envelope *= 0.95f;  //sciszamh szum zgodnie z obwiednią- obwiednia 5% w dól
                        }
                        push_sample(voices[v].delay_line, noise);
                    }
                }

                voices[v].is_active = 1;
                voices[v].has_been_plucked = 1;
                voices[v].hpf_state = 0.0f; 
            }

            if (voices[v].is_active) {
                //probka z bufora
                float current_sample;
                float processed_sample = 0.0f;

                //Filtrdolnoprzepustowy z alfa
                if(voices[v].delay_line != NULL){
                    if (voices[v].mode == MODE_HIHAT) {
                        //próbka z losowego miejsca blisko końca bufora, powiino dać metalicznosc
                        int mod_offset = rand() % 10; 
                        current_sample = read_sample(voices[v].delay_line, voices[v].delay_line->size - mod_offset);
                    } else if (voices[v].mode == MODE_PIANO){
                        current_sample = read_sample_interpolated(voices[v].delay_line, voices[v].exact_delay);
                    }
                    
                    else {
                        //odczyt standardowey
                        current_sample = read_sample(voices[v].delay_line, voices[v].delay_line->size);
                    }
                }

                //filtracja zależna od trybu
                switch (voices[v].mode) {
                    case MODE_STRING:
                    case MODE_DRUM:
                        processed_sample = (voices[v].alpha * current_sample) + ((1.0f - voices[v].alpha) * voices[v].previous_sample);
                        break;

                    case MODE_HIHAT: {
                        float sign = (rand() % 100 < 50) ? -1.0f : 1.0f; 
                        processed_sample = sign * ((voices[v].alpha * current_sample) + ((1.0f - voices[v].alpha) * voices[v].previous_sample));
                        break;
                    }

                    case MODE_SNARE: {
                        float sign = (rand() % 100 < 50) ? -1.0f : 1.0f; 
                        float smoothed = 0.5f * current_sample + 0.5f * voices[v].previous_sample;
                        processed_sample = sign * smoothed;
                        
                        float hpf_out = processed_sample - voices[v].hpf_state;
                        voices[v].hpf_state = processed_sample;
                        processed_sample = hpf_out;
                        break;
                    }

                    case MODE_PIANO: {
                        voices[v].sample_count++;
                        voices[v].amp_envelope *= 0.999996f; 
                        voices[v].brightness *= 0.9992f;
                        
                        float next_sample = read_sample_interpolated(voices[v].delay_line, voices[v].exact_delay - 1.0f);
                        float smoothed = (voices[v].previous_sample * 0.25f) + (current_sample * 0.5f) + (next_sample * 0.25f);
                        smoothed *= 0.995f;

                        float c = voices[v].ap_coef;
                        float ap_out = -c * smoothed + voices[v].ap_prev_in + c * voices[v].ap_prev_out;
                        voices[v].ap_prev_in  = smoothed;
                        voices[v].ap_prev_out = ap_out;

                        float ap2_out = -c * ap_out + voices[v].ap2_prev_in + c * voices[v].ap2_prev_out;
                        voices[v].ap2_prev_in  = ap_out;
                        voices[v].ap2_prev_out = ap2_out;

                        processed_sample = ap2_out; 
                        out_sample = processed_sample * voices[v].amp_envelope;
                        break;
                    }

                    case MODE_FDTD_GONG:
                    case MODE_FDTD_METALIC:
                    case MODE_FDTD_DRUM: {
                        int is_drum = (voices[v].mode == MODE_FDTD_DRUM) ? 1 : 0;
                        float raw_mesh_sample = process_drum2d(voices[v].drum_mesh, is_drum) * 2.0f; 
                        
                        if (voices[v].mode == MODE_FDTD_METALIC || voices[v].mode == MODE_FDTD_DRUM) {
                            processed_sample = (voices[v].alpha * raw_mesh_sample) + ((1.0f - voices[v].alpha) * voices[v].previous_sample);
                            voices[v].previous_sample = processed_sample;
                        } else {
                            processed_sample = raw_mesh_sample;
                        }
                        break;
                    }
                    
                    default:
                        processed_sample = 0.0f;
                        break;
                }

                if (voices[v].delay_line != NULL) {
                    
                    voices[v].previous_sample = processed_sample;
    
                    processed_sample *= voices[v].damping;
                    push_sample(voices[v].delay_line, processed_sample);
                }

                //miksowanie
                if (voices[v].mode == MODE_PIANO) {
                    mixed_piano += (out_sample * 0.33f);
                } else {
                    mixed_others += processed_sample;
                }
            }
        }

        //zabezpieczenie na clipping
        if (num_voices > 0) {
            float final_piano = mixed_piano / (float)num_voices;
            float final_others = mixed_others / (float)num_voices;
            
            float dry = final_piano; // Tylko pianino wchodzi do pogłosu
            
            float c1_out = read_sample(comb1, comb1->size);
            float c2_out = read_sample(comb2, comb2->size);
            float c3_out = read_sample(comb3, comb3->size);
            float c4_out = read_sample(comb4, comb4->size);
            
            float wet = (c1_out + c2_out + c3_out + c4_out) * 0.25f;
            
            push_sample(comb1, dry + c1_out * rev_feedback);
            push_sample(comb2, dry + c2_out * rev_feedback);
            push_sample(comb3, dry + c3_out * rev_feedback);
            push_sample(comb4, dry + c4_out * rev_feedback);
            
            // Miksujemy: Pianino(z pogłosem) + Inne Instrumenty(czyste)
            float output_mixed = (dry * 0.6f) + (wet * 0.4f) + final_others;
            
            if (output_mixed > 1.0f) output_mixed = 1.0f;
            if (output_mixed < -1.0f) output_mixed = -1.0f;
            
            output[i] = output_mixed;
        } else {
            output[i] = 0.0f;
        }
    }
    free_buffer(comb1);
    free_buffer(comb2);
    free_buffer(comb3);
    free_buffer(comb4);
}