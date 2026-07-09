#!/bin/bash
# Build and run the four-lepton ntuple producer.
#
#   ./run.sh                              # example single file -> output/fourlepton_example.root
#   ./run.sh <filelist> <output> [nthr]   # custom run
#
# e.g. full sample:
#   ./run.sh filelists/GluGlutoHHto2B2Zto4L.txt output/fourlepton_GluGlutoHHto2B2Zto4L.root 16
WD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$WD"

# Set up ROOT (must be sourced before enabling -e; scram touches unset vars)
source ~/rooutil/bin/setuproot.sh

set -e
make

LIST="${1:-filelists/example.txt}"
OUT="${2:-output/fourlepton_example.root}"
NTHREADS="${3:-8}"

mkdir -p "$(dirname "$OUT")"
echo "=== running: $LIST -> $OUT ($NTHREADS threads) ==="
./create_fourlepton_ntuple "$LIST" "$OUT" "$NTHREADS"
