#!/bin/bash
set -u
# set -e

echo "Project 2 $0 start consistency test"

# Color variables
red='\033[0;31m'
green='\033[0;32m'
yellow='\033[0;33m'
blue='\033[0;34m'
clear='\033[0m'

if [ $# -lt 2 ]; then
    echo -e "${red}Incorrect args${clear}"
    echo -e "${red}[error]${clear} Usage: $0 input_file output_file"
    exit 1
fi

# Define variables
input_file=$1
output_file=$2

# Number of times to check the output
loop=100
time=5
executable=proj2

# Print script settings
echo "[debug] Selected input file: $input_file, expected output file: $output_file"
echo "[debug] Loop and compare: $loop times per file"
echo "[debug] Timeout: $time seconds per loop"

# Build Project just in case 

echo "[info] Building project again"
rm -f $executable
make clean
make

# Generate output and compare
echo "[info] Generate output, sort, then diff..."

file_errors=0

# Loop multiple times to check deadlock
loop_errors=0
for ((j = 0; j < $loop; ++j)); do
    # Generate output file
    timeout $time ./$executable $input_file
    if [ $? -ne 0 ]; then
        echo -e "${red}[error] $input_file: generate error: process failed or deadlock${clear}"
        loop_errors=$((loop_errors + 1))
        file_errors=$((file_errors + 1))
        break
    fi

    # Compare using diff
    out_text=$(diff <(sort output/gantt-$(basename $input_file)) <(sort $output_file))
    if [ -n "$out_text" ]; then
        echo -e "${red}[error] $input_file: diff error:${clear} incorrect output"
        echo "$out_text"
        loop_errors=$((loop_errors + 1))
        file_errors=$((file_errors + 1))
        break
    fi
done
if [ $loop_errors -eq 0 ]; then
    echo "$input_file: no error"
fi

# Print pass/fail for this file
if [ $file_errors -eq 0 ]; then
    echo -e "${green}[success] $input_file: OK${clear} (no errors for this input file)"
else
    echo -e "${red}[error]$input_file: Fail${clear} (one or more errors for this input file)"
fi