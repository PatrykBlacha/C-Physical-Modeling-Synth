#include "fdtd_drum.h"
#include <stdlib.h>

// Tworzenie i zerowanie pamięci
Drum2D* create_drum2d(float rho, float damping) {
    Drum2D* drum = (Drum2D*)malloc(sizeof(Drum2D));
    drum->rho = rho;
    drum->damping = damping;

    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            drum->u_past[x][y] = 0.0f;
            drum->u_present[x][y] = 0.0f;
            drum->u_next[x][y] = 0.0f;
        }
    }
    return drum;
}

// Uderzenie pałką w punkt (x,y)
void strike_drum2d(Drum2D* drum, int x, int y, float force) {
    if (x > 0 && x < GRID_SIZE - 1 && y > 0 && y < GRID_SIZE - 1) {
        drum->u_present[x][y] = force; 
        // Można też wzbudzić punkty obok, dla "grubszej" pałki
    }
}

// Główna matematyka: oblicza nową siatkę, zwraca próbkę do pliku
float process_drum2d(Drum2D* drum) {
    float real_damping = 1.0f - ((1.0f - drum->damping) * 0.0005f);

    // 1. Liczymy nowy stan membrany
    for (int x = 1; x < GRID_SIZE - 1; x++) {
        for (int y = 1; y < GRID_SIZE - 1; y++) {
            float neighbors = drum->u_present[x+1][y] + drum->u_present[x-1][y] + 
                              drum->u_present[x][y+1] + drum->u_present[x][y-1];
                              
            drum->u_next[x][y] = (drum->rho * drum->rho) * (neighbors - 4.0f * drum->u_present[x][y]) 
                                 + 2.0f * drum->u_present[x][y] 
                                 - drum->u_past[x][y];
                                 
            // Używamy naszego przeskalowanego, bezpiecznego tłumienia!
            drum->u_next[x][y] *= real_damping; 
        }
    }

    // 2. Mikrofon: Mnożymy razy 20.0f, żeby zrekompensować utratę energii w 2D!
    float output_sample = drum->u_next[GRID_SIZE / 3][GRID_SIZE / 3] * 1.0f;

    // 3. Przesuwamy czas (teraźniejszość staje się przeszłością)
    for (int x = 1; x < GRID_SIZE - 1; x++) {
        for (int y = 1; y < GRID_SIZE - 1; y++) {
            drum->u_past[x][y] = drum->u_present[x][y];
            drum->u_present[x][y] = drum->u_next[x][y];
        }
    }

    return output_sample;
}

// Czyszczenie pamięci
void free_drum2d(Drum2D* drum) {
    if (drum) free(drum);
}