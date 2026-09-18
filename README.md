# CUPID-CJPL Bolometer Simulation

Geant4 Monte Carlo simulation of a dual-crystal LMO (Li₂MoO₄) bolometer detector
for the CUPID-CJPL experiment, with a Th-232 radioactive source for energy
calibration. The simulation records per-particle energy deposition in each
crystal independently and provides a full post-processing chain from
reconstructed energies to pile-up-rejected spectra.

## Physics Setup

- **Detector:** two 2×2×2 cm³ LMO crystals (ρ = 3.07 g/cm³), 25 mm
  center-to-center along z (Crystal1 at z = −12.5 mm, Crystal2 at z = +12.5 mm).
- **Source:** Th-232 (Z=90, A=232) at rest, sampled uniformly over the volume of
  a WTh wire (Φ1.6 mm × 10 mm, 2% ThO₂ + 98% W) inside a copper source cup
  below Crystal1. The wire self-absorption stops most α particles (only
  surface-escape αs reach the crystals) and attenuates low-energy γs.
- **Physics list:** custom modular list —
  `G4DecayPhysics` → `G4EmStandardPhysics` → `G4HadronElasticPhysics` →
  `G4HadronPhysicsFTFP_BERT` → `G4IonElasticPhysics` → `G4IonPhysics` →
  `G4RadioactiveDecayPhysics`.
  The radioactive-decay time threshold is raised to 1e60 years (both in code
  and in `run.mac`) so that Th-232 (t½ ≈ 14 Gyr) is forced to decay.

Large external shielding structures are commented out in the source.

## Build

**Prerequisites:** Geant4 (with CMake support) and ROOT (for the
`G4AnalysisManager`-based ntuple output).

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

Source the Geant4 and ROOT environment scripts before running.

## Run

**Batch mode** (1×10⁸ Th-232 decays, from the build directory):

```bash
./exampleB1 run.mac
```

**Interactive mode** (with visualization):

```bash
./exampleB1
```

Output: `LMO_Th232.root` — two TTrees (`Crystal1`, `Crystal2`), per-track rows
with columns: EventID, TrackID, ParticleName, PDG, ParentID, CreatorProcess,
VertexVolumeName, edep_keV. A track depositing in both crystals appears in both
trees; multi-step deposits of one track are deduplicated into a single row.

## Post-Processing Chain

ROOT macros live in `macros/`; run them from `build/` (input/output ROOT files
are there):

| Step | Macro | Input → Output |
|---|---|---|
| 1. Energy reconstruction | `EnergyReconstruction.C` | `LMO_Th232.root` → `LMO_Th232_TotalEdep.root` |
| 2. Arrival-time series | `ParticleTimeInformation.C` | adds shuffled rows + Poisson Time branch |
| 3. Trigger | `Trigger.C` | → `LMO_Th232_TotalEdep_trigger.root` |
| 4. Pile-up rejection | `PileUp.C` | → `LMO_Th232_TotalEdep_trigger_PileUp.root` |
| 5. Spectrum | `spectrum.C` | → energy spectrum plot |

**Step 1 — `EnergyReconstruction.C`.** Walks the track parent-child tree to
reconstruct the total energy of each incident particle (born outside the
crystal), drops zero-deposit rows, and applies the per-crystal experimental
energy resolution:
`σ_Crystal2(LMO1) = 0.00735·E + 6.24 keV`, `σ_Crystal1(LMO2) = 0.01039·E +
3.69 keV`. Output columns include `TotalEdep_keV` (true) and
`MeasuredEdep_keV` (smeared).

```bash
root -l -b -q ../macros/EnergyReconstruction.C+
```

**Step 2 — `ParticleTimeInformation.C`.** Shuffles row order (Fisher–Yates,
fixed seed) and assigns monotonically increasing Poisson arrival times
(rate = N_rows/1e8 × source activity), breaking same-event time correlations
for pile-up studies.

**Step 3 — `Trigger.C`.** Per-crystal trigger cut
`MeasuredEdep_keV + Gaus(0, σ) > threshold`:
Crystal1 threshold 198.5 keV (σ = 105.7 keV),
Crystal2 threshold 320.3 keV (σ = 158.7 keV).
All branches are passed through unchanged.

**Step 4 — `PileUp.C`.** Sliding ±300 ms pile-up rejection on the arrival-time
series: any hit with a neighbour inside the window (whole clusters) is dropped.

**Step 5 — `spectrum.C`.** Plots `MeasuredEdep_keV` (1-keV bins, 0–4000 keV)
with a Gaussian fit to the 2614.5 keV Tl-208 line (exclude residual surface-escape
αs via `excludeAlphas`).

## Notes

- The ntuple structure (column order) is fixed by `RunAction`/`EventAction`;
  the reconstruction macros must be kept in sync with it.
- `run.mac` in `build/` is refreshed from the source copy by CMake; edit the
  source macro and re-run `cmake ..` after changes.
