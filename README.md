# Parallel N-Body Simulation

A C++17 implementation of a 2D gravitational N-body simulation in three forms:

- sequential baseline
- OpenMP shared-memory parallel version
- MPI distributed-memory parallel version

All three executables share the same physics core, input format, and benchmark summary format, which makes it easy to compare correctness and performance across implementations.

## Features

- Shared simulation core in `core/`
- Sequential, OpenMP, and MPI executables
- Plain-text simulation inputs
- Optional plain-text benchmark summaries
- Batch experiment script for the prepared test cases
- Simple plotting utility for generated results

## Repository Layout

```text
HPC Project/
  core/          shared config, I/O, physics, and utility code
  data/          prepared input cases
  sequential/    sequential executable
  openmp/        OpenMP executable
  mpi/           MPI executable
  results/       benchmark summaries and plots
  report/        write-up and walkthrough documents
```

## Requirements

- A C++17 compiler
- `mingw32-make` or `make`
- OpenMP support for the OpenMP build
- An MPI compiler wrapper such as `mpic++`
- `mpiexec` for running the MPI version
- Python 3 with `matplotlib` if you want to generate plots
- PowerShell if you want to use the provided experiment script

## Build

From the project root, build everything with:

```powershell
mingw32-make
```

Or build individual targets:

```powershell
mingw32-make -C sequential
mingw32-make -C openmp
mingw32-make -C mpi
```

The build produces:

- `sequential/nbody_seq.exe`
- `openmp/nbody_omp.exe`
- `mpi/nbody_mpi.exe`

## Run

### Sequential

```powershell
.\sequential\nbody_seq.exe --input data\case32.txt --output results\case32_seq_summary.txt
```

### OpenMP

```powershell
.\openmp\nbody_omp.exe --input data\case32.txt --threads 4 --output results\case32_omp4_summary.txt
```

### MPI

```powershell
mpiexec -n 4 .\mpi\nbody_mpi.exe --input data\case32.txt --output results\case32_mpi4_summary.txt
```

### Optional Overrides

All three executables accept:

- `--iterations <count>`
- `--dt <value>`
- `--output <summary_file>`

OpenMP also accepts:

- `--threads <count>`

## Input Format

The shared input format is documented in [core/formats.md](core/formats.md).

Each case uses:

```text
N iterations dt G softening
id mass pos_x pos_y vel_x vel_y
```

Example:

```text
4 1000 0.01 1.0 1e-9
0 10.0 -1.0  0.0  0.0  0.2
1 10.0  1.0  0.0  0.0 -0.2
2 20.0  0.0  1.0 -0.2  0.0
3 20.0  0.0 -1.0  0.2  0.0
```

## Output Format

When `--output` is provided, each executable writes a plain-text summary like:

```text
implementation openmp
particles 128
iterations 200
dt 0.005
gravitational_constant 1.0
softening 1e-6
elapsed_seconds 0.123456
threads 4
processes 1
```

For MPI, the summary records the process count. For the sequential version, `threads` and `processes` are both `1`.

## Benchmark Workflow

The repository includes a PowerShell script that runs the prepared benchmark suite:

```powershell
.\run_experiments.ps1
```

This script runs the `32`, `128`, and `512` particle cases with:

- sequential
- OpenMP with 2 and 4 threads
- MPI with 2 and 4 processes

It stores the generated summary files in `results/`.

To turn the summaries into a graph:

```powershell
python plot_results.py
```

## Implementation Notes

- The simulation is 2D Newtonian gravity with softening.
- Force computation is `O(N^2)` per timestep.
- The sequential version is the reference implementation.
- The OpenMP version parallelizes the outer particle loop.
- The MPI version partitions contiguous particle ranges and synchronizes updated particle state with `MPI_Allgatherv` each timestep.
