#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PI 3.14159265358979323846

const double amplitudes[10] = {100, 2000, 1000, 500, 250, 100, 2000, 1000, 500, 250};
const double frequencies[10] = {0, 31.25, 500, 2000, 4000, 44, 220, 440, 1760, 3960};
const int N = 10;
const int wavetype = 4;

typedef struct {
    char chunkId[4];
    uint32_t chunkSize;
    char format[4];
    char subChunk1Id[4];
    uint32_t subChunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char subChunk2Id[4];
    uint32_t subChunk2Size;
} WavHeader;

void saveWavfile(const char* filename, short* signal, int numofpoints, int sampleRate, int bitsperpoint){
    WavHeader header;
    
    strncpy(header.chunkId, "RIFF", 4);
    header.chunkSize = 36 + numofpoints * (bitsperpoint / 8);
    strncpy(header.format, "WAVE", 4);
    
    strncpy(header.subChunk1Id, "fmt ", 4);
    header.subChunk1Size = 16;
    header.audioFormat = 1;
    header.numChannels = 1;
    header.sampleRate = sampleRate;
    header.bitsPerSample = bitsperpoint;
    header.byteRate = sampleRate * (bitsperpoint / 8);
    header.blockAlign = (bitsperpoint / 8);
    strncpy(header.subChunk2Id, "data", 4);
    header.subChunk2Size = numofpoints * (bitsperpoint / 8);
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open file");
        return;
    }

    fwrite(&header, sizeof(WavHeader), 1, file);
    fwrite(signal, sizeof(int16_t), numofpoints, file);
    fclose(file);
}

void generateWaveform(short *audioBuffer, int sampleRate, double T, int waveType, double frequency, double amplitude) {
    int numSamples = sampleRate * T;

    for (int i = 0; i < numSamples; i++) {
        double t = (double)i / sampleRate;
        double value = 0;

        double u1 = (t > 0) ? 1 : 0;
        double u2 = (t - 0.1 > 0) ? 1 : 0;
        double s = 0;

        if (waveType == 0)
            s = sin(2 * PI * frequency * t);
        else if (waveType == 1)
            s = frequency * t - floor(frequency * t);
        else if (waveType == 2)
            s = (sin(2 * PI * frequency * t) > 0) ? 1 : -1;
        else if (waveType == 3)
            s = 2 * fabs(2 * (frequency * t - floor(frequency * t + 0.5))) - 1;

        value = amplitude * (u1 - u2) * s;
        audioBuffer[i] = (short)value;
    }
}

int main() {
    double T = 0.1;
    int sampleRates[2] = {8000, 16000};

    FILE *scpFile = fopen("waveforms.scp", "w");
    if (!scpFile) {
        perror("Failed to create SCP file");
        return 1;
    }

    for (int srIndex = 0; srIndex < 2; srIndex++) {
        int sampleRate = sampleRates[srIndex];
        int numSamples = (int)(sampleRate * T);
        short *audioBuffer = (short *)malloc(numSamples * sizeof(short));

        for (int waveType = 0; waveType < wavetype; waveType++) {
            for (int freqIndex = 0; freqIndex < N; freqIndex++) {
                char waveTypeName[10];
                if (waveType == 0) strncpy(waveTypeName, "sine", 10);
                else if (waveType == 1) strncpy(waveTypeName, "sawtooth", 10);
                else if (waveType == 2) strncpy(waveTypeName, "square", 10);
                else if (waveType == 3) strncpy(waveTypeName, "triangle", 10);

                char filename[50];
                snprintf(filename, sizeof(filename), "waveform_%s_freq_%d_%dk.wav", waveTypeName, (int)frequencies[freqIndex], sampleRate / 1000);
				//生成檔案名可用snprintf 
                generateWaveform(audioBuffer, sampleRate, T, waveType, frequencies[freqIndex], amplitudes[freqIndex]);
                saveWavfile(filename, audioBuffer, numSamples, sampleRate, 16);

                fprintf(scpFile, "%s label_waveType%d_freq%d_sr%d\n", filename, waveType, (int)frequencies[freqIndex], sampleRate);
            }
        }

        free(audioBuffer);
    }

    fclose(scpFile);
    return 0;
}

