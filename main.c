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

    // 1. ZAPytanie o nazwę pliku
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


    // 2. Otwieramy plik w trybie do odczytu ("r" - read)
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Blad: Nie udalo sie otworzyc pliku %s!\n", filename);
        return 1;
    }

    float duration_sec;
    int num_voices;

    // 3. Wczytujemy z pierwszej linijki czas trwania (float) i ilosc nut (int)
    if (fscanf(file, "%f %d", &duration_sec, &num_voices) != 2) {
        printf("Blad formatu w pierwszej linijce pliku.\n");
        fclose(file);
        return 1;
    }

    // Alokacja pamieci na plik i struny (Zostaje jak było)
    size_t num_samples = (size_t)(sample_rate * duration_sec);
    float* output_audio = (float*)malloc(num_samples * sizeof(float));
    Voice* voices = (Voice*)malloc(num_voices * sizeof(Voice));

    if (!output_audio || !voices) {
        fclose(file);
        return 1;
    }

    printf("Pobrano z pliku: %d instrumentow. Czas nagrania: %.2f s\n", num_voices, duration_sec);

    // 4. Pętla wczytująca kolejne linijki instrumentów
    for (int i = 0; i < num_voices; i++) {
        float freq, damping, alpha, start_time;
        int mode_input;

        // Magia fscanf - zaciągamy 5 zmiennych naraz z jednej linijki!
        if (fscanf(file, "%d %f %f %f %f", &mode_input, &freq, &damping, &alpha, &start_time) != 5) {
            printf("Blad formatu danych w linii %d!\n", i + 2);
            fclose(file);
            return 1;
        }

        if (mode_input < 0 || mode_input > 5) mode_input = 0; // Zabezpieczenie

        // Wstrzykniecie wartosci do struny (Zostaje jak było)
        size_t buffer_size = (size_t)(sample_rate / freq);
        voices[i].delay_line = create_buffer(buffer_size);
        
        for(size_t j = 0; j < buffer_size; j++) {
            push_sample(voices[i].delay_line, 0.0f); 
        }

        voices[i].damping = damping;
        voices[i].alpha = alpha;
        voices[i].previous_sample = 0.0f;
        voices[i].ap_prev_in = 0.0f;
        voices[i].ap_prev_in = 0.0f;
        voices[i].ap_prev_out = 0.0f;
        
        voices[i].mode = (InstrumentMode)mode_input; 

        //membrana 2d
        if (voices[i].mode == MODE_FDTD_DRUM) {
            // rho = 0.70f (Maksymalne stabilne naprężenie wg warunku CFL)
            voices[i].drum_mesh = create_drum2d(0.15f, damping);
        } else {
            voices[i].drum_mesh = NULL; //inne 
        }
        voices[i].is_active = 0;
        voices[i].has_been_plucked = 0;
        voices[i].start_time_sec = start_time;
    }

    // 5. Zamykamy plik - skończyliśmy go czytać
    fclose(file);

    // Renderowanie 
    printf("Trwa renderowanie DSP...\n");
    synthesize_poly(voices, num_voices, output_audio, num_samples);

    save_to_wav(out_filename, output_audio, num_samples, sample_rate);

    // Sprzątanie
    for (int i = 0; i < num_voices; i++) {
        free_buffer(voices[i].delay_line);
        
        //siatka 2d
        if (voices[i].mode == MODE_FDTD_DRUM) {
            free_drum2d(voices[i].drum_mesh);
        }
    }
    free(voices);
    free(output_audio);

    printf("Zrobione! Otworz plik: wynik.wav\n\n");
    return 0;
}

/*
int main() {
    init_noise_generator();
    int sample_rate = 44100;

    printf("\n=== SEKWEKCER KARPLUS-STRONG ===\n");

    int num_voices;
    printf("Ile instrumentow/strun chcesz uzyc? ");
    if (scanf("%d", &num_voices) != 1 || num_voices <= 0) {
        printf("Zla liczba\n");
        return 1;
    }

    float duration_sec;
    printf("Ile sekund ma trwac cale nagranie? ");
    if (scanf("%f", &duration_sec) != 1 || duration_sec <= 0.0f) {
        printf("Zly czas\n");
        return 1;
    }

    //Alokacja pamieci na plik i struny
    size_t num_samples = (size_t)(sample_rate * duration_sec);
    float* output_audio = (float*)malloc(num_samples * sizeof(float));
    Voice* voices = (Voice*)malloc(num_voices * sizeof(Voice)); //dynamiczna tablica glosow

    if (!output_audio || !voices) return 1;

    for (int i = 0; i < num_voices; i++) {
        float freq, damping, alpha, start_time;
        int mode_input;

        printf("\n--- KONFIGURACJA GLOSU %d ---\n", i + 1);
        
        printf("Wybierz instrument (0=STRUNA, 1=STOPA, 2=WERBEL, 3=HI-HAT, 4=PIANINO): ");
        scanf("%d", &mode_input);
        if (mode_input < 0 || mode_input > 4) mode_input = 0; //zabiezpieczenie

        printf("Czestotliwosc w Hz (np. bas=82, werbel=120, hihat=300): ");
        scanf("%f", &freq);

        printf("Tlumienie (np. gitara=0.999, perkusja=0.950): ");
        scanf("%f", &damping);

        printf("Ostrosc (0.9 = jasny brzek, 0.1 = matowy, gluchy): ");
        scanf("%f", &alpha);

        printf("Czas startu w sekundach (np. 0.0, 1.5): ");
        scanf("%f", &start_time);

        //wstrzykniecie wartosci do struny
        size_t buffer_size = (size_t)(sample_rate / freq);
        voices[i].delay_line = create_buffer(buffer_size);
        
        //zapełniamy bufor początkowy ciszą (0.0f)
        for(size_t j = 0; j < buffer_size; j++) {
            push_sample(voices[i].delay_line, 0.0f); 
        }

        voices[i].damping = damping;
        voices[i].alpha = alpha;
        voices[i].previous_sample = 0.0f;

        voices[i].ap_prev_in = 0.0f;
        voices[i].ap_prev_out = 0.0f;
        
        //mapowanie wpisanej liczby na tryb instrumentu
        voices[i].mode = (InstrumentMode)mode_input; 
        
        //ustawienia sekwencera
        voices[i].is_active = 0;
        voices[i].has_been_plucked = 0;
        voices[i].start_time_sec = start_time;
    }

    // Renderowanie 
    synthesize_poly(voices, num_voices, output_audio, num_samples);

    save_to_wav("pianino2.wav", output_audio, num_samples, sample_rate);

    // Sprzątanie
    for (int i = 0; i < num_voices; i++) {
        free_buffer(voices[i].delay_line);
    }
    free(voices);
    free(output_audio);

    printf("Otworz plik: orkiestra.wav\n\n");6
    return 0;
}
*/


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