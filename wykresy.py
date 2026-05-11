import matplotlib.pyplot as plt
import numpy as np

# Parametry symulacji
fs = 44100     
duration = 0.5 
t = np.linspace(0, duration, int(fs * duration))

damping_factor = 5.0 
amplitude = np.exp(-damping_factor * t)

brightness = np.exp(-15.0 * t)

plt.figure(figsize=(12, 8))

plt.subplot(2, 1, 1)
plt.plot(t, amplitude, color='blue', linewidth=2)
plt.title('Zanik Amplitudy w czasie (Współczynnik Damping)')
plt.ylabel('Amplituda (Głośność)')
plt.grid(True, linestyle='--')

plt.subplot(2, 1, 2)
plt.plot(t, brightness, color='orange', linewidth=2)
plt.title('Zanik Wysokich Częstotliwości (Działanie Filtru Dolnoprzepustowego)')
plt.xlabel('Czas [s]')
plt.ylabel('Ilość wysokich tonów')
plt.grid(True, linestyle='--')

plt.tight_layout()
plt.show()