#include <stdio.h>
#include <stdlib.h>
#include "core.h"
#include "wav.h"
#include "synth.h"

int main() {
    init_noise_generator();
    int sample_rate = 44100;

    int num_voices;
    printf("Ile strun? ");
    if (scanf("%d", &num_voices) != 1 || num_voices <= 0) {
        printf("Zla liczba\n");
        return 1;
    }

    float duration_sec;
    printf("Ile sekund? ");
    if (scanf("%f", &duration_sec) != 1 || duration_sec <= 0.0f) {
        printf("Zly czas\n");
        return 1;
    }

    //Alokacja pamieci na plik i struny
    size_t num_samples = (size_t)(sample_rate * duration_sec);
    float* output_audio = (float*)malloc(num_samples * sizeof(float));
    Voice* voices = (Voice*)malloc(num_voices * sizeof(Voice)); //dynamiczna tablica głospw

    if (!output_audio || !voices) return 1;

    for (int i = 0; i < num_voices; i++) {
        float freq, damping, alpha, start_time;

        printf("\n--- KONFIGURACJA STRUNY %d ---\n", i + 1);
        
        printf("Czestotliwosc w Hz (bas=82, gitara=330): ");
        scanf("%f", &freq);

        printf("Tlumienie- twardosc struny: ");
        scanf("%f", &damping);

        printf("Ostrosc (0.9 = jasny brzek, 0.1 = matowy, gluchy): ");
        scanf("%f", &alpha);

        printf("Czas startu w sekundach: ");
        scanf("%f", &start_time);

        //Wstrykniecie wartosci do struny
        size_t buffer_size = (size_t)(sample_rate / freq);
        voices[i].delay_line = create_buffer(buffer_size);
        voices[i].damping = damping;
        voices[i].alpha = alpha;
        voices[i].previous_sample = 0.0f;
        voices[i].is_active = 0;
        voices[i].mode = MODE_STRING;
        voices[i].has_been_plucked=0;
        voices[i].start_time_sec=start_time;

        //Szarpniecie
        for (size_t j = 0; j < buffer_size; j++) {
            push_sample(voices[i].delay_line, generate_white_noise());
        }
    }

    //renderowanie 
    synthesize_poly(voices, num_voices, output_audio, num_samples);

    save_to_wav("drum.wav", output_audio, num_samples, sample_rate);

    //sprzątanie
    for (int i = 0; i < num_voices; i++) {
        free_buffer(voices[i].delay_line);
    }
    free(voices);
    free(output_audio);

    printf("Otworz plik: wlasny_akord.wav\n\n");
    return 0;
}



/*
#include <stdio.h>
#include <stdlib.h>
#include "core.h"
#include "wav.h"
#include "synth.h"


int main(int argc, char* argv[]) {
    init_noise_generator();
    
    int sample_rate = 44100;
    
    //default
    float frequency = 110.0f; 
    float damping = 0.999f;
    InstrumentMode mode = MODE_STRING;

    //frequency
    if (argc > 1) {
        frequency = atof(argv[1]); 
        if (frequency <= 0.0f) {
            printf("Blad: Czestotliwosc musi byc wieksza od 0!\n");
            return 1;
        }
    }

    if (argc > 2) {
        damping = atof(argv[2]);
        if (damping < 0.0f || damping > 1.0f) {
            printf("Blad: Tlumienie powinno byc w przedziale od 0.0 do 1.0!\n");
            return 1;
        }
    }

    printf("\n--- SYNTEZATOR KARPLUS-STRONG ---\n");
    printf("Czestotliwosc: %.2f Hz\n", frequency);
    printf("Tlumienie: %.4f\n", damping);
    
    //obliczanie bufora
    size_t buffer_size = (size_t)(sample_rate / frequency);
    CircularBuffer* delay_line = create_buffer(buffer_size);
    
    size_t num_samples = sample_rate * 3; //3 sekundy dźwięku
    float* output_audio = (float*)malloc(num_samples * sizeof(float));

    if (!delay_line || !output_audio) return 1;

    //moduł excitation- linia opóźniająca
    for (size_t i = 0; i < buffer_size; i++) {
        push_sample(delay_line, generate_white_noise());
    }

    //synthesis
    printf("Trwa renderowanie DSP...\n");
    synthesize(delay_line, output_audio, num_samples, mode, damping);

    //zapis
    save_to_wav("test_wysokosci.wav", output_audio, num_samples, sample_rate);

    free_buffer(delay_line);
    free(output_audio);
    
    printf("Wygenerowano plik: test_wysokosci.wav\n\n");

    return 0;
}

*/


/*
int main(int argc, char* argv[]) {
    init_noise_generator();
    
    int sample_rate = 44100;
    //110hz-struna w basie
    float frequency = 110.0f; 

    if (argc > 1) {
        frequency = atof(argv[1]); 
        
        if (frequency <= 0.0f) {
            printf("Blad: Czestotliwosc musi byc wieksza od 0!\n");
            return 1;
        }
    }

    printf("Generowanie dzwieku o czestotliwosci: %.2f Hz\n", frequency);
    
    //rozmiar bufora: L = SR / f 
    size_t buffer_size = (size_t)(sample_rate / frequency);
    CircularBuffer* delay_line = create_buffer(buffer_size);
    
    //3s
    size_t num_samples = sample_rate * 3; 
    float* output_audio = (float*)malloc(num_samples * sizeof(float));

    if (!delay_line || !output_audio) return 1;

    //Moduł Excitation - wzbudzenie linii opóźniającej
    for (size_t i = 0; i < buffer_size; i++) {
        push_sample(delay_line, generate_white_noise());
    }

    //Moduł Synthesis- pętla Karplus-Strong 
    printf("Generowanie dzwieku struny...\n");
    //tłumienność
    synthesize(delay_line, output_audio, num_samples, MODE_STRING, 0.999f); //0.999-stal

    //zapis do pliku
    save_to_wav("struna_stalowa.wav", output_audio, num_samples, sample_rate);

    free_buffer(delay_line);
    
    //tryb perkusyjny- werbel
    float drum_freq = 400.0f; 
    size_t drum_buffer_size = (size_t)(sample_rate / drum_freq);

    //krótsza linia opóźniająca
    delay_line = create_buffer(drum_buffer_size);
    for (size_t i = 0; i < drum_buffer_size; i++) {
        push_sample(delay_line, generate_white_noise());
    }
    
    printf("Generowanie dzwieku werbla...\n");
    //Potrzebuje silniejszego tłumienia (np. 0.990), by szybko zgasnąć
    synthesize(delay_line, output_audio, num_samples, MODE_DRUM, 0.900f);
    save_to_wav("werbel.wav", output_audio, num_samples, sample_rate);

    free_buffer(delay_line);
    free(output_audio);

    return 0;
}
*/

/*

int main() {
    int sample_rate = 44100;
    float frequency = 110.0f; 
    size_t buffer_size = (size_t)(sample_rate / frequency);
    CircularBuffer* delay_line = create_buffer(buffer_size);
    size_t num_samples = sample_rate * 3; 
    float* output_audio = (float*)malloc(num_samples * sizeof(float));

    if (!delay_line || !output_audio) return 1;

    //wzbudzenie
    printf("Wczytywanie zewnetrznego sampla...\n");
    if (!load_wav_to_buffer("dlugopisy.wav", delay_line)) {
        free_buffer(delay_line);
        free(output_audio);
        return 1;
    }

    //Moduł Synthesis
    printf("Generowanie hybrydowego dzwieku...\n");
    synthesize(delay_line, output_audio, num_samples, MODE_STRING, 0.999f);

    //Zapis
    save_to_wav("dlugopisypo.wav", output_audio, num_samples, sample_rate);

    free_buffer(delay_line);
    free(output_audio);
    printf("Zrobione!\n");

    return 0;
}

*/