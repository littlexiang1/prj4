#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#define PI 3.14159265358979323846

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
} WaveHeader;

void rectangularWnd(float *window, size_t windowlength) {
    for (size_t i = 0; i < windowlength; i++) {
        window[i] = 1.0;
    }
}

void HammingWnd(float *window, size_t length) {
    for (size_t i = 0; i < length; i++) {
        window[i] = 0.54 - 0.46 * cos(2 * PI * i / (length - 1));
    }
}

void record_sine(int N, float **sine) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float rad = 2.0 * PI / N * i * j;
            sine[i][j] = sin(rad);
        }
    }
}

void record_cosine(int N, float **cosine) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            float rad = 2.0 * PI / N * i * j;
            cosine[i][j] = cos(rad);
        }
    }
}

void dft(int N, float *x_realpart, float *x_imaginarypart, float *X_realpart, float *X_imaginarypart, float **cos, float **sin) {
    for (int i = 0; i < N; i++) {
        X_realpart[i] = 0;
        X_imaginarypart[i] = 0;
        for (int j = 0; j < N; j++) {
            X_realpart[i] += x_realpart[j] * cos[i][j] - x_imaginarypart[j] * sin[i][j];
            X_imaginarypart[i] += x_imaginarypart[j] * cos[i][j] + x_realpart[j] * sin[i][j];
        }
    }
}

int data_size(FILE *fptr) {
    fseek(fptr, 44, SEEK_SET);
    int start = ftell(fptr);
    fseek(fptr, 0, SEEK_END);
    int end = ftell(fptr);
    int dataSize = end - start;
    fseek(fptr, 0, SEEK_SET);
    return dataSize;
}

void combineBytesToShort(char *c1, char *c2, short *output) {
    unsigned char uc1 = *c1, uc2 = *c2;
    *output = (uc1 << 8) | uc2;
}

void read_wav(FILE *fptr, short *sdata, int dataSize) {
    fseek(fptr, 44, SEEK_SET);
    char *audiobuffer = (char *)malloc(dataSize * sizeof(char));
    fread(audiobuffer, sizeof(char), dataSize, fptr);
    for (int i = 0, n = 0; i < dataSize; i += 2, n++) {
        combineBytesToShort(audiobuffer + i + 1, audiobuffer + i, sdata + n);
    }
    free(audiobuffer);
}

void spectrogram(short *wave_val, int samplerate, float analysis_window_size, float DFT_window_size, int windowtype, float frame_interval, int sample_amount, char filename[]) {
    size_t DFT_window_num = (size_t)(samplerate * DFT_window_size);
    size_t analysis_window_num = (size_t)(samplerate * analysis_window_size);

    float *wnd = (float *)calloc(analysis_window_num, sizeof(float));
    if (windowtype == 0) {
        rectangularWnd(wnd, analysis_window_num);
    } else if (windowtype == 1) {
        HammingWnd(wnd, analysis_window_num);
    }

    int frame_num = (int)(frame_interval * samplerate);
    int frame_amount = sample_amount / frame_num;
    float *frame_buff = (float *)calloc(DFT_window_num, sizeof(float));
    float *x_im = (float *)calloc(DFT_window_num, sizeof(float));
    float *X_re = (float *)calloc(DFT_window_num, sizeof(float));
    float *X_im = (float *)calloc(DFT_window_num, sizeof(float));
    float *Magnitude = (float *)calloc(DFT_window_num, sizeof(float));
    float **sine = (float **)calloc(DFT_window_num, sizeof(float *));
    float **cosine = (float **)calloc(DFT_window_num, sizeof(float *));

    for (size_t i = 0; i < DFT_window_num; i++) {
        sine[i] = (float *)calloc(DFT_window_num, sizeof(float));
        cosine[i] = (float *)calloc(DFT_window_num, sizeof(float));
    }
    record_sine(DFT_window_num, sine);
    record_cosine(DFT_window_num, cosine);

    FILE *fp_save = fopen(filename, "w");
    for (int i = 0; i < frame_amount; i++) {
        for (size_t n = 0; n < analysis_window_num; n++) {
            if (i * frame_num + n < sample_amount) {
                frame_buff[n] = wave_val[i * frame_num + n] * wnd[n];
            } else {
                frame_buff[n] = 0;
            }
        }
        dft(DFT_window_num, frame_buff, x_im, X_re, X_im, cosine, sine);
        for (size_t n = 0; n <= analysis_window_num / 2; n++) {
            float tmp = (X_re[n] * X_re[n] + X_im[n] * X_im[n]);
            if (tmp > 0) {
                Magnitude[n] = 10 * log10(tmp);
            } else {
                Magnitude[n] = -100;
            }
            fprintf(fp_save, "%.10f\t", Magnitude[n]);
        }
        fprintf(fp_save, "\n");
    }

    fclose(fp_save);
    free(wnd);
    for (size_t i = 0; i < DFT_window_num; i++) {
        free(sine[i]);
        free(cosine[i]);
    }
    free(sine);
    free(cosine);
    free(frame_buff);
    free(x_im);
    free(X_re);
    free(X_im);
    free(Magnitude);
}

int main() {
    int fs_8k = 8000;
    int fs_16k = 16000;

    char filenames[16][30] = {"s-8k.Set1.txt", "s-8k.Set2.txt", "s-8k.Set3.txt", "s-8k.Set4.txt",
                              "s-16k.Set1.txt", "s-16k.Set2.txt", "s-16k.Set3.txt", "s-16k.Set4.txt",
                              "aeueo-8kHz.Set1.txt", "aeueo-8kHz.Set2.txt", "aeueo-8kHz.Set3.txt", "aeueo-8kHz.Set4.txt",
                              "aeueo-16kHz.Set1.txt", "aeueo-16kHz.Set2.txt", "aeueo-16kHz.Set3.txt", "aeueo-16kHz.Set4.txt"};

    FILE *fptr_8k = fopen("s-8k.wav", "rb");
    if (!fptr_8k) {
        perror("Failed to open s-8k.wav");
        exit(EXIT_FAILURE);
    }
    int dataSize8k = data_size(fptr_8k);
    short *data_8k = (short *)malloc(dataSize8k / 2 * sizeof(short));
    read_wav(fptr_8k, data_8k, dataSize8k);
    spectrogram(data_8k, fs_8k, 0.032, 0.032, 0, 0.01, dataSize8k / 2, filenames[0]);
    spectrogram(data_8k, fs_8k, 0.032, 0.032, 1, 0.01, dataSize8k / 2, filenames[1]);
    spectrogram(data_8k, fs_8k, 0.030, 0.032, 0, 0.01, dataSize8k / 2, filenames[2]);
    spectrogram(data_8k, fs_8k, 0.030, 0.032, 1, 0.01, dataSize8k / 2, filenames[3]);
    free(data_8k);

    FILE *fptr_16k = fopen("s-16k.wav", "rb");
    if (!fptr_16k) {
        perror("Failed to open s-16k.wav");
        exit(EXIT_FAILURE);
    }
    int dataSize16k = data_size(fptr_16k);
    short *data_16k = (short *)malloc(dataSize16k / 2 * sizeof(short));
    read_wav(fptr_16k, data_16k, dataSize16k);
    spectrogram(data_16k, fs_16k, 0.032, 0.032, 0, 0.01, dataSize16k / 2, filenames[4]);
    spectrogram(data_16k, fs_16k, 0.032, 0.032, 1, 0.01, dataSize16k / 2, filenames[5]);
    spectrogram(data_16k, fs_16k, 0.030, 0.032, 0, 0.01, dataSize16k / 2, filenames[6]);
    spectrogram(data_16k, fs_16k, 0.030, 0.032, 1, 0.01, dataSize16k / 2, filenames[7]);
    free(data_16k);

    FILE *fptr_8k_aeueo = fopen("aeueo-8kHz.wav", "rb");
    if (!fptr_8k_aeueo) {
        perror("Failed to open aeueo-8kHz.wav");
        exit(EXIT_FAILURE);
    }
    int dataSize8k_aeueo = data_size(fptr_8k_aeueo);
    short *data_8k_aeueo = (short *)malloc(dataSize8k_aeueo / 2 * sizeof(short));
    read_wav(fptr_8k_aeueo, data_8k_aeueo, dataSize8k_aeueo);
    spectrogram(data_8k_aeueo, fs_8k, 0.032, 0.032, 0, 0.01, dataSize8k_aeueo / 2, filenames[8]);
    spectrogram(data_8k_aeueo, fs_8k, 0.032, 0.032, 1, 0.01, dataSize8k_aeueo / 2, filenames[9]);
    spectrogram(data_8k_aeueo, fs_8k, 0.030, 0.032, 0, 0.01, dataSize8k_aeueo / 2, filenames[10]);
    spectrogram(data_8k_aeueo, fs_8k, 0.030, 0.032, 1, 0.01, dataSize8k_aeueo / 2, filenames[11]);
    free(data_8k_aeueo);

    FILE *fptr_16k_aeueo = fopen("aeueo-16kHz.wav", "rb");
    if (!fptr_16k_aeueo) {
        perror("Failed to open aeueo-16kHz.wav");
        exit(EXIT_FAILURE);
    }
    int dataSize16k_aeueo = data_size(fptr_16k_aeueo);
    short *data_16k_aeueo = (short *)malloc(dataSize16k_aeueo / 2 * sizeof(short));
    read_wav(fptr_16k_aeueo, data_16k_aeueo, dataSize16k_aeueo);
    spectrogram(data_16k_aeueo, fs_16k, 0.032, 0.032, 0, 0.01, dataSize16k_aeueo / 2, filenames[12]);
    spectrogram(data_16k_aeueo, fs_16k, 0.032, 0.032, 1, 0.01, dataSize16k_aeueo / 2, filenames[13]);
    spectrogram(data_16k_aeueo, fs_16k, 0.030, 0.032, 0, 0.01, dataSize16k_aeueo / 2, filenames[14]);
    spectrogram(data_16k_aeueo, fs_16k, 0.030, 0.032, 1, 0.01, dataSize16k_aeueo / 2, filenames[15]);
    free(data_16k_aeueo);

    return 0;
}

