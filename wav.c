#include "wav.h"
#include "core.h"
#include <stdio.h>
#include <stdint.h>

void save_to_wav(const char* filename, float* buffer, size_t num_samples, int sample_rate) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("Błąd: Nie można otworzyć pliku %s do zapisu.\n", filename);
        return;
    }

    //parametry formatu:44.1 kHz,16bit, Mono
    uint16_t num_channels = 1;
    uint16_t bits_per_sample = 16;
    uint32_t byte_rate = sample_rate * num_channels * (bits_per_sample / 8);    //szybkość tramsisji- sample rate* block align
    uint16_t block_align = num_channels * (bits_per_sample / 8);    //rozmiar pojedynczej ramki dla wszystkich kanałów(suma)
    uint32_t data_size = num_samples * block_align;
    uint32_t chunk_size = 36 + data_size; //całkowity rozmiar pliku bez pierwszych 8 bajtów- fmt, data

    //nagłówek
    fwrite("RIFF", 1, 4, file);
    fwrite(&chunk_size, 4, 1, file);
    fwrite("WAVE", 1, 4, file);

    //subchunk 1: "fmt "
    fwrite("fmt ", 1, 4, file);
    uint32_t subchunk1_size = 16; //nieskompesowane PCM
    fwrite(&subchunk1_size, 4, 1, file);
    uint16_t audio_format = 1;    //format PCM 
    fwrite(&audio_format, 2, 1, file);  //liniowy PCM bez kompresji
    fwrite(&num_channels, 2, 1, file);  //kanały dźwiękowe
    fwrite(&sample_rate, 4, 1, file);
    fwrite(&byte_rate, 4, 1, file);
    fwrite(&block_align, 2, 1, file);  //rozmiar pojedynczej ramki dla wszystich łącznie
    fwrite(&bits_per_sample, 2, 1, file);   //rozdzielczość bitowa dla bojedynczej próbki 

    //subchunk2: data
    fwrite("data", 1, 4, file);
    fwrite(&data_size, 4, 1, file);

    for (size_t i = 0; i < num_samples; i++) {
        float sample = buffer[i];
        
        //zabezpieczenie na clipping
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;

        //konwersja
        int16_t int_sample = (int16_t)(sample * 32767.0f);
        fwrite(&int_sample, 2, 1, file);
    }

    fclose(file);
    printf("Zapisano plik: %s (%zu probek, %d Hz)\n", filename, num_samples, sample_rate);
}

int load_wav_to_buffer(const char* filename, CircularBuffer* cb) {
    FILE* file = fopen(filename, "rb"); // "rb" oznacza Read Binary (odczyt binarny)
    if (!file) {
        printf("Blad: Nie mozna otworzyc pliku zrodlowego %s!\n", filename);
        return 0;
    }

    //fsek żeby pominąć nagłówek
    fseek(file, 44, SEEK_SET);

    //Karplus Strong- zapis do bufora
    for (size_t i = 0; i < cb->size; i++) {
        int16_t int_sample = 0;
        
        //czytanie 2 bajtów
        if (fread(&int_sample, 2, 1, file) != 1) {
            break; 
        }

        //reverse konwersja pcm
        float float_sample = (float)int_sample / 32768.0f;
        
        //próbka do bufora
        push_sample(cb, float_sample);
    }

    fclose(file);
    return 1;
}