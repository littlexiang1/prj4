#!/bin/bash

set -e  # 如果任何命令出錯，腳本將停止執行

log_file="script_debug.log"
exec > >(tee -a $log_file) 2>&1  # 將所有輸出重定向到日誌文件

echo "=== STARTING SCRIPT ==="

# Variables
WAV_SCP_FILE="waveforms.scp"
OUTPUT_WAV_8K="s-8k.wav"
OUTPUT_WAV_16K="s-16k.wav"
AEUEO_8K="aeueo-8kHz.wav"
AEUEO_16K="aeueo-16kHz.wav"
SAMPLE_RATE_8K=8000
SAMPLE_RATE_16K=16000

# Step 1: Compile C programs
echo "[INFO] Compiling C programs..."
gcc -o sinegen sinegen.c -lm
gcc -o cascade cascade.c -lm
gcc -o spectrogram spectrogram.c -lm
echo "[INFO] Compilation completed successfully."

# Step 2: Generate 80 waveforms using sinegen
echo "[INFO] Generating waveforms..."
./sinegen
echo "[INFO] Waveform generation completed."

# Step 3: Cascade waveforms
echo "[INFO] Cascading waveforms..."
./cascade $WAV_SCP_FILE $OUTPUT_WAV_8K $SAMPLE_RATE_8K
./cascade $WAV_SCP_FILE $OUTPUT_WAV_16K $SAMPLE_RATE_16K
echo "[INFO] Cascading completed."

# Step 4: Generate spectrogram data
echo "[INFO] Generating spectrogram data..."
SPECTROGRAM_PARAMS=(
    "$OUTPUT_WAV_8K 0.032 0.032 0 0.01 s-8k.Set1.txt"
    "$OUTPUT_WAV_8K 0.032 0.032 1 0.01 s-8k.Set2.txt"
    "$OUTPUT_WAV_8K 0.030 0.032 0 0.01 s-8k.Set3.txt"
    "$OUTPUT_WAV_8K 0.030 0.032 1 0.01 s-8k.Set4.txt"
    "$OUTPUT_WAV_16K 0.032 0.032 0 0.01 s-16k.Set1.txt"
    "$OUTPUT_WAV_16K 0.032 0.032 1 0.01 s-16k.Set2.txt"
    "$OUTPUT_WAV_16K 0.030 0.032 0 0.01 s-16k.Set3.txt"
    "$OUTPUT_WAV_16K 0.030 0.032 1 0.01 s-16k.Set4.txt"
    "$AEUEO_8K 0.032 0.032 0 0.01 aeueo-8kHz.Set1.txt"
    "$AEUEO_8K 0.032 0.032 1 0.01 aeueo-8kHz.Set2.txt"
    "$AEUEO_8K 0.030 0.032 0 0.01 aeueo-8kHz.Set3.txt"
    "$AEUEO_8K 0.030 0.032 1 0.01 aeueo-8kHz.Set4.txt"
    "$AEUEO_16K 0.032 0.032 0 0.01 aeueo-16kHz.Set1.txt"
    "$AEUEO_16K 0.032 0.032 1 0.01 aeueo-16kHz.Set2.txt"
    "$AEUEO_16K 0.030 0.032 0 0.01 aeueo-16kHz.Set3.txt"
    "$AEUEO_16K 0.030 0.032 1 0.01 aeueo-16kHz.Set4.txt"
)
for param in "${SPECTROGRAM_PARAMS[@]}"; do
    echo "[INFO] Running spectrogram with parameters: $param"
    ./spectrogram $param
done
echo "[INFO] Spectrogram data generation completed."

# Step 5: Generate PDF visualizations
echo "[INFO] Generating PDF visualizations..."
for txt_file in *.txt; do
    base_name=$(basename "$txt_file" .txt)
    wav_file="${base_name%.*}.wav"
    pdf_file="${base_name}.pdf"
    echo "[INFO] Generating PDF for $txt_file..."
    python spectshow.py "$wav_file" "$txt_file" "$pdf_file"
done
echo "[INFO] PDF generation completed."

echo "=== SCRIPT FINISHED SUCCESSFULLY ==="


