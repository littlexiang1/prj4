#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define BUFFER_SIZE 4096

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

void appendWavFile(const char* inputFile, FILE* outputFile, int* totalSamples, uint32_t targetSampleRate) {
    FILE* input = fopen(inputFile, "rb");
    if (!input) {
        perror("Failed to open input WAV file");
        return;
    }

    WavHeader header;
    fread(&header, sizeof(WavHeader), 1, input);

    if (header.sampleRate != targetSampleRate) {
        printf("Skipping file: %s (Sample rate mismatch: %u != %u)\n", inputFile, header.sampleRate, targetSampleRate);
        fclose(input);
        return;
    }

    int dataSize = header.subChunk2Size / (header.bitsPerSample / 8);
    int16_t* buffer = (int16_t*)malloc(dataSize * sizeof(int16_t));

    fread(buffer, sizeof(int16_t), dataSize, input);
    fwrite(buffer, sizeof(int16_t), dataSize, outputFile);

    *totalSamples += dataSize;

    printf("File: %s, Samples: %d, Duration: %.2f seconds\n", inputFile, dataSize, (float)dataSize / header.sampleRate);

    free(buffer);
    fclose(input);
}

void createCombinedWav(const char* scpFile, const char* outputFile, uint32_t sampleRate) {
    FILE* scp = fopen(scpFile, "r");
    if (!scp) {
        perror("Failed to open SCP file");
        return;
    }

    FILE* output = fopen(outputFile, "wb");
    if (!output) {
        perror("Failed to open output WAV file");
        fclose(scp);
        return;
    }

    WavHeader outputHeader = {
        .chunkId = "RIFF",
        .format = "WAVE",
        .subChunk1Id = "fmt ",
        .subChunk1Size = 16,
        .audioFormat = 1,
        .numChannels = 1,
        .sampleRate = sampleRate,
        .byteRate = sampleRate * 2,
        .blockAlign = 2,
        .bitsPerSample = 16,
        .subChunk2Id = "data",
        .subChunk2Size = 0 // will update later
    };

    fwrite(&outputHeader, sizeof(WavHeader), 1, output);

    char line[BUFFER_SIZE]; //存放從scpFile中讀取的每行檔案名稱
    int totalSamples = 0;

    while (fgets(line, BUFFER_SIZE, scp)) {
        line[strcspn(line, "\n")] = 0; // Remove newline character
        char* fileName = strtok(line, " "); // Extract the file name before the space
        if (fileName) {
            appendWavFile(fileName, output, &totalSamples, sampleRate);
        }
    }

    outputHeader.subChunk2Size = totalSamples * 2;
    outputHeader.chunkSize = 36 + outputHeader.subChunk2Size;

    printf("Total samples: %d, Total duration: %.2f seconds\n", totalSamples, (float)totalSamples / sampleRate);

    fseek(output, 0, SEEK_SET);
    fwrite(&outputHeader, sizeof(WavHeader), 1, output);

    fclose(scp);
    fclose(output);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <scp file> <output file> <sample rate>\n", argv[0]);
        return 1;
    }

    const char* scpFile = argv[1];
    const char* outputFile = argv[2];
    uint32_t sampleRate = atoi(argv[3]);

    createCombinedWav(scpFile, outputFile, sampleRate);

    return 0;
}

