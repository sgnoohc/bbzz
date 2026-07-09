# Four-lepton ntuple workflow (HH → bb ZZ → bb 4ℓ)

Produces flat four-lepton (ZZ → 4ℓ) ntuples from **RunIII2024Summer24 NanoAODv15**,
starting from the [`rdfutil`](https://github.com/sgnoohc/rdfutil) RDataFrame toolkit.

Reference sample:
`GluGlutoHHto2B2Zto4L_Par-c2-0p00-kl-1p00-kt-1p00_TuneCP5_13p6TeV_powheg-pythia8`
(NANOAODSIM, `150X_mcRun3_2024_realistic_v2-v2`).

## Layout

| path | what |
|------|------|
| `create_fourlepton_ntuple.cc` | the producer (Run-3 object IDs + ZZ→4ℓ reco) |
| `Makefile`                    | builds it (`-Icode/rdfutil` for the toolkit header) |
| `setup.sh`                    | `source` it to get ROOT (CMSSW_15_1_0 via CVMFS) |
| `make_filelist.sh`            | build input lists from `mc/` |
| `run.sh`                      | build + run (example single file by default) |
| `submit.sh`                   | SLURM array submission over a full file list |
| `code/rdfutil/`               | the rdfutil toolkit clone (source of `rdfutil.h`) |
| `mc/`                         | local NanoAOD inputs (populated by `dl.sh`) |
| `filelists/`, `output/`       | input lists / output ntuples |

## Quick start

```bash
source setup.sh          # ROOT from CMSSW_15_1_0
./make_filelist.sh       # -> filelists/GluGlutoHHto2B2Zto4L.txt + filelists/example.txt
./run.sh                 # build + run on the example file -> output/fourlepton_example.root
# full sample:
./run.sh filelists/GluGlutoHHto2B2Zto4L.txt output/fourlepton_full.root 16
# batch (SLURM):
./submit.sh hh4l filelists/GluGlutoHHto2B2Zto4L.txt 8
```

`create_fourlepton_ntuple <filelist.txt> <output.root> [nthreads]` —
`filelist.txt` is one input path per line (`#` comments / blank lines ignored).

## Run-3 (NanoAODv15) adaptations vs. the Run-2 template

The stock `rdfutil` template uses UL-2018 branches; these no longer exist in
NanoAODv15, so the producer re-implements the object IDs:

- **Electrons** — no `Electron_mvaFall17V2*`. Uses `Electron_mvaIso_WP90/80`
  (+ `Electron_cutBased` for the loose tier), with HZZ-4ℓ preselection
  (pt>7, |η_SC|<2.5, |dxy|<0.5, |dz|<1.0, |SIP3D|<4).
- **Muons** — HZZ-4ℓ selection (pt>5, |η|<2.4, |dxy|<0.5, |dz|<1.0, |SIP3D|<4,
  looseId→loose / mediumId+iso→analysis).
- **Jets** — `Jet_jetId` was removed; the Run-3 **Tight** PF jet ID is recomputed
  from energy fractions/multiplicities (central region, |η|<2.5). PNet + DeepJet
  b-tags are kept for the bb pair.
- **MET** — no `MET` collection; **PuppiMET** is stored.
- **Triggers** — dilepton / trilepton / single-lepton OR flags (`Trig_*`), built
  only from HLT paths that exist in the file (era-safe).

## Event content

Selection: **≥4 analysis leptons** (`nGoodLepton>=4`). Per event the ZZ→4ℓ
candidate is built (Z1 = OSSF pair closest to m_Z, Z2 = best remaining OSSF pair)
and stored as `ZZ_*` (`mZ1`, `mZ2`, `m4l`, `pt`, `channel`, lepton indices `l1..l4`, …).
Also written: `GoodLepton_*`, `GoodJet_*` (+ b-tags), `PuppiMET_pt/phi`, `Trig_*`,
`genWeight`, `run`, `luminosityBlock`, `event`.

> Physics note: ZZ→4ℓ comes from one H→ZZ*, so expect m_Z1 ≈ 91 (on-shell),
> m_Z2 low (off-shell Z*), and m4ℓ peaking at ~125 GeV.
