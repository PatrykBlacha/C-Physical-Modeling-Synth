#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"
#include "wav.h"
#include "synth.h"


int main() {
    init_noise_generator();
    int sample_rate = 44100;

    printf("\n=== SEKWEKCER KARPLUS-STRONG (Z PLIKU) ===\n");

    //ZAPytanie o nazwę plikus
    char filename[100];
    printf("Podaj nazwe pliku z parametrami: ");
    scanf("%99s", filename);

    char out_filename[100];
    strcpy(out_filename, filename);
    
    char *dot = strrchr(out_filename, '.');   //plik wyjsciowy
    if (dot != NULL) {
        strcpy(dot, ".wav");
    } else {
        strcat(out_filename, ".wav");
    }


    //r to tryb odczytu
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Blad: Nie udalo sie otworzyc pliku %s!\n", filename);
        return 1;
    }

    float duration_sec;
    int num_voices;

    //1. linia
    if (fscanf(file, "%f %d", &duration_sec, &num_voices) != 2) {
        printf("Blad formatu w pierwszej linijce pliku.\n");
        fclose(file);
        return 1;
    }

    //alokacja pamieci na plik i struny
    size_t num_samples = (size_t)(sample_rate * duration_sec);
    float* output_audio = (float*)malloc(num_samples * sizeof(float));
    Voice* voices = (Voice*)malloc(num_voices * sizeof(Voice));

    if (!output_audio || !voices) {
        fclose(file);
        return 1;
    }

    printf("Pobrano z pliku: %d instrumentow. Czas nagrania: %.2f s\n", num_voices, duration_sec);

    //pętla wczytująca kolejne linijki instrumentów
    for (int i = 0; i < num_voices; i++) {
        int mode_input;

        if (fscanf(file, "%d", &mode_input) != 1) {
            printf("Blad formatu danych (brak trybu) w linii %d!\n", i + 2);
            fclose(file);
            return 1;
        }

        if (mode_input < 0 || mode_input > 7) mode_input = 0; // Zabezpieczenie

        voices[i].mode = (InstrumentMode)mode_input;
        voices[i].is_active = 0;
        voices[i].has_been_plucked = 0;
        voices[i].previous_sample = 0.0f;
        voices[i].amp_envelope = 0.0f;
        voices[i].sample_count = 0;

       if (voices[i].mode == MODE_FDTD_GONG || voices[i].mode == MODE_FDTD_METALIC || voices[i].mode == MODE_FDTD_DRUM) {
            
            //Format: Tryb | Rho | Damping | Alpha | StartTime
            float rho, damping, alpha, start_time;
            if (fscanf(file, "%f %f %f %f", &rho, &damping, &alpha, &start_time) != 4) {
                printf("Blad parametrow FDTD w linii %d!\n", i + 2); return 1;
            }

            voices[i].damping = damping;
            voices[i].alpha = alpha;
            voices[i].start_time_sec = start_time;
            
            //kształt bębna
            int shape = (voices[i].mode == MODE_FDTD_METALIC) ? 1 : 0; 
            voices[i].drum_mesh = create_drum2d(rho, damping, shape);
            
            //nie używne przez FDTD
            voices[i].delay_line = NULL;
       } else {
            //Format: Tryb | Freq | Damping | Alpha | StartTime
            float freq, damping, alpha, start_time;
            if (fscanf(file, "%f %f %f %f", &freq, &damping, &alpha, &start_time) != 4) {
                printf("Blad parametrow Karplus-Strong w linii %d!\n", i + 2); return 1;
            }

            voices[i].damping = damping;
            voices[i].alpha = alpha;
            voices[i].start_time_sec = start_time;
            
            voices[i].drum_mesh = NULL;

            //parametry dla struny 
            float inharmonicity = 0.0002f * freq; 
            if (inharmonicity > 0.6f) inharmonicity = 0.6f; 

            voices[i].ap_coef = inharmonicity; 

            float allpass_delay = (1.0f - voices[i].ap_coef) / (1.0f + voices[i].ap_coef);
            voices[i].exact_delay = ((float)sample_rate / freq) - (2.0f * allpass_delay);

            if (voices[i].exact_delay < 2.0f) voices[i].exact_delay = 2.0f;

            size_t buffer_size = (size_t)voices[i].exact_delay + 2;
            voices[i].delay_line = create_buffer(buffer_size);

            for(size_t j = 0; j < buffer_size; j++) {
                push_sample(voices[i].delay_line, 0.0f); 
            }

            // Inicjalizacja filtrów
            voices[i].ap2_prev_in  = 0.0f;
            voices[i].ap2_prev_out = 0.0f;
            voices[i].ap_prev_in = 0.0f;
            voices[i].ap_prev_out = 0.0f;
            voices[i].brightness   = 1.0f;
            voices[i].attack_samples = 0.0f;
        }
    } 

    fclose(file);

    // Renderowanie 
    printf("Trwa renderowanie DSP...\n");
    synthesize_poly(voices, num_voices, output_audio, num_samples);

    save_to_wav(out_filename, output_audio, num_samples, sample_rate);

    // Sprzątanie
    for (int i = 0; i < num_voices; i++) {
        free_buffer(voices[i].delay_line);
        
        //siatka 2d
        if (voices[i].mode == MODE_FDTD_GONG || voices[i].mode == MODE_FDTD_METALIC || voices[i].mode== MODE_FDTD_DRUM) {
            free_drum2d(voices[i].drum_mesh);
        }
    }
    free(voices);
    free(output_audio);

    printf("Zrobione! Otworz plik: wynik.wav\n\n");
    return 0;
}