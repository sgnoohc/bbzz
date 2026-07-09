#!/bin/bash
# ============================================================================
# SLURM array submission for the four-lepton ntuple workflow.
#   ./submit.sh <jobname> [filelist] [files_per_job]
# e.g.
#   ./submit.sh hh4l filelists/GluGlutoHHto2B2Zto4L.txt 8
#
# Each array task processes FILES_PER_JOB input files and writes one output
# ntuple to OUTDIR. Inputs here are local /blue files, so no grid proxy is
# needed (unlike xrootd/root:// inputs).
# ============================================================================
if [ -z "$1" ]; then
    echo "Usage: $0 <jobname> [filelist] [files_per_job]"
    exit 1
fi

WD="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$WD"

JOBNAME="$1"
FILELIST="${2:-filelists/GluGlutoHHto2B2Zto4L.txt}"
FILES_PER_JOB="${3:-8}"

OUTDIR="$WD/output/${JOBNAME}"
EXECUTABLE="$WD/create_fourlepton_ntuple"
CPUS_PER_TASK=4
MEM="8gb"
TIME="04:00:00"
ACCOUNT="avery"
QOS="avery-b"

if [ ! -x "$EXECUTABLE" ]; then
    echo "Executable not built. Run:  source setup.sh && make"
    exit 1
fi

# Clean file list (strip comments / blanks) on the shared filesystem
CLEAN_FILELIST="$WD/.filelist_clean.txt"
grep -vE '^\s*#|^\s*$' "$FILELIST" > "$CLEAN_FILELIST"
TOTAL_FILES=$(wc -l < "$CLEAN_FILELIST")
if [ "$TOTAL_FILES" -eq 0 ]; then
    echo "No files found in $FILELIST"
    exit 1
fi
NJOBS=$(( (TOTAL_FILES + FILES_PER_JOB - 1) / FILES_PER_JOB ))

echo "Total files:   $TOTAL_FILES"
echo "Files per job: $FILES_PER_JOB"
echo "Number of jobs:$NJOBS"
echo "Output dir:    $OUTDIR"

mkdir -p "$OUTDIR" "logs/${JOBNAME}"

sbatch <<EOF
#!/bin/bash
#SBATCH --job-name=${JOBNAME}
#SBATCH --account=${ACCOUNT}
#SBATCH --qos=${QOS}
#SBATCH --array=1-${NJOBS}
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=${CPUS_PER_TASK}
#SBATCH --mem=${MEM}
#SBATCH --time=${TIME}
#SBATCH --output=${WD}/logs/${JOBNAME}/slurm_%A_%a.out
#SBATCH --error=${WD}/logs/${JOBNAME}/slurm_%A_%a.err

source ~/rooutil/bin/setuproot.sh

# Slice this task's files out of the clean list
START=\$(( (\${SLURM_ARRAY_TASK_ID} - 1) * ${FILES_PER_JOB} + 1 ))
END=\$((   \${SLURM_ARRAY_TASK_ID}      * ${FILES_PER_JOB} ))
TASK_LIST="${OUTDIR}/files_\${SLURM_ARRAY_TASK_ID}.txt"
sed -n "\${START},\${END}p" "${CLEAN_FILELIST}" > "\$TASK_LIST"

OUT="${OUTDIR}/${JOBNAME}_\${SLURM_ARRAY_TASK_ID}.root"
echo "Task \${SLURM_ARRAY_TASK_ID}: \$(wc -l < \$TASK_LIST) files -> \$OUT"
${EXECUTABLE} "\$TASK_LIST" "\$OUT" ${CPUS_PER_TASK}
EOF
