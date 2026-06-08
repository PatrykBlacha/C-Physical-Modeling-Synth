#ifndef FDTD_DRUM_H
#define FDTD_DRUM_H

#define GRID_SIZE 50 // Rozdzielczość bębna (np. 40x40 węzłów)

// Struktura naszej wirtualnej membrany
typedef struct {
    float u_past[GRID_SIZE][GRID_SIZE];
    float u_present[GRID_SIZE][GRID_SIZE];
    float u_next[GRID_SIZE][GRID_SIZE];
    
    float rho;       // Naprężenie membrany (max ok. 0.7)
    float damping;   // Tłumienie (np. 0.999)
} Drum2D;

// Funkcje do obsługi bębna
Drum2D* create_drum2d(float rho, float damping);
void strike_drum2d(Drum2D* drum, int x, int y, float force);
float process_drum2d(Drum2D* drum);
void free_drum2d(Drum2D* drum);

#endif