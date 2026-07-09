# Running the four-lepton ntuple workflow

Full, copy-pasteable instructions for producing the ZZ→4ℓ ntuples on **HiPerGator**,
from a fresh checkout through a full-sample SLURM run. For *what* the producer does
(object IDs, event content, Run-3 adaptations) see [`README.md`](README.md).

---

## 0. Get the code

The rdfutil toolkit lives in `code/rdfutil` as a **git submodule**, so clone with
`--recurse-submodules` (or init it afterwards):

```bash
cd /blue/avery/p.chang/work        # or wherever you keep it
git clone --recurse-submodules git@github.com:sgnoohc/bbzz.git
cd bbzz

# if you cloned without --recurse-submodules:
git submodule update --init --recursive
```

---

## 1. Environment (ROOT)

Everything (build + run) needs ROOT from `CMSSW_15_1_0` via CVMFS. `setup.sh` just
sources the rooutil helper:

```bash
source setup.sh          # == source ~/rooutil/bin/setuproot.sh
```

`root-config`, `g++`, and `root` are on `$PATH` afterwards. `run.sh` and the SLURM
job script source this themselves, so you only need it by hand for a bare `make`.

> Do **not** `module load root` — this workflow is pinned to the CMSSW ROOT
> (ROOT 6.32.13). See the `root-setup-hipergator` note.

---

## 2. Build

```bash
source setup.sh
make                     # -> ./create_fourlepton_ntuple
# make clean / make rebuild also available
```

The Makefile compiles every `*.cc` in the top dir with `-Icode/rdfutil` (for
`rdfutil.h`). If `code/rdfutil/rdfutil.h` is missing, your submodule isn't checked
out — see step 0.

---

## 3. Inputs (`mc/` + file lists)

The NanoAODv15 inputs are **not** in git (5 GB). They live under `mc/` on `/blue`
and are fetched with `dl.sh`. If `mc/` is already populated (152 files, ~5 GB),
skip the download.

```bash
# (only if mc/ is empty) download the sample
./dl.sh                  # populates mc/RunIII2024Summer24NanoAODv15/GluGlutoHHto2B2Zto4L.../

# build the input file lists from whatever is in mc/
./make_filelist.sh
#   -> filelists/GluGlutoHHto2B2Zto4L.txt   (full sample, one abs path per line)
#   -> filelists/example.txt                (single reference file)
```

A file list is just one input ROOT path per line; `#` comments and blank lines are
ignored. You can hand-edit these to run on a subset.

---

## 4. Run

`create_fourlepton_ntuple <filelist.txt> <output.root> [nthreads]`

### 4a. Quick smoke test (one file, ~6k events)

```bash
./run.sh                 # build + run example.txt -> output/fourlepton_example.root
```

`run.sh` with no args = `./run.sh filelists/example.txt output/fourlepton_example.root 8`.

### 4b. Full sample, interactively

Only do this on a compute node (grab one with `srun`/`salloc`), not the login node:

```bash
srun --account=avery --qos=avery-b --cpus-per-task=16 --mem=16gb --time=02:00:00 --pty bash
source setup.sh
./run.sh filelists/GluGlutoHHto2B2Zto4L.txt output/fourlepton_full.root 16
```

### 4c. Full sample via SLURM array (recommended)

```bash
./submit.sh <jobname> [filelist] [files_per_job]
# e.g. 152 files, 8 per task -> 19 array tasks:
./submit.sh hh4l filelists/GluGlutoHHto2B2Zto4L.txt 8
```

Per array task: `FILES_PER_JOB` inputs -> one `output/<jobname>/<jobname>_<N>.root`.
Defaults baked into `submit.sh`:

| setting | value |
|---------|-------|
| account / qos    | `avery` / `avery-b` |
| cpus-per-task    | 4 |
| mem              | 8gb |
| time             | 04:00:00 |
| output dir       | `output/<jobname>/` |
| logs             | `logs/<jobname>/slurm_%A_%a.{out,err}` |

Edit those at the top of `submit.sh` if you need more resources. Inputs are local
`/blue` files, so **no grid proxy** is needed.

Monitor and inspect:

```bash
squeue -u $USER
tail -f logs/hh4l/slurm_*_1.out
ls -la output/hh4l/
```

---

## 5. Merge the outputs

After a SLURM run you have one ROOT file per array task. Merge them:

```bash
source setup.sh
hadd -f output/fourlepton_GluGlutoHHto2B2Zto4L.root output/hh4l/hh4l_*.root
```

---

## 6. Sanity check an output

```bash
source setup.sh
root -l -b -q output/fourlepton_example.root -e \
  'Events->Print(); std::cout << "entries = " << Events->GetEntries() << std::endl;'
```

The example file should have **6,330 entries** in the `Events` tree (42 branches).
The ntuple stores the ZZ→4ℓ candidate as `ZZ_*` plus `GoodLepton_*`, `GoodJet_*`
(+ b-tags), `PuppiMET_pt/phi`, `Trig_*`, `genWeight`, `run`, `luminosityBlock`,
`event` — see [`README.md`](README.md#event-content).

---

## Troubleshooting

| symptom | fix |
|---------|-----|
| `rdfutil.h: No such file or directory` | submodule not checked out — `git submodule update --init --recursive` |
| `root-config: command not found` | you forgot `source setup.sh` |
| `Executable not built` from `submit.sh` | run `source setup.sh && make` first |
| empty / tiny output, no errors | your file list points at missing/empty inputs — re-run `./make_filelist.sh` and check `mc/` |
| jobs pending forever | check `avery-b` QoS limits: `squeue -u $USER`, or lower `--mem`/`--time` in `submit.sh` |
