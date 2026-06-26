# Physical Modeling Synthesizer in C

![C](https://img.shields.io/badge/Language-C-blue.svg)
![DSP](https://img.shields.io/badge/Domain-Digital_Signal_Processing-orange.svg)
![Dependencies](https://img.shields.io/badge/Dependencies-None-brightgreen.svg)

A multi-model, polyphonic digital synthesizer built entirely in **pure C from scratch** (no external audio libraries). This project simulates the physical behavior of acoustic waves inside string and percussion instruments using the **Karplus-Strong algorithm** and **2D FDTD (Finite-Difference Time-Domain)** methods.

The engine handles everything from raw memory management and DSP filtering to custom binary encoding of the final `.wav` output file.

## Audio Demos
* [Listen to the 2D FDTD Drum](examples/drum.wav)
* [Listen to the Karplus-Strong Acoustic Guitar](examples/CEG_guitar.wav)

## Key Features & DSP Architecture

This engine goes far beyond simple delays, implementing complex acoustic physics phenomena:

### 1. Karplus-Strong Engine (Plucked Strings)
* **Custom Circular Buffers:** Raw pointer management for optimal latency and frequency control.
* **Low-Pass Filtering:** Running average filters with adjustable damping coefficients (simulating material stiffness, e.g., steel vs. nylon).
* **Random Phase Inversion:** Generates inharmonic, metallic noise for cymbals and hi-hats.

### 2. Advanced Piano Mode (Struck Strings)
**This mode was experimental and because od various of reasons didn't quite work.**
* **Commuted Synthesis:** Simulates the felt hammer strike using a heavily sculpted impulse (Thump + Bite) instead of basic white noise.
* **Fractional Delay & Linear Interpolation:** Solves digital tuning limitations for high frequencies, ensuring perfect pitch.
* **All-Pass Filters (Inharmonicity):** Introduces non-linear phase shifts to simulate the physical stiffness of thick piano wire.
* **Sympathetic Resonance (Double-Decay):** Supports multiple slightly detuned strings per key (chorus effect) running through a master Schroeder Reverb algorithm (4x parallel comb filters).

### 3. 2D FDTD Wave Propagation (Percussion)
* **Real-time 2D Mesh:** Solves the 2D wave equation on a discrete 120x120 grid.
* **Courant Number ($\rho$) & Lossy Stiffness:** accurately simulates membrane tension, wave propagation speed, and internal friction.
* **Spatial Excitation:** Simulates a soft felt mallet striking a 7x7 area to prevent numerical dispersion.
* **Virtual Multi-Microphone Array:** Reads audio from 3 different spatial points (Center, Side, Edge) to prevent phase cancellation and capture rich harmonics.

### 4. Custom Polyphonic Mixer & Binary I/O
* **DSP Mixer:** Sums parallel delay lines with automatic normalization to prevent integer overflow and hard clipping.
* **Zero Dependencies:** Custom `wav.c` module manually constructs the 44-byte RIFF/WAVE header and serializes 16-bit PCM float data directly to the hard drive.

## How to Build and Run
Since the project uses no external libraries, compilation is straightforward.

**1. Clone the repository:**
```bash
git clone [https://github.com/PatrykBlacha/C-Physical-Modeling-Synth.git](https://github.com/PatrykBlacha/C-Physical-Modeling-Synth.git)
cd C-Physical-Modeling-Synth
```
**2. Compile using GCC:**
```bash
gcc -o main main.c core.c wav.c synth.c fdtd_drum.c -lm
```
**3. Run the synthesizer:**
```bash
./main
```
The program will prompt you for an input configuration file. You can find ready-to-use examples in the /examples folder.
```
=== SEKWEKCER KARPLUS-STRONG ===
Podaj nazwe pliku z parametrami: examples_inputs/piano_chord.txt
```

## Input file format (.txt)
The synthesizer acts as an offline sequencer driven by text files.

**Header:** [Duration_Seconds] [Number_Of_Voices]
Instrument Rows:

* Modes 0-4 (Karplus-Strong): [Mode] [Frequency_Hz] [Damping] [Alpha_Filter] [Start_Time]

* Modes 5-7 (FDTD 2D): [Mode] [Tension_Rho] [Damping] [Alpha_Filter] [Start_Time]

Example (A4 note on acoustic guitar):
```
3.0 1
0 440.0 0.995 0.95 0.0
```

## Instrument modes
* `0` - Acoustic String (Standard KS)

* `1` - Kick Drum (Heavy LP filter)

* `2` - Snare (HPF filter)

* `3` - Hi-Hat (Phase modulation)

* `4` - Piano (Commuted Synthesis & All-Pass)

* `5` - FDTD Gong (Square boundary)

* `6` - FDTD Metallic (Circular boundary, hard attack)

* `7` - FDTD Drum (Circular boundary, soft attack, Helmholtz body filter)

## About
This project was developed as part of the "Complex Digital Systems" (Złożone Systemy Cyfrowe) coursework to explore the boundaries of computational acoustics and low-level DSP in C.

**Author:** [Patryk Blacha](https://github.com/PatrykBlacha)