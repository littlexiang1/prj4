import sys
import matplotlib.pyplot as plt
import numpy as np
import wave

def read_wave_file(filename):
    with wave.open(filename, 'r') as wav_file:
        n_frames = wav_file.getnframes()
        frame_rate = wav_file.getframerate()
        signal = np.frombuffer(wav_file.readframes(n_frames), dtype=np.int16)
        time = np.linspace(0, len(signal) / frame_rate, num=len(signal))
    return time, signal

def read_spectrogram_data(filename):
    data = []
    with open(filename, 'r') as file:
        for line in file:
            row = list(map(float, line.strip().split()))
            data.append(row)
    return np.array(data)

def plot_waveform_and_spectrogram(wave_time, wave_signal, spec_data, output_pdf):
    plt.figure(figsize=(10, 8))

    # Plot waveform
    plt.subplot(2, 1, 1)
    plt.plot(wave_time, wave_signal, color='blue')
    plt.title('Waveform')
    plt.xlabel('Time')
    plt.ylabel('Amplitude')

    # Plot spectrogram
    plt.subplot(2, 1, 2)
    plt.imshow(
        spec_data.T, aspect='auto', origin='lower',
        interpolation='nearest', cmap='jet'
    )
    plt.title('Spectrogram')
    plt.xlabel('Frame Index')
    plt.ylabel('Frequency Bin Index')
    plt.colorbar(label='Magnitude (dB)')

    plt.tight_layout()
    plt.savefig(output_pdf)
    plt.close()

def main():
    if len(sys.argv) != 4:
        print("Usage: python3 spectshow.py <in_wav> <in_txt> <out_pdf>")
        sys.exit(1)

    in_wav = sys.argv[1]
    in_txt = sys.argv[2]
    out_pdf = sys.argv[3]

    # Read wave file
    wave_time, wave_signal = read_wave_file(in_wav)

    # Read spectrogram data
    spec_data = read_spectrogram_data(in_txt)

    # Plot and save the waveform and spectrogram
    plot_waveform_and_spectrogram(wave_time, wave_signal, spec_data, out_pdf)

if __name__ == "__main__":
    main()

