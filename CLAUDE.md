# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Bolometer simulation — Geant4 Monte Carlo for a dual LMO (Li₂MoO₄) crystal bolometer detector (CUPID-CJPL). Simulates Th-232 radioactive decay in a source cup below the crystals and records per-particle energy deposition in each crystal independently. Derived from the Geant4 basic example B1.

## Build & Run

**Prerequisites:** Geant4 (built with CMake support and ROOT-enabled analysis — `G4AnalysisManager` is used for the ntuple output) and ROOT (for the post-processing macros). Source the environment scripts before building:

```bash
source <geant4-install>/bin/geant4.sh
source <root-install>/bin/thisroot.sh
```

```bash
cd CUPID-CJPL
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

**Batch mode:** `./exampleB1 run.mac` (from `build/`)

**ROOT macros live in `macros/`** (`EnergyReconstruction.C`, `spectrum.C`, `ParticleTimeInformation.C`, `Trigger.C`, `PileUp.C`). Run them from `build/` (the input/output ROOT files are there):

```bash
cd CUPID-CJPL/build
root -l
.L ../macros/EnergyReconstruction.C+
EnergyReconstruction()                  # or batch: root -l -b -q ../macros/EnergyReconstruction.C+
.x ../macros/spectrum.C
.x ../macros/ParticleTimeInformation.C  # or root -l -b -q ../macros/ParticleTimeInformation.C
```

ACLiC artifacts (`EnergyReconstruction_C.so`, `EnergyReconstruction_C.d`, `..._rdict.pcm`) are generated next to the macro, in `macros/`, whenever it is compiled with `+` (they are the compiled library and its dependency cache, independent of the function name; git-ignored).

## Architecture

### Class Structure (all in namespace `B1`)

| Class | Role |
|---|---|
| `DetectorConstruction` | Geometry: World, 2 LMO crystals, Cu source cup, Cu tape |
| `PrimaryGeneratorAction` | Th-232 ion at rest inside the source cup |
| `PhysicsList` | Custom modular: EM + hadronic + radioactive decay |
| `ActionInitialization` | Wires all user actions; `RunAction` registered for both master and worker |
| `RunAction` | Creates ROOT file `LMO_Th232.root` with two ntuples (`Crystal1`, `Crystal2`) |
| `EventAction` | Per-event `std::map<(eventID,trackID), ParticleInfo>` dedup + accumulate |
| `SteppingAction` | Per-step energy deposition collection in Crystal1/Crystal2 |

**Dependency chain:** `RunAction` → `EventAction(runAction)` → `SteppingAction(eventAction)`

### Geometry (active components only)

- **World:** 2 m³ cube, G4_Galactic
- **Crystal1:** 2×2×2 cm LMO (density 3.07 g/cm³), center at z = -12.5 mm
- **Crystal2:** 2×2×2 cm LMO, center at z = +12.5 mm (25 mm center-to-center)
- **Source cup:** Copper, 18 mm diameter × 4.9 mm height, with 14.6 mm lid, 2 mm central hole. Placed below Crystal1 (7.5 mm gap)
- **Copper tape:** 0.005 mm thick, 1.5 mm radius, between cup and Crystal1
- **WTh wire (source):** cylinder Φ1.6 mm × 10 mm, axis along x, at the cup cavity center (z = −31.95 mm). Material: 2% ThO₂ + 98% W by mass, density 18.95 g/cm³. Wire self-absorption stops most αs (only surface-escape αs reach the crystal) and attenuates low-energy γs
- Large shielding structures (Cu buckets, steel layer) are **commented out** in source

### Physics List

Custom `G4VModularPhysicsList` registering:
`G4DecayPhysics` → `G4EmStandardPhysics` → `G4HadronElasticPhysics` → `G4HadronPhysicsFTFP_BERT` → `G4IonElasticPhysics` → `G4IonPhysics` → `G4RadioactiveDecayPhysics`

**Critical settings:**
- `G4HadronicParameters::SetTimeThresholdForRadioactiveDecay(1.0e+60*CLHEP::year)` — forces Th-232 (half-life ~14 Gyr) to be treated as radioactive
- `G4NuclideTable::SetThresholdOfHalfLife(0.1*ps)` — includes nuclides with half-life > 0.1 ps
- `run.mac` also sets `/process/had/rdm/thresholdForVeryLongDecayTime 1e60 year` (double insurance)

### Primary Generator

Th-232 ion (Z=90, A=232), at rest, sampled **uniformly over the WTh wire volume** (radius 0.8 mm, half-length 5 mm, axis along x, center (0,0,−31.95 mm)): cross-section sampling `r = R·√u` (area-uniform), φ and length uniform.

## ROOT Output Structure

**File:** `LMO_Th232.root`

Two TTrees (`Crystal1`, `Crystal2`), each with columns:

| Index | Column | Type | Content |
|---|---|---|---|
| 0 | EventID | I | Event number |
| 1 | TrackID | I | Track number (unique within event) |
| 2 | ParticleName | S | e.g. "gamma", "e-", "e+" |
| 3 | PDG | I | PDG encoding |
| 4 | ParentID | I | Parent track ID |
| 5 | CreatorProcess | S | Process that created this track |
| 6 | VertexVolumeName | S | Volume where track was born |
| 7 | edep_keV | D | Track's total energy deposit in this crystal (keV) |

**Granularity:** Per-track per-crystal. Same track depositing in both crystals → one row in each TTree. Same track with multiple steps in one crystal → single row (edep accumulated via `std::map` dedup).

## Post-Processing Chain

### Step 1: `EnergyReconstruction.C` → `LMO_Th232_TotalEdep.root`

Reconstructs per-incident-particle total energy by walking the track parent-child tree, then applies the detector energy resolution.

**Algorithm:**
1. For each event, build `eventTrackMap` (TrackID → TrackInfo) and `eventChildMap` (ParentID → child TrackIDs)
2. Identify incident particles: `VertexVolumeName ≠ crystalName` (born outside the crystal)
3. For each incident particle, recursively sum: `TotalEdep = own edep + Σ children's TotalEdep`
4. **Zero-deposit filter:** rows with `TotalEdep_keV <= 0` are skipped (pass-through gammas, neutrinos) — applied before smearing
5. Output new TTree with columns: EventID, TrackID, ParticleName, PDG, CreatorProcess, VertexVolumeName, **TotalEdep_keV** (true), **MeasuredEdep_keV** (smeared)

**Energy resolution smearing (2026-08 added):**

- `MeasuredEdep_keV = TotalEdep_keV + Gaus(0, σ(E_true))` — true branch preserved unchanged
- **Per-crystal experimental resolution models** (linear in E, fitted from experiment; E and σ in keV):
  - `Sigma1(E) = 0.00735·E + 6.24` — **LMO1** = Crystal2; σ(2614.5) ≈ 25.5 keV (FWHM ≈ 60 keV)
  - `Sigma2(E) = 0.01039·E + 3.69` — **LMO2** = Crystal1; σ(2614.5) ≈ 30.9 keV (FWHM ≈ 73 keV)
  - Crystal↔experiment mapping: **Crystal1 ↔ exp LMO2, Crystal2 ↔ exp LMO1** (user-confirmed 2026-08-26). The `ApplyEnergyResolutionLMO1/LMO2` function names in `EnergyReconstruction.C` are the formula's own labels and are correct; the **crystal assignment** was swapped in 2026-08-26 (previously Crystal1 wrongly took the LMO1/`Sigma1` model). Any data smeared before that date has the models on the wrong crystals.
- `FillIncidentParticle` selects the model by crystal (`isLMO2` flag passed from `ProcessEvent` — `crystalName == "Crystal1"`)
- `gSmearOn = false` reproduces the old behavior (Measured == Total); `gSeed = 12345` makes smearing reproducible
- Smearing is applied to the **per-incident-particle TotalEdep** (the calorimetric quantity), never per track or per step
- **Pull check (validate after every regen):** pull distribution on peak events, (Measured−Total)/σ(Total). Old numbers (C1: mean −0.004, σ 0.998; C2: mean −0.003, σ 0.969) were obtained with the reversed crystal assignment — the Gaussian shape check is assignment-independent, but re-run after the 2026-08-26 swap to confirm.
- Note: with these widths the 2614.5 peak is a broad bump (peak fits need Gaussian+linear background and ±3σ windows; fit-derived σ can deviate up to ~±10% from the model due to background systematics — the pull check is the strict validation). Peak-height numbers in older notes were produced with the reversed assignment — re-derive after regeneration.

### Step 2: `spectrum.C` → energy spectrum plot

Reads `LMO_Th232_TotalEdep.root`, draws `MeasuredEdep_keV` by default (`useMeasured = true`; set `false` to draw the true `TotalEdep_keV` for comparison), 1-keV bins, 0-4000 keV, cut `>0 && PDG != 1000020040` (`excludeAlphas = true` drops the residual surface-escape αs from the WTh wire; the `>0` cut is redundant now that the reconstruction filters zero-deposit rows, but harmless).

**Peak fitting (current method):** with the resolution smearing the 2614.5 keV peak is a genuine Gaussian and Gaussian fitting is well-conditioned (before smearing the true peak was a δ function and the fit failed — Crystal2 once converged to μ=2611.31, χ²/ndf=inf):
- Both crystals: **Gaussian + linear background** `[0]*exp(-0.5*((x-[1])/[2])^2)+[3]+[4]*(x-2614.5)`, window ±3σ with σ from the experimental models (`sigC1` = LMO2 model for Crystal1, `sigC2` = LMO1 model for Crystal2 — mirror of EnergyReconstruction.C, keep the two files in sync)
- ROI(3σ) peak counts computed from the fitted mean/σ
- Fit-derived σ deviates up to ~±10% from the model due to background systematics — use the pull distribution for strict validation

**Gamma-line markers:** the Th-232 chain γ-line marker block (NDC-based `TLine::SetNDC()` + `TLatex::SetNDC()`, immune to log-axis coordinate pitfalls and zoom) is currently **commented out** in `spectrum.C` — re-enable when needed.

**Note:** the plot title says "1.6cm WTh wire" — the geometry now IS a WTh wire source, but its actual size is 1.6 mm diameter × 10 mm (not 1.6 cm) — update the `DrawLatex` title string to match exactly.

### Step 3: `ParticleTimeInformation.C` → shuffled rows + Time branch (2026-08-28 rewrite)

Rewrites both trees of `LMO_Th232_TotalEdep.root` with **shuffled row order** and a new `Time` branch (ms):
- **Row shuffle first:** each tree's rows are permuted (Fisher–Yates, fixed seed `gShuffleSeed + ic`, `gShuffleSeed = 20250828`) before time assignment, to break the time correlation between particles from the same event (the raw file is ordered by EventID)
- First row of each tree: Time = 0; intervals between successive rows sampled from the exponential distribution I(t) = r·exp(−r·t) (Poisson arrival process, Δt = −ln(u)/r)
- Per-crystal rate: `r = N_rows / 1e8 × 3.0992 Bq` (1e8 = `run.mac` beamOn; 3.0992 Bq = experimental source activity), converted to ms⁻¹
- The two trees are **independent** time sequences (no C1-C2 coincidence correlation); both span the same exposure time ≈ 1e8/3.0992 s
- Fixed seeds: time sequence `gTimeSeed = 12345`, shuffle `gShuffleSeed = 20250828`; shuffle and time use two **independent `TRandom3` instances** so neither stream depends on the other
- Skips trees that already have a Time branch; writes to a temp file and only replaces the original if at least one tree was rewritten (idempotency guard)
- Purpose: pile-up studies — merge particles within a time window into single detector events, with same-event correlations removed by the shuffle
- **Implementation (final, 2026-08-28):** read the whole tree into memory sequentially (source is a single giant basket — sequential read is fast, random access is not); Fisher–Yates permutation (`perm[i]` = output row i's original row index); write the output tree in **shuffled row order**; time is assigned **after the shuffle, monotonically along the new row order**: output row 0 gets Time = 0, row i gets Time = prev + Exp(1/ratePerMs). Result: EventID is decorrelated from row index, Time strictly increases with row. String branches must be bound as `char[64]` (NOT `std::string` — ROOT reports "pointer type given string does not correspond to Char_t" and silently reads garbage). Do NOT use `tree->CloneTree(-1,"fast")` here — with the single-basket source layout it silently doubled the entries (10.86M → 21.7M, discovered 2026-08-28). Peak memory ≈ 2.4 GB (Crystal1's 10.86M rows in memory); swap released per tree.
- **Validated 2026-08-28:** Time[0] = 0 and Time strictly monotonic over first 500k rows (0 violations); EventID up/down fraction between adjacent rows ≈ 50%/50% (vs ≈100% up before shuffling) — same-event particles are decorrelated in time. String branches intact (spot check: ParticleName="gamma").
- **Run flow:** re-run `EnergyReconstruction.C` first (it writes the file without a Time branch), then this macro, then `Trigger.C`
- **Keep in sync:** `gNSimDecays` in this macro, `kNSimEvents` in `Trigger.C`, and `run.mac`'s `/run/beamOn` must all equal the same number of simulated decays. Current value: **1e8** (changed 2026-08-26; was 2e8 when this macro was written)

### Step 3b: `Trigger.C` → trigger-simulated spectrum (full-format passthrough since 2026-08-31)

Applies a trigger cut to `LMO_Th232_TotalEdep.root`: a particle passes if `TotalEdep_keV + Gaus(0, sigma) > threshold`, and the recorded value is `MeasuredEdep_keV` (reconstructed energy, trigger noise NOT added to it). Per-crystal trigger parameters: Crystal1 threshold = 198.5 keV, σ = 105.7 keV; Crystal2 threshold = 320.3 keV, σ = 158.7 keV. Fixed seed 20250825. Output: `LMO_Th232_TotalEdep_trigger.root` — **all 9 branches passed through unchanged for passing rows** (same schema as `LMO_Th232_TotalEdep_PileUp.root`), so `PileUp.C` can run on it directly. Metadata: `NSimEvents`, `comment` (file intro + normalization), `Trigger_Crystal1/2` (per-tree pass-rate stats). Latest run (2026-08-31): pass rates 52.25% (C1) / 42.18% (C2), noise triggers 1685/184.

### Step 4: `PileUp.C` → pile-up rejection (2026-08-29 added, 2026-08-31 re-parameterized)

Cuts pile-up events from the trigger output. Requires the Time branch and relies on Time being monotonic in row order (the shuffled file satisfies this):
- **Definition (user-confirmed):** event = one row (one particle arrival); sliding window ±300 ms (`kPileUpWindowMs = 300.0`); any hit with a neighbor within the window is rejected; whole clusters (3+ rows) are rejected. Equivalent: any two rows with Δt ≤ 300 ms are both dropped.
- **Input/output parameterized:** `PileUp(inputName, outputName)` with defaults `LMO_Th232_TotalEdep_trigger.root` → `LMO_Th232_TotalEdep_trigger_PileUp.root` (chain order: trigger first, then pile-up — mirrors real detector logic).
- **Algorithm:** read the Time branch only (`SetBranchStatus`), mark rows whose nearest neighbor (adjacent row — Time is sorted) is within the window, then sequential-copy survivors (same 9-branch schema). Metadata: `PileUpWindow_ms`, `comment` (file intro), `PileUp_Crystal1/2` (per-tree rejection stats).
- **Validated 2026-08-31:** on the trigger output, rejection 10.03% (C1) / 0.93% (C2) vs theory 1−exp(−2r·w) with post-trigger rates = 10.01% / 0.93%. Earlier (pre-trigger) validation: 18.32% / 2.20% vs 18.3% / 2.19%.

## Critical Design Constraint: Track Recording — History of Two Fixes (do not regress)

The 2614.5 keV peak's correctness depends on which tracks `SteppingAction` records. Two bugs have been found and fixed here; both fixes are in the current code.

### Problem 1 (historical): incident gammas dropped → 2615 keV peak missing

The original `SteppingAction` had `if (edep <= 0.) return;` before recording. Gammas deposit energy only through secondaries (their own step edep is 0), so all incident gammas — including the Tl-208 2614.5 keV line — were silently discarded. The Compton electrons were recorded with `VertexVolumeName="Crystal1"`, but their parent gamma was missing from the track tree, so the full-energy peak was completely absent from the processed spectrum. (Evidence from a 1000-event test: 402 of 471 incident gammas had edep=0 and were dropped.)

**Fix (stage 2):** record incident particles (`bornOutside = vertexVolumeName != volumeName`) even with edep=0; record internal particles only if edep > 0.

### Problem 2: internal zero-deposit photons orphaned → ~13 keV energy deficit

Stage 2 still skipped internal photons with edep=0 (fluorescence X-rays, annihilation 511s, bremsstrahlung). When such a photon is absorbed inside the same crystal, its photoelectron's `ParentID` points to a track absent from the ROOT file — `EnergyReconstruction.C` can never reach it, and its energy is lost. Estimate: Mo Kα (≈17.4 keV, absorption length ≈0.1 mm in LMO, K fluorescence yield ≈77%) lost per K-shell photoelectric event → full-energy peak ~13 keV low (near 2600 keV).

**Fix (stage 3, current code):** `SteppingAction` now records **every track appearing in Crystal1/Crystal2 unconditionally** — only the volume-name filter remains; the `bornOutside || edep > 0.` guard is commented out in the source. Internal zero-deposit photons become intermediate tree nodes: harmless for incident classification (their `VertexVolumeName == crystalName`), essential for energy conservation. Verified 2026-08: peak sits exactly at 2614.5 keV (1-keV peak-bin counts: C1 = 13932, C2 = 1971, background not subtracted).

### Critical Constraints (DO NOT VIOLATE)

1. **Never re-add any edep-based filter in `SteppingAction`** — no `if (edep <= 0.) return;`, no `bornOutside || edep > 0.` guard. Every track entering a crystal must be recorded. Incident classification belongs exclusively to `EnergyReconstruction.C` (`VertexVolumeName != crystalName`).

2. **Never remove or modify the `EventAction` track dedup mechanism.** The `std::map<(eventID,trackID), ParticleInfo>` with first-occurrence-wins metadata + subsequent-edep-accumulation is essential. Without it, the same track would create duplicate entries for every transport step (potentially hundreds per track).

3. **`EnergyReconstruction.C` must always be run before `spectrum.C`.** The raw `LMO_Th232.root` contains per-track data. Only `EnergyReconstruction.C` can reconstruct the incident particle energy by walking the parent-child tree. Running `spectrum.C` directly on the raw file will produce a physically meaningless spectrum.

4. **Do not change the ROOT ntuple structure** without updating both `EnergyReconstruction.C` (branch names in `SetBranchAddress`) and `EventAction` (column indices in `FillNtuple*Column`). The two must stay in sync.

## Key Implementation Details

### SteppingAction: unconditional track recording

SteppingAction records **every track appearing in Crystal1/Crystal2** (only the volume-name filter remains; the old `bornOutside || edep > 0.` guard is commented out in the source). Rationale: internal zero-deposit photons (fluorescence X-rays, annihilation gammas, bremsstrahlung) must appear in the tree as intermediate nodes, otherwise their absorbed secondaries are orphaned in `EnergyReconstruction.C` and ~13 keV per photoelectric event is lost. Incident classification happens only in `EnergyReconstruction.C` (`VertexVolumeName != crystalName`).

### SteppingAction: volume matching

Uses string comparison (`volumeName == "Crystal1"`) rather than the `fScoringVolume` pointer (which is commented out in `DetectorConstruction`).

### EventAction: Track dedup

`AddParticleInfo1/2` uses `std::map<std::pair<G4int,G4int>, ParticleInfo>` with key `(eventID, trackID)`. First occurrence creates the entry (storing all metadata); subsequent occurrences only accumulate edep. Maps are cleared at end of each event.

### Unit conversion in edep accumulation (bug fixed)

`step->GetTotalEnergyDeposit()` returns MeV; the value stored in the ntuple is always keV. Both branches of `EventAction::AddParticleInfo1/2` now convert correctly:

```cpp
// First call:
info.edep = edep/keV;

// Subsequent calls:
particleMap1[key].edep += edep/keV;
```

An earlier version had a bug where subsequent calls did `+= edep` (raw MeV added to a keV value), underestimating multi-step charged-particle tracks by ~1000×. **That bug is fixed — do not reintroduce it.**

### EnergyReconstruction.C: Cross-crystal tracks

Crystal1 and Crystal2 are processed independently. A particle born in Crystal1 that enters Crystal2 will be recorded in Crystal2's TTree with `VertexVolumeName="Crystal1"`, correctly identified as incident for Crystal2. Its parent-child links within Crystal2's data are limited to tracks that also appear in Crystal2's TTree.

## Data Flow Summary

```
run.mac (1×10⁸ Th-232 decays, WTh wire source)
  → exampleB1 (Geant4)
    → LMO_Th232.root (per-track, 2 TTrees)
      → macros/EnergyReconstruction.C (tree-building + recursive sum
        + zero-deposit filter + resolution smearing)
        → LMO_Th232_TotalEdep.root (per-incident-particle;
          TotalEdep_keV = true, MeasuredEdep_keV = smeared)
          ├─ macros/ParticleTimeInformation.C (shuffled rows + Time branch, ms,
          │    Poisson arrival times for pile-up studies)
          ├─ macros/Trigger.C (trigger cut, 9-branch passthrough
          │    → LMO_Th232_TotalEdep_trigger.root)
          │    └─ macros/PileUp.C (pile-up cut ±300 ms
          │         → LMO_Th232_TotalEdep_trigger_PileUp.root)
          └─ macros/spectrum.C (histogram + Gaussian fit on MeasuredEdep_keV)
               → energy spectrum plot
```
