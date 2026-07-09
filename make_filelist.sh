#!/bin/bash
# Generate input file lists (absolute paths) from the local mc/ tree.
#   ./make_filelist.sh
set -e
WD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$WD"
mkdir -p filelists

SAMPLE=GluGlutoHHto2B2Zto4L

# Full sample list
find "$WD/mc" -name '*.root' | sort > "filelists/${SAMPLE}.txt"
echo "wrote filelists/${SAMPLE}.txt ($(wc -l < filelists/${SAMPLE}.txt) files)"

# Single-file example list (the reference file from the task)
EX="$WD/mc/RunIII2024Summer24NanoAODv15/GluGlutoHHto2B2Zto4L_Par-c2-0p00-kl-1p00-kt-1p00_TuneCP5_13p6TeV_powheg-pythia8/NANOAODSIM/150X_mcRun3_2024_realistic_v2-v2/2810000/761240ac-ebf7-4510-a089-8549c7beb8aa.root"
echo "$EX" > filelists/example.txt
echo "wrote filelists/example.txt (1 file)"
