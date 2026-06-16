#include "fdtd_drum.h"
#include <stdlib.h>

//#include <math.h> //potrzebne tylko do rozkładu Gaussa do uderzenia w membranę

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
    //uderzamy w obszar 7x7 miękką, filcową pałką- powinno eliminować szum
    for (int dx = -3; dx <= 3; dx++) {
        for (int dy = -3; dy <= 3; dy++) {
            int x = cx + dx;
            int y = cy + dy;
            int y_correct = cy + dy;
            if (x > 0 && x < GRID_SIZE - 1 && y_correct > 0 && y_correct < GRID_SIZE - 1) {
                //siła maleje łagodnie wraz z odległością od środka uderzenia
                float dist = (float)(dx*dx + dy*dy);
                float local_force = force * (1.0f / (1.0f + dist)); 
                
                drum->u_present[x][y_correct] += local_force;
                drum->u_past[x][y_correct] += local_force;  //powstrzymanie metalu
            }
        }
    }
}


//zakomentowana wersja z rozkładem Gaussa
    /*
void strike_drum2d(Drum2D* drum, int cx, int cy, float force) {
    int radius = 3;
    float sigma = 1.0f; //twardosc
    //uderzamy w obszar 5x5 miękką, filcową pałką- powinno eliminować szum

    float variance2 = 2.0f * sigma * sigma; //wariancja do rozkładu gasusa

    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            int x = cx + dx;
            int y = cy + dy;
            
            if (x > 0 && x < GRID_SIZE - 1 && y > 0 && y < GRID_SIZE - 1) {
                
                float dist_sq = (float)(dx*dx + dy*dy);
                
                float gauss_weight = expf(-dist_sq / variance2);
                
                float local_force = force * gauss_weight*0.5; 
                
                drum->u_present[x][y] += local_force;
                //drum->u_past[x][y] += local_force;
            }
        }
    }
}
    */

//matma
float process_drum2d(Drum2D* drum, int is_drum) {
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
                              
            float lossy_stiffness = 0.0f;
            
            if (is_drum) {
                lossy_stiffness = 0.002f * (neighbors - 4.0f * drum->u_present[x][y] - 
                                          (drum->u_past[x+1][y] + drum->u_past[x-1][y] + 
                                           drum->u_past[x][y+1] + drum->u_past[x][y-1] - 4.0f * drum->u_past[x][y]));
            }
                                        
                              
            drum->u_next[x][y] = (drum->rho * drum->rho) * (neighbors - 4.0f * drum->u_present[x][y]) 
                                 + 2.0f * drum->u_present[x][y] 
                                 - drum->u_past[x][y]
                                 + lossy_stiffness;
                                 
            //używamy naszego przeskalowanego, bezpiecznego tłumienia!
            drum->u_next[x][y] *= real_damping; 
        }
    }   


    //MIKRO
    float out_center = 0.0f;
    float out_side   = 0.0f;
    float out_edge   = 0.0f;

    //wspolrzedne mikro
    int mic1_x = (int)(GRID_SIZE * 0.45f); //środek
    int mic1_y = (int)(GRID_SIZE * 0.55f);

    int mic2_x = (int)(GRID_SIZE * 0.25f); //bok
    int mic2_y = (int)(GRID_SIZE * 0.65f);

    int mic3_x = (int)(GRID_SIZE * 0.80f); //krawedz
    int mic3_y = (int)(GRID_SIZE * 0.40f);

    if (mic1_x > 0 && mic1_x < GRID_SIZE && mic1_y > 0 && mic1_y < GRID_SIZE) 
        out_center = drum->u_next[mic1_x][mic1_y];
        
    if (mic2_x > 0 && mic2_x < GRID_SIZE && mic2_y > 0 && mic2_y < GRID_SIZE) 
        out_side = drum->u_next[mic2_x][mic2_y];
        
    if (mic3_x > 0 && mic3_x < GRID_SIZE && mic3_y > 0 && mic3_y < GRID_SIZE) 
        out_edge = drum->u_next[mic3_x][mic3_y];

    //MIKS MIKRO- PARAMETRY
    float output_sample = (out_center * 0.3f) + (out_side * 0.5f) + (out_edge * 0.2f);

    float c_weight = 0.9998f;
    float n_weight = (1.0f - c_weight) / 4.0f; 
    //przesunięcie czasu 
    for (int x = 1; x < GRID_SIZE - 1; x++) {
        for (int y = 1; y < GRID_SIZE - 1; y++) {
            drum->u_past[x][y] = drum->u_present[x][y];
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