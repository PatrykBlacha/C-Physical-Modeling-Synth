#include "fdtd_drum.h"
#include <stdlib.h>

//tworzenie i zerowanie pamięci
Drum2D* create_drum2d(float rho, float damping, int is_circular) {
    Drum2D* drum = (Drum2D*)malloc(sizeof(Drum2D));
    drum->rho = rho;
    drum->damping = damping;
    drum->is_circular = is_circular;

    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            drum->u_past[x][y] = 0.0f;
            drum->u_present[x][y] = 0.0f;
            drum->u_next[x][y] = 0.0f;
        }
    }
    return drum;
}

void strike_drum2d(Drum2D* drum, int cx, int cy, float force) {
    //uderzamy w obszar 5x5 miękką, filcową pałką- powinno eliminować szum
    for (int dx = -3; dx <= 3; dx++) {
        for (int dy = -3; dy <= 3; dy++) {
            int x = cx + dx;
            int y = cy + dy;
            int y_correct = cy + dy;
            if (x > 0 && x < GRID_SIZE - 1 && y_correct > 0 && y_correct < GRID_SIZE - 1) {
                //siła maleje łagodnie wraz z odległością od środka uderzenia
                float dist = (float)(dx*dx + dy*dy);
                float local_force = force * (1.0f / (1.0f + dist)); 
                //float local_force = force * (1.0f / (1.0f + (dist * 2.0f)));
                
                //drum->u_present[x][y] = local_force; 
                //drum->u_past[x][y] = local_force; //zatrzymanie
                drum->u_present[x][y_correct] += local_force;
            }
        }
    }
}

//matma
float process_drum2d(Drum2D* drum) {
    float real_damping = 1.0f - ((1.0f - drum->damping) * 0.02f);

    float cx = GRID_SIZE / 2.0f;
    float cy = GRID_SIZE / 2.0f;
    float r_sq = (GRID_SIZE / 2.0f - 1.0f) * (GRID_SIZE / 2.0f - 1.0f); // Promień do kwadratu

    for (int x = 1; x < GRID_SIZE - 1; x++) {
        for (int y = 1; y < GRID_SIZE - 1; y++) {

            if (drum->is_circular) {
                float dx = (float)x - cx;
                float dy = (float)y - cy;
                if (dx*dx + dy*dy >= r_sq) {
                    drum->u_next[x][y] = 0.0f; //0 poza okregiem
                    continue; // Pomijamy matme
                }
            }

            float neighbors = drum->u_present[x+1][y] + drum->u_present[x-1][y] + 
                              drum->u_present[x][y+1] + drum->u_present[x][y-1];
                              
            drum->u_next[x][y] = (drum->rho * drum->rho) * (neighbors - 4.0f * drum->u_present[x][y]) 
                                 + 2.0f * drum->u_present[x][y] 
                                 - drum->u_past[x][y];
                                 
            //używamy naszego przeskalowanego, bezpiecznego tłumienia!
            drum->u_next[x][y] *= real_damping; 
        }
    }   

    //MIKROFON PARAMETRY
    //float output_sample = drum->u_next[GRID_SIZE / 3][GRID_SIZE / 3] * 1.0f;

    float output_sample = 0.0f;
    int mic_cx = (int)(GRID_SIZE * 0.25f); // np. 25% w osi X
    int mic_cy = (int)(GRID_SIZE * 0.45f); // np. 45% w osi Y
    int mic_r = 4; // Zbieramy dźwięk z siatki 9x9 węzłów
    float points = 0.0f;

    for (int dx = -mic_r; dx <= mic_r; dx++) {
        for (int dy = -mic_r; dy <= mic_r; dy++) {
            // Zabezpieczenie przed wyjściem poza siatkę
            int mx = mic_cx + dx;
            int my = mic_cy + dy;
            if (mx > 0 && mx < GRID_SIZE - 1 && my > 0 && my < GRID_SIZE - 1) {
                output_sample += drum->u_next[mx][my];
                points += 1.0f;
            }
        }
    }
    
    // Uśredniamy wynik i dajemy lekki Gain
    if (points > 0.0f) {
        output_sample = (output_sample / points) * 3.0f; 
    } else {
        output_sample = 0.0f;
    }


    float c_weight = 0.9998f;
    float n_weight = (1.0f - c_weight) / 4.0f; 
    //przesunięcie czasu 
    for (int x = 1; x < GRID_SIZE - 1; x++) {
        for (int y = 1; y < GRID_SIZE - 1; y++) {
            //drum->u_past[x][y] = drum->u_present[x][y];
            //drum->u_present[x][y] = drum->u_next[x][y];

            float smoothed_present = drum->u_present[x][y] * c_weight + 
                                (drum->u_present[x+1][y] + drum->u_present[x-1][y] + 
                                 drum->u_present[x][y+1] + drum->u_present[x][y-1]) * n_weight;

            drum->u_past[x][y] = smoothed_present;
            //drum->u_present[x][y] = drum->u_next[x][y];
        }
    }

    for (int x = 1; x < GRID_SIZE - 1; x++) {
        for (int y = 1; y < GRID_SIZE - 1; y++) {
            drum->u_present[x][y] = drum->u_next[x][y];
        }
    }

    return output_sample;
}

//czyszczenie
void free_drum2d(Drum2D* drum) {
    if (drum) free(drum);
}